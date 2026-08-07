#include "anvlpch.h"

#include "File/file.h"
#include <stdio.h>

typedef struct AnvlFile
{
    FILE*       pointer;
    const char* path;
    const char* mode;
} AnvlFile;

const char* _filemode_to_string(AnvlFileMode mode);

AnvlFile* anvl_file_open(const char* path, AnvlFileMode mode)
{
    AnvlFile* file = malloc(sizeof(AnvlFile));
    if (!file) { return NULL; }

    ANVIL_ASSERT(path != NULL);
    file->path    = strdup(path);
    file->mode    = _filemode_to_string(mode);
    file->pointer = fopen(path, file->mode);
    if (!file->pointer)
    {
        free(file);
        ANVIL_CORE_ERROR("Failed to open file: %s (mode: %s)", path, file->mode);
        return NULL;
    }

    return file;
}

uint64 anvl_file_read(AnvlFile* file, void* buffer, uint64 size)
{
    ANVIL_ASSERT(file != NULL && file->pointer != NULL);

    size_t result = fread(buffer, 1, size, file->pointer);

    if (result == 0 && ferror(file->pointer)) { return 0; }

    return (uint64)result;
}

uint64 anvl_file_write(AnvlFile* file, const void* buffer, uint64 size)
{
    ANVIL_ASSERT(file != NULL && file->pointer != NULL);

    size_t result = fwrite(buffer, 1, size, file->pointer);

    if (result == 0 && ferror(file->pointer)) { return 0; }

    return (uint64)result;
}

bool anvl_file_close(AnvlFile* file)
{
    if (!file || !file->pointer) { return false; }

    uint32 result = fclose(file->pointer);

    if (result != 0) { return false; }

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

uint64 anvl_file_get_size(AnvlFile* file)
{
    ANVIL_ASSERT(file != NULL && file->pointer != NULL);

    if (fseek(file->pointer, 0, SEEK_END) != 0) { return 0; }

    int64 file_size = ftell(file->pointer);
    rewind(file->pointer);

    if (file_size < 0) { return 0; }

    return (uint64)file_size;
}

const char* _filemode_to_string(AnvlFileMode mode)
{
    switch (mode)
    {
        case ANVL_FILE_MODE_READ  : return "r"; break;
        case ANVL_FILE_MODE_WRITE : return "w"; break;
        case ANVL_FILE_MODE_APPEND: return "a"; break;
    }

    ANVIL_ASSERT_MSG(0, "Invalid AnvlFileMode: %d", mode);
}
