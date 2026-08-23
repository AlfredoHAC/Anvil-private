# P10 — Anvil Platform Graphics Integration

> **Status:** WGL (Windows) ✅ concluído · GLX (X11) ✅ concluído · Wayland/EGL ✅ concluído
>
> **Última atualização:** 2026-08-22

## 1. Contexto e Motivação

### 1.1 O problema atual

Anvil é a camada de plataforma da Forge Engine. Atualmente, ele gerencia janelas (Win32, X11, Wayland), eventos, input, filesystem e threading. Porém, para que o Furnace (módulo de renderização) possa desenhar na tela, é necessário um **contexto gráfico** — um objeto que integra a janela nativa com a API gráfica (OpenGL, Vulkan, DirectX).

Esse contexto é **inerentemente platform-specific**:
- **WGL (Windows):** `HDC` + `HGLRC` — handles do Windows GDI
- **GLX (X11/Linux):** `Display*` + `GLXContext` — handles do X Server
- **EGL (Wayland/DRM):** `EGLDisplay` + `EGLContext` — handles abstratos
- **Vulkan/DirectX:** não usam WGL/GLX/EGL, mas ainda precisam de um window handle do Anvil

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

### 2.2 Arquitetura: Integração Direta no Windowing

Diferente do design original (módulo `Graphics/` separado), a implementação real **integra o contexto gráfico diretamente ao backend de windowing**. Cada backend de janela (Win32, X11, Wayland) cria e gerencia seu próprio contexto gráfico.

```
┌─────────────────────────────────────────────────────────────┐
│  ANVIL                                                      │
│                                                             │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  Windowing + Graphics Integration (integrados)       │  │
│  │                                                      │  │
│  │  Win32:  HWND → HDC → PixelFormat → HGLRC           │  │
│  │  X11:    XID → Display → FBConfig → GLXContext       │  │
│  │  Wayland: wl_surface → EGLDisplay → EGLContext       │  │
│  └──────────────────────────────────────────────────────┘  │
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

Os surface requirements são **integrados ao `AnvlWindowOptions`** via `AnvlGraphicRequirements`. O modo gráfico é selecionado via `AnvlWindowGraphicsMode`.

```c
// include/Anvil/Window/window.h

typedef enum AnvlWindowGraphicsMode
{
    ANVL_WINDOW_GRAPHICS_MODE_NONE = 0,
    ANVL_WINDOW_GRAPHICS_MODE_OPENGL
} AnvlWindowGraphicsMode;

typedef struct AnvlWindowOptions
{
    const char* title;
    uint16      width;
    uint16      height;

    AnvlWindowGraphicsMode graphics_mode;
    struct AnvlGraphicRequirements
    {
        int32 red_bits;
        int32 green_bits;
        int32 blue_bits;
        int32 alpha_bits;
        int32 depth_bits;
        int32 stencil_bits;
        int32 sample_count;
        int32 major_version;
        int32 minor_version;
    } graphics_requirements;
} AnvlWindowOptions;
```

**Notas:**
- `graphics_mode == ANVL_WINDOW_GRAPHICS_MODE_NONE` → janela sem contexto gráfico
- `graphics_mode == ANVL_WINDOW_GRAPHICS_MODE_OPENGL` → contexto WGL/GLX/EGL criado junto com a janela
- `AnvlGraphicRequirements` são **internos a Anvil** — determinam PFD (WGL), GLXFBConfig (GLX) ou EGLConfig (EGL)
- O Furnace consome o `HWND`/`XID`/`wl_surface` via `anvl_window_get_handle()` e inicializa OpenGL com `gladLoaderLoadGL()` + `eglGetProcAddress()` (sem necessidade de native handles explícitos)

### 2.4 Coordenação Janela ↔ Contexto

A criação do contexto gráfico **depende da plataforma** e é feita **dentro do próprio backend de windowing**.

#### WGL (Windows) — ✅ Implementado

```
Janela (HWND) → HDC → SetPixelFormat → HGLRC
```

Fluxo em `win32_window.c`:

1. `anvl_window_create()` verifica `graphics_mode == ANVL_WINDOW_GRAPHICS_MODE_OPENGL`
2. Chama `wgl_context_load_extensions()` — dummy window pattern para carregar `wglGetProcAddress`
3. Chama `wgl_context_create(window->handle, requirements)`:
   - Internamente obtém HDC via `GetDC()`
   - Escolhe pixel format via `wglChoosePixelFormatARB`
   - Aplica pixel format via `SetPixelFormat(hdc, pf, NULL)`
   - Cria contexto via `wglCreateContextAttribsARB` com versão configurável
   - Torna contexto atual via `wglMakeCurrent(hdc, hglrc)`

**Arquivos:**
- `src/Windowing/Windows/win32_window.c` — coordenação (criação/destruição da janela)
- `src/Windowing/Windows/wgl_context.c` — backend WGL (`wgl_context_create/destroy/load_extensions`)
- `src/Windowing/Windows/wgl_context.h` — interface interna

#### GLX (X11/Linux) — ✅ Implementado

```
Display → FBConfig → XVisualInfo → Colormap → XCB Window → GLXWindow → GLXContext
```

Fluxo em `x11_backend.c`:

1. `x11_backend_init()` — `XOpenDisplay(NULL)` → `XGetXCBConnection()` → obtém screen
2. `x11_window_create()`:
   - Escolhe GLXFBConfig baseado em `AnvlGraphicRequirements`
   - Obtém XVisualInfo via `glXGetVisualFromFBConfig(display, fb_config)`
   - Cria colormap via `xcb_create_colormap()` (root como parent)
   - Cria XCB window com o visual correto
   - Cria GLXWindow via `glXCreateWindow(display, fb_config, x11_window, NULL)`
   - Cria GLXContext via `glXCreateContextAttribsARB(display, fb_config, NULL, True, attrs)`
   - Torna contexto atual via `glXMakeContextCurrent(display, glx_window, glx_window, glx_context)`

**Arquivos:**
- `src/Windowing/Linux/X11/x11_backend.c` — backend XCB + coordenação GLX
- `src/Windowing/Linux/X11/x11_backend.h` — structs (`X11Backend`, `AnvlGLXGraphicsContext`)
- `src/Windowing/Linux/X11/glx_context.c` — funções auxiliares GLX (FBConfig, VisualInfo, load)
- `src/Windowing/Linux/X11/glx_context.h` — interface interna

**Nota:** Display é compartilhado entre Xlib e XCB via `XGetXCBConnection()`. Colormap é criada antes da window e destruída no `x11_window_destroy`.

#### Wayland/EGL — ✅ Implementado

```
wl_display → wl_compositor → wl_shell → wl_surface → EGLDisplay → EGLConfig → EGLContext → EGLSurface
```

Fluxo em `wayland_backend.c`:

1. `wayland_backend_init()` — `wl_display_connect(NULL)` → `wl_compositor_create()`
2. `wayland_window_create()`:
   - Cria `wl_surface` e `wl_egl_window`
   - `eglGetPlatformDisplay(EGL_PLATFORM_WAYLAND, wl_display, NULL)`
   - `eglInitialize(display, ...)`
   - `eglChooseConfig(display, attribs, &config, ...)`
   - `eglCreateWindowSurface(display, config, wl_egl_window, NULL)`
   - `eglCreateContext(display, config, EGL_NO_CONTEXT, ctx_attribs)`
   - `eglMakeCurrent(display, surface, surface, context)`

**Arquivos:**
- `src/Windowing/Linux/Wayland/wayland_backend.c` — backend Wayland + coordenação EGL
- `src/Windowing/Linux/Wayland/egl_context.c` — backend EGL (`egl_context_create/destroy`)
- `src/Windowing/Linux/Wayland/egl_context.h` — interface interna

**Nota:** EGL não precisa de dummy window. `gladLoaderLoadGL()` com `eglGetProcAddress` funciona sem contexto válido.

#### Resumo

| Plataforma | Status | Ordem de Criação | Surface Requirements |
|------------|--------|------------------|----------------------|
| WGL        | ✅ Concluído | Janela → Contexto | Internos (PFD)       |
| GLX        | ✅ Concluído | FBConfig → XCB Window → GLXWindow → Contexto | Internos (GLXFBConfig) |
| Wayland/EGL | ✅ Concluído | EGLDisplay → Config → Contexto → Surface | Internos (EGLConfig) |

**Surface requirements são internos a Anvil** — o Furnace não os conhece, apenas consome os native handles resultantes.

### 2.5 Comparação entre Backends

| Aspecto | WGL | GLX | EGL |
|---------|-----|-----|-----|
| Dummy window | Sim (carregar extensões) | Não (Xlib display) | Não (EGL display) |
| Colormap | Não | Sim (`xcb_create_colormap`) | Não |
| Native window wrapper | Não (HDC direto) | Sim (`glXCreateWindow`) | Sim (`wl_egl_window`) |
| Display management | `GetDC`/`ReleaseDC` | `XOpenDisplay`/`XCloseDisplay` | `eglGetDisplay`/`eglTerminate` |
| Extension loading | `wglGetProcAddress` | `glXGetProcAddress` | `eglGetProcAddress` |
| glad integration | `gladLoaderLoadWGL(hdc)` | `gladLoaderLoadGL()` | `gladLoaderLoadGL((GLADloadfunc)eglGetProcAddress)` |

**Diferença fundamental:** WGL e EGL adquirem o native handle (HDC/EGLDisplay) dentro do contexto gráfico. GLX segue o padrão "envolve" — o FBConfig é escolhido independentemente, a XCB window é criada com o visual correto, e depois o GLX "envelopa" a XCB window com `glXCreateWindow`.

---

## 3. Estrutura de Arquivos

### 3.1 Estrutura Atual

```
Anvil/
├── include/Anvil/Window/
│   └── window.h                                # AnvlGraphicRequirements integrado
├── src/Windowing/
│   ├── Windows/
│   │   ├── win32_window.c                      # Win32 window + WGL coordenação
│   │   ├── wgl_context.h                       # WGL internal header
│   │   └── wgl_context.c                       # WGL implementation
│   ├── Linux/
│   │   ├── X11/
│   │   │   ├── x11_backend.c                   # XCB backend + GLX coordenação
│   │   │   ├── x11_backend.h                   # X11Backend + AnvlGLXGraphicsContext
│   │   │   ├── glx_context.c                   # GLX auxiliares (FBConfig, VisualInfo, load)
│   │   │   └── glx_context.h                   # GLX interface interna
│   │   └── Wayland/
│   │       ├── wayland_backend.c               # Wayland backend + EGL coordenação
│   │       ├── egl_context.c                   # EGL implementation
│   │       └── egl_context.h                   # EGL interface interna
```

---

## 4. Integração com a Codebase Existente

### 4.1 Dependências de Build

**Anvil CMakeLists.txt:**
```cmake
# Glad (WGL + OpenGL loaders)
target_link_libraries(Anvil PUBLIC glad)

# Windows
if(WIN32)
    target_link_libraries(Anvil PRIVATE user32 gdi32 opengl32)
endif()

# Linux
if(UNIX AND NOT APPLE)
    find_package(X11)
    if(X11_FOUND)
        target_link_libraries(Anvil PRIVATE X11 xcb)
    endif()
    # EGL: find_package(OpenGL) ou link direto com EGL
endif()
```

### 4.2 PCH

`src/anvlpch.h` inclui os headers condicionais por plataforma:
```c
#ifdef ANVL_PLATFORM_WINDOWS
#include <glad/wgl.h>
#include <windows.h>
#include <wingdi.h>
#include <winuser.h>
#elif defined(ANVL_PLATFORM_LINUX)
#include <glad/glx.h>
#include <GL/glx.h>
#include <xcb/xcb.h>
#include <xcb/xproto.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <wayland-client.h>
#include <wayland-egl.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#endif
```

### 4.3 Ordem de Build

A Platform Graphics Integration é implementada **antes** do Furnace P1, pois o Furnace depende dela.

### 4.4 Fluxo de Integração

```
1. Consumer cria janela com modo gráfico:
   AnvlWindowOptions opts = {
       .title = "Sandbox",
       .width = 1280,
       .height = 720,
       .graphics_mode = ANVL_WINDOW_GRAPHICS_MODE_OPENGL,
       .graphics_requirements = {
           .depth_bits = 24,
           .stencil_bits = 8,
           .sample_count = 4,
           .major_version = 3,
           .minor_version = 3,
       },
   };
   AnvlWindow* window = anvl_window_create(opts);

2. Anvil cria o contexto gráfico nativo da plataforma:
   - Windows: HDC + WGL 3.3 Core
   - Linux/X11: GLXContext 3.3 Core + GLXWindow
   - Linux/Wayland: EGLContext + EGLSurface

3. Furnace obtém o handle nativo via anvl_window_get_handle() e inicializa:
   - gladLoaderLoadGL() + wglGetProcAddress (WGL)
   - gladLoaderLoadGL() + glXGetProcAddress (GLX)
   - gladLoaderLoadGL() + eglGetProcAddress (EGL)
```

---

## 5. O que a Platform Graphics Integration faz (e não faz)

### Faz:
- Gerenciar surface requirements internamente (PFD para WGL, GLXFBConfig para GLX, EGLConfig para EGL)
- Criar contexto gráfico da plataforma (WGL/GLX/EGL)
- Expor handles nativos para o Furnace
- Coordenar window + context internamente
- Padronizar failure paths com padrão create/rollback/destroy

### Não faz:
- Expor surface requirements ao Furnace (são internos)
- Criar shaders, VBOs, texturas (isso é Furnace)
- Renderizar triângulos ou formas (isso é Furnace)
- Gerenciar swap chains (isso é Furnace)
- Conhecer o Furnace diretamente (dependência unilateral: Furnace → Anvil)

---

## 6. Plano de Implementação (Passo a Passo)

### ✅ Etapa 1: Integrar requirements ao `AnvlWindowOptions`
`AnvlGraphicRequirements` (pixel format + versão) e `AnvlWindowGraphicsMode` adicionados ao struct de opções da janela.

### ✅ Etapa 2: Criar WGL backend (`src/Windowing/Windows/wgl_context.h` + `wgl_context.c`)
- `wgl_context_load_extensions()` — dummy window pattern para carregar `wglGetProcAddress`
- `wgl_context_create(HWND, HDC, major_version, minor_version)` — cria contexto via `wglCreateContextAttribsARB`
- `wgl_context_destroy(HWND, HGLRC)` — unbind + delete context

### ✅ Etapa 3: Integrar WGL ao `win32_window.c`
- Setup de pixel format via `wglChoosePixelFormatARB`
- `SetPixelFormat` com NULL PFD (correto para ARB)
- Criação do contexto WGL dentro do fluxo de criação da janela
- Cleanup integrado no `anvl_window_destroy`

### ✅ Etapa 4: Atualizar `CMakeLists.txt`
Adicionar glad (PUBLIC), sources Windows condicionalizados, `opengl32` linkado.

### ✅ Etapa 5: Atualizar PCH (`src/anvlpch.h`)
Incluir headers condicionais por plataforma.

### ✅ Etapa 6: Validação Windows
Build passa, contexto WGL 3.3 Core criado com sucesso, extensões carregadas.

### ✅ Etapa 7: Criar GLX backend (`src/Windowing/Linux/X11/glx_context.h` + `glx_context.c`)
- `XOpenDisplay` + `XGetXCBConnection` (display compartilhado)
- Escolher GLXFBConfig a partir de `AnvlGraphicRequirements`
- `glXGetVisualFromFBConfig`
- Criação de colormap via `xcb_create_colormap`
- `glXCreateWindow` + `glXCreateContextAttribsARB` + `glXMakeContextCurrent`

### ✅ Etapa 8: Integrar GLX ao `x11_backend.c`
- Backend XCB com eventos (keyboard, mouse, resize, close)
- Setup de FBConfig + criação de contexto dentro do fluxo de criação da janela
- Cleanup integrado no `x11_window_destroy`

### ✅ Etapa 9: Criar EGL backend (`src/Windowing/Linux/Wayland/egl_context.h` + `egl_context.c`)
- `eglGetPlatformDisplay` + `eglInitialize`
- `eglChooseConfig` a partir de `AnvlGraphicRequirements`
- `eglCreateContext` + `eglCreateWindowSurface` + `eglMakeCurrent`
- `eglGetProcAddress` para extensões
- `gladLoaderLoadGL()` funciona sem contexto válido

### ✅ Etapa 10: Integrar EGL ao `wayland_backend.c`
- Backend Wayland com wl_shell + wl_egl_window
- Setup de EGLConfig + criação de contexto dentro do fluxo de criação da janela
- Cleanup integrado no `wayland_window_destroy`

---

## 7. O que NÃO está incluído (Escopo Explícito)

- **Furnace P1:** Estrutura base do Furnace (tipos, backend abstrato, stubs)
- **Vulkan/DirectX:** Não implementados no P10
- **Renderer:** Shaders, buffers, pipelines, draw calls (Furnace P2+)
- **Módulo `Graphics/` separado:** A integração é feita diretamente no windowing, não há dispatch genérico
- **Failure path padrão:** create/rollback/destroy — implementado em todas as três plataformas

---

## 8. Relação com Furnace P1

O Furnace P1 **depende** desta spec para funcionar:

```
Anvil P10 (Platform Graphics Integration)
    ↓ gerencia surface requirements internamente (PFD/GLXFBConfig/EGLConfig)
    ↓ expõe HWND/XID/wl_surface via anvl_window_get_handle()
Furnace P1 (Core Structure)
    ↓ inicializa OpenGL com gladLoaderLoadGL() + eglGetProcAddress/wglGetProcAddress
Furnace P2 (OpenGL Backend)
    ↓ cria objetos
Furnace P3 (Renderer)
```

**Surface requirements são internos a Anvil** — o Furnace P1 não os conhece, apenas consome o handle nativo da janela e inicializa o OpenGL.

O Anvil P10 implementa WGL/GLX/EGL e coordena a criação de janela + contexto. O Furnace P1 apenas define a interface que o backend OpenGL deve seguir.

---

## 9. Resumo

| Item | Descrição |
|------|-----------|
| **Módulo** | Anvil |
| **Responsabilidade** | Platform Graphics Integration (WGL/GLX/EGL) |
| **Interface Pública** | `AnvlGraphicRequirements` integrado ao `AnvlWindowOptions` |
| **Surface Requirements** | Internos (PFD para WGL, GLXFBConfig para GLX, EGLConfig para EGL) |
| **Handles Expostos** | `HWND`/`XID`/`wl_surface` via `anvl_window_get_handle()` |
| **Plataformas** | Win32 (WGL ✅), X11 (GLX ✅), Wayland (EGL ✅) |
| **Dependências** | `glad` (PUBLIC), `opengl32` (Win), `X11`/`xcb`/`wayland`/`EGL` (Linux) |
| **Failure Path** | create → rollback (em falha) → destroy (tolerante a estado parcial) |
| **Pré-requisito para** | Furnace P1, P2, P3 |
