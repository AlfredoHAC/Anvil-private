#ifndef ANVL_FILEIO_HEADER
#define ANVL_FILEIO_HEADER

#include "Core/typedefs.h"

typedef struct FileHandle FileHandle;

typedef enum FileMode
{
    ANVL_FILE_MODE_READ = 0,
    ANVL_FILE_MODE_WRITE,
    ANVL_FILE_MODE_APPEND,
} FileMode;

FileHandle* anvl_file_open(const char* path, FileMode mode);
uint64      anvl_file_read(FileHandle* file, void* buffer, uint64 size);
uint64      anvl_file_write(FileHandle* file, const void* buffer, uint64 size);
bool        anvl_file_close(FileHandle* file);
bool        anvl_file_exists(const char* path);
uint64      anvl_file_get_size(FileHandle* file);

#endif // ANVL_FILEIO_HEADER
