#ifndef ANVIL_FILE_HEADER
#define ANVIL_FILE_HEADER

#include "Anvil/Core/types.h"

typedef struct AnvlFile AnvlFile;

typedef enum AnvlFileMode
{
    ANVL_FILE_MODE_READ = 0,
    ANVL_FILE_MODE_WRITE,
    ANVL_FILE_MODE_APPEND,
} AnvlFileMode;

AnvlFile* anvl_file_open(const char* path, AnvlFileMode mode);
uint64      anvl_file_read(AnvlFile* file, void* buffer, uint64 size);
uint64      anvl_file_write(AnvlFile* file, const void* buffer, uint64 size);
bool        anvl_file_close(AnvlFile* file);
bool        anvl_file_exists(const char* path);
uint64      anvl_file_get_size(AnvlFile* file);

#endif // !ANVIL_FILE_HEADER
