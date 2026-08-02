# P8 — Melhorias de Qualidade de Código: Forward Declarations e Assertions

> **Status:** ✅ Concluída  
> **Dependência:** Nenhuma (P1–P7 concluídas)  
> **Módulo:** `Tools/` (assert), `src/` (forward decls + assertions)  
> **Tipo:** Refatoração

---

## 1. Contexto e Motivação

### 1.1 Forward Declarations de Funções Estáticas

**O problema:** Em C, quando uma função `static` é chamada antes de ser definida, o compilador assume implicitamente que retorna `int` e aceita argumentos variádicos. Isso funciona em C++ (que exige protótipos) mas em C é um comportamento implícito que esconde bugs.

**Por que importa agora:** Com 27 arquivos `.c`/`.h`, é fácil chamar uma função antes de defini-la. Um erro de tipo no parâmetro não seria detectado pelo compilador — seria um bug silencioso em runtime.

**Estado atual da codebase:**

| Arquivo | Forward declarations? | Funções estáticas |
|---------|----------------------|-------------------|
| `src/anvlpch.c` | N/A (sem funções estáticas) | 0 |
| `src/Core/application.c` | ✅ | `_on_application_event`, `_on_application_window_close` |
| `src/Core/layer.c` | ❌ | 0 (apenas variáveis estáticas) |
| `src/FileIO/Linux/posix_file.c` | ✅ | `_filemode_to_string` (renomeado de `_filemode_to_stdio`) |
| `src/FileIO/Windows/win32_fileio.c` | ✅ | `_filemode_to_desired_access`, `_filemode_to_creation_disposition` (já existiam) |
| `src/Tools/logger.c` | ❌ | `_print_timestamp_label`, `_print_level_label`, `_log_message` |
| `src/Windowing/Windows/win32_window.c` | ❌ | `_dispatch_win32_event`, `_native_window_proc`, `_peek_and_dispatch_win32_messages` |
| `src/Windowing/Linux/linux_window.c` | ✅ | `_window_backend_create`, `_window_backend_detect` |
| `src/Windowing/Linux/X11/x11_backend.c` | ✅ | 8 funções (todas declaradas) |
| `src/Windowing/Linux/Wayland/wayland_backend.c` | ✅ | ~20 funções (todas declaradas) |

**2 arquivos precisavam de forward declarations.**

### 1.2 Sistema de Assertions

**O problema:** Não há mecanismo de assertion no engine. Erros de pré-condição (ponteiros NULL, valores fora de intervalo, estados inválidos) são tratados com `if` + `return` silencioso ou `ANVIL_CORE_ERROR` + `return`. Em desenvolvimento, queremos que esses erros **parem a execução** para ser vistos imediatamente.

**Por que não usar `assert()` do `<assert.h>`?**
- `assert()` não tem mensagem customizável (só imprime a expressão).
- `assert()` é compilado fora em builds de release (via `NDEBUG`), então bugs desaparecem em produção.
- Não se integra com o sistema de logging do engine.

**Solução:** Um sistema de assertion customizado que:
- Tem mensagem formatada (como `printf`).
- Pode ser desabilitado em release builds via flag de compile-time.
- Se integra com o logger do engine.
- Mostra arquivo + linha + função (via `__FILE__`, `__LINE__`, `__func__`).

---

## 2. Design

### 2.1 Forward Declarations

**Regra:** Toda função `static` deve ter sua declaração antes do primeiro uso.

**Formato:**
```c
// Antes da primeira função pública ou struct, após includes:
static ReturnType _function_name(Type1 arg1, Type2 arg2);
```

**Posição no arquivo:** Após includes e typedefs, antes da primeira definição de função. Mantém a ordem:
1. Includes
2. Typedefs / struct definitions
3. Forward declarations de `static` functions
4. Static variables (se houver)
5. Definições de funções

### 2.2 Assertion System

**Arquivo:** `src/Tools/assert.h` (header puro, sem `.c`).

**Design:** Macros puramente em header. Sem `.c` necessário.

```c
// assert.h

#ifndef ANVL_ASSERT_HEADER
#define ANVL_ASSERT_HEADER

// ANVL_ASSERT(expr) — assertion sem mensagem.
// Em debug: aborta com mensagem padrão.
// Em release: compilado fora.
#define ANVL_ASSERT(expr)

// ANVL_ASSERT_MSG(expr, msg, ...) — assertion com mensagem formatada.
// Em debug: aborta com mensagem customizável.
// Em release: compilado fora.
#define ANVL_ASSERT_MSG(expr, msg, ...)

#endif // !ANVL_ASSERT_HEADER
```

**Comportamento:**

| Configuração | `ANVL_ASSERT(expr)` | `ANVL_ASSERT_MSG(expr, msg, ...)` |
|---|---|---|
| Debug (`ANVL_CONFIG_DEBUG=1`) | Break + log FATAL | Break + log FATAL com msg |
| Release (`ANVL_CONFIG_DEBUG=0`) | Null (compila fora) | Null (compila fora) |

**Implementação:**
```c
#ifdef ANVL_CONFIG_DEBUG
#    define ANVL_ASSERT(expr)                                            \
        do {                                                             \
            if (!(expr)) {                                               \
                ANVIL_CORE_FATAL("Assertion failed: %s\n  at %s:%d in %s", \
                                 #expr, __FILE__, __LINE__, __func__);   \
                ANVIL_BREAK();                                           \
            }                                                            \
        } while (0)

#    define ANVL_ASSERT_MSG(expr, msg, ...)                              \
        do {                                                             \
            if (!(expr)) {                                               \
                ANVIL_CORE_FATAL("Assertion failed: %s\n  %s\n  at %s:%d in %s", \
                                 #expr, msg, __FILE__, __LINE__, __func__, \
                                 ##__VA_ARGS__);                         \
                ANVIL_BREAK();                                           \
            }                                                            \
        } while (0)
#else
#    define ANVL_ASSERT(expr)            ((void)0)
#    define ANVL_ASSERT_MSG(expr, msg, ...) ((void)0)
#endif
```

**Nota sobre `##__VA_ARGS__`:** O operador `##` remove a vírgula antes de `__VA_ARGS__` quando vazio, evitando `ANVIL_CORE_FATAL("msg", , file, line, func)` que causaria erro de compilação. Suportado por GCC, Clang e MSVC.

**Nota sobre `ANVIL_BREAK()`:** Definido em `Platform/platform_detection.h` — `__debugbreak()` no Windows (INT3, para no debugger), `raise(SIGTRAP)` no Linux. Mais útil que `abort()` para debugging: para no debugger em vez de crashar silenciosamente.

**Integração com PCH:** Adicionar `#include "Tools/assert.h"` ao `anvlpch.h` para que esteja disponível em todos os arquivos.

---

## 3. API Pública — Mudanças

### 3.1 Novo: `Tools/assert.h`

```c
#ifndef ANVL_ASSERT_HEADER
#define ANVL_ASSERT_HEADER

#include "Platform/platform_detection.h"
#include "Tools/logger.h"

#ifdef ANVL_CONFIG_DEBUG
#    define ANVL_ASSERT(expression)                                                               \
        do                                                                                         \
        {                                                                                          \
            if (!expression)                                                                       \
            {                                                                                      \
                ANVIL_CORE_FATAL("Assertion failed: %s\n at %s:%d in %s",                          \
                                 #expression,                                                      \
                                 __FILE__,                                                         \
                                 __LINE__,                                                         \
                                 __func__);                                                        \
                ANVIL_BREAK();                                                                     \
            }                                                                                      \
        } while (0)

#    define ANVL_ASSERT_MSG(expression, msg, ...)                                                 \
        do                                                                                         \
        {                                                                                          \
            if (!expression)                                                                       \
            {                                                                                      \
                ANVIL_CORE_FATAL("Assertion failed: %s\n %s\n at %s:%d in %s",                     \
                                 #expression,                                                      \
                                 msg,                                                              \
                                 __FILE__,                                                         \
                                 __LINE__,                                                         \
                                 __func__,                                                         \
                                 ##__VA_ARGS__);                                                   \
                ANVIL_BREAK();                                                                     \
            }                                                                                      \
        } while (0)
#else
#    define ANVL_ASSERT(expression)               ((void)0)
#    define ANVL_ASSERT_MSG(expression, msg, ...) ((void)0)
#endif

#endif // ANVL_ASSERT_HEADER
```

### 3.2 PCH Alterado

`anvlpch.h` ganha `#include "Tools/assert.h"`.

### 3.3 Forward Declarations (3 arquivos)

| Arquivo | Adicionar |
|---------|-----------|
| `src/Tools/logger.c` | `static void _print_timestamp_label();` + `static void _print_level_label(LogLevel level);` + `static void _log_message(LogLevel level, const char* call_module, const char* msg_format, va_list args);` |
| `src/Windowing/Windows/win32_window.c` | `static LRESULT _dispatch_win32_event(NativeWindow* window, UINT umsg, WPARAM wparam, LPARAM lparam);` + `static LRESULT CALLBACK _native_window_proc(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam);` + `static void _peek_and_dispatch_win32_messages(NativeWindow* window);` |
| `src/FileIO/Linux/posix_file.c` | `const char* _filemode_to_string(FileMode mode);` (já existia, só renomeada de `_filemode_to_stdio`) |

**Nota:** `src/FileIO/Windows/win32_fileio.c` já tinha forward declarations (`_filemode_to_desired_access`, `_filemode_to_creation_disposition`). `src/Core/layer.c` não tem funções estáticas (apenas variáveis estáticas), então não precisa de forward declarations.

### 3.4 Assertions Aplicadas (8 arquivos)

| Arquivo | Assertions |
|---------|-----------|
| `src/Core/application.c` | `app != NULL` em `anvl_application_run` e `anvl_application_shutdown` |
| `src/Core/layer.c` | `layer != NULL` em `push`, `layer_stack_length > 0` em `push`/`pop`/`remove`, `layer != NULL` em `remove` |
| `src/FileIO/Linux/posix_file.c` | `path != NULL` em `anvl_file_open`, `file != NULL && file->pointer != NULL` em `read`/`write`/`get_size` |
| `src/FileIO/Windows/win32_fileio.c` | `path != NULL` em `anvl_file_open`, `file != NULL && file->pointer != INVALID_HANDLE_VALUE` em `read`/`write`/`get_size`, `ANVL_ASSERT_MSG` para FileMode inválido em `_filemode_to_desired_access`/`_filemode_to_creation_disposition` |
| `src/Windowing/Linux/linux_window.c` | `window != NULL` em `create`/`destroy`, `backend != NULL` em `create`, `backend_data != NULL` em `create`, `destroy != NULL` em `destroy`, `callback != NULL` em `set_event_callback` |
| `src/Windowing/Windows/win32_window.c` | `window != NULL` em `create`, `callback != NULL` em `set_event_callback` |
| `src/Windowing/Linux/X11/x11_backend.c` | `backend != NULL` em `x11_backend_shutdown` |
| `src/Windowing/Linux/Wayland/wayland_backend.c` | `backend != NULL` em `wayland_backend_shutdown` |

**Nota:** `src/Tools/logger.c` também tem assertions internas: `level` range em `anvl_logger_set_level` e `_log_message`, `call_module != NULL` e `msg_format != NULL` em `_log_message`.

---

## 4. Estrutura de Arquivos

### 4.1 Novo Arquivo

```
src/
└── Tools/
    └── assert.h              ← Novo: macros de assertion
```

### 4.2 Arquivos Modificados

| Arquivo | Mudança |
|---------|---------|
| `src/anvlpch.h` | Adicionar `#include "Tools/assert.h"` |
| `src/Tools/logger.c` | Adicionar forward declarations de `_print_timestamp_label`, `_print_level_label`, `_log_message` |
| `src/Windowing/Windows/win32_window.c` | Adicionar forward declarations de `_dispatch_win32_event`, `_native_window_proc`, `_peek_and_dispatch_win32_messages` |
| `src/FileIO/Linux/posix_file.c` | Forward declaration de `_filemode_to_string` (já existia, só renomeada) |
| `src/Core/application.c` | Adicionar `ANVIL_ASSERT(app != NULL)` em `run` e `shutdown` |
| `src/Core/layer.c` | Adicionar assertions em `push`, `pop`, `remove` |
| `src/FileIO/Linux/posix_file.c` | Adicionar assertions de null check em `open`, `read`, `write`, `get_size` |
| `src/FileIO/Windows/win32_fileio.c` | Adicionar assertions de null check em `open`, `read`, `write`, `get_size` + `ANVL_ASSERT_MSG` para FileMode inválido |
| `src/Windowing/Linux/linux_window.c` | Adicionar assertions em `create` e `destroy` |
| `src/Windowing/Linux/X11/x11_backend.c` | Adicionar `ANVIL_ASSERT(backend != NULL)` em `x11_backend_shutdown` |
| `src/Windowing/Linux/Wayland/wayland_backend.c` | Adicionar `ANVIL_ASSERT(backend != NULL)` em `wayland_backend_shutdown` |
| `src/Platform/platform_detection.h` | `ANVIL_BREAK()` definido como `((void)0)` para plataformas não suportadas |

---

## 5. Detalhamento das Mudanças por Arquivo

### 5.1 `src/Tools/assert.h` — Novo

Header puro com macros. Inclui `Platform/platform_detection.h` para `ANVIL_BREAK()` e `Tools/logger.h` para `ANVIL_CORE_FATAL`.

**Posição:** Após includes, antes de qualquer uso.

### 5.2 `src/anvlpch.h` — Modificado

```c
#ifndef ANVL_PRECOMPILED_HEADER
#define ANVL_PRECOMPILED_HEADER
// IWYU pragma: begin_exports

#include "Core/typedefs.h"
#include "Tools/logger.h"
#include "Tools/assert.h"    // ← ADICIONAR

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

// IWYU pragma: end_exports
#endif // !ANVL_PRECOMPILED_HEADER
```

### 5.3 `src/Tools/logger.c` — Forward Declarations

```c
#include "anvlpch.h"
#include "Platform/platform_detection.h"
#include "logger.h"
#include <time.h>

#define MAX_LOG_MSG_LENGTH 256
static LogLevel current_level;

// Forward declarations
static void _print_timestamp_label();
static void _print_level_label(LogLevel level);
static void _log_message(LogLevel level, const char* call_module,
                         const char* msg_format, va_list args);
```

### 5.4 `src/Windowing/Windows/win32_window.c` — Forward Declarations

```c
#include "anvlpch.h"
#include "Windowing/window.h"
#include <windows.h>
#include <windowsx.h>
#include <winuser.h>

struct NativeWindow { ... };

// Forward declarations
static LRESULT _dispatch_win32_event(NativeWindow* window, UINT umsg,
                                     WPARAM wparam, LPARAM lparam);
static LRESULT CALLBACK _native_window_proc(HWND hwnd, UINT umsg,
                                            WPARAM wparam, LPARAM lparam);
static void _peek_and_dispatch_win32_messages(NativeWindow* window);
```

### 5.5 `src/FileIO/Linux/posix_file.c` — Forward Declaration

```c
#include "anvlpch.h"
#include "FileIO/fileio.h"
#include <stdio.h>

typedef struct FileHandle { ... } FileHandle;

const char* _filemode_to_string(FileMode mode);  // ← JÁ EXISTIA, só renomeada
```

### 5.6 `src/Core/application.c` — Assertions

```c
void anvl_application_run(Application* app)
{
    ANVIL_ASSERT(app != NULL);  // ← ADICIONAR
    // ...
}

void anvl_application_shutdown(Application* app)
{
    ANVIL_ASSERT(app != NULL);  // ← ADICIONAR
    // ...
}
```

### 5.7 `src/Core/layer.c` — Assertions

```c
void anvl_layer_stack_push(Layer* layer)
{
    ANVIL_ASSERT(layer != NULL);                          // ← ADICIONAR
    ANVIL_ASSERT(layer_stack_length + 1 <= LAYER_STACK_MAX_LENGTH);  // ← ADICIONAR
    // ...
}

void anvl_layer_stack_pop()
{
    ANVIL_ASSERT(layer_stack_length > 0);  // ← ADICIONAR
    // ...
}

void anvl_layer_stack_remove(Layer* layer)
{
    ANVIL_ASSERT(layer != NULL);  // ← ADICIONAR
    ANVIL_ASSERT(layer_stack_length > 0);  // ← ADICIONAR
    // ...
}
```

### 5.8 `src/FileIO/Linux/posix_file.c` — Assertions

```c
FileHandle* anvl_file_open(const char* path, FileMode mode)
{
    FileHandle* file = malloc(sizeof(FileHandle));
    if (!file) { return NULL; }

    ANVIL_ASSERT(path != NULL);  // ← ADICIONAR
    // ...
}

uint64 anvl_file_read(FileHandle* file, void* buffer, uint64 size)
{
    ANVIL_ASSERT(file != NULL && file->pointer != NULL);  // ← ADICIONAR
    // ...
}

uint64 anvl_file_write(FileHandle* file, const void* buffer, uint64 size)
{
    ANVIL_ASSERT(file != NULL && file->pointer != NULL);  // ← ADICIONAR
    // ...
}

uint64 anvl_file_get_size(FileHandle* file)
{
    ANVIL_ASSERT(file != NULL && file->pointer != NULL);  // ← ADICIONAR
    // ...
}
```

### 5.9 `src/FileIO/Windows/win32_fileio.c` — Assertions

```c
FileHandle* anvl_file_open(const char* path, FileMode mode)
{
    FileHandle* file = malloc(sizeof(FileHandle));
    if (!file) { return NULL; }

    ANVIL_ASSERT(path != NULL);  // ← ADICIONAR
    // ...
}

uint64 anvl_file_read(FileHandle* file, void* buffer, uint64 size)
{
    ANVIL_ASSERT(file != NULL && file->pointer != INVALID_HANDLE_VALUE);  // ← ADICIONAR
    // ...
}

uint64 anvl_file_write(FileHandle* file, const void* buffer, uint64 size)
{
    ANVIL_ASSERT(file != NULL && file->pointer != INVALID_HANDLE_VALUE);  // ← ADICIONAR
    // ...
}

uint64 anvl_file_get_size(FileHandle* file)
{
    ANVIL_ASSERT(file != NULL && file->pointer != INVALID_HANDLE_VALUE);  // ← ADICIONAR
    // ...
}

uint32 _filemode_to_desired_access(FileMode mode)
{
    switch (mode) {
        case ANVL_FILE_MODE_READ  : return GENERIC_READ; break;
        case ANVL_FILE_MODE_WRITE : return GENERIC_WRITE; break;
        case ANVL_FILE_MODE_APPEND: return GENERIC_WRITE; break;
    }
    ANVIL_ASSERT_MSG(0, "Invalid FileMode: %d", mode);  // ← ADICIONAR
}

uint8 _filemode_to_creation_disposition(FileMode mode)
{
    switch (mode) {
        case ANVL_FILE_MODE_READ  : return OPEN_EXISTING; break;
        case ANVL_FILE_MODE_WRITE : return CREATE_ALWAYS; break;
        case ANVL_FILE_MODE_APPEND: return OPEN_EXISTING; break;
    }
    ANVIL_ASSERT_MSG(0, "Invalid FileMode: %d", mode);  // ← ADICIONAR
}
```

### 5.10 `src/Windowing/Linux/linux_window.c` — Assertions

```c
NativeWindow* anvl_platform_window_create(const char* window_title,
                                          uint16      window_width,
                                          uint16      window_height)
{
    NativeWindow* window = malloc(sizeof(NativeWindow));
    ANVIL_ASSERT(window != NULL);  // ← ADICIONAR

    window->backend = _window_backend_create(window);
    ANVIL_ASSERT(window->backend != NULL);  // ← ADICIONAR

    window->backend_data = window->backend->backend_init();
    ANVIL_ASSERT(window->backend_data != NULL);  // ← ADICIONAR

    window->backend->window_create(window->backend_data, window_title, window_width, window_height);

    return window;
}

void anvl_platform_window_destroy(NativeWindow* window)
{
    ANVIL_ASSERT(window != NULL);  // ← ADICIONAR
    // ...
}

void anvl_platform_window_set_event_callback(NativeWindow* window, EventCallbackFn event_callback)
{
    ANVIL_ASSERT(event_callback != NULL);  // ← ADICIONAR
    // ...
}
```

### 5.11 `src/Windowing/Linux/X11/x11_backend.c` — Assertion

```c
void x11_backend_shutdown(void* backend)
{
    ANVIL_ASSERT(backend != NULL);  // ← ADICIONAR
    // ...
}
```

### 5.12 `src/Windowing/Linux/Wayland/wayland_backend.c` — Assertion

```c
void wayland_backend_shutdown(void* backend)
{
    ANVIL_ASSERT(backend != NULL);  // ← ADICIONAR
    // ...
}
```

### 5.13 `src/Platform/platform_detection.h` — `ANVIL_BREAK()` fallback

```c
#if defined(ANVIL_CONFIG_DEBUG)
#    if defined(ANVIL_PLATFORM_WINDOWS)
#        define ANVIL_BREAK() __debugbreak()
#    elif defined(ANVIL_PLATFORM_LINUX)
#        define ANVIL_BREAK() raise(SIGTRAP)
#    else
#        error OS platform not supported!
#    endif
#else
#    define ANVIL_BREAK()      ((void)0)  // ← Fallback para plataformas não suportadas
#    define ANVIL_CONFIG_DEBUG 0
#endif
```

---

## 6. Integração com a Codebase Existente

### 6.1 Build System (premake5.lua)

**Sem alterações.** Os globs existentes (`"./src/Tools/**"`) capturam o novo `assert.h`.

### 6.2 PCH

`anvlpch.h` ganha `#include "Tools/assert.h"`. Todos os arquivos que incluem o PCH terão `ANVL_ASSERT` disponível.

### 6.3 Dependências

- `assert.h` depende de `logger.h` (para `ANVIL_CORE_FATAL`).
- `assert.h` depende de `platform_detection.h` (para `ANVIL_BREAK()`).

### 6.4 Ordem de Build

```
assert.h          ← novo, sem dependências internas (só logger.h + platform_detection.h)
Forward decls     ← modificam arquivos existentes
Assertions        ← aplicadas em 8 arquivos
```

---

## 7. Padrões de Código a Seguir

### 7.1 Naming

- Assertions: `ANVL_ASSERT`, `ANVL_ASSERT_MSG` (macro, UPPER_SNAKE_CASE).
- Internos: `_filemode_*`, `_print_*`, `_log_*`, `_dispatch_*`, `_native_*`, `_peek_*` (underscore + nome do backend).

### 7.2 Ownership

- **Forward declarations:** Zero custo em runtime, zero ownership.
- **Assertion:** Zero ownership — é uma macro.

### 7.3 Error Handling

- `ANVL_ASSERT`: aborta o programa. Não há recovery.
- `ANVL_ASSERT_MSG`: aborta com mensagem customizada.
- **Regra:** Assertions são para bugs de programação (condições que *nunca* deveriam acontecer). Não usar em caminhos de inicialização onde `NULL` é um resultado válido e esperado.

### 7.4 Translation Unit Encapsulation

- `assert.h`: header puro, sem `.c`.
- Cada `.c` modificado: apenas adiciona declarações antes das definições e/ou assertions nos pontos corretos.

---

## 8. Exemplo de Uso

### 8.1 Assertion

```c
#include "Tools/assert.h"

void process_data(int* data, int size)
{
    ANVL_ASSERT(data != NULL);                    // Sem msg
    ANVL_ASSERT_MSG(size > 0, "Size must be positive, got %d", size);  // Com msg
    // ... process data ...
}
```

### 8.2 Assertion em Engine (caso real)

```c
void anvl_layer_stack_push(Layer* layer)
{
    ANVIL_ASSERT(layer != NULL);
    ANVIL_ASSERT(layer_stack_length + 1 <= LAYER_STACK_MAX_LENGTH);
    // ...
}

uint64 anvl_file_read(FileHandle* file, void* buffer, uint64 size)
{
    ANVIL_ASSERT(file != NULL && file->pointer != NULL);
    // ...
}
```

---

## 9. Plano de Implementação (Passo a Passo)

### Etapa 1: Assertion System (1 arquivo)

1. Criar `src/Tools/assert.h` com macros `ANVL_ASSERT` e `ANVL_ASSERT_MSG`.
2. Adicionar `#include "Tools/assert.h"` ao `anvlpch.h`.
3. Validar: compilar e verificar que `ANVL_ASSERT(1 == 1)` não faz nada e `ANVL_ASSERT(1 == 2)` aborta em debug.

### Etapa 2: Forward Declarations (3 arquivos)

1. `src/Tools/logger.c` — adicionar `_print_timestamp_label`, `_print_level_label`, `_log_message`.
2. `src/Windowing/Windows/win32_window.c` — adicionar `_dispatch_win32_event`, `_native_window_proc`, `_peek_and_dispatch_win32_messages`.
3. `src/FileIO/Linux/posix_file.c` — forward declaration de `_filemode_to_string` (já existia, só renomeada).

### Etapa 3: Assertions nos Arquivos Existentes (8 arquivos)

1. `src/Core/application.c` — `ANVIL_ASSERT(app != NULL)` em `run` e `shutdown`.
2. `src/Core/layer.c` — assertions em `push`, `pop`, `remove`.
3. `src/FileIO/Linux/posix_file.c` — assertions de null check em `open`, `read`, `write`, `get_size`.
4. `src/FileIO/Windows/win32_fileio.c` — assertions de null check + `ANVL_ASSERT_MSG` para FileMode inválido.
5. `src/Windowing/Linux/linux_window.c` — assertions em `create` e `destroy`.
6. `src/Windowing/Windows/win32_window.c` — assertions em `create` e `set_event_callback`.
7. `src/Windowing/Linux/X11/x11_backend.c` — assertion em `x11_backend_shutdown`.
8. `src/Windowing/Linux/Wayland/wayland_backend.c` — assertion em `wayland_backend_shutdown`.

### Etapa 4: Validação

- Compilar no Windows (Win32).
- Compilar no Linux (X11 e Wayland).
- Teste: `ANVL_ASSERT(1 == 2)` aborta em debug, compila limpo em release.

---

## 10. O que NÃO está incluído (Escopo Explícito)

| Funcionalidade | Status | Razão |
|----------------|--------|-------|
| Sandbox layer | Não incluído | Ver P9 (Sandbox Executable) |
| Assertion em release | Não incluído | `ANVL_CONFIG_DEBUG` controla; release builds não abortam |
| Forward declarations em layer.c | Não aplicável | layer.c não tem funções estáticas |
| Forward declarations em anvlpch.c | Não aplicável | anvlpch.c não tem funções estáticas |
| Assertion com stack trace | Não incluído | Complexidade desnecessária; `__FILE__` + `__LINE__` + `__func__` é suficiente |
| Assertions em caminhos de inicialização | Não incluído | `anvl_application_init`, `anvl_platform_window_create`, etc. retornam `NULL` em falha — é comportamento esperado, não bug |

---

## 11. Resumo

| Item | Detalhe |
|------|---------|
| **Arquivos novos** | 1 (`Tools/assert.h`) |
| **Arquivos modificados** | 11 (`anvlpch.h`, `logger.c`, `win32_window.c`, `posix_file.c`, `win32_fileio.c`, `application.c`, `layer.c`, `linux_window.c`, `x11_backend.c`, `wayland_backend.c`, `platform_detection.h`) |
| **Novas dependências** | Nenhuma |
| **Novos links** | Nenhum |
| **PCH alterado** | Sim (`assert.h` adicionado) |
| **Macros novas** | `ANVL_ASSERT`, `ANVL_ASSERT_MSG` |
| **Forward declarations** | 6 funções em 2 arquivos (3 se contar renomeação) |
| **Assertions aplicadas** | 8 arquivos, ~25 assertions totais |
