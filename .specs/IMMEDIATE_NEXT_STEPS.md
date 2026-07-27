# Próximos Passos Imediatos — Forge Engine (Estado Real da Codebase)

> Baseado na análise do `FORGE_CORE_ROADMAP.md` + inspeção direta dos arquivos em `src/`.  
> **Status real:** 5 tarefas concluídas, ~35 não iniciadas.

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
├── Linux/
│   ├── linux_platform.c     ← detecção X11/Wayland + dispatch via vtable
│   ├── window_backend.h     ← WindowBackend vtable interface
│   ├── X11/x11_backend.*    ← backend X11/XCB completo (event capturing)
│   └── Wayland/wayland_backend.c  ← backend completo (xdg-shell, shm, keyboard, pointer)
└── Tools/
    ├── logger.h         ← LogLevel enum + anvlLog*/ANVIL_CORE_* macros
    └── logger.c         ← timestamp, cores ANSI, fprintf(stderr)
```

### Tarefas do Roadmap vs. Implementação Real

| Seção | Tarefa | Status | Evidência na Codebase |
|-------|--------|--------|----------------------|
| **1.2 Eventos de Entrada** | Captura teclado/mouse/janela | ✅ Concluída | `event.h` (8 tipos), `win32_platform.c` (WM_* mapeados) |
| **1.3 Logging** | Sistema com níveis | ✅ Concluída | `logger.h/c` (6 níveis + macros core/client) |
| **1.1 Gerenciamento de Janelas** | Criação/redimensionamento/fechamento | ✅ Concluída | Backend Win32 completo; X11/XCB completo (event capturing); Wayland completo (xdg-shell, shm, keyboard, pointer); opaque pointer aplicado corretamente (`platform.h`)
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

### Problemas Reais de Barreira de Abstração — Executados / Pendentes

~~#### 1. Camada de Plataforma conhece struct interno do Core (inversão de dependência)~~ ✅ **Executado via P1**
- Event types movidos para `Platform/event.h`
- Callback usa tipo definido por Platform, não pelo Core

~~#### 2. Detecção de platform via PCH~~ ✅ **Executado via P2**
- `#include "platform_detection.h"` removido do PCH e isolado em `logger.c`

#### 3. Callback passa `Event` por valor (copia da union inteira)
**Arquivo:** `src/Platform/platform.c`, linha 24-28

#### 3. Callback passa `Event` por valor (copia da union inteira)
**Arquivo:** `src/Platform/platform.c`, linha 24-28:
```c
Event event = { .type = WindowClose, ... };
window->EventCallback(event);  // ← copia toda a union de Event
```
#### 3. Callback passa `Event` por valor (copia da union inteira) — **Pendente**
**Arquivo:** `src/Platform/platform.c`, linha 24-28

#### 4. Wayland stub e macOS sem backend
**Arquivo:** `src/Platform/Linux/Wayland/*`
- ✅ X11/XCB completo em `src/Platform/Linux/X11/x11_backend.c`
- ✅ Wayland completo: xdg-shell, shared memory buffers, keyboard, pointer, server-side decorations
- ❌ macOS sem backend

---

### Plano de Refatoração Concreto — Executado

#### Passo A: Quebrar dependência Platform → Core no callback ✅ **Executado (P1)**
- Event types movidos para `Platform/event.h`
- Callback usa tipo definido por Platform, não pelo Core

#### Passo B: Isolar detecção de platform do PCH ✅ **Executado (P2)**
- `#include "Platform/platform_detection.h"` removido do PCH
- Movido para `logger.c`, onde as macros são realmente consumidas



---

## 🏗️ Tarefas de Implementação (Baseado na Codebase Real)

### P1 — Quebrar acoplamento Platform → Core no callback
**Estado atual:** ✅ Concluído — `event.h` movido para `Platform/event.h`, `platform.h` inclui apenas módulos internos.  
**Arquivos afetados:** `src/Platform/platform.h`, `src/Platform/event.h`, `src/Platform/typedefs.h`, `src/Core/application.h`, `src/Platform/Windows/win32_platform.c`

- [x] Definir tipos de evento em `Platform/event.h` (EventType, Event structs, union)
- [x] Alterar `PFEVENTCALLBACKFUNC` para usar `Event` definido por Platform
- [x] `win32_platform.c` constrói `Event` sem incluir Core diretamente
- [x] `application.c` inclui `Platform/event.h` e usa `Event` diretamente

### P2 — Remover platform_detection do PCH
**Estado atual:** ✅ Concluído — `platform_detection.h` removido do PCH e movido para `logger.c` onde é consumido.  
**Arquivos afetados:** `src/anvlpch.h`, `src/Tools/logger.c`

- [x] Remover `#include "Platform/platform_detection.h"` do PCH
- [x] Adicionar `#include "Platform/platform_detection.h"` dentro de `logger.c` (onde as macros são realmente usadas para `localtime_s` vs `localtime_r`)
- [x] Nenhum outro arquivo precisa das macros — o PCH agora é 100% cross-platform

### P3 — Backend Linux (stub ou mínimo)
**Estado atual:** ✅ Concluída — backends X11/XCB e Wayland completos com event capturing.  
**Arquivos afetados:** `src/Platform/Linux/linux_platform.c`, `src/Platform/Linux/X11/x11_backend.c`, `src/Platform/Linux/Wayland/wayland_backend.c`

- [x] Criar stub com `#error` para progresso incremental
- [x] Implementar X11/XCB mínimo com event capturing (`xcb_connect`, `xcb_create_window`, `xcb_map_window`, `xcb_poll_for_event`)
- [x] Implementar Wayland mínimo (`wl_display`, `wl_registry`, surface creation, event loop)

### P4 — Refatoração de Arquitetura: Separação por Responsabilidades
**Estado atual:** ✅ Fase 1 concluída — backends X11/XCB e Wayland completos, vtable `WindowBackend` validada em runtime. Fase 2 (separação por responsabilidades) ainda pendente.
**Decisão estratégica:** Manter a estrutura atual **temporariamente** para focar na implementação correta das APIs X11/XCB e Wayland sem distrações. Após os backends estarem funcionais, refatorar para separação por responsabilidades.

#### Fase 1 — Implementação dos Backends (estrutura atual)
**Estado atual:** ✅ Concluída — validação runtime confirmou que todos os itens estão corretos.
- [x] Corrigir `getenv` sem check de `NULL` em `_window_backend_detect()`
  → ✅ **Sem bug** — as variáveis retornadas por `getenv` são verificadas com `!= NULL` antes de qualquer uso (`strcmp`). O código está correto.
- [x] Implementar `free(backend)` nos `destroy` do X11 e Wayland backends
  → ✅ **Sem bug** — design intencional. O `free(backend)` ocorre no `backend_shutdown`, que limpa conexões com as libs nativas (XCB display / wl_display) e libera o struct. O `window_destroy` limpa apenas recursos da janela (XCB window / Wayland surface + buffer). Como a aplicação para após o fechamento da janela, não há leak.
- [x] Completar implementação real do backend Wayland (xdg-shell, shared memory, keyboard, pointer)
  → ✅ **Concluído** — xdg-shell, shared memory buffers, keyboard, pointer, server-side decorations implementados.
- [x] Completar implementação real do backend X11/XCB (criação de janela, surface, event loop)
  → ✅ **Concluído** — WM_DELETE_WINDOW, configure notify, key/mouse events implementados.
- [x] Garantir que a vtable `WindowBackend` funcione corretamente em runtime
  → ✅ **Correto** — padrão vtable bem aplicado: `x11_backend()` e `wayland_backend()` retornam `const WindowBackend*` para instâncias `static`; `linux_platform.c` acessa via `window->backend->function_name(...)`; todos os 7 ponteiros de função preenchidos em ambos os backends; vtable imutável após inicialização.

#### Fase 2 — Refatoração para Separação por Responsabilidades
**Nova estrutura planejada:**
```
src/
├── Core/              ← math, containers, utilities (cross-platform)
├── Platform/          ← SOMENTE primitivas OS puras: memory, threading, time, CPU info
├── Windowing/         ← janela + event loop (nova pasta)
│   ├── api/
│   │   └── window_backend.h  ← vtable interface (move de Platform/Linux/)
│   └── backends/
│       ├── win32/
│       ├── x11/
│       └── wayland/
├── FileSystem/        ← file I/O abstraction (nova pasta)
├── Audio/             ← audio abstraction (futuro)
├── Input/             ← keyboard/mouse/gamepad (futuro)
└── Tools/
    ├── logger.h
    └── logger.c
```

**Critério de decisão:** "Se eu posso descrever o que ele faz sem mencionar 'sistema operacional', não pertence a Platform/."

- [ ] Mover `window_backend.h` para `src/Windowing/api/`
- [ ] Mover backends X11/Wayland para `src/Windowing/backends/`
- [ ] Criar stub para `src/Windowing/backends/win32/`
- [ ] Reduzir `Platform/` ao mínimo (primitivas OS puras)
- [ ] Atualizar includes e paths de todos os consumers
- [ ] Configurar `premake5.lua` para a nova estrutura

**Motivo:** Cada subsystem tem suas próprias abstrações, APIs públicas e backends. Manter tudo em `Platform/` cria acoplamento, dificulta testabilidade isolada e torna o refatoramento futuro mais doloroso. Mas fazer agora enquanto se aprende XCB/Wayland seria contraproducente — a refatoração será mais acertiva após entender as dores reais das APIs.

### P5 — I/O de Arquivos abstrato (Novo)
**Estado:** Zero implementação.  
**Dependência:** Refatoração (P4) deve estar estável antes de começar, para que o FileSystem nasça na estrutura correta.

- [ ] Criar `src/FileSystem/filesystem.h`:
```c
typedef struct FileHandle FileHandle;
FileHandle* anvlFileSystemOpen(const char* path, const char* mode);
uint64 anvlFileSystemRead(FileHandle* file, void* buffer, uint64 size);
uint64 anvlFileSystemWrite(FileHandle* file, const void* buffer, uint64 size);
bool anvlFileSystemClose(FileHandle* file);
bool anvlFileSystemExists(const char* path);
```
- [ ] Criar `src/Platform/Windows/windows_filesystem.c` + `src/Platform/Linux/linux_filesystem.c`

### P6 — Layer System / Event Dispatcher (Novo)

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
| **P2** | Backend Linux (stub ou mínimo) | `src/Platform/Linux/linux_platform.c`, `X11/*`, `Wayland/*` | ✅ Concluída |
| **P3** | Refatoração de Arquitetura (separação por responsabilidades) | Move: `window_backend.h`, backends → `Windowing/` | Refatoração |
| **P4** | I/O de Arquivos abstrato | Novo: `src/FileSystem/filesystem.h` + backends por plataforma | Implementação |
| **P5** | Layer System / Event Dispatcher | Novo: `src/Core/event_layer.h`, `src/Core/event_layer.c` | Implementação |
