#include "anvlpch.h"

#include "FileIO/fileio.h"
#include <stdio.h>

typedef struct FileHandle
{
    FILE*       pointer;
    const char* path;
    const char* mode;
} FileHandle;

const char* _filemode_to_stdio(FileMode mode);

FileHandle* anvl_file_open(const char* path, FileMode mode)
{
    FileHandle* file = malloc(sizeof(FileHandle));
    if (!file) { return NULL; }

    file->path    = strdup(path);
    file->mode    = _filemode_to_stdio(mode);
    file->pointer = fopen(path, file->mode);
    if (!file->pointer)
    {
        free(file);
        ANVIL_CORE_ERROR("Failed to open file: %s (mode: %s)", path, file->mode);
        return NULL;
    }

    return file;
}

uint64 anvl_file_read(FileHandle* file, void* buffer, uint64 size)
{
    if (!file || !file->pointer)
    {
        ANVIL_CORE_ERROR("Failed to read file %s", file->path);
        return 0;
    }

    size_t result = fread(buffer, 1, size, file->pointer);

    if (result == 0 && ferror(file->pointer))
    {
        ANVIL_CORE_ERROR("Failed to read file %s", file->path);
        return 0;
    }

    return (uint64)result;
}

uint64 anvl_file_write(FileHandle* file, const void* buffer, uint64 size)
{
    if (!file || !file->pointer)
    {
        ANVIL_CORE_ERROR("Failed to write to file %s", file->path);
        return 0;
    }

    size_t result = fwrite(buffer, 1, size, file->pointer);

    if (result == 0 && ferror(file->pointer))
    {
        ANVIL_CORE_ERROR("Failed to write to file %s", file->path);
        return 0;
    }

    return (uint64)result;
}

bool anvl_file_close(FileHandle* file)
{
    if (!file || !file->pointer)
    {
        ANVIL_CORE_ERROR("Failed to close file %s", file->path);
        return false;
    }

    uint32 result = fclose(file->pointer);

    if (result != 0)
    {
        ANVIL_CORE_ERROR("Failed to close file %s", file->path);
        return false;
    }

    free((void*)file->path);
    free(file);

    return true;
}

bool anvl_file_exists(const char* path)
{
    FILE* result = fopen(path, "r");
    if (!result) { return false; }

    fclose(result);

    return true;
}

uint64 anvl_file_get_size(FileHandle* file)
{
    if (!file || !file->pointer)
    {
        ANVIL_CORE_ERROR("Failed to open file %s", file->path);
        return 0;
    }

    if (fseek(file->pointer, 0, SEEK_END) != 0)
    {
        ANVIL_CORE_ERROR("Failed to seek in file %s", file->path);
        return 0;
    }

    int64 file_size = ftell(file->pointer);
    rewind(file->pointer);

    if (file_size < 0)
    {
        ANVIL_CORE_ERROR("Failed to get file %s size", file->path);
        return 0;
    }

    return (uint64)file_size;
}

const char* _filemode_to_stdio(FileMode mode)
{
    switch (mode)
    {
        case ANVL_FILE_MODE_READ  : return "r"; break;
        case ANVL_FILE_MODE_WRITE : return "w"; break;
        case ANVL_FILE_MODE_APPEND: return "a"; break;
    }

    return NULL;
}
