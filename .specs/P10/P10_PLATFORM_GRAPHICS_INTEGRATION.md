# P10 — Anvil Platform Graphics Integration (WGL/GLX)

## 1. Contexto e Motivação

### 1.1 O problema atual

Anvil é a camada de plataforma da Forge Engine. Atualmente, ele gerencia janelas (Win32, X11, Wayland), eventos, input, filesystem e threading. Porém, para que o Furnace (módulo de renderização) possa desenhar na tela, é necessário um **contexto gráfico** — um objeto que integra a janela nativa com a API gráfica (OpenGL, Vulkan, DirectX).

Esse contexto é **inerentemente platform-specific**:
- **WGL (Windows):** `HDC` + `HGLRC` — handles do Windows GDI
- **GLX (X11/Linux):** `Display*` + `GLXContext` — handles do X Server
- **Vulkan/DirectX:** não usam WGL/GLX, mas ainda precisam de um window handle do Anvil

Sem essa integração, o Furnace não tem como dizer "desenhe nesta janela".

### 1.2 Por que P10?

Esta é a **primeira tarefa da Seção 2 do Roadmap** (Sistema de Renderização):

> **2.1 Inicialização da API gráfica (OpenGL, Vulkan e DirectX):**
> - [ ] Criação do contexto gráfico para as APIs, com base no sistema operacional.
> - [ ] Inicialização do sistema de renderização.

O Furnace precisa de um **canvas pronto** antes de poder criar shaders, buffers ou pipelines. Esse canvas é fornecido pelo Anvil através da Platform Graphics Integration.

### 1.3 Analogia

Pense no Anvil como o **eletricista** e o Furnace como o **eletrônico**:
- O eletricista (Anvil) instala a tomada na parede (cria a janela + contexto gráfico)
- O eletrônico (Furnace) conecta seu aparelho na tomada (usa o contexto para renderizar)

Sem a tomada, o eletrônico não funciona. Sem o eletrônico, a tomada existe mas não faz nada.

---

## 2. Design

### 2.1 Responsabilidades

**Anvil (Platform):**
- Criar e gerenciar o contexto gráfico da plataforma
- Expor handles nativos para o Furnace
- Coordenar window + context internamente

**Furnace (Rendering):**
- Consumir os handles nativos fornecidos pelo Anvil
- Criar objetos da API gráfica (shaders, VBOs, etc.)
- Renderizar

### 2.2 A Hot Side: Platform Graphics Integration

Esta é a camada fina entre **Windowing** e **Graphics API**:

```
┌─────────────────────────────────────────────────────────────┐
│  ANVIL                                                      │
│                                                             │
│  ┌──────────────────────┐    ┌──────────────────────────┐  │
│  │  Windowing Hot Side  │    │  Graphics Integration    │  │
│  │  (Win32/X11/Wayland) │    │  Hot Side (WGL/GLX)      │  │
│  │                      │    │                          │  │
│  │  HWND, XID,          │    │  HDC, HGLRC,             │  │
│  │  wl_surface          │───▶│  Display*, GLXContext    │  │
│  └──────────────────────┘    └──────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
                            │
                            ▼ (handles nativos)
┌─────────────────────────────────────────────────────────────┐
│  FURNACE                                                    │
│                                                             │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  Graphics API Hot Side                               │  │
│  │  (OpenGL/Vulkan/DX)                                  │  │
│  │                                                      │  │
│  │  GLuint, VkInstance, ID3D11Device                   │  │
│  └──────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
```

### 2.3 Interface Pública (Anvil)

A interface entre Anvil e Furnace é **pequena e específica**:

```c
// include/Anvil/Graphics/graphics_integration.h

#ifndef ANVIL_GRAPHICS_INTEGRATION_HEADER
#define ANVIL_GRAPHICS_INTEGRATION_HEADER

#include "Anvil/Core/types.h"

// Opaque handle para o contexto gráfico da plataforma
typedef struct AnvlGraphicsContext AnvlGraphicsContext;

// Opções de criação de contexto gráfico (surface requirements internos)
typedef struct AnvlWindowGraphicsOptions
{
    uint32 depth_bits;        // Bits de depth buffer (default: 24)
    uint32 stencil_bits;      // Bits de stencil buffer (default: 8)
    uint32 sample_count;      // Samples para MSAA (default: 1)
    bool   require_dummy_window; // Se true, cria janela temporária para WGL
} AnvlWindowGraphicsOptions;

// Funções de lifecycle
AnvlGraphicsContext* anvl_graphics_context_create(
    void* native_window_handle,
    const AnvlWindowGraphicsOptions* options
);
void                 anvl_graphics_context_destroy(AnvlGraphicsContext* context);

// Acesso aos handles nativos (para o Furnace consumir)
void* anvl_graphics_context_get_native_handle(
    const AnvlGraphicsContext* context,
    AnvlGraphicsHandleType type
);

// Tipos de handle que o Furnace pode solicitar
typedef enum AnvlGraphicsHandleType
{
    ANVL_GRAPHICS_HANDLE_WGL_HDC,       // Windows: Device Context
    ANVL_GRAPHICS_HANDLE_WGL_HGLRC,     // Windows: OpenGL Rendering Context
    ANVL_GRAPHICS_HANDLE_GLX_DISPLAY,   // X11: Display connection
    ANVL_GRAPHICS_HANDLE_GLX_CONTEXT,   // X11: GLX Context
    ANVL_GRAPHICS_HANDLE_NATIVE_WINDOW, // Handle nativo da janela (HWND, XID, etc.)
} AnvlGraphicsHandleType;

#endif // !ANVIL_GRAPHICS_INTEGRATION_HEADER
```

**Notas:**
- `native_window_handle` é `void*` — pode ser `HWND` (Win32), `xcb_window_t` (X11), etc.
- `AnvlWindowGraphicsOptions` são **internos a Anvil** — determinam PFD/GLXFBConfig
- O Furnace não conhece surface requirements — recebe apenas native handles
- O Furnace solicita handles específicos via `AnvlGraphicsHandleType`
- O Anvil retorna o handle correto baseado na plataforma

### 2.4 Estrutura Interna

```c
// src/Graphics/graphics_context.h (privado)

typedef struct AnvlGraphicsContext
{
    AnvlGraphicsPlatform platform;  // WIN32, X11, WAYLAND
    void*                impl;      // Platform-specific data
} AnvlGraphicsContext;

typedef enum AnvlGraphicsPlatform
{
    ANVL_GRAPHICS_PLATFORM_WIN32,
    ANVL_GRAPHICS_PLATFORM_X11,
    ANVL_GRAPHICS_PLATFORM_WAYLAND,
} AnvlGraphicsPlatform;
```

### 2.4 Coordenação Janela ↔ Contexto

A criação do contexto gráfico **depende da plataforma** devido a diferenças fundamentais entre WGL e GLX:

#### WGL (Windows)

```
Janela (HWND) → HDC → SetPixelFormat → HGLRC
```

1. Consumer cria janela via `anvl_window_create()` → obtém `HWND`
2. Consumer chama `anvl_graphics_context_create(hwnd, options)`
3. Anvil internamente:
   - Obtém HDC via `GetDC(hwnd)`
   - Constrói PFD baseado em `AnvlWindowGraphicsOptions`
   - Aplica pixel format via `SetPixelFormat(hdc, pf, &pfd)`
   - Cria HGLRC via `wglCreateContext(hdc)`
   - Torna contexto atual via `wglMakeCurrent(hdc, hglrc)`

**Nota:** A janela pode ser "real" (criada pelo consumer) ou "dummy" (criada internamente se `require_dummy_window == true`).

#### GLX (X11/Linux)

```
FBConfig → XVisualInfo → Janela (com Visual correto) → GLXContext
```

1. Consumer cria janela via `anvl_window_create()` → obtém `XID`
2. Consumer chama `anvl_graphics_context_create(xid, options)`
3. Anvil internamente:
   - Escolhe GLXFBConfig baseado em `AnvlWindowGraphicsOptions`
   - Obtém XVisualInfo via `glXGetVisualFromFBConfig()`
   - **Recria a janela** com o XVisualInfo correto (se necessário)
   - Cria GLXContext via `glXCreateNewContext()`
   - Torna contexto atual via `glXMakeCurrent(display, window, glx_context)`

**Nota:** Em GLX, o FBConfig/Visual deve ser conhecido ANTES de criar a janela. Se a janela foi criada sem o Visual correto, Anvil a recria internamente.

#### Resumo

| Plataforma | Ordem de Criação | Surface Requirements |
|------------|------------------|----------------------|
| WGL        | Janela → Contexto | Internos (PFD)       |
| GLX        | FBConfig → Janela → Contexto | Internos (GLXFBConfig) |
| Wayland    | Não suportado    | N/A                  |

**Surface requirements são internos a Anvil** — o Furnace não os conhece, apenas consome os native handles resultantes.

#### 2.5.3 Interface Interna

```c
// src/Graphics/Windows/wgl_context.c

static AnvlGraphicsContext* _wgl_context_create(HWND hwnd);
static void                 _wgl_context_destroy(AnvlGraphicsContext* context);
static void*                _wgl_context_get_handle(
    const AnvlGraphicsContext* context,
    AnvlGraphicsHandleType type
);
```

### 2.6 GLX Backend (Linux/X11)

#### 2.6.1 Estrutura

```c
// src/Graphics/Linux/glx_context.h (privado)

typedef struct GLXContextData
{
    Display*       display;      // X Server connection
    GLXContext     context;      // OpenGL context
    Window         window;       // X11 window handle
    GLXFBConfig    fb_config;    // Best matching FBConfig
    XVisualInfo*   visual_info;  // Visual information
} GLXContextData;
```

#### 2.6.2 Fluxo de Criação

```
1. Receber XID (Window) do Anvil Windowing
2. Obter Display* via XOpenDisplay(NULL) (se não existente)
3. Escolher FBConfig via glXChooseFBConfig(display, screen, attributes, &n_fbs)
4. Obter VisualInfo via glXGetVisualFromFBConfig(display, fb_config)
5. Criar contexto OpenGL via glXCreateNewContext(display, fb_config, GLX_RGBA_TYPE, NULL, True)
6. Tornar contexto atual via glXMakeCurrent(display, window, glx_context)
7. Carregar extensões GLX (glXGetProcAddress)
```

#### 2.6.3 Interface Interna

```c
// src/Graphics/Linux/glx_context.c

static AnvlGraphicsContext* _glx_context_create(Window window);
static void                 _glx_context_destroy(AnvlGraphicsContext* context);
static void*                _glx_context_get_handle(
    const AnvlGraphicsContext* context,
    AnvlGraphicsHandleType type
);
```

### 2.7 Wayland

Wayland **não suporta** WGL/GLX diretamente. Para OpenGL sobre Wayland, é necessário usar EGL ou OSMesa.

**Decisão para P10:** Wayland não é suportado nesta primeira implementação. O backend Wayland retornará `NULL` para `anvl_graphics_context_create()`.

```c
#include "Anvil/Platform/platform_detection.h"

AnvlGraphicsContext* anvl_graphics_context_create(void* native_window_handle)
{
#if defined(ANVIL_PLATFORM_WINDOWS)
    // WGL creation
#elif defined(ANVIL_PLATFORM_LINUX)
    // GLX creation
#else
    ANVIL_CORE_WARN("ANVIL", "Wayland/other platform graphics integration not yet implemented.");
    return NULL;
#endif
}
```

---

## 3. Estrutura de Arquivos

### 3.1 Novos Arquivos

```
Anvil/
├── include/
│   └── Anvil/
│       └── Graphics/
│           └── graphics_integration.h          # Interface pública
├── src/
│   ├── Graphics/
│   │   ├── graphics_context.c                  # Implementation dispatch
│   │   ├── Windows/
│   │   │   ├── wgl_context.h                   # WGL internal header
│   │   │   └── wgl_context.c                   # WGL implementation
│   │   ├── Linux/
│   │   │   ├── glx_context.h                   # GLX internal header
│   │   │   └── glx_context.c                   # GLX implementation
│   │   └── Wayland/
│   │       └── wayland_context.c               # Stub (not implemented)
```

### 3.2 Arquivos Modificados

```
Anvil/
├── CMakeLists.txt                              # Adicionar Graphics/
├── src/
│   └── anvlpch.h                               # Adicionar includes de Graphics/
```

---

## 4. Integração com a Codebase Existente

### 4.1 Dependências de Build

**Anvil CMakeLists.txt:**
```cmake
# Windows
if(WIN32)
    target_link_libraries(Anvil PRIVATE opengl32)
endif()

# Linux
if(UNIX AND NOT APPLE)
    find_package(X11 REQUIRED)
    find_package(GL REQUIRED)
    target_link_libraries(Anvil PRIVATE X11 GL)
endif()
```

### 4.2 PCH

`src/anvlpch.h` deve incluir:
```c
#include "Anvil/Graphics/graphics_integration.h"
```

### 4.3 Ordem de Build

A Platform Graphics Integration é implementada **antes** do Furnace P1, pois o Furnace depende dela.

### 4.4 Fluxo de Integração

```
1. Consumer cria janela:
   AnvlWindow* window = anvl_window_create(options);

2. Consumer cria contexto gráfico com surface requirements internos:
   AnvlWindowGraphicsOptions gfx_opts = {
       .depth_bits = 24,
       .stencil_bits = 8,
       .sample_count = 4,
       .require_dummy_window = false,
   };
   AnvlGraphicsContext* ctx = anvl_graphics_context_create(window, &gfx_opts);

3. Consumer obtém native handles para o Furnace:
   void* hdc = anvl_graphics_context_get_native_handle(ctx, ANVL_GRAPHICS_HANDLE_WGL_HDC);
   void* hglrc = anvl_graphics_context_get_native_handle(ctx, ANVL_GRAPHICS_HANDLE_WGL_HGLRC);

4. Consumer cria backend do Furnace com native handles:
   FrncBackend* backend = frnc_backend_create(options);
   frnc_backend_create_context(backend, native_handle);
```

---

## 5. O que a Platform Graphics Integration faz (e não faz)

### Faz:
- Gerenciar surface requirements internamente (PFD para WGL, GLXFBConfig para GLX)
- Criar contexto gráfico da plataforma (WGL/GLX)
- Expor handles nativos para o Furnace
- Coordenar window + context internamente

### Não faz:
- Expor surface requirements ao Furnace (são internos)
- Criar shaders, VBOs, texturas (isso é Furnace)
- Renderizar triângulos ou formas (isso é Furnace)
- Gerenciar swap chains (isso é Furnace)
- Conhecer o Furnace diretamente (dependência unilateral: Furnace → Anvil)

---

## 6. Plano de Implementação (Passo a Passo)

### ✅ Etapa 1: Criar `include/Anvil/Graphics/graphics_integration.h`
Interface pública com `AnvlGraphicsContext`, `AnvlWindowGraphicsOptions` (surface requirements internos), `anvl_graphics_context_create/destroy`, e `anvl_graphics_context_get_native_handle`.

### ✅ Etapa 2: Criar `src/Graphics/graphics_context.c`
Implementation dispatch baseado na plataforma (`WIN32` → WGL, `X11` → GLX, `WAYLAND` → stub).

### ✅ Etapa 3: Criar WGL backend (`src/Graphics/Windows/wgl_context.h` + `wgl_context.c`)
- Receber HWND do Anvil Windowing
- Dummy window (se `require_dummy_window == true`)
- `GetDC` → construir PFD a partir de `AnvlWindowGraphicsOptions` → `ChoosePixelFormat` → `SetPixelFormat`
- `wglCreateContext` + `wglMakeCurrent`
- `wglGetProcAddress` para extensões

### ✅ Etapa 4: Criar GLX backend (`src/Graphics/Linux/glx_context.h` + `glx_context.c`)
- `XOpenDisplay`
- Escolher GLXFBConfig a partir de `AnvlWindowGraphicsOptions`
- `glXGetVisualFromFBConfig`
- Recriar janela com XVisualInfo correto (se necessário)
- `glXCreateNewContext` + `glXMakeCurrent`
- `glXGetProcAddress` para extensões

### ✅ Etapa 5: Atualizar `CMakeLists.txt`
Adicionar novos arquivos e dependências (`opengl32`, `X11`, `GL`).

### ✅ Etapa 6: Atualizar PCH (`src/anvlpch.h`)
Incluir `Anvil/Graphics/graphics_integration.h`.

### ✅ Etapa 7: Validação
- Compilar para Windows (WGL)
- Compilar para Linux (GLX)
- Verificar que os handles são retornados corretamente

---

## 7. O que NÃO está incluído (Escopo Explícito)

- **Furnace P1:** Estrutura base do Furnace (tipos, backend abstrato, stubs)
- **Vulkan/DirectX:** Não implementados no P10
- **Wayland:** Stub apenas (retorna NULL)
- **Renderer:** Shaders, buffers, pipelines, draw calls (Furnace P2+)

---

## 8. Relação com Furnace P1

O Furnace P1 **depende** desta spec para funcionar:

```
Anvil P10 (Platform Graphics Integration)
    ↓ gerencia surface requirements internamente (PFD/GLXFBConfig)
    ↓ fornece handles nativos (HDC, HGLRC, Display*, GLXContext)
Furnace P1 (Core Structure)
    ↓ consome native handles
Furnace P2 (OpenGL Backend)
    ↓ cria objetos
Furnace P3 (Renderer)
```

**Surface requirements são internos a Anvil** — o Furnace P1 não os conhece, apenas consome os native handles resultantes.

O Anvil P10 implementa WGL/GLX e coordena a criação de janela + contexto. O Furnace P1 apenas define a interface que o backend OpenGL deve seguir.

---

## 9. Resumo

| Item | Descrição |
|------|-----------|
| **Módulo** | Anvil |
| **Responsabilidade** | Platform Graphics Integration (WGL/GLX) |
| **Interface Pública** | `anvl_graphics_context_create/destroy/get_native_handle` |
| **Surface Requirements** | Internos (`AnvlWindowGraphicsOptions` → PFD/GLXFBConfig) |
| **Handles Expostos** | `HDC`, `HGLRC`, `Display*`, `GLXContext`, native window |
| **Plataformas** | Win32 (WGL), X11 (GLX), Wayland (stub) |
| **Dependências** | `opengl32` (Win), `X11` + `GL` (Linux) |
| **Pré-requisito para** | Furnace P1, P2, P3 |
