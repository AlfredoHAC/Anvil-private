# P9 — Sandbox Executable (Consumer Application)

> **Status:** ✅ Concluída  
> **Dependência:** P8 (forward declarations + assertions)  
> **Módulo:** Novo — `example/Sandbox/` (dentro do ForgeCore)  
> **Tipo:** Novo executável + refatoração do build

---

## 1. Contexto e Motivação

### O problema atual

O entry point do engine era `src/anvil.c` — um arquivo dentro do módulo Anvil que continha `main()`. Isso violava a separação de módulos:

```
src/
├── anvil.c              ← main() DENTRO do módulo Anvil ❌
├── anvlpch.h
├── Core/
│   ├── application.c    ← framework do engine
│   └── ...
└── ...
```

O roadmap define **Sandbox** como o executável separado:

> **Sandbox** — Application executable. Responsável por: `main()`, engine initialization, module orchestration, experiments and validation.

### Por que Sandbox como executável separado?

| Como `src/anvil.c` (antigo) | Como `Sandbox/` (separado) |
|---|---|
| `main()` dentro do engine | `main()` fora do engine |
| Viola dependência: engine não deve saber de consumers | Respeita: Sandbox **depende** de Anvil |
| Não valida a API como consumer real | Valida a API escrevendo código de consumo real |
| Padrão errado para futuros consumers | Estabelece o padrão correto |
| Sem build separado | Build separado (static lib + console app) |

### Analogia

Pense no Anvil como uma **biblioteca** (como `libpthread` ou `libcurl`). Você não coloca o `main()` do seu programa dentro do `libcurl`. O Sandbox é o "seu programa" — ele usa o Anvil, não é parte dele.

---

## 2. Design

### 2.1 Estrutura de Diretórios (Implementada)

```
example/Sandbox/
├── src/
│   ├── main.c                      ← entry point: main() + engine init + shutdown
│   └── Layer/
│       ├── sandbox_layer.h          ← API pública: sndbx_attach_layer / sndbx_detach_layer
│       └── sandbox_layer.c          ← layer: state + callbacks + attach/detach
├── scripts/
│   ├── build.bat                    ← Windows build script
│   └── build.sh                     ← Linux build script
├── CMakeLists.txt
└── .gitignore
```

A Sandbox layer é **autocontida**: ela se adiciona e se remove da layer stack. O `main()` não gerencia a stack manualmente.

**Princípio:** Assim como a `Application` gerencia sua própria janela (criação, callback, destroy), a Sandbox layer gerencia seu próprio ciclo de vida na stack.

### 2.2 `example/Sandbox/src/Layer/sandbox_layer.h` — API Pública

```c
#ifndef SANDBOX_LAYER_HEADER
#define SANDBOX_LAYER_HEADER

#include <Core/typedefs.h>

void sndbx_attach_layer();
void sndbx_detach_layer();

#endif // SANDBOX_LAYER_HEADER
```

- `sndbx_attach_layer()` — empilha a sandbox na stack.
- `sndbx_detach_layer()` — remove da stack.

### 2.3 `example/Sandbox/src/Layer/sandbox_layer.c` — Layer Implementation

```c
#include <anvlpch.h>

#include "Layer/sandbox_layer.h"

#include <Core/layer.h>
#include <Windowing/event.h>

static void _on_sndbx_update(Layer* layer);
static void _on_sndbx_event(Layer* layer, Event* event);

static Layer sndbx_layer = {
    .name      = "Sandbox_Layer",
    .on_update = _on_sndbx_update,
    .on_event  = _on_sndbx_event,
};

void sndbx_attach_layer()
{
    ANVIL_DEBUG("SANDBOX", "Layer Attached.\n -> Stack length: %u", anvl_layer_stack_length());
    anvl_layer_stack_push(&sndbx_layer);
}

void sndbx_detach_layer()
{
    anvl_layer_stack_remove(&sndbx_layer);
    ANVIL_DEBUG("SANDBOX", "Layer Detached.\n -> Stack length: %u", anvl_layer_stack_length());
}

static void _on_sndbx_update(Layer* layer)
{
    // TODO: game logic
}

static void _on_sndbx_event(Layer* layer, Event* event)
{
    ANVIL_DEBUG("SANDBOX", "Event captured!");

    switch (event->type)
    {
        case ANVL_EVENT_TYPE_WINDOW_RESIZE:
            ANVIL_DEBUG("SANDBOX", "Window resize: %dx%d",
                        event->window_resize.width, event->window_resize.height);
            break;
        case ANVL_EVENT_TYPE_KEY_PRESS:
            ANVIL_DEBUG("SANDBOX", "Key press: %d (Mod: %d)",
                        event->key_press.key_code, event->key_press.modifier_set);
            break;
        case ANVL_EVENT_TYPE_KEY_RELEASE:
            ANVIL_DEBUG("SANDBOX", "Key release: %d (Mod: %d)",
                        event->key_release.key_code, event->key_release.modifier_set);
            break;
        case ANVL_EVENT_TYPE_MOUSE_MOVE:
            ANVIL_DEBUG("SANDBOX", "Mouse move: (%.1f,%.1f)",
                        event->mouse_move.x, event->mouse_move.y);
            break;
        case ANVL_EVENT_TYPE_MOUSE_BUTTON_CLICK:
            ANVIL_DEBUG("SANDBOX", "Mouse button click: %d (%.1f,%.1f)",
                        event->mouse_button_click.button_code,
                        event->mouse_button_click.x, event->mouse_button_click.y);
            break;
        case ANVL_EVENT_TYPE_MOUSE_BUTTON_RELEASE:
            ANVIL_DEBUG("SANDBOX", "Mouse button release: %d (%.1f,%.1f)",
                        event->mouse_button_release.button_code,
                        event->mouse_button_release.x, event->mouse_button_release.y);
            break;
        case ANVL_EVENT_TYPE_MOUSE_SCROLL:
            ANVIL_DEBUG("SANDBOX", "Mouse scroll: (%.1f,%.1f)",
                        event->mouse_scroll.x_offset, event->mouse_scroll.y_offset);
            break;
        default: return; break;
    }

    event->handled = true;
}
```

**Por que `sandbox_layer.c` e não tudo em `main.c`?**

- Separação de responsabilidades: `main.c` orquestra o engine, `sandbox_layer.c` implementa a layer.
- Padrão consistente com `Core/application.c` (application orquestra, mas a lógica interna fica no `.c`).
- Facilita testes futuros: a layer pode ser testada independentemente do `main()`.

### 2.4 `example/Sandbox/src/main.c` — Entry Point

```c
#include <anvlpch.h>

#include "Layer/sandbox_layer.h"

#include <Core/application.h>

int main()
{
    const ApplicationOptions opts = {
        .name   = "AnvilFramework",
        .width  = 1280,
        .height = 720,
    };

    Application* app = anvl_application_init(opts);
    if (!app) { return 1; }

    sndbx_attach_layer();

    anvl_application_run(app);

    sndbx_detach_layer();
    anvl_application_shutdown(app);
    app = NULL;

    return 0;
}
```

**Fluxo:**
1. `anvl_application_init` — cria janela, empilha a application layer interna.
2. `sndbx_attach_layer` — empilha a sandbox **acima** da application layer.
3. `anvl_application_run` — loop de update + events (LIFO: sandbox primeiro).
4. `sndbx_detach_layer` — remove sandbox da stack.
5. `anvl_application_shutdown` — remove application layer, destroy janela, free.

**Nota:** A ordem de shutdown é importante. A sandbox é removida **antes** do shutdown do framework, para que a application layer (interna) processe eventos finais antes do destroy da janela.

### 2.5 Remoção de `src/anvil.c`

`src/anvil.c` foi removido. Não há mais entry point dentro do Anvil.

**Antes:**
```
src/
├── anvil.c              ← main() — REMOVIDO
├── anvlpch.h
├── anvlpch.c
└── ...
```

**Depois:**
```
src/
├── anvlpch.h
├── anvlpch.c
└── ...

example/Sandbox/src/
├── main.c                  ← main() — NOVO
└── Layer/
    ├── sandbox_layer.h     ← API pública — NOVO
    └── sandbox_layer.c     ← layer implementation — NOVO
```

### 2.6 Build System (CMakeLists.txt)

O Anvil é uma **static library**. A Sandbox é um **console app** que linka contra ela.

#### ForgeCore/CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.20)
project(ForgeCore LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_subdirectory(Anvil)
add_subdirectory(example/Sandbox)
```

#### example/Sandbox/CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.20)
project(Sandbox LANGUAGES C)

# Export compile_commands.json for linters
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Try to find Anvil, or build it from source if not found
if(NOT TARGET Anvil)
    set(ANVIL_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../Anvil")
    if(EXISTS "${ANVIL_ROOT}/CMakeLists.txt")
        add_subdirectory("${ANVIL_ROOT}" Anvil)
    else()
        message(FATAL_ERROR "Anvil not found at ${ANVIL_ROOT}")
    endif()
endif()

add_executable(Sandbox
    src/main.c
    src/Layer/sandbox_layer.c
)

target_link_libraries(Sandbox PRIVATE Anvil)
target_include_directories(Sandbox PRIVATE src)

target_compile_definitions(Sandbox PRIVATE
    $<$<CONFIG:Debug>:ANVIL_CONFIG_DEBUG>
    $<$<CONFIG:RelWithDebInfo>:ANVIL_CONFIG_OPTIMIZED>
    $<$<CONFIG:Release>:ANVIL_CONFIG_RELEASE>
)

# Windows
if(WIN32)
    target_compile_definitions(Sandbox PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_link_libraries(Sandbox PRIVATE user32 gdi32 opengl32)
    target_compile_options(Sandbox PRIVATE /wd4201)
endif()

# Linux
if(UNIX AND NOT APPLE)
    target_compile_definitions(Sandbox PRIVATE _GNU_SOURCE)
    find_package(X11)
    if(X11_FOUND)
        target_link_libraries(Sandbox PRIVATE X11 xcb)
    endif()

    # Wayland client library
    find_library(WAYLAND_CLIENT_LIBRARY wayland-client)
    find_path(WAYLAND_CLIENT_INCLUDE_DIR wayland-client.h)
    if(WAYLAND_CLIENT_LIBRARY AND WAYLAND_CLIENT_INCLUDE_DIR)
        target_link_libraries(Sandbox PRIVATE ${WAYLAND_CLIENT_LIBRARY})
        target_include_directories(Sandbox PRIVATE ${WAYLAND_CLIENT_INCLUDE_DIR})
    endif()
endif()

# Sanitizers (Debug only)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(Sandbox PRIVATE
        $<$<CONFIG:Debug>:-fsanitize=address>
    )
    target_link_options(Sandbox PRIVATE
        $<$<CONFIG:Debug>:-fsanitize=address>
    )
endif()
```

**Notas:**
- O Anvil é compilado como `StaticLib` (`.lib`/`.a`).
- A Sandbox é um `ConsoleApp` que linka contra o Anvil.
- O fallback `add_subdirectory` permite build standalone do Sandbox.
- `target_include_directories(Sandbox PRIVATE src)` permite includes como `"Layer/sandbox_layer.h"`.
- Scripts de build em `example/Sandbox/scripts/` usam `build-script-builder` skill.

---

## 3. Estrutura de Arquivos

### 3.1 Novos Arquivos

```
example/Sandbox/
├── src/
│   ├── main.c                      ← entry point: main() + engine init + shutdown
│   └── Layer/
│       ├── sandbox_layer.h          ← API pública: sndbx_attach_layer / sndbx_detach_layer
│       └── sandbox_layer.c          ← layer: state + callbacks + attach/detach
├── scripts/
│   ├── build.bat                    ← Windows build script
│   └── build.sh                     ← Linux build script
├── CMakeLists.txt
└── .gitignore
```

### 3.2 Arquivos Removidos

```
src/anvil.c              ← main() removido (Sandbox assume)
```

### 3.3 Arquivos Modificados

| Arquivo | Mudança |
|---------|---------|
| `ForgeCore/CMakeLists.txt` | Adicionado `add_subdirectory(example/Sandbox)` |
| `Anvil/CMakeLists.txt` | Removido `src/anvil.c` das fontes |

---

## 4. Integração com a Codebase Existente

### 4.1 Dependências de Link

**Anvil (static lib):**
- Linux: `X11`, `xcb`, `wayland-client` (via `find_library`/`find_path`).
- Windows: `user32`, `gdi32`, `opengl32`.

**Sandbox (console app):**
- Linka contra `Anvil` (a static lib).
- Herda as dependências de link do Anvil via `target_link_libraries(Sandbox PRIVATE Anvil)`.
- Linux: `X11`, `xcb`, `wayland-client` (via Anvil).
- Windows: `user32`, `gdi32`, `opengl32` (via Anvil).

### 4.2 PCH

A Sandbox reutiliza o PCH do Anvil (`anvlpch.h` + `anvlpch.c`). Isso evita duplicação e garante que a Sandbox tenha acesso a todos os tipos e macros do engine.

**Nota:** A Sandbox **não** é parte do Anvil. Ela usa o PCH do Anvil como referência, mas compila separadamente. O CMake gera o PCH uma vez e ambos os projetos o reutilizam.

### 4.3 Ordem de Build

```
1. Anvil (StaticLib)  → build/Debug/Anvil.lib (Windows)
                                   build/Debug/libAnvil.a (Linux)
2. Sandbox (ConsoleApp) → build/Debug/Sandbox.exe (Windows)
                                   build/Debug/Sandbox (Linux)
```

O CMake resolve a dependência automaticamente via `add_subdirectory` e `target_link_libraries`.

### 4.4 Dependências de Include

```
example/Sandbox/src/main.c
    ├── anvlpch.h              (Core/typedefs.h, Tools/logger.h, Tools/assert.h)
    ├── Layer/sandbox_layer.h  (Core/typedefs.h)
    └── Core/application.h

example/Sandbox/src/Layer/sandbox_layer.c
    ├── anvlpch.h
    ├── Layer/sandbox_layer.h
    ├── Core/layer.h
    └── Windowing/event.h
```

---

## 5. O que a Sandbox faz (e não faz)

### Faz:
- `main()` — entry point em `src/main.c`.
- Inicializa o engine (`anvl_application_init`).
- Empilha a sandbox layer (autocontida: attach push, detach remove).
- Roda o loop (`anvl_application_run`).
- Shutdown limpo (detach sandbox → shutdown framework).
- Log de eventos de input (mouse, teclado, resize, scroll).

### Não faz:
- Renderização (Furnace não existe).
- Física, áudio, ECS (não existem).
- Editor (seção 7 do roadmap).
- Testes automatizados (futuro).

### Evolução futura:
- Adicionar `FurnaceLayer` quando o Furnace existir.
- Adicionar experimentos de renderização.
- Tornar-se o ponto de entrada para validação do engine.

---

## 6. Plano de Implementação (Passo a Passo)

### ✅ Etapa 1: Criar `example/Sandbox/src/Layer/sandbox_layer.h` + `sandbox_layer.c`

- `sandbox_layer.h`: API pública `sndbx_attach_layer()` e `sndbx_detach_layer()`.
- `sandbox_layer.c`: state da layer, forward declarations, callbacks (update + event), attach/detach.

### ✅ Etapa 2: Criar `example/Sandbox/src/main.c`

- `main()` que inicializa, attach sandbox, roda, detach, shutdown.

### ✅ Etapa 3: Remover `src/anvil.c`

`src/anvil.c` removido. Não há mais entry point no Anvil.

### ✅ Etapa 4: Criar `example/Sandbox/CMakeLists.txt`

- Configurar projeto Sandbox como executável.
- Linkar contra Anvil (`target_link_libraries(Sandbox PRIVATE Anvil)`).
- Adicionar fallback para build do Anvil se não encontrado.
- `target_include_directories(Sandbox PRIVATE src)`.

### ✅ Etapa 5: Atualizar `ForgeCore/CMakeLists.txt`

Adicionado `add_subdirectory(example/Sandbox)`.

### ✅ Etapa 6: Criar scripts de build

Criados `example/Sandbox/scripts/build.bat` e `build.sh` com:
- Seleção de configuração (debug/optimized/release/clean).
- Verificação de dependências (cmake, ninja).
- Build com Ninja (Linux) ou Visual Studio (Windows).
- Sanitizers (Address Sanitizer em debug).

### ✅ Etapa 7: Validação

- Build bem-sucedido em Linux (Ninja) e Windows (Visual Studio).
- Execução: janela abre, logs de eventos aparecem, ESC fecha.
- `Anvil.lib`/`libAnvil.a` gerado corretamente.

---

## 7. O que NÃO está incluído (Escopo Explícito)

| Funcionalidade | Status | Razão |
|----------------|--------|-------|
| Sandbox com renderização | Não incluído | Furnace não existe |
| Sandbox com física | Não incluído | Sistema de física não existe |
| Múltiplos executáveis sandbox | Não incluído | Um por vez; adicionar depois |
| Sandbox como DLL separada | Não incluído | Static lib é suficiente agora |
| Testes automatizados | Não incluído | Futuro |
| ImGui integrado | Não incluído | Futuro |

---

## 8. Relação com P8

| P8 (Quality of Life) | P9 (Sandbox Executable) |
|---|---|
| Forward declarations em 4 arquivos | `sandbox_layer.c` tem forward decls internas (`_on_sndbx_update`, `_on_sndbx_event`) |
| Assertion system (`Tools/assert.h`) | Sandbox usa `ANVIL_DEBUG`/`ANVIL_CORE_*` via PCH |

**P8 e P9 são independentes:** P8 cuida da qualidade interna do engine (forward declarations + assertions). P9 cuida da separação de módulos (sandbox como executável consumer). A sandbox layer existe apenas na P9.

---

## 9. Resumo

| Item | Detalhe |
|------|---------|
| **Arquivos novos** | 6 (`src/main.c`, `src/Layer/sandbox_layer.h`, `src/Layer/sandbox_layer.c`, `CMakeLists.txt`, `.gitignore`, 2 scripts) |
| **Arquivos removidos** | 1 (`src/anvil.c`) |
| **Arquivos modificados** | 2 (`ForgeCore/CMakeLists.txt`, `Anvil/CMakeLists.txt`) |
| **Novas dependências** | Nenhuma |
| **Novos links** | `Sandbox` linka contra `Anvil` (static lib) |
| **PCH alterado** | Não (Sandbox reutiliza o PCH do Anvil) |
| **API pública** | Nenhuma mudança no engine (sandbox layer é consumer code) |
| **Módulos afetados** | Novo: `Sandbox/`; Anvil vira static lib |
| **Build** | 2 targets: `Anvil` (StaticLib) + `Sandbox` (ConsoleApp) |
| **Validação** | ✅ Build funciona, janela abre, eventos de input processados |
