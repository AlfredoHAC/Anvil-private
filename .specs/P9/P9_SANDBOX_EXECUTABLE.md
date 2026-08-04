# P9 — Sandbox Executable (Consumer Application)

> **Status:** ✅ Em implementação  
> **Dependência:** P8 (forward declarations + assertions)  
> **Módulo:** Novo — `example/Sandbox/` (dentro do ForgeCore)  
> **Tipo:** Novo executável + refatoração do build

---

## 1. Contexto e Motivação

### O problema atual

O entry point do engine é `src/anvil.c` — um arquivo dentro do módulo Anvil que contém `main()`. Isso viola a separação de módulos:

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

| Como `src/anvil.c` (atual) | Como `Sandbox/main.c` (separado) |
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

### 2.1 Estrutura de Diretórios

```
example/Sandbox/
├── src/
│   ├── main.c              ← entry point: main() + engine init + shutdown
│   ├── sandbox_layer.h     ← API pública: create / destroy (autocontida)
│   └── sandbox_layer.c     ← implementação da sandbox layer
├── scripts/
│   ├── build.bat           ← Windows build script
│   └── build.sh            ← Linux build script
├── CMakeLists.txt
└── .gitignore
```

A Sandbox layer é **autocontida**: ela se adiciona e se remove da layer stack. O `main()` não gerencia a stack manualmente.

**Princípio:** Assim como a `Application` gerencia sua própria janela (criação, callback, destroy), a Sandbox layer gerencia seu próprio ciclo de vida na stack.

### 2.2 `example/Sandbox/src/sandbox_layer.h` — API Pública

```c
#ifndef SANDBOX_LAYER_HEADER
#define SANDBOX_LAYER_HEADER

#include "Core/layer.h"

Layer* anvl_sandbox_layer_create();
void   anvl_sandbox_layer_destroy(Layer* layer);

#endif // !SANDBOX_LAYER_HEADER
```

- `anvl_sandbox_layer_create()` — aloca, inicializa, **empilha** na stack, retorna `Layer*`.
- `anvl_sandbox_layer_destroy(Layer* layer)` — **remove** da stack, libera memória.

### 2.3 `example/Sandbox/src/sandbox_layer.c` — Implementação

```c
#include "anvlpch.h"

#include "sandbox_layer.h"
#include "Windowing/event.h"
#include "Tools/logger.h"

// ---------------------------------------------------------------------------
// Sandbox Layer internals
// ---------------------------------------------------------------------------

typedef struct SandboxLayer
{
    Layer  base;
    uint32 frame_count;
} SandboxLayer;

static void _sandbox_on_update(Layer* layer);
static void _sandbox_on_event(Layer* layer, Event* event);

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

Layer* anvl_sandbox_layer_create()
{
    SandboxLayer* self = malloc(sizeof(SandboxLayer));
    if (!self) { return NULL; }

    self->frame_count = 0;
    self->base = (Layer){
        .name      = "Sandbox_Layer",
        .on_update = _sandbox_on_update,
        .on_event  = _sandbox_on_event,
    };

    anvl_layer_stack_push(&self->base);

    return &self->base;
}

void anvl_sandbox_layer_destroy(Layer* layer)
{
    ANVIL_ASSERT(layer != NULL);

    anvl_layer_stack_remove(layer);
    free(layer);
}

// ---------------------------------------------------------------------------
// Callbacks
// ---------------------------------------------------------------------------

static void _sandbox_on_update(Layer* layer)
{
    SandboxLayer* self = (SandboxLayer*)layer;
    self->frame_count++;

    if (self->frame_count % 60 == 0)
    {
        ANVIL_CORE_DEBUG("Sandbox: frame %u", self->frame_count);
    }
}

static void _sandbox_on_event(Layer* layer, Event* event)
{
    (void)layer;

    switch (event->type)
    {
        case ANVL_EVENT_TYPE_KEY_PRESS:
            switch (event->key_press.key_code)
            {
                case 41: // ESC
                    ANVIL_CORE_INFO("Sandbox: ESC pressed, exiting.");
                    event->handled = true;
                    break;
                case 19: // R
                    ANVIL_CORE_INFO("Sandbox: R pressed, reset.");
                    break;
                default: break;
            }
            break;
        case ANVL_EVENT_TYPE_WINDOW_RESIZE:
            ANVIL_CORE_DEBUG("Sandbox: resize %ux%u",
                             event->window_resize.width,
                             event->window_resize.height);
            break;
        default: break;
    }
}
```

**Por que `sandbox_layer.c` e não tudo em `main.c`?**

- Separação de responsabilidades: `main.c` orquestra o engine, `sandbox_layer.c` implementa a layer.
- Padrão consistente com `Core/application.c` (application orquestra, mas a lógica interna fica no `.c`).
- Facilita testes futuros: a layer pode ser testada independentemente do `main()`.

### 2.4 `example/Sandbox/src/main.c` — Entry Point

```c
#include "anvlpch.h"

#include "Core/application.h"
#include "Core/layer.h"
#include "sandbox_layer.h"

int main()
{
    const ApplicationOptions opts = {
        .name     = "Forge Sandbox",
        .width    = 1280,
        .height   = 720,
    };

    Application* app = anvl_application_init(opts);
    if (!app) { return 1; }

    Layer* sandbox = anvl_sandbox_layer_create();
    if (!sandbox)
    {
        ANVIL_CORE_ERROR("Failed to create sandbox layer.");
        anvl_application_shutdown(app);
        return 1;
    }

    anvl_application_run(app);

    anvl_sandbox_layer_destroy(sandbox);
    anvl_application_shutdown(app);

    return 0;
}
```

**Fluxo:**
1. `anvl_application_init` — cria janela, empilha a application layer interna.
2. `anvl_sandbox_layer_create` — aloca, empilha **acima** da application layer.
3. `anvl_application_run` — loop de update + events (LIFO: sandbox primeiro).
4. `anvl_sandbox_layer_destroy` — remove sandbox da stack, free.
5. `anvl_application_shutdown` — remove application layer, destroy janela, free.

**Nota:** A ordem de shutdown é importante. A sandbox é removida **antes** do shutdown do framework, para que a application layer (interna) processe eventos finais antes do destroy da janela.

### 2.5 Remoção de `src/anvil.c`

`src/anvil.c` é removido. Não há mais entry point dentro do Anvil.

**Antes:**
```
src/
├── anvil.c              ← main() — REMOVER
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
├── main.c              ← main() — NOVO
├── sandbox_layer.h     ← API pública — NOVO
└── sandbox_layer.c     ← implementação — NOVO
```

### 2.6 Build System (CMakeLists.txt)

O Anvil vira uma **static library**. A Sandbox é um **console app** que linka contra ela.

#### ForgeCore/CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.20)
project(ForgeCore LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Submodules
add_subdirectory(Anvil)

# Executables
add_subdirectory(example/Sandbox)
```

#### example/Sandbox/CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.20)
project(Sandbox LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

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
    src/sandbox_layer.c
)

target_link_libraries(Sandbox PRIVATE Anvil)

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
```

**Notas:**
- O Anvil é compilado como `StaticLib` (`.lib`/`.a`).
- A Sandbox é um `ConsoleApp` que linka contra o Anvil.
- O fallback `add_subdirectory` permite build standalone do Sandbox.
- Scripts de build em `example/Sandbox/scripts/` usam `build-script-builder` skill.

---

## 3. Estrutura de Arquivos

### 3.1 Novos Arquivos

```
example/Sandbox/
├── src/
│   ├── main.c                  ← entry point: main() + engine init + shutdown
│   ├── sandbox_layer.h         ← API pública: anvl_sandbox_layer_create / destroy
│   └── sandbox_layer.c         ← implementação da sandbox layer
├── scripts/
│   ├── build.bat               ← Windows build script
│   └── build.sh                ← Linux build script
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
- Linux: `X11`, `xcb`, `wayland-client` (já linkados no CMake).
- Windows: `user32`, `gdi32`, `opengl32` (já linkados no CMake).

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
    ├── anvlpch.h          (Core/typedefs.h, Tools/logger.h, Tools/assert.h)
    ├── Core/application.h
    ├── Core/layer.h
    └── sandbox_layer.h
            └── Core/layer.h (re-exportado)

example/Sandbox/src/sandbox_layer.c
    ├── anvlpch.h
    ├── sandbox_layer.h
    ├── Windowing/event.h
    └── Tools/logger.h
```

---

## 5. O que a Sandbox faz (e não faz)

### Faz:
- `main()` — entry point.
- Inicializa o engine (`anvl_application_init`).
- Empilha a sandbox layer (autocontida: create push, destroy remove).
- Roda o loop (`anvl_application_run`).
- Shutdown limpo (destroy sandbox → shutdown framework).

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

### Etapa 1: Criar `example/Sandbox/src/sandbox_layer.h` + `sandbox_layer.c`

- Header com `anvl_sandbox_layer_create()` e `anvl_sandbox_layer_destroy()`.
- Implementação: alocação, push na stack, callbacks (update + event).
- `ANVIL_ASSERT` no destroy para verificar pointer válido.

### Etapa 2: Criar `example/Sandbox/src/main.c`

- Includes do engine.
- `main()` que inicializa, cria sandbox, roda, destroy sandbox, shutdown.

### Etapa 3: Remover `src/anvil.c`

Excluir `src/anvil.c`. Não há mais entry point no Anvil.

### Etapa 4: Criar `example/Sandbox/CMakeLists.txt`

- Configurar projeto Sandbox como executável.
- Linkar contra Anvil (`target_link_libraries(Sandbox PRIVATE Anvil)`).
- Adicionar fallback para build do Anvil se não encontrado.

### Etapa 5: Atualizar `ForgeCore/CMakeLists.txt`

Adicionar `add_subdirectory(example/Sandbox)`.

### Etapa 6: Criar scripts de build

Usar `build-script-builder` skill para criar `example/Sandbox/scripts/build.bat` e `build.sh`.

### Etapa 7: Validação

- Rodar `scripts/build.bat debug` (ou `build.sh debug`).
- Compilar e verificar que `Sandbox.exe` é gerado.
- Executar: janela abre, log de frames aparece, ESC fecha.
- Verificar que `Anvil.lib`/`libAnvil.a` é gerado.

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
| Forward declarations em 4 arquivos | `sandbox_layer.c` tem forward decls internas (`_sandbox_on_update`, `_sandbox_on_event`) |
| Assertion system (`Tools/assert.h`) | Sandbox usa `ANVL_ASSERT` via PCH (no `anvl_sandbox_layer_destroy`) |

**P8 e P9 são independentes:** P8 cuida da qualidade interna do engine (forward declarations + assertions). P9 cuida da separação de módulos (sandbox como executável consumer). A sandbox layer existe apenas na P9.

---

## 9. Resumo

| Item | Detalhe |
|------|---------|
| **Arquivos novos** | 5 (`example/Sandbox/src/main.c`, `example/Sandbox/src/sandbox_layer.h`, `example/Sandbox/src/sandbox_layer.c`, `example/Sandbox/CMakeLists.txt`, `example/Sandbox/.gitignore`) + 2 scripts |
| **Arquivos removidos** | 1 (`src/anvil.c`) |
| **Arquivos modificados** | 2 (`ForgeCore/CMakeLists.txt`, `Anvil/CMakeLists.txt`) |
| **Novas dependências** | Nenhuma |
| **Novos links** | `Sandbox` linka contra `Anvil` (static lib) |
| **PCH alterado** | Não (Sandbox reutiliza o PCH do Anvil) |
| **API pública** | Nenhuma mudança no engine (sandbox layer é consumer code) |
| **Módulos afetados** | Novo: `Sandbox/`; Anvil vira static lib |
| **Build** | 2 targets: `Anvil` (StaticLib) + `Sandbox` (ConsoleApp) |
