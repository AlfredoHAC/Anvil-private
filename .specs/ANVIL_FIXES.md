# Relatório de Análise — Bugs, UB e Code Smells

> Analisado em: 2026-06-19

> Finalizado em: 2026-06-21

---

## 🐛 Bugs Críticos / Comportamento Não Definido (UB)

### 1. `win32_platform.c:215` — Uso de `window->instance` antes da inicialização
```c
NativeWindow* window = malloc(sizeof(NativeWindow));
// ...
window_class.hInstance = window->instance;  // ← instance NÃO foi inicializado!
```
`malloc` não zera a memória. `window->instance` contém lixo e é usado como `hInstance` do `WNDCLASSEXA`.

**Correção:** Adicionar `memset(window, 0, sizeof(NativeWindow));` logo após o `malloc`, ou inicializar com `{0}`.

---

### 2. `win32_platform.c:186` — `WM_CLOSE` retorna `DefWindowProcA` em vez de `0`
```c
case WM_CLOSE:
    window->EventCallback(event);
    break;
// ...
return DefWindowProcA(window->handle, umsg, wparam, lparam);
```
O padrão Win32 para `WM_CLOSE` é retornar `0` para impedir o fechamento. Retornar `DefWindowProcA` ignora completamente a lógica do callback e força o fechamento. Se o callback setou `handled = true`, isso não tem efeito.

**Correção:** Verificar `event.handled` e retornar `0` se verdadeiro, ou apenas `return 0;` para `WM_CLOSE`.

---

### 3. `win32_platform.c:260-271` — Falta `UnregisterClass` no destroy
```c
void anvlPlatformWindowDestroy(NativeWindow* window)
{
    if (window->handle)
        DestroyWindow(window->handle);
    free(window);
    window = NULL;  // ← Inútil: é parâmetro por valor
}
```
A classe de janela registrada em `anvlPlatformWindowCreate` nunca é desregistrada. Isso causa vazamento de recursos do sistema e pode causar falha se a função for chamada múltiplas vezes (a segunda chamada ao `RegisterClassExA` falha com `ERROR_CLASS_ALREADY_EXISTS`).

**Correção:** Adicionar `UnregisterClassA("ANVL Main Window", ...)` no destroy, ou mover o unregister para um cleanup global.

---

### 4. `win32_platform.c:203-207` — `free(window)` após `malloc` falhar
```c
NativeWindow* window = malloc(sizeof(NativeWindow));
if (!window)
{
    free(window);  // ← free(NULL) é safe, mas este código é enganoso
    return NULL;
}
```
Se `malloc` retorna `NULL`, chamar `free(NULL)` é tecnicamente seguro pelo padrão C, mas é um **code smell** que indica confusão. Se o desenvolvedor pensar que está "limpando" algo, isso não faz sentido.

**Correção:** Apenas `return NULL;`.

---

### 5. `application.c:20` — Mesmo padrão de `free(NULL)`
```c
app->internal = malloc(sizeof(ApplicationData));
if (!app->internal)
{
    free(app->internal);  // ← app->internal é NULL aqui
    return false;
}
```

**Correção:** Remover a linha `free(app->internal)`.

---

### 6. `win32_platform.c:191` — `GetWindowLongPtr` sem verificação de erro
```c
NativeWindow* window = (NativeWindow*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
if (window && window->handle == hwnd)
{
    return _ProcessEvent(window, umsg, wparam, lparam);
}
```
`GetWindowLongPtr` pode retornar `0` se o valor não foi definido. O check `window->handle == hwnd` é uma heurística frágil — um ponteiro corrompido com `handle == hwnd` acidentalmente passaria.

**Correção:** Verificar explicitamente que o retorno de `GetWindowLongPtr` não é `0` (ou usar `LONG_PTR` como tipo intermediário).

---

### 7. `application.c:8` — Variável global não inicializada
```c
static bool app_running;  // ← Valor indeterminado!
```
Variáveis estáticas globais em C são zero-initialized, então tecnicamente vale `false`, mas é **code smell** que deve ser explícito.

**Correção:** `static bool app_running = false;`

---

### 8. `anvil.c:8` — Struct `Application` parcialmente inicializada
```c
Application app = {.name = "AnvilFramework", .hints = {.window_width = 1280, .window_height = 720}};
```
O campo `internal` fica com valor indeterminado. Se `anvlAppInit` falhar antes de usá-lo (ou se `anvlAppShutdown` for chamada sem `anvlAppInit`), `free(app->internal)` em `application.c:66` causa **UB**.

**Correção:** Inicializar com `{0}` ou `.internal = NULL`.

---

### 9. `logger.c:15-25` — `localtime()` não é thread-safe
```c
current_localtime = localtime(&current_time_raw);
fprintf(stderr, timestamp_format, current_localtime->tm_hour, ...);
```
Se duas threads logarem simultaneamente, `localtime` sobrescreve o mesmo buffer estático interno.

**Correção:** Usar `localtime_r()` (POSIX) ou `localtime_s()` (MSVC).

---

### 10. `logger.c:65-78` — Parâmetro `call_module` ignorado em `_LogMessage`
```c
static void _LogMessage(LogLevel level, const char* call_module, const char* msg_format, va_list args)
{
    // ...
    fprintf(stderr, "%s: ", call_module);  // ← Usado aqui
    // ...
}

void anvlLogInfo(const char* call_module, const char* msg_format, ...)
{
    _LogMessage(Info, "ANVIL", msg_format, args);  // ← call_module é IGNORADO!
}
```
Todos os `anvlLogX` functions passam `"ANVIL"` hardcoded em vez do parâmetro `call_module`. O parâmetro é completamente inútil.

**Correção:** Passar `call_module` real para `_LogMessage`, ou remover o parâmetro das assinaturas públicas.

---

## ⚠️ Code Smells / Problemas de Qualidade

### 11. `win32_platform.c:245-253` — `_PollEvents` não verifica retorno de `PeekMessage`
```c
while (PeekMessageA(&msg, window->handle, 0, 0, PM_REMOVE))
{
    TranslateMessage(&msg);
    DispatchMessageA(&msg);
}
```
Se `PeekMessage` retorna `-1` (erro), o `while` interpreta como verdadeiro e entra num loop infinito com dados inválidos.

**Correção:** Verificar `(int32)PeekMessageA(...) != 0`.

---

### 12. `win32_platform.c:40-53` — Teclas virtuais mapeadas diretamente como `key_code`
```c
.key_code = (uint16)wparam  // WPARAM de WM_KEYDOWN é um VK code, não scan code
```
VK codes são inteiros maiores que `uint16` em alguns casos. Além disso, nenhum modificador (Shift, Ctrl, Alt) é capturado — `modifier_set` é sempre `0`.

**Correção:** Usar `GetKeyState()` para capturar modificadores e validar o range de `key_code`.

---

### 13. `win32_platform.c:166-167, 177-178` — Scroll com perda de precisão
```c
.y_offset = (float32)GET_WHEEL_DELTA_WPARAM(wparam) > 0 ? 1.0f : -1.0f
```
`GET_WHEEL_DELTA_WPARAM` retorna `WHEEL_DELTA` (120) por notch, mas aqui é truncado para `±1`. O scroll real pode ser múltiplos de 120 em hardware moderno.

**Correção:** Usar o valor bruto ou dividir por `WHEEL_DELTA` para manter a proporção correta.

---

### 14. `win32_platform.c:28-35` — `WM_SIZE` com `SIZE_MINIMIZED` não tratado
Quando a janela é minimizada, `LOWORD(lparam)` e `HIWORD(lparam)` retornam `0`. Isso pode causar crashes downstream se o engine tentar criar um framebuffer de tamanho `0x0`.

**Correção:** Adicionar check para `wparam != SIZE_MINIMIZED` antes de gerar o evento.

---

### 15. `logger.c:34-60` — `switch` sem `default` case
```c
switch (level)
{
case Fatal: ...
// ...
}
fprintf(stderr, level_label_format, color, level_str, "\033[0m");
```
Se um valor inválido de `LogLevel` for passado, `color`, `level_str` são usados não-inicializados.

**Correção:** Adicionar `default:` com fallback seguro.

---

### 16. `logger.c:67` — Comparação signed/unsigned implícita
```c
if (level > current_level)  // LogLevel é enum (signed), mas pode mudar
```
Enums em C são assinados por padrão, então funciona agora, mas é frágil se alguém mudar o tipo.

**Correção:** Cast explícito ou garantir que ambos sejam do mesmo tipo.

---

### 17. `application.c:50-56` — Loop principal sem yield / sleep
```c
void anvlAppRun(Application* app)
{
    while (app_running)
    {
        anvlPlatformWindowUpdate(app->internal->window);  // ← CPU spin!
    }
}
```
O loop roda a **100% de um core** da CPU. Não há `Sleep(0)` ou mecanismo de throttling.

**Correção:** Adicionar `Sleep(1)` ou usar `MsgWaitForMultipleObjects` para yield.

---

### 18. `event.h:73-86` — Union anônimo sem tagging explícito seguro
```c
typedef struct Event {
    EventType type;
    bool handled;
    union { ... };  // ← Acesso direto por campo, sem verificação de type
} Event;
```
Qualquer código que acesse `event.mouse_move.x` sem verificar `event.type == MouseMove` causa **UB**.

**Correção:** Adicionar um macro de acesso seguro ou documentar o padrão obrigatório de verificação.

---

### 19. `application.h:25-26` — Callbacks globais sem contexto
```c
void anvlApplicationOnEvent(Event event);       // ← Sem context/user_data
void anvlApplicationOnWindowClose();            // ← Estado global implícito via app_running
```
Não há forma de passar `user_data` para os callbacks, impedindo múltiplas instâncias de `Application`.

**Correção:** Adicionar parâmetro `void* user_data` aos callbacks.

---

### 20. `win32_platform.c:225-227` — Janela criada sem message loop próprio
```c
window->handle = CreateWindowExA(WS_EX_APPWINDOW | WS_EX_ACCEPTFILES, ...);
```
A janela é criada com `WS_EX_APPWINDOW` mas não há verificação se o processo já tem um message queue. Se o thread principal não for o dono da janela, mensagens podem ser perdidas.

---

## 📋 Resumo: Lista de Correções Prioritárias

|Feito?| No. | Arquivo | Linha | Severidade | Descrição |
|---|---|---------|-------|-----------|-----------|
|[x]| 1 | `win32_platform.c` | 215 | 🔴 Crítico | Uso de `window->instance` não inicializado |
|[x]| 2 | `win32_platform.c` | 186 | 🔴 Crítico | `WM_CLOSE` sempre fecha a janela (ignora callback) |
|[x]| 3 | `win32_platform.c` | 260-271 | 🔴 Crítico | Falta `UnregisterClass`; loop de classe se recriado |
|[x]| 4 | `application.c` | 8 | 🟡 Médio | Global `app_running` não inicializada explicitamente |
|[x]| 5 | `anvil.c` | 8 | 🟡 Médio | Struct com `internal` não inicializado (UB no shutdown) |
|[x]| 6 | `win32_platform.c` | 191 | 🟡 Médio | `GetWindowLongPtr` sem validação de erro |
|[x]| 7 | `logger.c` | 21 | 🟡 Médio | `localtime()` não thread-safe |
|[x]| 8 | `application.c` | 20 | 🔵 Baixo | `free(NULL)` desnecessário após malloc falhar |
|[x]| 9 | `win32_platform.c` | 203 | 🔵 Baixo | Mesmo padrão de `free(NULL)` |
|[x]| 10 | `logger.c` | 85+ | 🔵 Baixo | Parâmetro `call_module` ignorado em todas as funções |
|[x]| 11 | `win32_platform.c` | 247 | 🟡 Médio | `PeekMessage` sem verificação de retorno `-1` |
|[ ]| 12 | `application.c` | 50-56 | 🟡 Médio | CPU spin no loop principal (sem yield) |
|[ ]| 13 | `win32_platform.c` | 40-53 | 🔵 Baixo | Modificadores de teclado não capturados |
|[ ]| 14 | `win32_platform.c` | 166 | 🔵 Baixo | Scroll wheel perde precisão (truncado para ±1) |
|[x]| 15 | `win32_platform.c` | 28-35 | 🟡 Médio | `WM_SIZE` com `SIZE_MINIMIZED` gera dimensões 0x0 |
|[x]| 16 | `logger.c` | 34-60 | 🟡 Médio | Switch sem default case (valores inválidos de LogLevel) |

---

## Prioridade de Ação Recomendada

### Bloco 1 — Corrigir imediatamente (causam crashes/UB):
1. Item #1: Inicializar `window->instance` com `{0}` no malloc
2. Item #2: Tratar `WM_CLOSE` retornando `0` quando handled
3. Item #3: Adicionar `UnregisterClass` e cleanup de classe
4. Item #5: Inicializar struct Application com `{0}`

### Bloco 2 — Corrigir em breve (comportamento incorreto):
5. Item #6: Validar retorno de `GetWindowLongPtr`
6. Item #7: Usar `localtime_s()` / `localtime_r()`
7. Item #11: Verificar retorno de `PeekMessage`
8. Item #12: Adicionar yield no game loop

### Bloco 3 — Melhorias (qualidade de código):
9. Itens #8, #9: Remover `free(NULL)` desnecessários
10. Item #10: Corrigir parâmetro `call_module` ou removê-lo
11. Items #13-16: Melhorar captura de eventos e logging
