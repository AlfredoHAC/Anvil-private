# P9 — Sandbox Executable (Consumer Application)

> **Status:** ❌ Não iniciada  
> **Dependência:** P8 (forward declarations + assertions)  
> **Módulo:** Novo — `Sandbox/` (fora de `src/`)  
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
Sandbox/
├── main.c                  ← entry point: main() + engine init + shutdown
├── sandbox_layer.h         ← API pública: create / destroy (autocontida)
└── sandbox_layer.c         ← implementação da sandbox layer
```

A Sandbox layer é **autocontida**: ela se adiciona e se remove da layer stack. O `main()` não gerencia a stack manualmente.

**Princípio:** Assim como a `Application` gerencia sua própria janela (criação, callback, destroy), a Sandbox layer gerencia seu próprio ciclo de vida na stack.

### 2.2 `Sandbox/sandbox_layer.h` — API Pública

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

### 2.3 `Sandbox/sandbox_layer.c` — Implementação

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

### 2.4 `Sandbox/main.c` — Entry Point

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

Sandbox/
├── main.c              ← main() — NOVO
└── sandbox_layer.h     ← API pública — NOVO
```

### 2.6 Build System (premake5.lua)

O Anvil vira uma **static library**. A Sandbox é um **console app** que linka contra ela.

```lua
workspace "ForgeCore"

    architecture "x86_64"
    startproject "Sandbox"

    configurations {
        "Debug",
        "Optimized",
        "Release"
    }

-- ---------------------------------------------------------------------------
-- Anvil: static library
-- ---------------------------------------------------------------------------
project "Anvil"
    kind "StaticLib"
    language "C"

    pchheader "anvlpch.h"
    pchsource "./src/anvlpch.c"

    targetdir "./bin/%{cfg.buildcfg}/lib/"
    objdir "./bin/obj/%{cfg.buildcfg}/anvil/"

    files {
        "./src/**.h",
        "./src/**.c"
    }

    includedirs {
        "./src/",
        "./src/**"
    }

    filter "system:windows"
        systemversion "latest"
        cdialect "C11"
        links { "user32", "gdi32", "opengl32" }
        defines { "_CRT_SECURE_NO_WARNINGS" }
        removefiles { "./**/Linux/**" }

    filter "system:linux"
        systemversion "latest"
        cdialect "gnu11"
        links { "X11", "xcb", "wayland-client" }
        defines { "_GNU_SOURCE" }
        removefiles { "./**/Windows/**" }

    filter "configurations:Debug"
        defines "ANVIL_CONFIG_DEBUG"
        runtime "Debug"
        symbols "On"
        sanitize { "Address", "Fuzzer" }
        editandcontinue "Off"
        incrementallink "Off"
        runtimechecks "Off"

    filter "configurations:Optimized"
        defines "ANVIL_CONFIG_OPTIMIZED"
        runtime "Release"
        optimize "On"

    filter "configurations:Release"
        defines "ANVIL_CONFIG_RELEASE"
        runtime "Release"
        optimize "Full"

-- ---------------------------------------------------------------------------
-- Sandbox: console application (consumer of Anvil)
-- ---------------------------------------------------------------------------
project "Sandbox"
    kind "ConsoleApp"
    language "C"

    pchheader "anvlpch.h"
    pchsource "./src/anvlpch.c"

    targetdir "./bin/%{cfg.buildcfg}/"
    objdir "./bin/obj/%{cfg.buildcfg}/sandbox/"

    files {
        "./Sandbox/**.h",
        "./Sandbox/**.c"
    }

    includedirs {
        "./src/",
        "./src/**",
        "./Sandbox/"
    }

    links { "Anvil" }

    filter "system:windows"
        systemversion "latest"
        cdialect "C11"
        links { "user32", "gdi32", "opengl32" }
        defines { "_CRT_SECURE_NO_WARNINGS" }

    filter "system:linux"
        systemversion "latest"
        cdialect "gnu11"
        links { "X11", "xcb", "wayland-client" }
        defines { "_GNU_SOURCE" }

    filter "configurations:Debug"
        defines "ANVIL_CONFIG_DEBUG"
        runtime "Debug"
        symbols "On"

    filter "configurations:Optimized"
        defines "ANVIL_CONFIG_OPTIMIZED"
        runtime "Release"
        optimize "On"

    filter "configurations:Release"
        defines "ANVIL_CONFIG_RELEASE"
        runtime "Release"
        optimize "Full"
```

**Mudanças no premake:**
- `startproject` muda de `"Anvil"` para `"Sandbox"`.
- `Anvil`: `kind "ConsoleApp"` → `kind "StaticLib"`.
- Novo projeto `Sandbox`: `kind "ConsoleApp"`, `links { "Anvil" }`.
- `targetdir` do Anvil muda para `./bin/%{cfg.buildcfg}/lib/` (padrão de libs).
- `objdir` do Sandbox muda para `./bin/obj/%{cfg.buildcfg}/sandbox/` (evita colisão com Anvil).
- `files` do Sandbox aponta para `./Sandbox/**` (não `./src/**`).
- `includedirs` do Sandbox inclui `./Sandbox/` além de `./src/`.
- `removefiles` **removido** do bloco Sandbox (não se aplica a um único arquivo).
- `links` do Sandbox herda as dependências de link do Anvil via `links { "Anvil" }`.

---

## 3. Estrutura de Arquivos

### 3.1 Novos Arquivos

```
Sandbox/
├── main.c                  ← entry point: main() + engine init + shutdown
├── sandbox_layer.h         ← API pública: anvl_sandbox_layer_create / destroy
└── sandbox_layer.c         ← implementação da sandbox layer
```

### 3.2 Arquivos Removidos

```
src/anvil.c              ← main() removido (Sandbox assume)
```

### 3.3 Arquivos Modificados

| Arquivo | Mudança |
|---------|---------|
| `premake5.lua` | Anvil vira StaticLib; novo projeto Sandbox; startproject muda; removefiles removido do Sandbox |

---

## 4. Integração com a Codebase Existente

### 4.1 Dependências de Link

**Anvil (static lib):**
- Linux: `X11`, `xcb`, `wayland-client` (já linkados no premake).
- Windows: `user32`, `gdi32`, `opengl32` (já linkados no premake).

**Sandbox (console app):**
- Linka contra `Anvil` (a static lib).
- Herda as dependências de link do Anvil via `links { "Anvil" }`.
- Linux: `X11`, `xcb`, `wayland-client` (via Anvil).
- Windows: `user32`, `gdi32`, `opengl32` (via Anvil).

### 4.2 PCH

A Sandbox reutiliza o PCH do Anvil (`anvlpch.h` + `anvlpch.c`). Isso evita duplicação e garante que a Sandbox tenha acesso a todos os tipos e macros do engine.

**Nota:** A Sandbox **não** é parte do Anvil. Ela usa o PCH do Anvil como referência, mas compila separadamente. O `pchsource` aponta para `./src/anvlpch.c` — o Premake gera o PCH uma vez e ambos os projetos o reutilizam.

### 4.3 Ordem de Build

```
1. Anvil (StaticLib)  → bin/Debug/lib/Anvil.lib (Windows)
                                             bin/Debug/lib/libAnvil.a (Linux)
2. Sandbox (ConsoleApp) → bin/Debug/Sandbox.exe (Windows)
                                       Sandbox (Linux)
```

O Premake resolve a dependência automaticamente via `links { "Anvil" }`.

### 4.4 Dependências de Include

```
Sandbox/main.c
    ├── anvlpch.h          (Core/typedefs.h, Tools/logger.h, Tools/assert.h)
    ├── Core/application.h
    ├── Core/layer.h
    └── Sandbox/sandbox_layer.h
            └── Core/layer.h (re-exportado)

Sandbox/sandbox_layer.c
    ├── anvlpch.h
    ├── Sandbox/sandbox_layer.h
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

### Etapa 1: Criar `Sandbox/sandbox_layer.h` + `sandbox_layer.c`

- Header com `anvl_sandbox_layer_create()` e `anvl_sandbox_layer_destroy()`.
- Implementação: alocação, push na stack, callbacks (update + event).
- `ANVIL_ASSERT` no destroy para verificar pointer válido.

### Etapa 2: Criar `Sandbox/main.c`

- Includes do engine.
- `main()` que inicializa, cria sandbox, roda, destroy sandbox, shutdown.

### Etapa 3: Remover `src/anvil.c`

Excluir `src/anvil.c`. Não há mais entry point no Anvil.

### Etapa 4: Atualizar `premake5.lua`

- Anvil: `kind "ConsoleApp"` → `kind "StaticLib"`.
- Novo projeto `Sandbox`: `kind "ConsoleApp"`, `links { "Anvil" }`.
- `startproject "Sandbox"`.
- Ajustar `targetdir` e `objdir` para evitar colisão.
- Remover `removefiles` do bloco Sandbox.

### Etapa 5: Validação

- Rodar `premake5.lua gmake` (ou generate correspondente).
- Compilar e verificar que `Sandbox` é gerado.
- Executar: janela abre, log de frames aparece, ESC fecha.
- Verificar que `Anvil` é gerado como `.lib`/`.a`.

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
| **Arquivos novos** | 3 (`Sandbox/main.c`, `Sandbox/sandbox_layer.h`, `Sandbox/sandbox_layer.c`) |
| **Arquivos removidos** | 1 (`src/anvil.c`) |
| **Arquivos modificados** | 1 (`premake5.lua`) |
| **Novas dependências** | Nenhuma |
| **Novos links** | `Sandbox` linka contra `Anvil` (static lib) |
| **PCH alterado** | Não (Sandbox reutiliza o PCH do Anvil) |
| **API pública** | Nenhuma mudança no engine (sandbox layer é consumer code) |
| **Módulos afetados** | Novo: `Sandbox/`; Anvil vira static lib |
| **Build** | 2 targets: `Anvil` (StaticLib) + `Sandbox` (ConsoleApp) |
