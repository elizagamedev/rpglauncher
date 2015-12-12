#include "hook.h"
#include "config.h"
#include "util.h"

#define HKEY_FAKE   ((HKEY)0x80000010)
#define HFILE_FAKE  ((HANDLE)1)

// The last directshow-rendered thing loaded
static Path lastDirectShowFile;

// Title of the game window
static WCHAR wtitle[256];
static size_t wtitleSize = 0;

/*******************
 * Hooked Functions
 */

// KERNEL

// These functions are vital for telling RPGM to use a MBCS
BOOL WINAPI
hook_GetCPInfo(UINT CodePage, LPCPINFO lpCPInfo)
{
    return GetCPInfo(config.codepage, lpCPInfo);
}

LCID WINAPI
hook_GetThreadLocale()
{
    return MAKELCID(MAKELANGID(LANG_JAPANESE, SUBLANG_JAPANESE_JAPAN),
                    SORT_DEFAULT);
}

DWORD WINAPI
hook_GetModuleFileNameA(HMODULE hModule, LPSTR lpFileName, DWORD nSize)
{
    lpFileName[0] = 0;
    return 0;
}

// Sound gets loaded as absolute path, for some reason, so trick it into using
// a relative path
DWORD WINAPI
hook_GetFullPathNameA(LPCSTR lpFileName, DWORD nBufferLength, LPSTR lpBuffer,
                      LPSTR *lpFilePart)
{
    strcpy(lpBuffer, lpFileName);
    if (lpFilePart)
        *lpFilePart = lpBuffer;
    return strlen(lpBuffer);
}

HANDLE WINAPI
hook_FindFirstFileA(LPCSTR lpFileName, LPWIN32_FIND_DATAA lpFindFileData)
{
    Path lpFileNameW;
    if (util_getPatchedPath(lpFileNameW, lpFileName)) {
        memset(lpFindFileData, 0, sizeof(WIN32_FIND_DATAA));
        return INVALID_HANDLE_VALUE;
    }

    WIN32_FIND_DATAW findFileDataW;
    HANDLE result = FindFirstFileW(lpFileNameW, &findFileDataW);
    memcpy(lpFindFileData, &findFileDataW,
           sizeof(WIN32_FIND_DATAA)
           - sizeof(lpFindFileData->cFileName)
           - sizeof(lpFindFileData->cAlternateFileName));

    util_fromWide(lpFindFileData->cFileName, findFileDataW.cFileName, MAX_PATH);
    util_fromWide(lpFindFileData->cAlternateFileName,
                  findFileDataW.cAlternateFileName, 14);
    return result;
}

HANDLE WINAPI
hook_CreateFileA(LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode,
                 LPSECURITY_ATTRIBUTES lpSecurityAttributes,
                 DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes,
                 HANDLE hTemplateFile)
{
    Path lpFileNameW;
    if (util_getPatchedPath(lpFileNameW, lpFileName))
        return INVALID_HANDLE_VALUE;

    // Get extension for DirectShow files
    LPWSTR ext = PathFindExtensionW(lpFileNameW) + 1;

    HANDLE result = CreateFileW(lpFileNameW, dwDesiredAccess, dwShareMode,
                                lpSecurityAttributes, dwCreationDisposition,
                                dwFlagsAndAttributes, hTemplateFile);

    if (!wcsicmp(ext, L"mp3") || !wcsicmp(ext, L"avi") || !wcsicmp(ext, L"mpg"))
        wcscpy(lastDirectShowFile, lpFileNameW);
    return result;
}

DWORD WINAPI
hook_GetFileAttributesA(LPCSTR lpFileName)
{
    Path lpFileNameW;
    if (util_getPatchedPath(lpFileNameW, lpFileName))
        return INVALID_FILE_ATTRIBUTES;
    return GetFileAttributesW(lpFileNameW);
}

DWORD WINAPI
hook_GetPrivateProfileStringA(LPCSTR lpAppName, LPCSTR lpKeyName,
                              LPCSTR lpDefault, LPSTR lpReturnedString,
                              DWORD nSize, LPCSTR lpFileName)
{
    // Force FullPackageFlag to be 0 (we trick the game into reading from the
    // "registry" that it exists anyway)
    if (!stricmp(lpKeyName, "FullPackageFlag")) {
        lpReturnedString[0] = '0';
        lpReturnedString[1] = 0;
        return 1;
    }

    // Don't do anything with the game title, we do that ourselves
    // Or anything else for that matter
    lpReturnedString[0] = 0;
    return 0;
}

int WINAPI
hook_MultiByteToWideChar(UINT CodePage, DWORD dwFlags, LPCSTR lpMultiByteStr,
                         int cbMultiByte, LPWSTR lpWideCharStr, int cchWideChar)
{
    if (CodePage == CP_ACP)
        CodePage = config.codepage;
    return MultiByteToWideChar(CodePage, dwFlags, lpMultiByteStr, cbMultiByte,
                               lpWideCharStr, cchWideChar);
}

int WINAPI
hook_WideCharToMultiByte(UINT CodePage, DWORD dwFlags, LPCWSTR lpWideCharStr,
                         int cchWideChar, LPSTR lpMultiByteStr, int cbMultiByte,
                         LPCSTR lpDefaultChar, LPBOOL lpUsedDefaultChar)
{
    if (CodePage == CP_ACP)
        CodePage = config.codepage;
    return WideCharToMultiByte(CodePage, dwFlags, lpWideCharStr, cchWideChar,
                               lpMultiByteStr, cbMultiByte, lpDefaultChar,
                               lpUsedDefaultChar);
}

// USER
// For MBCS
int WINAPI
hook_GetSystemMetrics(int nIndex)
{
    if (nIndex == SM_DBCSENABLED)
        return TRUE;
    return GetSystemMetrics(nIndex);
}

HWND WINAPI
hook_CreateWindowExA(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName,
                     DWORD dwStyle, int x, int y, int nWidth, int nHeight,
                     HWND hWndParent, HMENU hMenu, HINSTANCE hInstance,
                     LPVOID lpParam)
{
    if (!strcmp(lpClassName, "TFormLcfGameMain")) {
        // Get the title of the game
        CHAR title[256];
        GetPrivateProfileStringA("RPG_RT", "GameTitle", "Untitled", title,
                                 256, ".\\RPG_RT.ini");
        util_toWide(wtitle, title, 256);
        wtitleSize = wcslen(wtitle);

        HWND result = CreateWindowExW(dwExStyle, L"TFormLcfGameMain", wtitle,
                                      dwStyle, x, y, nWidth, nHeight,
                                      hWndParent, hMenu, hInstance, lpParam);
        return result;
    }
    WCHAR *lpClassNameW = util_toWideAlloc(lpClassName);
    HWND result = CreateWindowExW(dwExStyle, lpClassNameW, L"", dwStyle, x, y,
                                  nWidth, nHeight, hWndParent, hMenu,
                                  hInstance, lpParam);
    free(lpClassNameW);
    return result;
}

ATOM WINAPI
hook_RegisterClassA(const WNDCLASSA *lpWndClass)
{
    WNDCLASSW wndClassW;
    memcpy(&wndClassW, lpWndClass, sizeof(WNDCLASSA) - sizeof(LPCSTR) * 2);
    wndClassW.lpszMenuName = util_toWideAlloc(lpWndClass->lpszMenuName);
    wndClassW.lpszClassName = util_toWideAlloc(lpWndClass->lpszClassName);
    ATOM result = RegisterClassW(&wndClassW);
    free((void *)wndClassW.lpszMenuName);
    free((void *)wndClassW.lpszClassName);
    return result;
}

BOOL WINAPI
hook_UnregisterClassA(LPCSTR lpClassName, HINSTANCE hInstance)
{
    LPWSTR lpClassNameW = util_toWideAlloc(lpClassName);
    BOOL result = UnregisterClassW(lpClassNameW, hInstance);
    free(lpClassNameW);
    return result;
}

int WINAPI
hook_MessageBoxA(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType)
{
    LPWSTR lpTextW = util_toWideAlloc(lpText);
    LPWSTR lpCaptionW = util_toWideAlloc(lpCaption);
    int result = MessageBoxW(hWnd, lpTextW, lpCaptionW, uType);
    free(lpTextW);
    free(lpCaptionW);
    return result;
}

LONG WINAPI
hook_SetWindowLongA(HWND hWnd, int nIndex, LONG dwNewLong)
{
    return SetWindowLongW(hWnd, nIndex, dwNewLong);
}

LONG WINAPI
hook_GetWindowLongA(HWND hWnd, int nIndex)
{
    return GetWindowLongW(hWnd, nIndex);
}

LRESULT WINAPI
hook_DefWindowProcA(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    switch (Msg) {
    case WM_SETTEXT:
        return TRUE;
    case WM_GETTEXT:
        memcpy((void *)lParam, wtitle, (wtitleSize + 1) * 2);
        return wtitleSize;
    case WM_GETTEXTLENGTH:
        return (wtitleSize + 1) * 2;
    }
    return DefWindowProcW(hWnd, Msg, wParam, lParam);
}

int WINAPI
hook_LoadStringA(HINSTANCE hInstance, UINT uID, LPSTR lpBuffer, int nBufferMax)
{
    LPWSTR lpBufferW = (LPWSTR)malloc(nBufferMax * 2);
    int result = LoadStringW(hInstance, uID, lpBufferW, nBufferMax);
    util_fromWide(lpBuffer, lpBufferW, nBufferMax);
    free(lpBufferW);
    return result;
}

// ADVAPI
LONG WINAPI
hook_RegCreateKeyExA(HKEY hKey, LPCSTR lpSubKey, DWORD Reserved, LPSTR lpClass,
                     DWORD dwOptions, REGSAM samDesired,
                     LPSECURITY_ATTRIBUTES lpSecurityAttributes,
                     PHKEY phkResult, LPDWORD lpdwDisposition)
{
    *phkResult = HKEY_FAKE;
    return ERROR_SUCCESS;
}

LONG WINAPI
hook_RegOpenKeyExA(HKEY hKey, LPCSTR lpSubKey, DWORD ulOptions,
                   REGSAM samDesired, PHKEY phkResult)
{
    if (*lpSubKey)
        return RegOpenKeyExA(hKey, lpSubKey, ulOptions, samDesired, phkResult);

    *phkResult = HKEY_FAKE;
    return ERROR_SUCCESS;
}

LONG WINAPI
hook_RegQueryValueExA(HKEY hKey, LPCSTR lpValueName, LPDWORD lpReserved,
                      LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData)
{
    // Only override RuntimePackagePath
    if (stricmp(lpValueName, "RuntimePackagePath")) {
        if (hKey == HKEY_FAKE)
            return ERROR_FILE_NOT_FOUND;
        return RegQueryValueExA(hKey, lpValueName, lpReserved, lpType, lpData,
                                lpcbData);
    }

    if (lpType)
        *lpType = REG_SZ;
    if (lpcbData) {
        if (lpData) {
            if (*lpcbData < ARRAY_SIZE(FAKE_RTP_PATH)) {
                *lpcbData = ARRAY_SIZE(FAKE_RTP_PATH);
                return ERROR_MORE_DATA;
            } else {
                memcpy(lpData, FAKE_RTP_PATH, ARRAY_SIZE(FAKE_RTP_PATH));
                return ERROR_SUCCESS;
            }
        } else {
            *lpcbData = ARRAY_SIZE(FAKE_RTP_PATH);
            return ERROR_SUCCESS;
        }
    }
    return ERROR_SUCCESS;
}

LONG WINAPI
hook_RegCloseKey(HKEY hKey)
{
    if (hKey == HKEY_FAKE)
        return ERROR_SUCCESS;
    return RegCloseKey(hKey);
}


// GDI
BOOL WINAPI
hook_GetTextExtentPoint32A(HDC hdc, LPCSTR lpString, int c, LPSIZE lpSize)
{
    LPWSTR lpStringW = util_toWideAlloc(lpString);
    BOOL result = GetTextExtentPoint32W(hdc, lpStringW, c, lpSize);
    free(lpStringW);
    return result;
}

BOOL WINAPI
hook_ExtTextOutA(HDC hdc, int X, int Y, UINT fuOptions, const RECT *lprc,
                 LPCSTR lpString, UINT cbCount, const INT *lpDx)
{
    LPWSTR lpStringW = util_toWideAlloc(lpString);
    BOOL result = ExtTextOutW(hdc, X, Y, fuOptions, lprc, lpStringW, cbCount,
                              lpDx);
    free(lpStringW);
    return result;
}

HFONT WINAPI
hook_CreateFontIndirectA(const LOGFONTA *lplf)
{
    LOGFONTW lfW;
    memcpy(&lfW, lplf, sizeof(LOGFONTA) - sizeof(lplf->lfFaceName));
    util_toWide(lfW.lfFaceName, lplf->lfFaceName, LF_FACESIZE);
    if (!wcscmp(lfW.lfFaceName, L"MS Pゴッシク"))
        wcscpy(lfW.lfFaceName, L"MS PGothic");
    else if (!wcscmp(lfW.lfFaceName, L"ＭＳ ゴシック"))
        wcscpy(lfW.lfFaceName, L"MS Gothic");
    else if (!wcscmp(lfW.lfFaceName, L"MS P明朝"))
        wcscpy(lfW.lfFaceName, L"MS PMincho");
    else if (!wcscmp(lfW.lfFaceName, L"ＭＳ 明朝"))
        wcscpy(lfW.lfFaceName, L"MS Mincho");
    return CreateFontIndirectW(&lfW);
}

// SHELL
// LSD files use this for some reason
BOOL
hook_SHGetPathFromIDListA(PCIDLIST_ABSOLUTE pidl, LPTSTR pszPath)
{
    *pszPath = 0;
    return TRUE;
}

// COM OBJECTS

void hook_QueryInterface(REFIID riid, void **ppvObject);

#define GET_VTBL(type) type* vtbl = ((void**)This->lpVtbl)[sizeof(type)/4]

ULONG WINAPI
hook_IGraphBuilder_Release(IGraphBuilder *This)
{
    GET_VTBL(IGraphBuilderVtbl);

    free((void *)This->lpVtbl);
    This->lpVtbl = vtbl;
    return vtbl->Release(This);
}

STDMETHODIMP
hook_IGraphBuilder_RenderFile(IGraphBuilder *This, LPCWSTR lpwstrFile,
                              LPCWSTR lpwstrPlayList)
{
    GET_VTBL(IGraphBuilderVtbl);

    HRESULT result = vtbl->RenderFile(This, lastDirectShowFile, lpwstrPlayList);

    return result;
}

// IUnknown
STDMETHODIMP
hook_IUnknown_QueryInterface(IUnknown *This, REFIID riid, void **ppvObject)
{
    GET_VTBL(IUnknownVtbl);

    HRESULT result = vtbl->QueryInterface(This, riid, ppvObject);

    hook_QueryInterface(riid, ppvObject);

    return result;
}

// Generic
void
hook_QueryInterface(REFIID riid, void **ppvObject)
{
    if (IsEqualIID(riid, (REFGUID)&IID_IGraphBuilder)) {
        IGraphBuilder *obj = (IGraphBuilder *)*ppvObject;
        IGraphBuilderVtbl *vtbl = malloc(sizeof(IGraphBuilderVtbl) + 4);
        ((void **)vtbl)[sizeof(IGraphBuilderVtbl)/4] = obj->lpVtbl;
        memcpy(vtbl, obj->lpVtbl, sizeof(IGraphBuilderVtbl));
        obj->lpVtbl = vtbl;

        vtbl->Release = hook_IGraphBuilder_Release;
        vtbl->RenderFile = hook_IGraphBuilder_RenderFile;
    }
}

ULONG WINAPI
hook_IUnknown_Release(IUnknown *This)
{
    GET_VTBL(IUnknownVtbl);

    free((void *)This->lpVtbl);
    This->lpVtbl = vtbl;
    return vtbl->Release(This);
}

// OLE
HRESULT WINAPI
hook_CoCreateInstance(REFCLSID rclsid, LPUNKNOWN pUnkOuter, DWORD dwClsContext,
                      REFIID riid, LPVOID *ppv)
{
    HRESULT result = CoCreateInstance(rclsid, pUnkOuter, dwClsContext,
                                      riid, ppv);

    // Do we need to hook this?
    if (IsEqualCLSID(rclsid, (REFGUID)&CLSID_FilterGraph)) {
        IUnknown *obj = (IUnknown *)*ppv;
        IUnknownVtbl *vtbl = malloc(sizeof(IUnknownVtbl) + 4);

        ((void **)vtbl)[sizeof(IUnknownVtbl)/4] = obj->lpVtbl;
        memcpy(vtbl, obj->lpVtbl, sizeof(IUnknownVtbl));
        obj->lpVtbl = vtbl;

        vtbl->QueryInterface = hook_IUnknown_QueryInterface;
        vtbl->Release = hook_IUnknown_Release;
    }

    return result;
}

/**********
 * Hooking
 */
typedef struct
{
    const char *name;
    void *funcHook;
    void *funcOrig;
} Hook;

#define MODULE(name)    {#name "32.dll", NULL, NULL},
#define HOOK(name)      {#name, hook_##name, NULL},

#define HOOK_MAX (sizeof(hooks) / sizeof(hooks[0]))
Hook hooks[] = {
    MODULE(kernel)
    HOOK(GetCPInfo)
    HOOK(GetThreadLocale)
    HOOK(GetModuleFileNameA)
    HOOK(GetFullPathNameA)
    HOOK(FindFirstFileA)
    HOOK(CreateFileA)
    HOOK(GetFileAttributesA)
    HOOK(GetPrivateProfileStringA)
    HOOK(MultiByteToWideChar)
    HOOK(WideCharToMultiByte)

    MODULE(user)
    HOOK(CreateWindowExA)
    HOOK(RegisterClassA)
    HOOK(UnregisterClassA)
    HOOK(GetSystemMetrics)
    HOOK(MessageBoxA)
    HOOK(SetWindowLongA)
    HOOK(GetWindowLongA)
    HOOK(DefWindowProcA)
    HOOK(LoadStringA)

    MODULE(advapi)
    HOOK(RegCreateKeyExA)
    HOOK(RegOpenKeyExA)
    HOOK(RegQueryValueExA)
    HOOK(RegCloseKey)

    MODULE(gdi)
    HOOK(GetTextExtentPoint32A)
    HOOK(ExtTextOutA)
    HOOK(CreateFontIndirectA)

    MODULE(shell)
    HOOK(SHGetPathFromIDListA)

    MODULE(ole)
    HOOK(CoCreateInstance)
};

#define MAKE_POINTER(cast, ptr, offset) (cast)((DWORD)(ptr)+(DWORD)(offset))

BOOL hook_init()
{
    // Initialize original function addresses
    HMODULE module = NULL;
    for (int i = 0; i < HOOK_MAX; i++) {
        if (hooks[i].funcHook)
            hooks[i].funcOrig = (void *)GetProcAddress(module, hooks[i].name);
        else
            module = GetModuleHandleA(hooks[i].name);
    }

    // Check DOS Header
    PIMAGE_DOS_HEADER dos = (PIMAGE_DOS_HEADER)GetModuleHandleW(NULL);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE)
        return FALSE;

    // Check NT Header
    PIMAGE_NT_HEADERS nt = MAKE_POINTER(PIMAGE_NT_HEADERS, dos, dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE)
        return FALSE;

    // Check import module table
    PIMAGE_IMPORT_DESCRIPTOR modules
        = MAKE_POINTER(PIMAGE_IMPORT_DESCRIPTOR, dos,
                       nt->OptionalHeader
                       .DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT]
                       .VirtualAddress);
    if (modules == (PIMAGE_IMPORT_DESCRIPTOR)nt)
        return FALSE;

    // Find the correct module
    while (modules->Name) {
        const char *moduleStr = MAKE_POINTER(const char *, dos, modules->Name);

        int i = 0;
        for (; i < HOOK_MAX; ++i) {
            if (!hooks[i].funcHook && !stricmp(hooks[i].name, moduleStr))
                break;
        }

        if (i < HOOK_MAX) {
            // Find the correct function
            PIMAGE_THUNK_DATA thunk = MAKE_POINTER(PIMAGE_THUNK_DATA, dos,
                                                   modules->FirstThunk);
            while (thunk->u1.Function) {
                for (int j = i + 1; j < HOOK_MAX && hooks[j].funcHook; j++) {
                    if (thunk->u1.Function == (DWORD)hooks[j].funcOrig) {
                        // Overwrite
                        DWORD flags;
                        if (!VirtualProtect(&thunk->u1.Function,
                                            sizeof(thunk->u1.Function),
                                            PAGE_READWRITE, &flags))
                            return FALSE;
                        thunk->u1.Function = (DWORD)hooks[j].funcHook;
                        if (!VirtualProtect(&thunk->u1.Function,
                                            sizeof(thunk->u1.Function),
                                            flags, &flags))
                            return FALSE;
                    }
                }
                ++thunk;
            }
        }
        ++modules;
    }
    return TRUE;
}
