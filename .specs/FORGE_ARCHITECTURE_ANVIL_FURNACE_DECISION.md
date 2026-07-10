# Forge Engine --- Decisão Arquitetural: Anvil × Furnace

## Objetivo

Separar plataforma e renderização sem buscar um desacoplamento absoluto,
reconhecendo que existe uma integração inevitável entre janela nativa e
API gráfica.

------------------------------------------------------------------------

## Responsabilidades

### Anvil

Responsável exclusivamente pela plataforma.

-   Criação e gerenciamento de janelas.
-   Eventos.
-   Input.
-   Filesystem.
-   Timing.
-   Abstrações do sistema operacional.
-   Aplicação das configurações de superfície fornecidas pelo renderer.

Anvil **não conhece Furnace**.

------------------------------------------------------------------------

### Furnace

Responsável exclusivamente pela renderização.

-   OpenGL (WGL, GLX, EGL...)
-   Vulkan
-   Direct3D
-   Futuros backends

Também é responsável por:

-   Bootstrap da API gráfica.
-   Criação do contexto/dispositivo gráfico.
-   VTables dos backends.

Furnace depende apenas da **API pública** de Anvil.

------------------------------------------------------------------------

## Dependências

``` text
Sandbox / Metal
│
├── Anvil
└── Furnace

Furnace
    ↓
API pública de Anvil

Anvil
    ✗ não depende de Furnace
```

------------------------------------------------------------------------

## Fluxo de inicialização

``` text
main()

↓

backend = furnaceCreateBackend(...)

↓

backend->initialize()

↓

requirements = backend->getSurfaceRequirements()

↓

app = anvlApplicationCreate()

↓

window = anvlWindowCreate(requirements)

↓

context = backend->createContext(window)

↓

anvlApplicationRun(app)
```

------------------------------------------------------------------------

## Bootstrap

Cada backend possui sua própria fase de bootstrap.

### WGL

-   Dummy Window
-   Dummy Context
-   Carregamento das extensões WGL

### GLX

-   Inicialização do Display
-   Descoberta das extensões
-   Escolha do FBConfig

### Vulkan

-   vkCreateInstance()
-   Enumeração de dispositivos

Essa etapa ocorre **antes** da criação do contexto real.

------------------------------------------------------------------------

## Surface Requirements

A decisão sobre a superfície pertence ao renderer.

Exemplos:

-   MSAA
-   sRGB
-   Double Buffer
-   Depth
-   Stencil

Furnace decide **o que precisa**.

Anvil sabe **como aplicar** isso na plataforma.

Em outras palavras:

-   Furnace = política
-   Anvil = mecanismo

------------------------------------------------------------------------

## Papel de cada módulo

### Furnace

Decide:

-   quais capacidades da superfície são necessárias;
-   quando criar o contexto;
-   como inicializar a API gráfica.

### Anvil

Sabe:

-   como criar uma janela Win32;
-   como criar uma janela X11;
-   como criar uma janela Wayland;
-   como aplicar os requisitos recebidos.

------------------------------------------------------------------------

## Acoplamento

Existe um pequeno acoplamento inevitável.

Ele é unilateral.

``` text
Furnace
    │
    ▼
API pública Anvil
```

Esse acoplamento é considerado aceitável porque toda API gráfica precisa
renderizar sobre uma superfície nativa.

------------------------------------------------------------------------

## O que NÃO fazer

Evitar:

-   Anvil criar contextos OpenGL/Vulkan/DX11.
-   Furnace criar janelas.
-   Dependência circular entre módulos.
-   Abstrações genéricas para APIs ainda não implementadas.

------------------------------------------------------------------------

## Princípios adotados

-   Implementar primeiro; abstrair depois.
-   Não criar abstrações especulativas.
-   Interface pequena entre módulos.
-   Separação por responsabilidade.
-   Bootstrap específico por backend.
-   Janela e contexto possuem ciclos de vida distintos.

------------------------------------------------------------------------

## Decisão final

-   Anvil permanece um módulo de plataforma.
-   Furnace permanece um módulo de renderização.
-   Furnace realiza o bootstrap da API gráfica.
-   Furnace informa os requisitos da superfície.
-   Anvil cria a janela usando esses requisitos.
-   Furnace cria o contexto gráfico sobre a janela.
-   O executável (Sandbox ou futuramente Metal) orquestra toda a
    sequência.
