# Forge Engine — Arquitetura Planejada

> Estado: Planejamento consolidado (Julho/2026)

---

# Visão Geral

A Forge Engine foi projetada como um conjunto de módulos independentes, cada um responsável por uma área específica do engine.

O objetivo principal da arquitetura é:

- Baixo acoplamento entre módulos
- Alta coesão
- APIs gráficas independentes
- Plataforma independente (Windows, Linux, etc.)
- Facilidade para adicionar novos renderizadores e backends futuramente.

A estrutura atualmente planejada é:

```
Sandbox
    │
    ▼
Forge Core
    │
    ▼
Anvil
    │
    ├────────► Plataforma
    │              │
    │              ├── Win32
    │              ├── X11/XCB
    │              └── Wayland
    │
    └────────► Furnace
                    │
                    ├── OpenGL
                    ├── Vulkan  (futuro)
                    └── DirectX (futuro)
```

---

# Camadas

## Forge Core

Forge Core é a camada base da engine.

Ela conterá todos os módulos necessários para que as outras partes/camadas da engine funcionem corretamente.

Os principais módulos serão:
- Anvil
- Furnace
- Módulo de Física (nome à ser definido, aceito sugestões)
- Módulo de Som (nome à ser definido, aceito sugestões)
- Módulo de Assets (nome à ser definido, aceito sugestões)
- Módulo de scripting (nome à ser definido, aceito sugestões)

Responsabilidades:

- ECS
- Runtime
- Game Loop
- Atualização do jogo
- Coordenação dos módulos

### Anvil

Anvil é a biblioteca de mais baixo nível da Forge Engine.

Ela é responsável por toda abstração do sistema operacional.

Responsabilidades:

- Criação de janela
- Loop principal
- Sistema de eventos
- Entrada (Input)
- Sistema de arquivos
- Logging
- Profiling
- Threads
- Temporizadores
- Abstrações da plataforma

Anvil **não possui lógica de renderização**.
Ela apenas fornece infraestrutura para que outros módulos funcionem.

---

### Furnace

Furnace é o módulo responsável pela renderização.

Ele abstrai APIs gráficas como:

- OpenGL
- Vulkan (futuro)
- DirectX (futuro)
- Metal (possível)
- WebGPU (possível)

Responsabilidades:

- Device
- Swapchain
- Pipelines
- Buffers
- Textures
- Shaders
- Command Buffers
- Meshes
- Recursos gráficos

Toda lógica de GPU fica dentro dele.

---

## Forge Workbench

Workbench é a camada de editor da engine.

Ele será construído sobre Core.

Cada editor interno será implementado como uma Layer.

Exemplos:

- Scene Editor
- Material Editor
- Animation Editor
- Script Editor

---

## Forge Ores

Ores será a camada que armazenará os recursos produzidos pelo pipeline de assets.
Será construida sobre Workbench.

Exemplos:

- Scripts
- Definições
- Assets processados
- Metadados

Inicialmente ainda não está completamente definido.

---

## Forge Metal

Metal é a camada runtime do JOGO.
Construída diretamente sobre Core.

Ele faz integração com Forge Core para disponibilizar o runtime do jogo.

---

# Dependências

O fluxo de dependências planejado foi:

## Camadas
```
Forge Metal / Forge Workbench

↓

Forge Core
```

Enquanto:
```
Forge Ores 

↓

Forge Workbench
```

## Módulos

Atualmente:

```
+----------------------------------+
| Sandbox                          |
+----------------------------------+
                │
+----------------------------------+
| Forge Core                       |
+----------------------------------+
        │                  │
        ▼                  ▼
+----------------+   +----------------+
| Anvil          |   | Furnace        |
+----------------+   +----------------+
        │                  │
        ▼                  ▼
+----------------+   +----------------+
| Plataforma     |   | OpenGL/Vulkan  |
+----------------+   +----------------+
        │
        ▼
+----------------+
| Sistema Oper.  |
+----------------+
```

---

# Filosofia do Projeto

A Forge Engine busca seguir alguns princípios arquiteturais fundamentais:

- Separação clara de responsabilidades.
- Baixo acoplamento entre módulos.
- Alta coesão interna.
- Independência entre plataforma e renderização.
- Modularidade.
- Possibilidade de substituir componentes sem impactar o restante do sistema.
- APIs públicas pequenas e bem definidas.
- Código escrito em C, priorizando composição em vez de orientação a objetos.

---
