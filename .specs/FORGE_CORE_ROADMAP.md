# Roteiro Inicial para Desenvolvimento da Forge Engine

## 1. Base de Plataforma (Biblioteca de Sistema)
**Objetivo:** Abstrair as funcionalidades específicas do sistema operacional.

**Tarefas principais:**

**1. Gerenciamento de Janelas:**
- [ ] Criação, redimensionamento e fechamento de janelas.
- [ ] Integração com os sistemas operacionais Windows e Linux.

**2. Eventos de Entrada:**
- [x] Captura de teclado, mouse e eventos de janela.

**3. Logging e Debugging:**
- [x] Criar um sistema de log com níveis (INFO, WARNING, ERROR, DEBUG).

**4. I/O de Arquivos:**
- [ ] Ler e gravar arquivos, abstraindo os detalhes do sistema de arquivos.

**5. Propagação de eventos:**
- [ ] Adicionar/Remover camadas do Layer System.
- [ ] Lidar com eventos capturados ou propagar as camadas existentes.

**Produtos Esperados:**
- [ ] Uma biblioteca estática ou dinâmica reutilizável que encapsule as interações específicas do sistema operacional.

---

## 2. Sistema de Renderização
**Objetivo:** Renderizar elementos básicos na tela.

**Tarefas principais:**

**1. Inicialização da API gráfica (OpenGL, Vulkan e DirectX):**
- [ ] Criação do contexto gráfico para as APIs, com base no sistema operacional.
- [ ] Inicialização do sistema de renderização.

**2. Pipeline Gráfica Simples:**
- [ ] Renderização de triângulos básicos.
- [ ] Criar shaders básicos (vertex e fragment shaders).

**3. Gerenciamento de Texturas:**
- [ ] Gerenciamento de Texturas.

**4. Sistema de Câmera:**
- [ ] Implementar câmeras básicas com perspectiva e ortográfica.

**Produtos Esperados:**
- [ ] Uma tela renderizando formas simples (triângulos, retângulos) com texturas básicas.

---

## 3. Sistema de Recursos (Assets)
**Objetivo:** Gerenciar os recursos do jogo (texturas, modelos, sons, etc.).

**Tarefas principais:**

**1. Gerenciamento de Texturas:**
- [ ] Cache para evitar carregamentos duplicados.

**2. Carregamento de Modelos:**
- [ ] Suporte para os formatos como OBJ e FBX.

**3. Sistema de Serialização:**
- [ ] Salvamento e carregamento de cenas, configurações e outros dados.

**Produtos Esperados:**
- [ ] Um sistema funcional que carregue, armazene em cache e gerencie texturas, modelos e outros recursos, com suporte para salvar e carregar cenas e configurações.

---

## 4. Sistema de Física Básica
**Objetivo:** Adicionar movimentação e interações físicas simples.

**Tarefas principais:**

**1. Detecção de Colisão:**
- [ ] AABB (Axis-Aligned Bounding Box) e Circle Collision.

**2. Resolução de Colisões:**
- [ ] Adicionar respostas básicas (ex.: empurrar objetos).

**3. Movimentação Física:**
- [ ] Gravitação, forças básicas e simulações simples.

**Produtos Esperados:**
- [ ] Um protótipo com objetos interativos que respondam a forças e colisões visíveis na tela, com movimentação física simples (como gravidade e empurrões).

---

## 5. Sistema de Áudio
**Objetivo:** Reproduzir sons e músicas.

**Tarefas principais:**

**1. Carregamento de Áudio:**
- [ ] Suporte para formatos básicos como WAV ou MP3.

**2. Reprodução de Sons:**
- [ ] Sons em 2D ou 3D com diferentes volumes e loops.

**Produtos Esperados:**
- [ ] Uma aplicação capaz de reproduzir sons e músicas em formatos básicos (WAV/MP3), com suporte a sons em 2D/3D e controle de volume.

---

## 6. Sistema de Entidades e Componentes (ECS - Entity Component System)
**Objetivo:** Criar um framework flexível para gerenciar objetos no jogo.

**Tarefas principais:**

**1. Implementação de Entidades:**
- [ ] Cada entidade representa um objeto no jogo.

**2. Sistemas e Componentes:**
- [ ] Componentes de posição, renderização, física, etc.
- [ ] Sistemas para processar lógica associada a esses componentes.

**Produtos Esperados:**
- [ ] Um framework que permita criar e gerenciar entidades no jogo, com componentes e sistemas interagindo para realizar tarefas específicas (ex.: renderização e física).

---

## 7. Ferramentas de Desenvolvimento
**Objetivo:** Melhorar a produtividade no desenvolvimento.

**Tarefas principais:**

**1. Editor de Cenas:**
- [ ] Uma interface gráfica para criar e editar cenas.

**2. Depurador Visual:**
- [ ] Ferramentas para inspecionar objetos, eventos e dados.

**Produtos Esperados:**
- [ ] Uma interface gráfica inicial para edição de cenas e ferramentas básicas para depuração visual de objetos e eventos.

---

## 8. Otimizações e Recursos Avançados
**Objetivo:** Polir e aprimorar a engine.

**Tarefas principais:**

**1. Instancing e Frustum Culling:**
- [ ] Renderização eficiente de múltiplos objetos.

**2. Sistema de Sombras:**
- [ ] Implementação de sombras dinâmicas.

**3. Otimizações de Performance:**
- [ ] Multi-threading e gerenciamento de memória.

**Produtos Esperados:**
- [ ] Melhorias visíveis no desempenho da engine (como suporte a instancing e culling), renderização eficiente de múltiplos objetos na implementação de sombras dinâmicas.
