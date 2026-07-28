# Próximos Passos Imediatos — Forge Engine (Estado Real da Codebase)

> Baseado na análise do `FORGE_CORE_ROADMAP.md` + inspeção direta dos arquivos em `src/`.  
> **Status real:** 8 tarefas concluídas, ~32 não iniciadas.

---

## 📁 Estado Atual da Codebase

### Estrutura de `src/`
```
src/
├── anvil.c              ← entry point (main)
├── anvlpch.h            ← precompiled header (inclui Core/typedefs, Tools/logger)
├── anvlpch.c
├── Core/
│   ├── application.h    ← Application struct + anvl_application_init/Run/Shutdown + callbacks
│   ├── application.c    ← implementação: cria janela, seta callback, event loop simples
│   └── typedefs.h       ← uint8..uint64, int8..int64, float32, float64
├── Windowing/
│   ├── event.h          ← EventType enum + structs de evento + union Event
│   ├── window.h         ← NativeWindow (opaque), EventCallbackFn, API pública (anvl_platform_window_*)
│   ├── window_backend.h ← WindowBackend vtable interface (7 funções)
│   ├── Linux/
│   │   ├── linux_window.c      ← define NativeWindow (vtable pattern), detecta X11/Wayland
│   │   ├── X11/
│   │   │   ├── x11_backend.c   ← backend X11/XCB completo (event capturing)
│   │   │   └── x11_backend.h
│   │   └── Wayland/
│   │       ├── wayland_backend.c  ← backend Wayland completo (xdg-shell, shm, keyboard, pointer)
│   │       ├── wayland_backend.h
│   │       ├── xdg_shell_client_protocol.c/h
│   │       └── xdg_shell_decoration_protocol.c/h
│   └── Windows/
│       └── win32_window.c     ← backend Win32: WNDCLASS, HWND, message loop, eventos mapeados
├── Platform/
│   └── platform_detection.h   ← detecção de compiler/OS (MSVC/GCC, Win/Linux/macOS)
└── Tools/
    ├── logger.h         ← LogLevel enum + anvlLog*/ANVIL_CORE_* macros
    └── logger.c         ← timestamp, cores ANSI, fprintf(stderr)
```

### Tarefas do Roadmap vs. Implementação Real

| Seção | Tarefa | Status | Evidência na Codebase |
|-------|--------|--------|----------------------|
| **1.2 Eventos de Entrada** | Captura teclado/mouse/janela | ✅ Concluída | `Windowing/event.h` (8 tipos), `win32_window.c` (WM_* mapeados) |
| **1.3 Logging** | Sistema com níveis | ✅ Concluída | `logger.h/c` (6 níveis + macros core/client) |
| **1.1 Gerenciamento de Janelas** | Criação/redimensionamento/fechamento | ✅ Concluída | Backends Win32/X11/Wayland completos; opaque pointer em `window.h`; vtable `WindowBackend` validada em runtime |
| 1.4 I/O de Arquivos | Abstração filesystem | ❌ Não iniciado | Nenhum arquivo em `src/` relacionado |
| 1.5 Propagação de Eventos | Layer System | ❌ Não iniciado | Callback vai direto para `anvl_application_on_event()` — sem stack de layers |

### Seções 2–8 do Roadmap
**Nenhum arquivo existe** na codebase atual. Renderização, Assets, Física, Áudio, ECS, Editor e Otimizações estão em branco.

---

## 🔧 Refatoração: Barreiras de Abstração (SICP) — Diagnóstico Real

### O que está correto na codebase atual

**`NativeWindow` usa opaque pointer corretamente:**
- `Windowing/window.h` linha 8: `typedef struct NativeWindow NativeWindow;` — forward declaration, tipo incompleto.
- `Windowing/Linux/linux_window.c` linha 8-12: definição concreta (`struct NativeWindow { const WindowBackend* backend; void* backend_data; }`) só existe no `.c` do backend Linux.
- `Windowing/Windows/win32_window.c`: definição concreta diferente (`struct NativeWindow { HWND hwnd; ... }`) só existe no `.c` do backend Win32.
- Consumers que incluem `window.h` **não podem acessar membros** — o padrão pImpl está aplicado corretamente.

**Vtable `WindowBackend` bem aplicada:**
- `Windowing/window_backend.h`: interface com 7 ponteiros de função.
- `x11_backend()` e `wayland_backend()` retornam `const WindowBackend*` para instâncias `static`.
- `linux_window.c` acessa via `window->backend->function_name(...)`.
- Todos os 7 ponteiros preenchidos em ambos os backends; vtable imutável após inicialização.

### Problemas Reais de Barreira de Abstração — Executados / Pendentes

~~#### 1. Camada de Plataforma conhece struct interno do Core (inversão de dependência)~~ ✅ **Executado via P1**
- Event types definidos em `Windowing/event.h` (módulo Windowing, não Core).
- Callback usa tipo definido por Windowing, não pelo Core.

~~#### 2. Detecção de platform via PCH~~ ✅ **Executado via P2**
- `#include "Platform/platform_detection.h"` removido do PCH e isolado em `logger.c`.

~~#### 3. Separação de responsabilidades: Windowing ≠ Platform~~ ✅ **Executado via P4 Fase 2**
- `Platform/` reduzido a SOMENTE `platform_detection.h` (detecção de compiler/OS).
- `Windowing/` contém toda a abstração de janela + event loop.
- Critério aplicado: "Se não menciona 'sistema operacional', não pertence a Platform/."

~~#### 4. Wayland stub e macOS sem backend~~ ✅/❌ **Parcial**
- ✅ X11/XCB completo em `Windowing/Linux/X11/x11_backend.c`
- ✅ Wayland completo: xdg-shell, shared memory buffers, keyboard, pointer, server-side decorations
- ❌ macOS sem backend

#### 5. Callback passa `Event` por valor (copia da union inteira) — **Pendente**
**Arquivo:** `Windowing/window.h`, linha 11:
```c
typedef void (*EventCallbackFn)(Event event);  // ← copia toda a union de Event
```
**Impacto:** Cada evento copia ~32-40 bytes (union + EventType + bool). Em teoria, não é crítico para eventos de janela (baixa frequência), mas é um smell de API.
**Correção sugerida:** Mudar para `const Event*` ou `Event*` (ownership transfer).

---

### Plano de Refatoração Concreto — Executado

#### Passo A: Quebrar dependência Platform → Core no callback ✅ **Executado (P1)**
- Event types definidos em `Windowing/event.h`
- Callback usa tipo definido por Windowing, não pelo Core

#### Passo B: Isolar detecção de platform do PCH ✅ **Executado (P2)**
- `#include "Platform/platform_detection.h"` removido do PCH
- Movido para `logger.c`, onde as macros são realmente consumidas

#### Passo C: Separação Windowing ≠ Platform ✅ **Executado (P4 Fase 2)**
- `Platform/platform.h` → `Windowing/window.h`
- `Platform/event.h` → `Windowing/event.h`
- `Platform/typedefs.h` → `Core/typedefs.h`
- `Platform/Linux/` → `Windowing/Linux/`
- `Platform/Windows/` → `Windowing/Windows/`
- `Platform/` reduzido a `platform_detection.h`

---

## 🏗️ Tarefas de Implementação (Baseado na Codebase Real)

### P1 — Quebrar acoplamento Platform → Core no callback
**Estado atual:** ✅ Concluído — `event.h` em `Windowing/event.h`, `window.h` em `Windowing/window.h`.  
**Arquivos afetados:** `Windowing/event.h`, `Windowing/window.h`, `Core/application.h`, `Windowing/Windows/win32_window.c`

- [x] Definir tipos de evento em `Windowing/event.h` (EventType, Event structs, union)
- [x] Alterar `EventCallbackFn` para usar `Event` definido por Windowing
- [x] `win32_window.c` constrói `Event` sem incluir Core diretamente
- [x] `application.c` inclui `Windowing/event.h` e usa `Event` diretamente

### P2 — Remover platform_detection do PCH
**Estado atual:** ✅ Concluído — `platform_detection.h` removido do PCH e movido para `logger.c` onde é consumido.  
**Arquivos afetados:** `anvlpch.h`, `Tools/logger.c`

- [x] Remover `#include "Platform/platform_detection.h"` do PCH
- [x] Adicionar `#include "Platform/platform_detection.h"` dentro de `logger.c` (onde as macros são realmente usadas para `localtime_s` vs `localtime_r`)
- [x] Nenhum outro arquivo precisa das macros — o PCH agora é 100% cross-platform

### P3 — Backend Linux (stub ou mínimo)
**Estado atual:** ✅ Concluída — backends X11/XCB e Wayland completos com event capturing.  
**Arquivos afetados:** `Windowing/Linux/linux_window.c`, `Windowing/Linux/X11/x11_backend.c`, `Windowing/Linux/Wayland/wayland_backend.c`

- [x] Criar stub com `#error` para progresso incremental
- [x] Implementar X11/XCB mínimo com event capturing (`xcb_connect`, `xcb_create_window`, `xcb_map_window`, `xcb_poll_for_event`)
- [x] Implementar Wayland mínimo (`wl_display`, `wl_registry`, surface creation, event loop)

### P4 — Refatoração de Arquitetura: Separação por Responsabilidades
**Estado atual:** ✅ Concluída — `Windowing/` separado de `Platform/`. Estrutura validada em runtime.

#### Fase 1 — Implementação dos Backends
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
  → ✅ **Correto** — padrão vtable bem aplicado: `x11_backend()` e `wayland_backend()` retornam `const WindowBackend*` para instâncias `static`; `linux_window.c` acessa via `window->backend->function_name(...)`; todos os 7 ponteiros de função preenchidos em ambos os backends; vtable imutável após inicialização.

#### Fase 2 — Separação Windowing ≠ Platform
**Estado atual:** ✅ Concluída — `Windowing/` criado com toda a abstração de janela + event loop; `Platform/` reduzido a `platform_detection.h`.

- [x] Mover `window.h` de `Platform/` para `Windowing/`
- [x] Mover `event.h` de `Platform/` para `Windowing/`
- [x] Mover `typedefs.h` de `Platform/` para `Core/`
- [x] Mover `window_backend.h` de `Platform/Linux/` para `Windowing/`
- [x] Mover backends X11/Wayland para `Windowing/Linux/`
- [x] Mover backend Win32 para `Windowing/Windows/`
- [x] Reduzir `Platform/` ao mínimo (apenas `platform_detection.h`)
- [x] Atualizar includes e paths de todos os consumers
- [x] `premake5.lua` funciona sem alterações (globs capturam recursivamente)

---

### P5 — I/O de Arquivos abstrato (Novo)
**Estado:** Zero implementação.  
**Dependência:** `Platform/` está estável (primitivas OS puras), então FileSystem pode nascer na estrutura correta.

- [ ] Criar `src/FileSystem/filesystem.h`:
```c
typedef struct FileHandle FileHandle;
FileHandle* anvl_filesystem_open(const char* path, const char* mode);
uint64 anvl_filesystem_read(FileHandle* file, void* buffer, uint64 size);
uint64 anvl_filesystem_write(FileHandle* file, const void* buffer, uint64 size);
bool anvl_filesystem_close(FileHandle* file);
bool anvl_filesystem_exists(const char* path);
```
- [ ] Criar `src/FileSystem/Windows/windows_filesystem.c` + `src/FileSystem/Linux/linux_filesystem.c`

### P6 — Layer System / Event Dispatcher (Novo)

**Estado atual:** Callback direto em `application.c` (linha 34):
```c
anvl_platform_set_window_event_callback(app->window, anvl_application_on_event);
```
Sem stack, sem layers.

- [ ] Criar `src/Core/event_layer.h`:
```c
typedef struct EventLayer {
    const char* name;
    void (*OnEvent)(struct EventLayer* layer, Event event);
    struct EventLayer* next;
} EventLayer;

void anvl_event_dispatcher_push_layer(EventLayer* layer);
void anvl_event_dispatcher_pop_layer(EventLayer* layer);
void anvl_event_dispatcher_dispatch(Event event);  // chama layers em ordem inversa
```
- [ ] Atualizar `application.c` para usar dispatcher ao invés de callback direto
- [ ] Adicionar `anvl_application_on_event` como camada default

### P7 — Corrigir callback: `Event` por valor → `const Event*` (Opcional)
**Estado atual:** `EventCallbackFn` copia a union inteira de `Event` (~32-40 bytes) a cada evento.

- [ ] Mudar `EventCallbackFn` de `(Event event)` para `(const Event* event)`
- [ ] Atualizar todos os consumers: `application.c`, `win32_window.c`, `x11_backend.c`, `wayland_backend.c`
- [ ] Atualizar `WindowCloseEvent` para ter dados reais (atualmente é `{ char _pad[1]; }`)

---

## 📊 Resumo Executivo (Baseado na Codebase Real)

| Prioridade | Tarefa | Arquivos Afetados | Tipo |
|------------|--------|-------------------|------|
| ~~**P0**~~ | ~~Quebrar acoplamento Platform → Core no callback~~ | ~~`Windowing/window.h`, `Windowing/Windows/win32_window.c`~~ | ✅ Concluída |
| ~~**P1**~~ | ~~Isolar detecção de platform do PCH~~ | ~~`anvlpch.h`, `Tools/logger.c`~~ | ✅ Concluída |
| ~~**P2**~~ | ~~Backend Linux (stub ou mínimo)~~ | ~~`Windowing/Linux/linux_window.c`, `X11/*`, `Wayland/*`~~ | ✅ Concluída |
| ~~**P3**~~ | ~~Refatoração de Arquitetura (separação por responsabilidades)~~ | ~~Move: `window.h`, `event.h`, backends → `Windowing/`~~ | ✅ Concluída |
| **P4** | I/O de Arquivos abstrato | Novo: `src/FileSystem/filesystem.h` + backends por plataforma | Implementação |
| **P5** | Layer System / Event Dispatcher | Novo: `src/Core/event_layer.h`, `src/Core/event_layer.c` | Implementação |
| **P6** | Corrigir callback: `Event` por valor → `const Event*` | `Windowing/window.h`, `application.c`, backends | Refatoração |
