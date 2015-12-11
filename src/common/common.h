#ifndef COMMON_H
#define COMMON_H

//Includes
#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0500
#define SECURITY_WIN32
#include <windows.h>
#include <shlwapi.h>
#include <shellapi.h>
#include <security.h>
#include <shlobj.h>
#include <malloc.h>
#include <stdio.h>
#include <time.h>
#define NO_DSHOW_STRSAFE
#include <dshow.h>

//Constants
#define ENV_SAVE_PATH L"RPGL_SAVE"
#define ENV_RTP_PATH L"RPGL_RTP"
#define ENV_CODEPAGE L"RPGL_CP"

//Types
typedef WCHAR Path[MAX_PATH];

//Macros
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#define STRLEN(str) (sizeof(str) / sizeof((str)[0]) - 1)

#define MESSAGEA(x) MessageBoxA(NULL, x, "", 0)
#define MESSAGEW(x) MessageBoxW(NULL, x, L"", 0)

#endif
