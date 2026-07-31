# P6 — Layer System (Update Loop)

> **Status:** ✅ Concluído  
> **Dependência:** Nenhuma (Platform estável)  
> **Módulo:** `Core/` (layer.h + layer.c)  
> **Branch:** `feat/layer-system` → `master`

---

## 1. Contexto e Motivação

### O problema atual

Cada módulo que precisa reagir a algo (eventos, update por frame, render) precisa de um callback separado. O `EventCallbackFn` só suporta **um** callback. Não há mecanismo para múltiplos consumers reagirem ao mesmo evento ou ao mesmo tick de atualização.

### Por que um Layer System?

Um **layer** é uma unidade de lógica que reage a ciclos do engine. Exemplos concretos:

| Layer | Responsabilidade |
|-------|-----------------|
| `ApplicationLayer` | Lógica default (fechar janela, resize) |
| `InputLayer` | Mapear teclas para ações (ESC = pause) |
| `FurnaceLayer` | Ajustar viewport no resize |
| `ImGuiLayer` | Consumir mouse/teclado para widgets |
| `DebugOverlay` | Capturar eventos para gizmos no editor |

O padrão **Update Method** ([Game Programming Patterns](https://gameprogrammingpatterns.com/update-method.html), Robert Nystrom) mostra que a solução é simples: **cada frame, o engine itera sobre uma coleção de objetos e chama `update()` em cada um**. Cada objeto encapsula seu próprio comportamento.

### Por que array ao invés de linked list?

| Critério | Linked List | Array |
|----------|-------------|-------|
| Cache performance | Pior (ponteiros espalhados) | Melhor (contíguo) |
| Push/Pop | O(1) / O(n) | O(1) / O(n) (swap+shift) |
| Iteração | Ponteiro `next` a cada passo | For-loop simples |
| Intuição | Abstrata | `for (i = 0; i < count; i++)` |
| Padrão em engines | Raro | Padrão (Nystrom, Unity, XNA) |

O array é **cache-friendly** e **intuitivo** — exatamente o que o Update Method recomenda.

---

## 2. Design: Array de Layers com Callbacks Genéricos

### Conceito

```
Application mantém um array de layers:

layers[0] = ApplicationLayer  ← default, empilhado primeiro
layers[1] = InputLayer        ← empilhado depois
layers[2] = DebugOverlay      ← empilhado por último

Por frame:
  for (i = count - 1; i >= 0; i--)
      layers[i]->on_update(delta_time);

  for (i = count - 1; i >= 0; i--)
      layers[i]->on_event(event);
```

**Ordem LIFO** (último empilhado = primeiro chamado):
- Layers adicionadas por último (ImGui, DebugOverlay) processam primeiro.
- Application processa por último (lógica default / fallback).
- Remoção segura: iterar de trás pra frente evita problemas ao remover durante iteração.

### Callbacks por Layer

Cada layer pode implementar **qualquer combinação** dos seguintes callbacks:

```c
typedef void (*LayerOnUpdateFn)(Layer* layer);
typedef void (*LayerOnEventFn)(Layer* layer, Event event);
typedef void (*LayerOnRenderFn)(Layer* layer);  // Futuro — Furnace
```

Um layer **não precisa implementar todos**. Callbacks não implementados são `NULL` e são pulados no loop.

### Por que callbacks opcionais (NULL = skip)?

- Zero overhead para layers que só precisam de `on_event` (não iteramos `on_update` se é NULL).
- Simplicidade: o consumer define apenas o que precisa.
- Sem necessidade de flags ou enums de capacidade.

---

## 3. API Pública — `Core/layer.h`

### 3.1 Tipos

```c
// Forward declaration — consumers não veem a struct interna.
typedef struct Layer Layer;

// Callbacks opcionais — NULL significa "não implementado".
typedef void (*LayerOnUpdateFn)(Layer* layer);
typedef void (*LayerOnEventFn)(Layer* layer, Event event);

// Estrutura do layer — consumers definem uma struct maior e fazem cast.
struct Layer
{
    const char*           name;
    LayerOnUpdateFn       on_update;
    LayerOnEventFn        on_event;
};
```

### 3.2 Funções

```c
// Registra um layer no array (empilha).
// O layer deve permanecer válido enquanto estiver no array.
void anvl_layer_stack_push(Layer* layer);

// Remove o último layer da pilha (LIFO).
// Não libera a memória do layer — o caller é responsável.
void anvl_layer_stack_pop();

// Remove um layer específico do array.
// Busca reverse (consistente com LIFO), shift dos seguintes.
void anvl_layer_stack_remove(Layer* layer);

// Dispara evento para todos os layers (ordem LIFO).
// Se um layer marcar `event.handled = true`, a propagação para.
void anvl_layer_stack_dispatch_event(Event event);

// Atualiza todos os layers (ordem LIFO).
// `on_update` NULL é pulado.
void anvl_layer_stack_call_update();

// Retorna o número de layers no array.
uint32 anvl_layer_stack_length();

// Limpa todos os layers (shutdown).
// Apenas reseta o count — não libera memória.
void anvl_layer_stack_clear();
```

### 3.3 Justificativa das decisões de design

| Decisão | Escolha | Por quê |
|---------|---------|---------|
| Array (não linked list) | Array com count | Cache-friendly, padrão Update Method |
| Callbacks opcionais (NULL) | NULL = skip | Zero overhead, consumer define só o que precisa |
| `Layer*` nos callbacks | Self-pointer | Permite cast para struct concreta |
| Ordem LIFO | Último empilhado = primeiro chamado | Sobreposição natural, remoção segura |
| `handled` como interrupção | `bool` no `Event` | Zero overhead, simples |
| Sem `on_render` ainda | Futuro | Furnace não existe; API preparada |
| Sem thread safety | Não incluído | Engine single-threaded |
| `layer_count` público | Sim | Debug/util |
| `clear` público | Sim | Shutdown limpo |

---

## 4. Estrutura de Arquivos

```
src/
└── Core/
    ├── layer.h              ← API pública
    └── layer.c              ← Implementação (array)
```

**2 arquivos no total.**

### 4.1 `layer.h` — Header Público

Contém SOMENTE:
- Forward declaration de `Layer`
- Typedefs dos callbacks (`LayerOnUpdateFn`, `LayerOnEventFn`)
- Definição da struct `Layer`
- Protótipos das 6 funções

**Não contém:**
- Definição do array interno
- Inclui de headers de plataforma
- Detalhes de implementação

### 4.2 `layer.c` — Implementação

Array interno:
```c
#define LAYER_STACK_MAX_LENGTH 32

static Layer* layer_stack[LAYER_STACK_MAX_LENGTH];
static uint32 layer_stack_length = 0;
```

- `push`: insere no próximo índice livre (O(1)), verifica limite, checa NULL.
- `pop`: remove o topo (LIFO), checa stack vazia, seta slot como NULL.
- `remove`: busca reverse, shift dos seguintes, checa NULL e stack vazia.
- `dispatch_event`: for-loop reverso (count-1 → 0), chama `on_event`, verifica `handled`.
- `call_update`: for-loop reverso (count-1 → 0), chama `on_update` se não for NULL.
- `length`: retorna `layer_stack_length`.
- `clear`: seta `layer_stack_length = 0` (não libera memória — caller responsável).

**Inclui:** `anvlpch.h` + `Core/layer.h`

---

## 5. Integração com a Codebase Existente

### 5.1 `application.c` — Mudança Principal

**Antes:**
```c
anvl_platform_set_window_event_callback(app->window, anvl_application_on_event);
```

**Depois:**
```c
// Criar e registrar o ApplicationLayer como default.
static Layer app_layer = {
    .name        = "Application_Layer",
    .on_update   = NULL,
    .on_event    = _on_application_event,
};

anvl_layer_stack_push(&app_layer);

// O callback da janela agora aponta para o dispatcher.
anvl_platform_window_set_event_callback(
    app->window, anvl_layer_stack_dispatch_event);
```

A função pública `anvl_application_on_event` foi removida. O callback interno `_on_application_event` recebe `Layer*` e `Event`, e chama `_on_application_window_close()` (agora `static`) para setar `app_running = false`.

**Shutdown order:**
```c
void anvl_application_shutdown(Application* app)
{
    if (!app) { return; }

    anvl_layer_stack_clear();
    anvl_platform_window_unset_event_callback(app->window);
    anvl_platform_window_destroy(app->window);

    free(app);
}
```

### 5.2 `application_run` — Adicionar `update`

```c
void anvl_application_run(Application* app)
{
    if (!app) { return; }

    while (app_running)
    {
        anvl_layer_stack_call_update();
        anvl_platform_window_update(app->window);
    }
}
```

> **Nota:** `on_update` não recebe `delta_time` por enquanto. O sistema de timing (roadmap seção 1) será implementado depois.

### 5.3 `window.h` — Sem alterações no callback

O `EventCallbackFn` permanece como `(Event event)` — o dispatcher é chamado como qualquer outro callback. A mudança é **apenas no consumer**.

> **Nota:** O P7 (closure pattern) mudará o callback para `(const Event*, void* data)`. O P6 é compatível com ambos.

### 5.4 Build System (premake5.lua)

**Sem alterações.** O glob `"./src/Core/**"` já captura `layer.c`.

### 5.5 PCH (anvlpch.h)

**Não adicionar nada ao PCH.**

### 5.6 Logging

- Sem log em push/pop (operações comuns).
- Sem log em update (frame a frame seria ruído).
- Logs de eventos permanecem nos layers individuais.

---

## 6. Padrões de Código a Seguir

### 6.1 Naming

- Funções: `anvl_layer_stack_*` (prefixo `anvl_` + módulo + recurso + ação)
- Tipos: `Layer`, `LayerOnUpdateFn`, `LayerOnEventFn` (PascalCase)
- Internos: `_on_application_*` (underscore + nome do layer)
- Constante: `LAYER_STACK_MAX_LENGTH` (UPPER_SNAKE_CASE)

### 6.2 Ownership

- **Array owns indices, caller owns data:** O array gerencia os ponteiros. O caller é responsável pela memória do struct concreto.
- **Lifetime:** O layer deve permanecer válido enquanto estiver no array. O `pop` não libera memória.
- **Shutdown:** `anvl_layer_stack_clear()` reseta o count (não libera memória).

### 6.3 Error Handling

- `push` loga erro e retorna se o array está cheio (limite de `LAYER_STACK_MAX_LENGTH`) ou `layer` é NULL.
- `pop` retorna silenciosamente se a stack está vazia.
- `remove` loga erro e retorna se `layer` é NULL ou a stack está vazia.
- `dispatch_event` / `call_update` nunca falham.

### 6.4 Translation Unit Encapsulation

- `layer.c` conhece apenas o array interno.
- Consumers incluem `layer.h` e definem suas próprias structs com cast.

---

## 7. Exemplo de Uso (Future Consumer)

```c
#include "Core/layer.h"
#include "Windowing/event.h"

// 1. Definir a struct concreta do layer (com dados privados).
typedef struct
{
    Layer base;              // Primeiro campo — permite cast seguro.
    bool  input_enabled;
} InputLayer;

// 2. Implementar os callbacks que precisa.
static void _input_layer_on_update(Layer* layer)
{
    InputLayer* self = (InputLayer*)layer;
    if (!self->input_enabled) { return; }
    // Lógica de atualização por frame.
}

static void _input_layer_on_event(Layer* layer, Event event)
{
    InputLayer* self = (InputLayer*)layer;
    if (!self->input_enabled) { return; }

    switch (event.type)
    {
        case ANVL_EVENT_TYPE_KEY_PRESS:
            if (event.key_press.key_code == 41) // ESC
            {
                ANVIL_CORE_INFO("Game paused!");
                event.handled = true;
            }
            break;
        default: break;
    }
}

// 3. Criar e registrar.
InputLayer input_layer = {
    .base = {
        .name        = "InputLayer",
        .on_update   = _input_layer_on_update,
        .on_event    = _input_layer_on_event,
    },
    .input_enabled = true,
};

anvl_layer_stack_push(&input_layer.base);
```

**Padrão chave:** O primeiro campo da struct concreta é sempre `Layer base` — cast seguro.

---

## 8. Plano de Implementação (Passo a Passo)

### Etapa 1: Header Público (`layer.h`)

Criar `src/Core/layer.h` com:
- Include guard `ANVL_LAYER_HEADER`
- `#include "Core/typedefs.h"` (para `uint32`, `float32`, `bool`)
- `#include "Windowing/event.h"` (para `Event`, `EventType`)
- Typedefs dos callbacks
- Definição da struct `Layer`
- Protótipos das 6 funções

### Etapa 2: Implementação (`layer.c`)

Criar `src/Core/layer.c` com:
- `#include "anvlpch.h"` + `#include "Core/layer.h"`
- Array estático + count
- Implementação das 6 funções

### Etapa 3: Integração com `application.c`

- Criar `app_layer` como `Layer` estático.
- Criar `_on_application_event(Layer*, Event)` como callback interno.
- Criar `_on_application_window_close()` como `static`.
- Registrar a layer no `anvl_application_init`.
- Mudar callback da janela para `anvl_layer_stack_dispatch_event`.
- Adicionar `anvl_layer_stack_call_update()` no `anvl_application_run`.
- Adicionar `anvl_layer_stack_clear()` e `anvl_platform_window_unset_event_callback()` no `anvl_application_shutdown`.

### Etapa 4: Build System

**Sem alterações.** Glob existente cobre `src/Core/**`.

### Etapa 5: Validação

- Compilar e verificar que a aplicação inicia normalmente.
- Verificar que eventos são logados via layers.
- Teste manual: abrir/fechar janela, pressionar teclas, mover mouse.

---

## 9. O que NÃO está incluído (Escopo Explícito)

| Funcionalidade | Status | Razão |
|----------------|--------|-------|
| `on_render` callback | Preparado na API, não implementado | Furnace não existe |
| Layer priorities | Não incluído | LIFO oferece sobreposição natural; priorities adicionam complexidade |
| Nested dispatch | Não incluído | Pode causar recursão |
| Event queuing | Não incluído | Responsabilidade do backend |
| Thread safety | Não incluído | Engine single-threaded |
| Dynamic resizing | Não incluído | Array fixo de 32 é suficiente |
| remove_by_name | Não incluído | Consumer mantém ponteiro |
| Active/inactive layers | Não incluído | NULL callback já resolve (skip) |

---

## 10. Relação com o P7 (Closure Pattern)

| Aspecto | P6 (Layer System) | P7 (Closure Pattern) |
|---------|-------------------|---------------------|
| Responsabilidade | Roteamento para múltiplos consumers + update loop | Eficiência + contexto no callback |
| Mudança de API | Nenhuma na interface do backend | `(Event)` → `(const Event*, void* data)` |
| Dependência | Independente | Pode depender do P6 |

**Ordem:** P6 primeiro, P7 depois. O P6 funciona perfeitamente com o callback atual.

---

## 11. Resumo

| Item | Detalhe |
|------|---------|
| **Arquivos novos** | 2 (`layer.h`, `layer.c`) |
| **Arquivos modificados** | 4 (`application.c`, `window.h`, `linux_window.c`, `win32_window.c`) |
| **Novas dependências** | Nenhuma |
| **Novos links** | Nenhum |
| **PCH alterado** | Não |
| **API pública** | 7 funções (`push`, `pop`, `remove`, `dispatch_event`, `call_update`, `length`, `clear`) + 2 callback typedefs + 1 tipo |
| **Implementação** | Array fixo (32 slots) + count |
| **Callbacks** | `on_update` (sem delta_time) + `on_event` (opcionais, NULL = skip) |
| **Ordem** | LIFO (último empilhado = primeiro chamado) |
| **Naming** | `anvl_layer_stack_*` (prefixo `anvl_` + módulo + recurso) |
| **Pattern** | Update Method (Nystrom) — array de objetos, loop por frame |
