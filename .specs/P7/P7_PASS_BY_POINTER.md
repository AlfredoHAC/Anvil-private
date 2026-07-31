# P7 — Event Callback: Pass by Value → Pass by Pointer

> **Status:** ✅ Concluída  
> **Dependência:** Nenhuma (P1–P6 concluídas)  
> **Módulo:** Refatoração — `Windowing/`, `Core/`  
> **Tipo:** Refatoração (mudança de assinatura)  
> **Branch:** `feat/event-pass-by-pointer` → `master`
> **Branch:** `feat/event-pass-by-pointer` → `master`

---

## 1. Contexto e Motivação

### O problema: `Event` é copiado em cada etapa do dispatch

Atualmente, o `Event` é passado **por valor** através de toda a cadeia de dispatch:

```c
// window.h — assinatura atual
typedef void (*EventCallbackFn)(Event event);
```

Isso causa **duas cópias** do struct (~16 bytes cada) em cada evento:

```
Backend cria Event na stack (16 bytes)
    │
    ▼ cópia #1 (~40 bytes de union)
[EventCallbackFn(Event event)]
    │
    ▼ cópia #2
[anvl_layer_stack_dispatch_event(Event event)]
    │
    ▼ cópia #3
[LayerOnEventFn(Layer* layer, Event event)]
```

### O bug funcional: `event.handled` não propaga

Porque cada função recebe uma **cópia diferente**, quando um layer marca `handled = true`, essa modificação **não é vista** pelas funções seguintes:

```c
// layer.c — dispatcher
void anvl_layer_stack_dispatch_event(Event event)   // ← cópia do dispatcher
{
    for (...)
    {
        layer_stack[i]->on_event(layer_stack[i], event);  // ← cópia do layer
        if (event.handled) { break; }   // ← lê a CÓPIA do dispatcher (sempre false!)
    }
}

// application.c — consumer
static void _on_application_event(Layer* layer, Event event)  // ← cópia do consumer
{
    event.handled = true;   // ← modifica a CÓPIA do consumer
}                           //     O dispatcher nunca vê isso.
```

**Resultado:** O dispatcher itera todos os layers mesmo quando um já tratou o evento. O backend Win32 recebe `handled = false` e chama `DefWindowProc` para fechar a janela novamente.

### A solução: passar por ponteiro

```c
// Antes: Event event          →  cópia de ~16 bytes a cada chamada
// Depois: Event* event         →  ponteiro de 8 bytes, mesmo objeto em toda a cadeia
```

Todos compartilham o **mesmo** `Event` na stack. Quando um marca `handled = true`, todos veem.

---

## 2. Design: Mudança de Assinatura

### 2.1 Resumo das Mudanças

```
Caminho atual (problema):
  [Backend] → cópia → [Callback] → cópia → [Dispatcher] → cópia → [Layer]
  handled=true NO dispatcher          handled=true NO layer

Caminho novo (solução):
  [Backend] → &event → [Callback] → &event → [Dispatcher] → &event → [Layer]
  handled=true VISTO em todos os nós
```

### 2.2 Por que `Event*` (não-const)?

O dispatcher e os layers precisam **modificar** `event.handled`. Um ponteiro const impediria isso:

```c
// const Event* → event->handled = true seria erro de compilação
// Event*      → event->handled = true funciona normalmente
```

### 2.3 O que NÃO muda

- **Nenhum `void* user_data`** — closure pattern não é escopo deste P7.
- **Nenhuma vtable** — a assinatura da vtable não muda.
- **Nenhum campo novo nos backends** — não há `user_data` para armazenar.
- **Nenhum arquivo novo** — apenas modificações nos existentes.

---

## 3. API Pública — Mudanças

### 3.1 `Windowing/window.h`

```c
// ANTES:
typedef void (*EventCallbackFn)(Event event);
void anvl_platform_window_set_event_callback(NativeWindow* window, EventCallbackFn event_callback);

// DEPOIS:
typedef void (*EventCallbackFn)(Event* event);
void anvl_platform_window_set_event_callback(NativeWindow* window, EventCallbackFn event_callback);
```

**Mudança:** `Event event` → `Event* event`. Sem `void* data` adicional.

`unset_event_callback` **não muda** — não precisa de parâmetros extras.

### 3.2 `Core/layer.h`

```c
// ANTES:
typedef void (*LayerOnEventFn)(Layer* layer, Event event);
void anvl_layer_stack_dispatch_event(Event event);

// DEPOIS:
typedef void (*LayerOnEventFn)(Layer* layer, Event* event);
void anvl_layer_stack_dispatch_event(Event* event);
```

**Mudança:** `Event event` → `Event* event` em ambos.

### 3.3 `Windowing/event.h` — WindowCloseEvent

**Sem alteração.** O `_pad[1]` permanece — structs vazios são rejeitados por alguns compiladores C. O placeholder garante que a struct tenha tamanho mínimo de 1 byte.

### 3.4 O que NÃO muda

| Arquivo | Status | Razão |
|---------|--------|-------|
| `window_backend.h` | **Sem alteração** | A vtable delega para o backend; a assinatura interna do backend muda, mas a vtable em si não precisa de campo extra |
| `layer.h` — `LayerOnUpdateFn` | **Sem alteração** | Update não recebe Event |
| `layer.h` — `LayerOnEventFn` | **Sem alteração** | O typedef muda, mas a assinatura do callback do layer muda junto |

> **Nota sobre a vtable:** O `window_backend.h` **não muda** porque o `window_set_event_callback` na vtable recebe `void* backend_data` + `EventCallbackFn`. O backend interno (X11/Wayland) armazena o callback no seu struct interno e o invoca com `&event`. A vtable é apenas um ponteiro de função que delega — ela não precisa saber sobre o `user_data` porque não há `user_data`.

---

## 4. Estrutura de Arquivos Afetados

```
src/
├── Windowing/
│   ├── event.h              ← NÃO altera (_pad[1] necessário)
│   ├── window.h             ← MODIFICAR: EventCallbackFn (Event → Event*)
│   ├── Windows/
│   │   └── win32_window.c   ← MODIFICAR: 15 invocações callback(event → &event)
│   └── Linux/
│       ├── linux_window.c   ← NÃO altera (vtable não muda)
│       ├── X11/
│       │   └── x11_backend.c← MODIFICAR: ~9 invocações callback(event → &event)
│       └── Wayland/
│           └── wayland_backend.c ← MODIFICAR: 5 invocações callback(event → &event)
└── Core/
    ├── layer.h              ← MODIFICAR: LayerOnEventFn + dispatch_event
    ├── layer.c              ← MODIFICAR: event.handled → event->handled
    └── application.c        ← MODIFICAR: _on_application_event + acesso via ->
```

**7 arquivos modificados, 3 não alterados.** Nenhum novo. Nenhum removido.

### Detalhamento por arquivo

| # | Arquivo | Linhas estimadas | Tipo |
|---|---------|-----------------|------|
| 1 | `window.h` | ~2 | Assinatura |
| 2 | `layer.h` | ~3 | Assinatura |
| 3 | `layer.c` | ~3 | `event.handled` → `event->handled` |
| 4 | `application.c` | ~10 | Assinatura + `event.` → `event->` |
| 5 | `win32_window.c` | ~15 | `callback(event)` → `callback(&event)` |
| 6 | `x11_backend.c` | ~10 | `callback(event)` → `callback(&event)` |
| 7 | `wayland_backend.c` | ~6 | `callback(event)` → `callback(&event)` |

---

## 5. Detalhamento das Mudanças por Arquivo

### 5.1 `src/Windowing/event.h`

**Sem alteração.** O `_pad[1]` no `WindowCloseEvent` é necessário — structs sem membros são rejeitados por alguns compiladores C (ex: MSVC em modo estrito). Mantido como está.

### 5.2 `src/Windowing/window.h`

```c
// Linha 11: EventCallbackFn
// ANTES:
typedef void (*EventCallbackFn)(Event event);
// DEPOIS:
typedef void (*EventCallbackFn)(Event* event);

// Linha 20: set_event_callback
// ANTES:
void anvl_platform_window_set_event_callback(NativeWindow* window, EventCallbackFn event_callback);
// DEPOIS:
void anvl_platform_window_set_event_callback(NativeWindow* window, EventCallbackFn event_callback);
// (sem alteração — o tipo já mudou via EventCallbackFn)
```

### 5.3 `src/Core/layer.h`

```c
// Linha 9: LayerOnEventFn
// ANTES:
typedef void (*LayerOnEventFn)(Layer* layer, Event event);
// DEPOIS:
typedef void (*LayerOnEventFn)(Layer* layer, Event* event);

// Linha 23: dispatch_event
// ANTES:
void anvl_layer_stack_dispatch_event(Event event);
// DEPOIS:
void anvl_layer_stack_dispatch_event(Event* event);
```

### 5.4 `src/Core/layer.c`

```c
// anvl_layer_stack_dispatch_event — mudar acesso a membros
// ANTES:
void anvl_layer_stack_dispatch_event(Event event)
{
    for (int8 i = layer_stack_length - 1; i >= 0; --i)
    {
        if (layer_stack[i] == NULL || layer_stack[i]->on_event == NULL) { continue; }
        layer_stack[i]->on_event(layer_stack[i], event);
        if (event.handled) { break; }
    }
}

// DEPOIS:
void anvl_layer_stack_dispatch_event(Event* event)
{
    for (int8 i = layer_stack_length - 1; i >= 0; --i)
    {
        if (layer_stack[i] == NULL || layer_stack[i]->on_event == NULL) { continue; }
        layer_stack[i]->on_event(layer_stack[i], event);
        if (event->handled) { break; }
    }
}
```

**Duas mudanças:** parâmetro `Event event` → `Event* event`, e `event.handled` → `event->handled`.

### 5.5 `src/Core/application.c`

```c
// Mudança 1: Assinatura do callback (linha 14)
// ANTES: static void _on_application_event(Layer* layer, Event event);
// DEPOIS: static void _on_application_event(Layer* layer, Event* event);

// Mudança 2: Implementação (linha 74-118)
// ANTES: switch (event.type) { ... }  event.handled = true;
// DEPOIS: switch (event->type) { ... }  event->handled = true;

// Mudança 3: Call site do setter (linha 45)
// ANTES: anvl_platform_window_set_event_callback(app->window, anvl_layer_stack_dispatch_event);
// DEPOIS: anvl_platform_window_set_event_callback(app->window, anvl_layer_stack_dispatch_event);
// (sem alteração — o tipo EventCallbackFn já mudou)
```

**Todas as ocorrências de `event.` que acessam membros do union ou `type`/`handled` mudam para `event->`.**

### 5.6 `src/Windowing/Windows/win32_window.c`

**Mudança única em cada local:** `window->event_callback(event)` → `window->event_callback(&event)`.

A construção do `Event` **não muda** — ainda é `Event event = { .type = ..., .handled = false, ... }`. Apenas a invocação muda.

**15 locais:**

| # | Linha | Evento |
|---|-------|--------|
| 1 | 29 | `WM_CLOSE` |
| 2 | 49 | `WM_SIZE` |
| 3 | 62 | `WM_KEYDOWN` / `WM_SYSKEYDOWN` |
| 4 | 74 | `WM_KEYUP` / `WM_SYSKEYUP` |
| 5 | 86 | `WM_MOUSEMOVE` |
| 6 | 103 | `WM_LBUTTONDOWN` |
| 7 | 120 | `WM_MBUTTONDOWN` |
| 8 | 137 | `WM_RBUTTONDOWN` |
| 9 | 154 | `WM_XBUTTONDOWN` |
| 10 | 171 | `WM_LBUTTONUP` |
| 11 | 188 | `WM_MBUTTONUP` |
| 12 | 205 | `WM_RBUTTONUP` |
| 13 | 222 | `WM_XBUTTONUP` |
| 14 | 237 | `WM_MOUSEWHEEL` |
| 15 | 252 | `WM_MOUSEHWHEEL` |

### 5.7 `src/Windowing/Linux/linux_window.c`

**Sem alteração de lógica.** O `linux_window.c` repassa o callback ao backend via vtable. Como a vtable **não muda** (o backend armazena o callback internamente e o invoca com `&event`), este arquivo **não precisa de alteração**.

> **Correção:** O `linux_window.c` chama `window->backend->window_set_event_callback(window->backend_data, event_callback)`. A vtable `window_set_event_callback` recebe `EventCallbackFn` — que agora é `void(*)(Event*)`. O backend interno armazena esse ponteiro e invoca com `&event`. A vtable em si **não muda de assinatura**.

**Veredito: `linux_window.c` NÃO precisa ser modificado.**

### 5.8 `src/Windowing/Linux/X11/x11_backend.c`

**Mudança única em cada local:** `b_end->event_callback(event)` → `b_end->event_callback(&event)`.

**9 locais:**

| # | Linha | Evento |
|---|-------|--------|
| 1 | 218 | `XCB_CLIENT_MESSAGE` (WM_DELETE_WINDOW) |
| 2 | 239 | `XCB_CONFIGURE_NOTIFY` (resize) |
| 3 | 253 | `XCB_KEY_PRESS` |
| 4 | 266 | `XCB_KEY_RELEASE` |
| 5 | 279 | `XCB_MOTION_NOTIFY` |
| 6 | 302 | `XCB_BUTTON_PRESS` (cliques 1-3) |
| 7 | 316 | `XCB_BUTTON_PRESS` (scroll up) |
| 8 | 330 | `XCB_BUTTON_PRESS` (scroll down) |
| 9 | 352 | `XCB_BUTTON_RELEASE` |

### 5.9 `src/Windowing/Linux/Wayland/wayland_backend.c`

**Mudança única em cada local:** `b_end->event_callback(event)` → `b_end->event_callback(&event)`.

**5 locais:**

| # | Linha | Função | Evento |
|---|-------|--------|--------|
| 1 | 510 | `_on_xdg_toplevel_close` | Window close |
| 2 | 592 | `_on_wl_keyboard_key` | Key press/release |
| 3 | 651 | `_on_wl_pointer_motion` | Mouse move |
| 4 | 690 | `_on_wl_pointer_button` | Mouse button |
| 5 | 713 | `_on_wl_pointer_axis` | Mouse scroll |

---

## 6. Integração com a Codebase Existente

### 6.1 Build System (premake5.lua)

**Sem alterações.** Nenhum arquivo novo, nenhum removido.

### 6.2 PCH (anvlpch.h)

**Sem alterações.**

### 6.3 Ordem de Dependências

Fazer em ordem, de dentro para fora:

```
```c
1. window.h         ← usa event.h
2. layer.h          ← usa event.h
3. layer.c          ← usa layer.h
4. application.c    ← usa layer.h, window.h
5. win32_window.c   ← usa window.h
6. x11_backend.c    ← usa window.h (via anvlpch)
7. wayland_backend.c← usa window.h (via anvlpch)
```

**Nota:** `event.h`, `linux_window.c` e `window_backend.h` NÃO precisam de alteração.

### 6.4 Compatibilidade com P6 (Layer System)

O P6 funciona perfeitamente com esta mudança. O layer dispatcher recebe `Event*`, passa o mesmo ponteiro para cada layer, e cada layer modifica o mesmo objeto. O bug de `handled` não propagar é resolvido.

### 6.5 Compatibilidade com Backends Futuros

Novos backends seguem o mesmo padrão: construir `Event` na stack local, invocar `callback(&event)`. Sem campos extras, sem `user_data`.

---

## 7. Exemplo de Uso (Sem Alteração)

O consumer **não muda** sua forma de usar. O layer system continua igual:

```c
// application.c — antes e depois, o consumer usa da mesma forma:
static void _on_application_event(Layer* layer, Event* event)
{
    // Apenas muda event.type → event->type e event.handled → event->handled
    switch (event->type)
    {
        case ANVL_EVENT_TYPE_WINDOW_CLOSE:
            app_running = false;
            event->handled = true;   // ← agora PROPAGA para o dispatcher
            break;
        case ANVL_EVENT_TYPE_KEY_PRESS:
            ANVIL_CORE_DEBUG("Key: %d", event->key_press.key_code);
            break;
    }
}
```

---

## 8. Plano de Implementação (Passo a Passo)

### Etapa 1: Headers (2 arquivos)

Modificar assinaturas:
1. **`window.h`** — `EventCallbackFn`: `Event` → `Event*`.
2. **`layer.h`** — `LayerOnEventFn` e `dispatch_event`: `Event` → `Event*`.

**Validação intermediária:** `premake5.lua generate`. Deve falhar nos `.c` (esperado).

### Etapa 2: Implementações internas (2 arquivos)

4. **`layer.c`** — `event.handled` → `event->handled`, parâmetro `Event*`.
5. **`application.c`** — `_on_application_event` recebe `Event*`, acesso via `->`.

**Validação intermediária:** `premake5.lua generate`. Deve falhar nos backends (esperado).

### Etapa 3: Backends (3 arquivos)

6. **`win32_window.c`** — 15 invocações: `callback(event)` → `callback(&event)`.
7. **`x11_backend.c`** — 9 invocações: `callback(event)` → `callback(&event)`.
8. **`wayland_backend.c`** — 5 invocações: `callback(event)` → `callback(&event)`.

**Nota:** `linux_window.c` NÃO muda (vtable não altera).

**Validação final:** Compilar e testar.

### Etapa 4: Validação

- **Windows (Win32):** Compilar, abrir janela, fechar (verificar que `WM_CLOSE` é tratado corretamente).
- **Linux (X11):** Compilar, mesmo teste.
- **Linux (Wayland):** Compilar, mesmo teste.

---

## 9. O que NÃO está incluído (Escopo Explícito)

| Item | Status | Razão |
|------|--------|-------|
| `void* user_data` / closure | Não incluído | Sem consumer que precise; adiar para quando aparecer |
| Inter-layer communication | Não incluído | Resolver via shared context ou APIs públicas quando necessário |
| Otimização de union | Não incluído | Struct já é 16 bytes; ganho marginal (< 3 bytes) |
| `const Event*` | Não incluído | Precisa-se modificar `handled` |
| Delta time no update | Não incluído | Sistema de timing será implementado depois |
| Vtable `window_backend.h` | Não alterada | Backend armazena callback internamente; vtable é apenas delegação |
| `linux_window.c` | Não alterado | Repassa callback via vtable; vtable não muda |

---

## 10. Riscos e Mitigações

### Risco 1: Esquecer uma invocação do callback

**Probabilidade:** Média (29 chamadas para atualizar).  
**Mitigação:** Usar `grep` para encontrar todas as ocorrências e verificar cada uma.

```bash
# Buscar todas as invocações de callback com Event por valor:
grep -rn "event_callback(event)" src/
# Ou mais amplo:
grep -rn "->event_callback(event)" src/
```

Após a refatoração, **zero** ocorrências de `callback(event)` sem `&` devem existir.

### Risco 2: `event.handled` não modificado pelo consumer

**Probabilidade:** Baixa.  
**Mitigação:** `Event*` (não-const) permite modificação. O dispatcher lê o mesmo objeto.

### Risco 3: `event.` vs `event->` esquecido

**Probabilidade:** Média (muitas ocorrências de acesso a membros).  
**Mitigação:** Compilador reporta erro em `event.handled` quando `event` é `Event*` (não `Event`). Fácil de corrigir.

---

## 11. Resumo

| Item | Detalhe |
|------|---------|
| **Arquivos novos** | 0 |
| **Arquivos modificados** | 7 (`window.h`, `layer.h`, `layer.c`, `application.c`, `win32_window.c`, `x11_backend.c`, `wayland_backend.c`) |
| **Arquivos NÃO alterados** | `event.h`, `linux_window.c`, `window_backend.h`, `premake5.lua`, `anvlpch.h` |
| **Novas dependências** | Nenhuma |
| **PCH alterado** | Não |
| **API pública alterada** | `EventCallbackFn`, `LayerOnEventFn`, `dispatch_event` |
| **Mudança central** | `Event` por valor → `Event*` por valor |
| **Bug resolvido** | `event.handled` não propaga entre dispatcher e layers |
| **Ganho de performance** | ~32 bytes economizados por evento (0 cópias vs 2 cópias) |
| **Total de invocações para atualizar** | 29 (15 Win32 + 9 X11 + 5 Wayland) |
| **Ordem de implementação** | Headers → layer.c → application.c → backends |
