#include "anvlpch.h"

#include "File/file.h"

// clang-format off
#include <windows.h>
#include <fileapi.h>
// clang-format on

typedef struct AnvlFile
{
    HANDLE      pointer;
    const char* path;
    uint32      mode;
    uint8       disposition;
} AnvlFile;

uint32 _filemode_to_desired_access(AnvlFileMode mode);
uint8  _filemode_to_creation_disposition(AnvlFileMode mode);

AnvlFile* anvl_file_open(const char* path, AnvlFileMode mode)
{
    AnvlFile* file = malloc(sizeof(AnvlFile));
    if (!file) { return NULL; }

    ANVIL_ASSERT(path != NULL);
    file->mode        = _filemode_to_desired_access(mode);
    file->disposition = _filemode_to_creation_disposition(mode);
    file->path        = _strdup(path);
    file->pointer     = CreateFileA(file->path,
                                    file->mode,
                                    FILE_SHARE_READ,
                                    NULL,
                                    file->disposition,
                                    FILE_ATTRIBUTE_NORMAL,
                                    NULL);
    if (file->pointer == INVALID_HANDLE_VALUE)
    {
        DWORD err = GetLastError();
        free(file);
        ANVIL_CORE_ERROR("Failed to open file %s: %lu", path, err);
        return NULL;
    }

    if (mode == ANVL_FILE_MODE_APPEND)
    {
        BOOL result = SetFilePointerEx(file->pointer, (LARGE_INTEGER){0}, NULL, FILE_END);
        if (!result)
        {
            CloseHandle(file->pointer);
            free(file);
            return NULL;
        }
    }

    return file;
}

uint64 anvl_file_read(AnvlFile* file, void* buffer, uint64 size)
{
    ANVIL_ASSERT(file != NULL && file->pointer != INVALID_HANDLE_VALUE);

    DWORD bytes_read = 0;
    BOOL  result     = ReadFile(file->pointer, buffer, (DWORD)size, &bytes_read, NULL);

    if (!result) { return 0; }

    if (bytes_read == 0) { return 0; }

    return (uint64)bytes_read;
}

uint64 anvl_file_write(AnvlFile* file, const void* buffer, uint64 size)
{
    ANVIL_ASSERT(file != NULL && file->pointer != INVALID_HANDLE_VALUE);

    DWORD bytes_written = 0;
    BOOL  result        = WriteFile(file->pointer, buffer, (DWORD)size, &bytes_written, NULL);

    if (!result) { return 0; }

    if (bytes_written == 0) { return 0; }

    return (uint64)bytes_written;
}

bool anvl_file_close(AnvlFile* file)
{
    if (!file || file->pointer == INVALID_HANDLE_VALUE) { return false; }

    BOOL closed = CloseHandle(file->pointer);
    if (!closed) { return false; }
    file->pointer = NULL;

    free((void*)file->path);
    free(file);

    return true;
}

bool anvl_file_exists(const char* path)
{
    DWORD result = GetFileAttributesA(path);
    if (result == INVALID_FILE_ATTRIBUTES) { return false; }

    return true;
}

uint64 anvl_file_get_size(AnvlFile* file)
{
    ANVIL_ASSERT(file != NULL && file->pointer != INVALID_HANDLE_VALUE);

    LARGE_INTEGER file_size;
    BOOL          result = GetFileSizeEx(file->pointer, &file_size);
    if (!result) { return 0; }

    if (file_size.QuadPart < 0) { return 0; }

    return (uint64)file_size.QuadPart;
}

uint32 _filemode_to_desired_access(AnvlFileMode mode)
{
    switch (mode)
    {
        case ANVL_FILE_MODE_READ  : return GENERIC_READ; break;
        case ANVL_FILE_MODE_WRITE : return GENERIC_WRITE; break;
        case ANVL_FILE_MODE_APPEND: return GENERIC_WRITE; break;
    }

    ANVIL_ASSERT_MSG(0, "Invalid AnvlFileMode: %d", mode);
    return 0;
}

uint8 _filemode_to_creation_disposition(AnvlFileMode mode)
{
    switch (mode)
    {
        case ANVL_FILE_MODE_READ  : return OPEN_EXISTING; break;
        case ANVL_FILE_MODE_WRITE : return CREATE_ALWAYS; break;
        case ANVL_FILE_MODE_APPEND: return OPEN_EXISTING; break;
    }

    ANVIL_ASSERT_MSG(0, "Invalid AnvlFileMode: %d", mode);
    return 0;
}
