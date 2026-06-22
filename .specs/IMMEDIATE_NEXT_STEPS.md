# Próximos Passos Imediatos — Forge Engine (Estado Real da Codebase)

> Baseado na análise do `FORGE_CORE_ROADMAP.md` + inspeção direta dos arquivos em `src/`.  
> **Status real:** 2 tarefas concluídas, 1 parcialmente implementada, ~35 não iniciadas.

---

## 📁 Estado Atual da Codebase

### Estrutura de `src/`
```
src/
├── anvil.c              ← entry point (main)
├── anvlpch.h            ← precompiled header (inclui Core/base, Core/typedefs, Tools/logger)
├── anvlpch.c
├── Core/
│   ├── application.h    ← Application struct + anvlAppInit/Run/Shutdown + callbacks de evento
│   ├── application.c    ← implementação: cria janela, seta callback, event loop simples
│   ├── base.h           ← detecção de compiler/OS (MSVC/GCC, Win/Linux/macOS), macros
│   ├── event.h          ← EventType enum + structs de evento + union Event
│   └── typedefs.h       ← uint8..uint64, int8..int64, float32, float64
├── Platform/
│   ├── platform.h       ← NativeWindow (opaque), PFEVENTCALLBACKFUNC, funções da plataforma
│   └── Windows/
│       └── win32_platform.c  ← backend Win32: WNDCLASS, HWND, message loop, eventos mapeados
└── Tools/
    ├── logger.h         ← LogLevel enum + anvlLog*/ANVIL_CORE_* macros
    └── logger.c         ← timestamp, cores ANSI, fprintf(stderr)
```

### Tarefas do Roadmap vs. Implementação Real

| Seção | Tarefa | Status | Evidência na Codebase |
|-------|--------|--------|----------------------|
| **1.2 Eventos de Entrada** | Captura teclado/mouse/janela | ✅ Concluída | `event.h` (8 tipos), `win32_platform.c` (WM_* mapeados) |
| **1.3 Logging** | Sistema com níveis | ✅ Concluída | `logger.h/c` (6 níveis + macros core/client) |
| **1.1 Gerenciamento de Janelas** | Criação/redimensionamento/fechamento | ⚠️ Parcial | Backend Win32 completo em `win32_platform.c`; **sem backend Linux**; opaque pointer aplicado corretamente (`platform.h` usa forward declaration) |
| 1.4 I/O de Arquivos | Abstração filesystem | ❌ Não iniciado | Nenhum arquivo em `src/` relacionado |
| 1.5 Propagação de Eventos | Layer System | ❌ Não iniciado | Callback vai direto para `anvlApplicationOnEvent()` — sem stack de layers |

### Seções 2–8 do Roadmap
**Nenhum arquivo existe** na codebase atual. Renderização, Assets, Física, Áudio, ECS, Editor e Otimizações estão em branco.

---

## 🔧 Refatoração: Barreiras de Abstração (SICP) — Diagnóstico Real

### O que está correto na codebase atual

**`NativeWindow` usa opaque pointer corretamente:**
- `platform.h` linha 8: `typedef struct NativeWindow NativeWindow;` — forward declaration, tipo incompleto.
- `win32_platform.c` linha 9: definição concreta só existe no `.c` do backend.
- Consumers que incluem `platform.h` **não podem acessar membros** — o padrão pImpl está aplicado.

### Problemas Reais de Barreira de Abstração

#### 1. Camada de Plataforma conhece struct interno do Core (inversão de dependência)
**Arquivo:** `src/Platform/platform.h`, linha 11
```c
#include "Core/event.h"          // ← plataforma inclui core
typedef void (*PFEVENTCALLBACKFUNC)(Event event);  // ← callback usa type do Core
```
**Problema:** A camada de infraestrutura (Platform) depende da camada de domínio (Core). Em uma biblioteca bem abstraída, a camada mais baixa não deve conhecer os tipos da camada superior. O callback deveria usar um tipo genérico (`void*` ou struct platform-specific), e o consumer faria o cast para `Event`.

**Impacto:** Adicionar um novo evento exige recompilar a plataforma; a plataforma vicia consumers ao layout de `Event`; circularidade potencial se o Core precisar de algo da Platform.

#### 2. Macros de platform vazam via PCH para módulos que não precisam
**Arquivo:** `src/anvlpch.h` → inclui `Tools/logger.h`  
**Arquivo:** `src/Tools/logger.c`, linhas 21-25:
```c
#if defined (ANVIL_PLATFORM_WINDOWS)
    localtime_s(&current_localtime, &current_time_raw);
#elif defined(ANVIL_PLATFORM_LINUX)
    localtime_r(&current_localtime, &current_time_raw);
#endif
```
**Problema:** `anvlpch.h` inclui `logger.h`, que contém `#include "Core/typedefs.h"`. O logger já usa macros de platform (`ANVIL_PLATFORM_WINDOWS`). Isso significa que **qualquer módulo que inclua o PCH herda dependências de platform detection**, mesmo módulos que são puramente cross-platform (como `typedefs.h` em si).

**Impacto:** Mudar a detecção de platform em `base.h` recompila tudo; módulos cross-platform ficam acoplados à detecção de OS.

#### 3. Callback passa `Event` por valor (copia da union inteira)
**Arquivo:** `src/Platform/platform.c`, linha 24-28:
```c
Event event = { .type = WindowClose, ... };
window->EventCallback(event);  // ← copia toda a union de Event
```
**Problema:** `Event` contém uma union com múltiplos structs — é um tipo grande. Passar por valor em cada callback copia dados desnecessariamente. Mais importante: o callback recebe um `Event` concreto do Core, não um handle genérico da plataforma.

#### 4. Sem separação Linux/macOS — stubs detectados mas inexistentes
**Arquivo:** `src/Platform/` contém apenas `Windows/`.  
`base.h` define `ANVIL_PLATFORM_LINUX` e `ANVIL_PLATFORM_MACOS`, mas nenhum backend existe para eles.

---

### Plano de Refatoração Concreto

#### Passo A: Quebrar dependência Platform → Core no callback
1. Em `platform.h`: definir um struct genérico `PlatformEvent`:
```c
typedef struct PlatformEvent {
    EventType type;
    bool handled;
    void* payload;  // consumer cast para o struct específico do Core
} PlatformEvent;

typedef void (*PFEVENTCALLBACKFUNC)(PlatformEvent event);
```
2. Em `win32_platform.c`: mapear WM_* → construir `PlatformEvent` com pointer ao struct concreto → passar via callback.
3. Consumer faz cast: `(const Event*)platform_event.payload`.

#### Passo B: Isolar detecção de platform do PCH
1. Mover macros de compiler/OS (`base.h`) **fora** do PCH — cada módulo que precisa inclui `base.h` explicitamente.
2. `anvlpch.h` deve incluir apenas tipos cross-platform (`typedefs.h`).
3. `logger.c` deve incluir `base.h` internamente, não via PCH.

#### Passo C: Criar `src/Platform/Linux/` com stub
1. `linux_window.c` — stub com `#error "Not implemented"` ou implementação mínima com XCB/Xlib.
2. Garantir que `premake5.lua` inclua arquivos do backend correto via define.

---

## 🏗️ Tarefas de Implementação (Baseado na Codebase Real)

### P1 — Quebrar acoplamento Platform → Core no callback
**Estado atual:** `platform.h` inclui `Core/event.h` e usa `Event event` diretamente no callback.  
**Arquivos afetados:** `src/Platform/platform.h`, `src/Platform/Windows/win32_platform.c`

- [ ] Definir `PlatformEvent` em `platform.h` (struct genérico com `void* payload`)
- [ ] Alterar `PFEVENTCALLBACKFUNC` para usar `PlatformEvent` ao invés de `Event`
- [ ] Em `win32_platform.c`: construir `PlatformEvent`, armazenar `Event` concreto no `payload`
- [ ] Atualizar `application.c` para cast do payload de volta para `Event`

### P2 — Isolar detecção de platform do PCH
**Estado atual:** `anvlpch.h` inclui `logger.h` que usa macros de platform.  
**Arquivos afetados:** `src/anvlpch.h`, `src/Tools/logger.c`, `src/Core/base.h`

- [ ] Remover `#include "Tools/logger.h"` de `anvlpch.h`
- [ ] Mover `#include "Core/base.h"` para fora do PCH ou tornar opcional
- [ ] Garantir que `logger.c` inclua `base.h` diretamente se precisar de macros de platform

### P3 — Backend Linux (stub ou mínimo)
**Estado atual:** Zero implementação.  
**Novos arquivos:** `src/Platform/Linux/linux_window.c`

- [ ] Criar stub com `#error "Linux backend not yet implemented"` para progresso incremental
- [ ] Ou implementar mínimo com XCB/Xlib: `xcb_connect`, `XCreateWindow`, `XMapWindow`, `XNextEvent`
- [ ] Configurar `premake5.lua` para condicionar arquivos compilados por plataforma

### P4 — I/O de Arquivos abstrato (Novo)
**Estado:** Zero implementação.  
**Dependência:** Barreiras de abstração (Passos A e B acima) devem estar estáveis antes de começar.

- [ ] Criar `src/Core/filesystem.h`:
```c
typedef struct FileHandle FileHandle;
FileHandle* anvlFileSystemOpen(const char* path, const char* mode);
uint64 anvlFileSystemRead(FileHandle* file, void* buffer, uint64 size);
uint64 anvlFileSystemWrite(FileHandle* file, const void* buffer, uint64 size);
bool anvlFileSystemClose(FileHandle* file);
bool anvlFileSystemExists(const char* path);
```
- [ ] Criar `src/Platform/Windows/windows_filesystem.c` + `src/Platform/Linux/linux_filesystem.c`

### P5 — Layer System / Event Dispatcher (Novo)

**Estado atual:** Callback direto em `application.c` (linha 42):
```c
anvlPlatformSetWindowEventCallback(app->internal->window, anvlApplicationOnEvent);
```
Sem stack, sem layers.

- [ ] Criar `src/Core/event_layer.h`:
```c
typedef struct EventLayer {
    const char* name;
    void (*OnEvent)(struct EventLayer* layer, Event event);
    struct EventLayer* next;
} EventLayer;

void anvlEventDispatcherPushLayer(EventLayer* layer);
void anvlEventDispatcherPopLayer(EventLayer* layer);
void anvlEventDispatcherDispatch(Event event);  // chama layers em ordem inversa
```
- [ ] Atualizar `application.c` para usar dispatcher ao invés de callback direto
- [ ] Adicionar `anvlApplicationOnEvent` como camada default

---

## 📊 Resumo Executivo (Baseado na Codebase Real)

| Prioridade | Tarefa | Arquivos Afetados | Tipo |
|------------|--------|-------------------|------|
| **P0** | Quebrar acoplamento Platform → Core no callback | `src/Platform/platform.h`, `src/Platform/Windows/win32_platform.c` | Refatoração |
| **P1** | Isolar detecção de platform do PCH | `src/anvlpch.h`, `src/Tools/logger.c`, `src/Core/base.h` | Refatoração |
| **P2** | Backend Linux (stub ou mínimo) | Novo: `src/Platform/Linux/linux_window.c` | Implementação |
| **P3** | I/O de Arquivos abstrato | Novo: `src/Core/filesystem.h` + backends por plataforma | Implementação |
| **P4** | Layer System / Event Dispatcher | Novo: `src/Core/event_layer.h`, `src/Core/event_layer.c` | Implementação |
| **P5** | Compilação cross-platform (`premake5.lua`) | `premake5.lua` | Configuração |
