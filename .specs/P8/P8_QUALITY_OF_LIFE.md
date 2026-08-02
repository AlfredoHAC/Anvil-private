# P8 — Melhorias de Qualidade de Código: Forward Declarations e Assertions

> **Status:** ❌ Não iniciada  
> **Dependência:** Nenhuma (P1–P7 concluídas)  
> **Módulo:** `Tools/` (assert), `src/` (forward decls)  
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

**2 arquivos precisam de forward declarations.**

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
| Debug (`ANVL_CONFIG_DEBUG=1`) | Aborta + log FATAL | Aborta + log FATAL com msg |
| Release (`ANVL_CONFIG_DEBUG=0`) | Null (compila fora) | Null (compila fora) |

**Implementação:**
```c
#ifdef ANVL_CONFIG_DEBUG
#    define ANVL_ASSERT(expr)                                            \
        do {                                                             \
            if (!(expr)) {                                               \
                ANVIL_CORE_FATAL("Assertion failed: %s\n  at %s:%d in %s", \
                                 #expr, __FILE__, __LINE__, __func__);   \
                abort();                                                 \
            }                                                            \
        } while (0)

#    define ANVL_ASSERT_MSG(expr, msg, ...)                              \
        do {                                                             \
            if (!(expr)) {                                               \
                ANVIL_CORE_FATAL("Assertion failed: %s\n  %s\n  at %s:%d in %s", \
                                 #expr, msg, __FILE__, __LINE__, __func__, \
                                 ##__VA_ARGS__);                         \
                abort();                                                 \
            }                                                            \
        } while (0)
#else
#    define ANVL_ASSERT(expr)            ((void)0)
#    define ANVL_ASSERT_MSG(expr, msg, ...) ((void)0)
#endif
```

**Nota sobre `##__VA_ARGS__`:** O operador `##` remove a vírgula antes de `__VA_ARGS__` quando vazio, evitando `ANVIL_CORE_FATAL("msg", , file, line, func)` que causaria erro de compilação. Suportado por GCC, Clang e MSVC.

**Integração com PCH:** Adicionar `#include "Tools/assert.h"` ao `anvlpch.h` para que esteja disponível em todos os arquivos.

---

## 3. API Pública — Mudanças

### 3.1 Novo: `Tools/assert.h`

```c
#ifndef ANVL_ASSERT_HEADER
#define ANVL_ASSERT_HEADER

#include "Tools/logger.h"

#ifdef ANVL_CONFIG_DEBUG

#define ANVL_ASSERT(expr)                                            \
    do {                                                             \
        if (!(expr)) {                                               \
            ANVIL_CORE_FATAL("Assertion failed: %s\n  at %s:%d in %s", \
                             #expr, __FILE__, __LINE__, __func__);   \
            abort();                                                 \
        }                                                            \
    } while (0)

#define ANVL_ASSERT_MSG(expr, msg, ...)                              \
    do {                                                             \
        if (!(expr)) {                                               \
            ANVIL_CORE_FATAL("Assertion failed: %s\n  %s\n  at %s:%d in %s", \
                             #expr, msg, __FILE__, __LINE__, __func__, \
                             ##__VA_ARGS__);                         \
            abort();                                                 \
        }                                                            \
    } while (0)

#else

#define ANVL_ASSERT(expr)            ((void)0)
#define ANVL_ASSERT_MSG(expr, msg, ...) ((void)0)

#endif // ANVL_CONFIG_DEBUG

#endif // !ANVL_ASSERT_HEADER
```

### 3.2 PCH Alterado

`anvlpch.h` ganha `#include "Tools/assert.h"`.

### 3.3 Forward Declarations (3 arquivos)

| Arquivo | Adicionar |
|---------|-----------|
| `src/Tools/logger.c` | `static void _print_timestamp_label();` + `static void _print_level_label(LogLevel level);` + `static void _log_message(LogLevel level, const char* call_module, const char* msg_format, va_list args);` |
| `src/Windowing/Windows/win32_window.c` | `static LRESULT _dispatch_win32_event(NativeWindow* window, UINT umsg, WPARAM wparam, LPARAM lparam);` + `static LRESULT CALLBACK _native_window_proc(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam);` + `static void _peek_and_dispatch_win32_messages(NativeWindow* window);` |

**Nota:** `src/FileIO/Linux/posix_file.c` já tinha forward declaration (só renomeada de `_filemode_to_stdio` → `_filemode_to_string`). `src/FileIO/Windows/win32_fileio.c` já tinha forward declarations (embora sem `static`). `src/Core/layer.c` não tem funções estáticas (apenas variáveis estáticas), então não precisa de forward declarations.

---

## 4. Estrutura de Arquivos

### 4.1 Novos Arquivos

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

---

## 5. Detalhamento das Mudanças por Arquivo

### 5.1 `src/Tools/assert.h` — Novo

Header puro com macros. Não precisa de `.c`. Inclui `Tools/logger.h` para usar `ANVIL_CORE_FATAL`.

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

---

## 6. Integração com a Codebase Existente

### 6.1 Build System (premake5.lua)

**Sem alterações.** Os globs existentes (`"./src/Tools/**"`) capturam o novo `assert.h`.

### 6.2 PCH

`anvlpch.h` ganha `#include "Tools/assert.h"`. Todos os arquivos que incluem o PCH terão `ANVL_ASSERT` disponível.

### 6.3 Dependências

- `assert.h` depende de `logger.h` (para `ANVIL_CORE_FATAL`).

### 6.4 Ordem de Build

```
assert.h          ← novo, sem dependências internas (só logger.h)
Forward decls     ← modificam arquivos existentes
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

### 7.4 Translation Unit Encapsulation

- `assert.h`: header puro, sem `.c`.
- Cada `.c` modificado: apenas adiciona declarações antes das definições.

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

---

## 9. Plano de Implementação (Passo a Passo)

### Etapa 1: Assertion System (1 arquivo)

1. Criar `src/Tools/assert.h` com macros `ANVL_ASSERT` e `ANVL_ASSERT_MSG`.
2. Adicionar `#include "Tools/assert.h"` ao `anvlpch.h`.
3. Validar: compilar e verificar que `ANVL_ASSERT(1 == 1)` não faz nada e `ANVL_ASSERT(1 == 2)` aborta em debug.

### Etapa 2: Forward Declarations (2 arquivos)

1. `src/Tools/logger.c` — adicionar `_print_timestamp_label`, `_print_level_label`, `_log_message`.
2. `src/Windowing/Windows/win32_window.c` — adicionar `_dispatch_win32_event`, `_native_window_proc`, `_peek_and_dispatch_win32_messages`.

### Etapa 3: Validação

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

---

## 11. Resumo

| Item | Detalhe |
|------|---------|
| **Arquivos novos** | 1 (`Tools/assert.h`) |
| **Arquivos modificados** | 3 (`anvlpch.h`, `logger.c`, `win32_window.c`) |
| **Novas dependências** | Nenhuma |
| **Novos links** | Nenhum |
| **PCH alterado** | Sim (`assert.h` adicionado) |
| **Macros novas** | `ANVL_ASSERT`, `ANVL_ASSERT_MSG` |
| **Forward declarations** | 6 funções em 2 arquivos |
