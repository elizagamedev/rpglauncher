#ifndef PATH_H
#define PATH_H

#include "common.h"

BOOL file_concatPath(Path dst, const WCHAR *a, const WCHAR *b);
BOOL file_fixPath(Path path, size_t size);
BOOL file_getFullPath(Path path, const WCHAR *src);

static inline BOOL file_exists(const Path filename)
{
    DWORD dwAttrib = GetFileAttributesW(filename);
    return dwAttrib != INVALID_FILE_ATTRIBUTES && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY);
}

#endif
