#ifndef UTIL_H
#define UTIL_H

#include "common.h"
#include "config.h"

#define FAKE_RTP_PATH "?:\\"
#define FAKE_MODULE_PATH "RPG_RT.exe"

//Unicode/CP functions
static inline BOOL util_toWide(WCHAR *dst, const CHAR *src, size_t size)
{
    return MultiByteToWideChar(config.codepage, 0, src, -1, dst, size) == 0;
}

static inline BOOL util_toWideACP(WCHAR *dst, const CHAR *src, size_t size)
{
    return MultiByteToWideChar(CP_ACP, 0, src, -1, dst, size) == 0;
}

static inline BOOL util_fromWide(CHAR *dst, const WCHAR *src, size_t size)
{
    return WideCharToMultiByte(config.codepage, 0, src, -1, dst, size, NULL, FALSE) == 0;
}

static inline BOOL util_fromWideACP(CHAR *dst, const WCHAR *src, size_t size)
{
    return WideCharToMultiByte(CP_ACP, 0, src, -1, dst, size, NULL, FALSE) == 0;
}

#define util_toWideAlloc(str) util_toWideAllocCP(str, config.codepage)
#define util_toWideAllocACP(str) util_toWideAllocCP(str, CP_ACP)
WCHAR *util_toWideAllocCP(const CHAR *str, int cp);

//Misc
BOOL util_getPatchedPath(Path dst, const CHAR *src);

#endif
