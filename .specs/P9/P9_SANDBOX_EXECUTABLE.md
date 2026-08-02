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
└── main.c              ← entry point: main() + engine init + sandbox layer
```

**Um único arquivo.** A Sandbox é simples: cria a aplicação, empilha a sandbox layer, roda o loop. Quando o Furnace chegar, a Sandbox ganhará uma `FurnaceLayer`.

### 2.2 `Sandbox/main.c`

```c
#include "anvlpch.h"
#include "Core/application.h"
#include "Core/layer.h"
#include "Windowing/event.h"
#include "Tools/logger.h"

// ---------------------------------------------------------------------------
// Sandbox Layer — demonstração do layer system
// ---------------------------------------------------------------------------

typedef struct SandboxLayer
{
    Layer  base;
    uint32 frame_count;
} SandboxLayer;

static void _sandbox_on_update(Layer* layer);
static void _sandbox_on_event(Layer* layer, Event* event);

static Layer* _sandbox_layer_create()
{
    SandboxLayer* self = malloc(sizeof(SandboxLayer));
    if (!self) { return NULL; }

    self->frame_count = 0;
    self->base = (Layer){
        .name      = "Sandbox_Layer",
        .on_update = _sandbox_on_update,
        .on_event  = _sandbox_on_event,
    };

    return &self->base;
}

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

// ---------------------------------------------------------------------------
// main()
// ---------------------------------------------------------------------------

int main()
{
    const ApplicationOptions opts = {
        .name     = "Forge Sandbox",
        .width    = 1280,
        .height   = 720,
    };

    Application* app = anvl_application_init(opts);
    if (!app) { return 1; }

    // Empilhar sandbox layer como demonstração do layer system.
    Layer* sandbox = _sandbox_layer_create();
    if (sandbox)
    {
        anvl_layer_stack_push(sandbox);
    }

    anvl_application_run(app);

    // Shutdown: remover sandbox, depois framework limpa o resto.
    if (sandbox)
    {
        anvl_layer_stack_remove(sandbox);
        free(sandbox);
    }

    anvl_application_shutdown(app);

    return 0;
}
```

### 2.3 Remoção de `src/anvil.c`

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
└── main.c              ← main() — NOVO
```

### 2.4 Build System (premake5.lua)

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

---

## 3. Estrutura de Arquivos

### 3.1 Novos Arquivos

```
Sandbox/
└── main.c              ← entry point + sandbox layer (consumer code)
```

### 3.2 Arquivos Removidos

```
src/anvil.c              ← main() removido (Sandbox assume)
```

### 3.3 Arquivos Modificados

| Arquivo | Mudança |
|---------|---------|
| `premake5.lua` | Anvil vira StaticLib; novo projeto Sandbox; startproject muda |

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

**Nota:** A Sandbox **não** é parte do Anvil. Ela usa o PCH do Anvil como referência, mas compila separadamente.

### 4.3 Ordem de Build

```
1. Anvil (StaticLib)  → bin/Debug/lib/Anvil.lib (Windows)
                                             bin/Debug/lib/libAnvil.a (Linux)
2. Sandbox (ConsoleApp) → bin/Debug/Sandbox.exe (Windows)
                                       Sandbox (Linux)
```

O Premake resolve a dependência automaticamente via `links { "Anvil" }`.

---

## 5. O que a Sandbox faz (e não faz)

### Faz:
- `main()` — entry point.
- Inicializa o engine (`anvl_application_init`).
- Empilha a sandbox layer (demonstração do layer system).
- Roda o loop (`anvl_application_run`).
- Shutdown limpo.

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

### Etapa 1: Criar `Sandbox/main.c`

Criar `Sandbox/main.c` com:
- Includes do engine (`Core/application.h`, `Core/layer.h`, `Windowing/event.h`, `Tools/logger.h`).
- Sandbox layer (struct + callbacks).
- `main()` que inicializa, empilha, roda, shutdown.

### Etapa 2: Remover `src/anvil.c`

Excluir `src/anvil.c`. Não há mais entry point no Anvil.

### Etapa 3: Atualizar `premake5.lua`

- Anvil: `kind "ConsoleApp"` → `kind "StaticLib"`.
- Novo projeto `Sandbox`: `kind "ConsoleApp"`, `links { "Anvil" }`.
- `startproject "Sandbox"`.
- Ajustar `targetdir` e `objdir` para evitar colisão.

### Etapa 4: Validação

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
| Forward declarations em 4 arquivos | `Sandbox/main.c` precisa das forward decls? Não — a sandbox layer é self-contained |
| Assertion system (`Tools/assert.h`) | Sandbox usa `ANVL_ASSERT` via PCH |

**P8 e P9 são independentes:** P8 cuida da qualidade interna do engine (forward declarations + assertions). P9 cuida da separação de módulos (sandbox como executável consumer). A sandbox layer existe apenas na P9.

---

## 9. Resumo

| Item | Detalhe |
|------|---------|
| **Arquivos novos** | 1 (`Sandbox/main.c`) |
| **Arquivos removidos** | 1 (`src/anvil.c`) |
| **Arquivos modificados** | 1 (`premake5.lua`) |
| **Novas dependências** | Nenhuma |
| **Novos links** | `Sandbox` linka contra `Anvil` (static lib) |
| **PCH alterado** | Não (Sandbox reutiliza o PCH do Anvil) |
| **API pública** | Nenhuma mudança |
| **Módulos afetados** | Novo: `Sandbox/`; Anvil vira static lib |
| **Build** | 2 targets: `Anvil` (StaticLib) + `Sandbox` (ConsoleApp) |
