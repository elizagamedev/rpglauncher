#include "file.h"

BOOL file_concatPath(Path dst, const WCHAR *a, const WCHAR *b)
{
    size_t aSize = wcslen(a);
    size_t bSize = wcslen(b);
    if (aSize + bSize >= MAX_PATH)
        return TRUE;
    wcscpy(dst, a);
    wcscat(dst, b);
    return FALSE;
}

BOOL file_fixPath(Path path, size_t size)
{
    if (size >= MAX_PATH)
        return TRUE;

    //Append backslash if necessary
    if (path[size - 1] != '\\') {
        if (size == MAX_PATH - 1)
            return TRUE;
        path[size] = '\\';
    }
    return FALSE;
}

BOOL file_getFullPath(Path path, const WCHAR *src)
{
    if (src == NULL || src[0] == 0) {
        return TRUE;
    }

    //Get full path name
    size_t size = GetFullPathNameW(src, MAX_PATH, path, NULL);
    return file_fixPath(path, size);
}
