# P5 — File I/O Abstrato

> **Status:** Planejamento  
> **Dependência:** Nenhuma (Platform está estável)  
> **Módulo:** Novo — `FileIO/`

---

## 1. Contexto e Motivação

### Por que um File I/O abstrato?

O engine precisa ler arquivos de forma portável — shaders, texturas, modelos, configs, assets. Sem abstração, cada módulo que precisa de I/O reinventa a roda com `fopen`/`CreateFileA` direto, misturando lógica de negócio com detalhes de plataforma.

Um módulo `FileIO` centralizado resolve isso:

- **Uma API** para todos os consumidores (Furnace, Sandbox, futuro Editor).
- **Backend por plataforma** isolado em `.c` específicos.
- **Zero overhead** quando não há backend específico — `fopen`/`fread`/`fwrite`/`fclose` no Linux, `CreateFile`/`ReadFile`/`WriteFile`/`CloseHandle` no Windows.

### Por que não usar stdio diretamente?

`FILE*` / `fopen` funcionam perfeitamente no Linux. No Windows, `fopen` com caminhos Unicode é problemático (só aceita ANSI). Para um engine que precisa suportar caminhos com caracteres especiais (japonês, emoji, acentos), `CreateFileW` é necessário no Windows. A abstração existe **agora** para que, quando o Furnace precisar carregar shaders com caminhos Unicode, não precise reescrever tudo.

---

## 2. Abordagem: Compile-Time Selection via Premake

### Por que não vtable?

Com exatamente dois backends (Linux e Windows) e seleção em compile-time, uma vtable adiciona overhead desnecessário:

- ~200 linhas de boilerplate (vtable struct, factory, delegação via ponteiros).
- Overhead de indireção em cada chamada de função.
- Complexidade adicional sem ganho real — não há seleção dinâmica, não há 3+ backends.

### Abordagem escolhida: `removefiles` do Premake

O `premake5.lua` já seleciona backends de janela por plataforma via `removefiles`:

```lua
filter "system:linux"
    removefiles { "./**/Windows/**" }

filter "system:windows"
    removefiles { "./**/Linux/**" }
```

Aplicamos o mesmo padrão ao FileIO: cada backend implementa as mesmas funções com o mesmo nome, e o premake garante que apenas o backend correto é compilado. O linker resolve.

**Vantagens:**
- Zero boilerplate de vtable/factory.
- Compilador otimiza chamadas diretas (inline, devirtualization).
- Cada backend é autocontido e self-explanatory.
- Se no futuro precisarmos de um terceiro backend, basta adicionar o `.c` e o `removefiles` correspondente.

---

## 3. API Pública — `FileIO/fileio.h`

### 3.1 Tipos

```c
// Opaque handle — consumers nunca veem a struct interna.
typedef struct FileHandle FileHandle;

// Modo de abertura — alinhado com stdio, mas explícito.
typedef enum FileMode
{
    ANVL_FILE_MODE_READ   = 0,   // "r"  — abre para leitura; arquivo deve existir.
    ANVL_FILE_MODE_WRITE  = 1,   // "w"  — abre para escrita; cria ou truncar.
    ANVL_FILE_MODE_APPEND = 2,   // "a"  — abre para append; cria se não existir.
} FileMode;
```

**Por que `FileMode` enum ao invés de strings?**
- Strings são propensas a typos e não podem ser validados em compile-time.
- Um enum com valores explícitos é mais seguro e mais fácil de mapear para `fopen` modes / `CreateFile` access modes.

**Por que não `FileAccess` / `FileShare` separados?**
- Simplicidade antes de abstração. Se no futuro precisarmos de share modes (concorrent read/write), adicionamos uma flag ou um segundo parâmetro. Não generalizamos antes de precisar.

### 3.2 Funções

```c
// Abre um arquivo. Retorna NULL em erro (logado via ANVIL_CORE_ERROR).
FileHandle* anvl_file_open(const char* path, FileMode mode);

// Lê bytes. Retorna bytes lidos, ou 0 em EOF/erro.
uint64 anvl_file_read(FileHandle* file, void* buffer, uint64 size);

// Escreve bytes. Retorna bytes escritos, ou 0 em erro.
uint64 anvl_file_write(FileHandle* file, const void* buffer, uint64 size);

// Fecha o arquivo e libera recursos.
bool anvl_file_close(FileHandle* file);

// Verifica se o arquivo existe.
bool anvl_file_exists(const char* path);

// Obtém o tamanho do arquivo em bytes.
// Retorna 0 se o arquivo não existe ou não é possível determinar.
uint64 anvl_file_get_size(FileHandle* file);
```

### 3.3 Justificativa das decisões de design

| Decisão | Escolha | Por quê |
|---------|---------|---------|
| `FileHandle*` opaque | Forward declaration no header | Padrão pImpl — consumers não acessam membros |
| `FileMode` enum | 3 valores (Read/Write/Append) | Cobertura mínima útil; expandir depois |
| `uint64` para sizes | `uint64` (typedef do engine) | Consistência com `Core/typedefs.h` |
| `bool` para close/exists | `bool` (stdbool.h) | Semântica clara: sucesso/falha |
| Sem `seek`/`tell` | Não incluído inicialmente | Ninguém pediu; adicionar quando necessário |
| Sem `directory` ops | Não incluído inicialmente | Responsabilidade separada; módulo futuro |
| Sem `copy`/`delete` | Não incluído inicialmente | Operações de metadados; módulo futuro |

---

## 4. Estrutura de Arquivos

```
src/
└── FileIO/
    ├── fileio.h                  ← API pública (opaque, protótipos)
    ├── Linux/
    │   └── posix_fileio.c        ← Backend POSIX (fopen/fread/fwrite/fclose)
    └── Windows/
        └── win32_fileio.c        ← Backend Win32 (CreateFileW/ReadFile/WriteFile/CloseHandle)
```

**3 arquivos no total.** Sem factory, sem `#ifdef`, sem vtable.

### 4.1 `fileio.h` — Header Público

Contém SOMENTE:
- Forward declaration de `FileHandle`
- Enum `FileMode`
- Protótipos das 6 funções públicas

**Não contém:**
- Definição de struct interna
- Inclui de headers de plataforma
- Detalhes de implementação

Cada backend inclui este header e implementa as funções declaradas nele.

### 4.2 Backend POSIX (`posix_fileio.c`)

Implementa as 6 funções usando stdio/POSIX:

| Função | Implementação |
|--------|---------------|
| `anvl_file_open` | `fopen(path, stdio_mode)` — mapeia `FileMode` para `"r"`, `"w"`, `"a"` |
| `anvl_file_read` | `fread(buffer, 1, size, file)` — retorna bytes lidos |
| `anvl_file_write` | `fwrite(buffer, 1, size, file)` — retorna bytes escritos |
| `anvl_file_close` | `fclose(file)` — retorna `true` se `fclose` retorna 0 |
| `anvl_file_exists` | `fopen(path, "r")` e fecha imediatamente (ou `access()` no POSIX) |
| `anvl_file_get_size` | `fseek(EOF) + ftell()` ou `fstat()` no POSIX |

**Dados do backend:** `FILE*` armazenado diretamente no `FileHandle*` (cast de/para `void*`).

**Inclui:** `anvlpch.h` + `FileIO/fileio.h` + `<stdio.h>` + `<sys/stat.h>`

### 4.3 Backend Win32 (`win32_fileio.c`)

Implementa as 6 funções usando Win32 API:

| Função | Implementação |
|--------|---------------|
| `anvl_file_open` | `CreateFileW` com conversão `const char*` → `wchar_t*` via `MultiByteToWideChar(CP_UTF8, ...)` |
| `anvl_file_read` | `ReadFile(handle, buffer, size, &bytes_read, NULL)` |
| `anvl_file_write` | `WriteFile(handle, buffer, size, &bytes_written, NULL)` |
| `anvl_file_close` | `CloseHandle(handle)` |
| `anvl_file_exists` | `GetFileAttributesW(path_wide) != INVALID_FILE_ATTRIBUTES` |
| `anvl_file_get_size` | `GetFileSizeEx(handle, &large_integer)` |

**Dados do backend:** `HANDLE` armazenado diretamente no `FileHandle*` (cast de/para `void*`).

**Conversão de path:** O engine usa `const char*` para paths (padrão C). No Windows, convertemos para `wchar_t*` usando `MultiByteToWideChar(CP_UTF8, ...)`. Caminhos no Windows são tratados como UTF-8 (alinhado com a tendência moderna do Windows 10+).

**Por que `CreateFileW` ao invés de `_wfopen`?**
- `CreateFileW` dá controle fino sobre share mode, creation disposition, e flags.
- Para um engine, o controle fino é preferível — pode-se estender para `FILE_FLAG_OVERLAPPED` no futuro se necessário.

**Inclui:** `anvlpch.h` + `FileIO/fileio.h` + `<windows.h>` + `<wchar.h>` + `<stdlib.h>`

---

## 5. Integração com a Codebase Existente

### 5.1 Build System (premake5.lua)

**Alteração mínima:** adicionar `removefiles` para a pasta errada do FileIO.

```lua
filter "system:linux"
    removefiles { "./src/FileIO/Windows/**" }

filter "system:windows"
    removefiles { "./src/FileIO/Linux/**" }
```

O glob `"./src/**.h"` e `"./src/**.c"` captura recursivamente todos os arquivos. O `removefiles` garante que apenas o backend correto é compilado por plataforma.

### 5.2 Dependências de Link

- **Linux:** Nenhuma nova dependência. POSIX `fopen`/`fread`/`fwrite`/`fclose` são parte da libc.
- **Windows:** Nenhuma nova dependência. `CreateFileW`/`ReadFile`/`WriteFile`/`CloseHandle` são parte do Windows API base.

### 5.3 PCH (anvlpch.h)

**Não adicionar nada ao PCH.** O PCH deve permanecer minimalista. Cada `.c` inclui o que precisa diretamente.

### 5.4 Logging

Usar `ANVIL_CORE_ERROR` para falhas de abertura (ex: arquivo não encontrado, permissão negada).

```c
// Exemplo: posix_fileio.c
FileHandle* anvl_file_open(const char* path, FileMode mode)
{
    const char* stdio_mode = _filemode_to_stdio(mode);
    FILE* fp = fopen(path, stdio_mode);
    if (!fp)
    {
        ANVIL_CORE_ERROR("Failed to open file: %s (mode: %d)", path, mode);
        return NULL;
    }
    return fp;
}
```

### 5.5 Integração com `Application` (futuro)

O `Application` **não** precisa saber sobre o FileIO agora. Quando o Furnace for implementado, ele incluirá `FileIO/fileio.h` diretamente.

---

## 6. Padrões de Código a Seguir

### 6.1 Naming

- Funções: `anvl_file_*` (prefixo `anvl_` + recurso + ação)
- Enums: `FileMode` (PascalCase para tipos, consistente com `EventType`, `LogLevel`)
- Internos: `_posix_*` / `_win32_*` (underscore + nome do backend, para helpers privados)

### 6.2 Ownership

- **Criador = Destroyer:** `anvl_file_open` aloca, `anvl_file_close` libera.
- **Ownership transfer:** O caller recebe o `FileHandle*` e é responsável por fechá-lo.
- **Nenhum shared ownership:** Cada `FileHandle` tem um único dono.

### 6.3 Error Handling

- `NULL` para `open` falhando.
- `0` para `read`/`write` falhando ou EOF.
- `false` para `close`/`exists` falhando.
- Sempre logar falhas com `ANVIL_CORE_ERROR`.
- Sempre checkar `NULL` antes de usar `FileHandle*`.

### 6.4 Translation Unit Encapsulation

Cada `.c` é self-contained:
- `posix_fileio.c` conhece apenas POSIX.
- `win32_fileio.c` conhece apenas Win32.
- Nenhum `.c` inclui headers de outro backend.
- Ambos incluem o mesmo `fileio.h` e implementam as mesmas funções.

---

## 7. Plano de Implementação (Passo a Passo)

### Etapa 1: Header Público (`fileio.h`)

Criar `src/FileIO/fileio.h` com:
- Include guard `ANVL_FILEIO_HEADER`
- `#include "Core/typedefs.h"` (para `uint64`, `bool`)
- Forward declaration `typedef struct FileHandle FileHandle;`
- Enum `FileMode`
- Protótipos das 6 funções

### Etapa 2: Backend POSIX (`posix_fileio.c`)

Criar `src/FileIO/Linux/posix_fileio.c` com:
- `#include "anvlpch.h"` + `#include "FileIO/fileio.h"`
- `#include <stdio.h>` + `#include <sys/stat.h>`
- Implementação direta das 6 funções usando stdio/POSIX
- `FileHandle*` como `FILE*` (cast de/para `void*`)

### Etapa 3: Backend Win32 (`win32_fileio.c`)

Criar `src/FileIO/Windows/win32_fileio.c` com:
- `#include "anvlpch.h"` + `#include "FileIO/fileio.h"`
- `#include <windows.h>` + `<wchar.h>` + `<stdlib.h>`
- Implementação direta das 6 funções usando Win32 API
- `FileHandle*` como `HANDLE` (cast de/para `void*`)
- Conversão de path: `const char*` → `wchar_t*` via `MultiByteToWideChar(CP_UTF8, ...)`

### Etapa 4: Build System

Adicionar `removefiles` ao `premake5.lua`:
```lua
filter "system:linux"
    removefiles { "./src/FileIO/Windows/**" }

filter "system:windows"
    removefiles { "./src/FileIO/Linux/**" }
```

### Etapa 5: Validação

- Compilar no Linux (X11) — verificar que `posix_fileio.c` compila e funciona.
- Compilar no Linux (Wayland) — mesmo teste.
- Compilar no Windows (Win32) — verificar que `win32_fileio.c` compila.
- Teste manual: abrir arquivo de texto, ler, escrever, verificar `exists`, verificar `get_size`.

---

## 8. O que NÃO está incluído (Escopo Explícito)

| Funcionalidade | Status | Razão |
|----------------|--------|-------|
| `anvl_file_seek()` | Não incluído | Ninguém pediu; adicionar quando necessário |
| `anvl_file_directory_create()` | Não incluído | Responsabilidade separada; módulo futuro |
| `anvl_file_copy()` / `delete()` | Não incluído | Operações de metadados; módulo futuro |
| `anvl_file_read_all()` | Não incluído | Helpers são conveniência; adicionar no consumo |
| Path normalization | Não incluído | Cada plataforma tem seu próprio separator; abstrair depois |
| Unicode path support no Linux | Não incluído | `fopen` no Linux aceita UTF-8 nativamente |
| Buffering customizado | Não incluído | `setvbuf()` pode ser adicionado se necessário |
| File locking | Não incluído | Casos de uso específicos; adicionar quando necessário |

---

## 9. Exemplo de Uso (Future Consumer)

```c
#include "FileIO/fileio.h"

// Ler um arquivo inteiro em memória
FileHandle* file = anvl_file_open("assets/shader.glsl", ANVL_FILE_MODE_READ);
if (!file) { ANVIL_CORE_ERROR("Shader not found: assets/shader.glsl"); return; }

uint64 size = anvl_file_get_size(file);
char* buffer = malloc(size + 1);
uint64 read = anvl_file_read(file, buffer, size);
buffer[read] = '\0';  // Null-terminate

anvl_file_close(file);

// Usar buffer...
free(buffer);
```

---

## 10. Resumo

| Item | Detalhe |
|------|---------|
| **Arquivos novos** | 3 (`fileio.h`, `posix_fileio.c`, `win32_fileio.c`) |
| **Arquivos modificados** | 1 (`premake5.lua` — adicionar `removefiles`) |
| **Novas dependências** | Nenhuma |
| **Novos links** | Nenhum |
| **PCH alterado** | Não |
| **API pública** | 6 funções + 1 enum + 1 tipo opaque |
| **Backend Linux** | POSIX (fopen/fread/fwrite/fclose) |
| **Backend Windows** | Win32 API (CreateFileW/ReadFile/WriteFile/CloseHandle) |
| **Seleção de backend** | Compile-time via `removefiles` do Premake |
| **Naming** | `anvl_file_*` (prefixo `anvl_` + recurso) |
