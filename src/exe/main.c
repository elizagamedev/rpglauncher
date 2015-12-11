#include "common.h"
#include "file.h"

#define DLL_NAME L"RPGLauncher.dll"
#define EXE_NAME L"RPG_RT.exe"
#define INI_NAME L"RPG_RT.ini"

#define LDB_NAME L"RPG_RT.ldb"

#ifdef GUESS_ENCODING
#include "codepage.h"
#define LIBGUESS_NAME L"libguess.dll"
#endif

enum {
    ERR_NONE,
    ERR_UNKNOWN,
    ERR_USAGE,
    ERR_EXE_DNE,
    ERR_ALREADY_RUNNING,
    ERR_INVALID_PATH,
    ERR_LAUNCH,
    ERR_INJECT,
};

int extract(HINSTANCE hInstance, const char *name, const Path filename)
{
    HRSRC foundRes;
    HGLOBAL	loadedRes;

    if ((foundRes = FindResourceA(hInstance, name, RT_RCDATA)) == NULL) {
	    return ERR_UNKNOWN;
	}
    if ((loadedRes = LoadResource(NULL, foundRes)) == NULL) {
	    return ERR_UNKNOWN;
	}

    VOID *data = (VOID*)LockResource(loadedRes);
    DWORD size = (size_t)SizeofResource(NULL, foundRes);
    HANDLE file = CreateFileW(filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_TEMPORARY, NULL);
    if (file == INVALID_HANDLE_VALUE) {
        return ERR_ALREADY_RUNNING;
    }
    DWORD foo;
    WriteFile(file, data, size, &foo, NULL);
    CloseHandle(file);
    return ERR_NONE;
}

BOOL getIniBool(const Path ini, const WCHAR *key, BOOL defval)
{
    WCHAR value[32];
    GetPrivateProfileStringW(L"RPGLauncher", key, L"", value, 32, ini);
    if (value[0] == 0)
        return defval;
    return !(value[0] == '0' && value[1] == 0);
}

int getIniInt(const Path ini, const WCHAR *key)
{
    WCHAR value[32];
    GetPrivateProfileStringW(L"RPGLauncher", key, L"0", value, 32, ini);
    return _wtoi(value);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow)
{
    int error = ERR_NONE;

    int argc;
    WCHAR **argv = CommandLineToArgvW(GetCommandLineW(), &argc);

    //Command line options
    BOOL optShowLogo = FALSE;
    BOOL optShowTitle = TRUE;
    BOOL optFullscreen = FALSE;
    int codepage = 0;

    //File paths
    Path gamePath;
    Path savePath;
    Path rtpPath;

    Path dllPath;
    Path exePath;
    Path iniPath;

    //Process variables
    size_t dllPathSize;
    WCHAR commandLine[MAX_PATH + 32];
    WCHAR env[(MAX_PATH + 2) * 2 + STRLEN(ENV_SAVE_PATH) + STRLEN(ENV_RTP_PATH)
              + STRLEN(ENV_CODEPAGE) + 1];
    size_t envSize;

    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    memset(&si, 0, sizeof(si));
    memset(&pi, 0, sizeof(pi));
    si.cb = sizeof(si);

    void *dllPathRemote = NULL;
    HMODULE kernel32 = NULL;
    void *pLoadLibraryW = NULL;
    HANDLE thread = NULL;

    //Parse arguments
    gamePath[0] = savePath[0] = rtpPath[0] = 0;
    for (int i = 1; i < argc; ++i) {
        if (!wcscmp(argv[i], L"--show-logo"))
            optShowLogo = TRUE;
        else if (!wcscmp(argv[i], L"--hide-title"))
            optShowTitle = FALSE;
        else if (!wcscmp(argv[i], L"--fullscreen"))
            optFullscreen = TRUE;
        else if (!wcscmp(argv[i], L"--game-path")) {
            if (++i >= argc) {
                error = ERR_USAGE;
                goto end;
            }
            if (file_getFullPath(gamePath, argv[i])) {
                error = ERR_INVALID_PATH;
                goto end;
            }
        } else if (!wcscmp(argv[i], L"--save-path")) {
            if (++i >= argc) {
                error = ERR_USAGE;
                goto end;
            }
            if (file_getFullPath(savePath, argv[i])) {
                error = ERR_INVALID_PATH;
                goto end;
            }
        } else if (!wcscmp(argv[i], L"--rtp-path")) {
            if (++i >= argc) {
                error = ERR_USAGE;
                goto end;
            }
            if (file_getFullPath(rtpPath, argv[i])) {
                error = ERR_INVALID_PATH;
                goto end;
            }
        } else if (!wcscmp(argv[i], L"--codepage")) {
            if (++i >= argc) {
                error = ERR_USAGE;
                goto end;
            }
            codepage = _wtoi(argv[i]);
        } else {
            error = ERR_USAGE;
            goto end;
        }
    }

    //Get the game path
    if (gamePath[0] == 0) {
        if (GetModuleFileNameW(NULL, gamePath, MAX_PATH) >= MAX_PATH) {
            error = ERR_INVALID_PATH;
            goto end;
        }
        PathRemoveFileSpecW(gamePath);
        PathAddBackslashW(gamePath);
    }

    //Get the exe path
    if (file_concatPath(exePath, gamePath, EXE_NAME)) {
        error = ERR_INVALID_PATH;
        goto end;
    }

    //See if the exe exists
    if (!file_exists(exePath)) {
        error = ERR_EXE_DNE;
        goto end;
    }

    //Get the ini path
    if (file_concatPath(iniPath, gamePath, INI_NAME)) {
        error = ERR_INVALID_PATH;
        goto end;
    }

    //Read some options from the ini if it exists
    if (file_exists(iniPath)) {
        if (!optShowLogo)
            optShowLogo = getIniBool(iniPath, L"logo", FALSE);
        if (optShowTitle)
            optShowTitle = getIniBool(iniPath, L"title", TRUE);
        if (!optFullscreen)
            optFullscreen = getIniBool(iniPath, L"fullscreen", FALSE);
        if (codepage == 0)
            codepage = getIniInt(iniPath, L"codepage");
    }

    //Get the RTP path
    if (rtpPath[0] == 0) {
        BOOL fail = FALSE;

        const WCHAR *keyStr;
        if (GetFileVersionInfoSizeW(exePath, NULL) != 0)
            keyStr = L"Software\\Enterbrain\\RPG2003";
        else
            keyStr = L"Software\\ASCII\\RPG2000";

        //Read from registry
        HKEY key = INVALID_HANDLE_VALUE;
        RegOpenKeyExW(HKEY_CURRENT_USER, keyStr, 0, KEY_READ, &key);
        if (key != INVALID_HANDLE_VALUE) {
            DWORD type;
            DWORD size = MAX_PATH * sizeof(WCHAR);
            if (RegQueryValueExW(key, L"RuntimePackagePath", 0, &type,
                                 (LPBYTE)rtpPath, &size) == ERROR_SUCCESS
                    && type == REG_SZ) {
                //Make sure string is null-terminated
                size /= 2;
                if (rtpPath[size - 1] != 0) {
                    if (size >= MAX_PATH) {
                        RegCloseKey(key);
                        error = ERR_INVALID_PATH;
                        goto end;
                    }
                    rtpPath[size] = 0;
                    ++size;
                }
                //Fix path
                if (file_fixPath(rtpPath, size)) {
                    RegCloseKey(key);
                    error = ERR_INVALID_PATH;
                    goto end;
                }
            } else {
                fail = TRUE;
            }
            RegCloseKey(key);
        } else {
            fail = TRUE;
        }

        if (fail) {
            //See if the program says it needs the RTP;
            //if it does, display a warning
            WCHAR fullPackageFlag[2];
            if (GetPrivateProfileStringW(L"RPG_RT", L"FullPackageFlag", L"0",
                                         fullPackageFlag, 2, L".\\RPG_RT.ini")
                    != 1 || fullPackageFlag[0] == '0') {
                MessageBoxW(NULL, L"This game is marked as requiring the "
                                  L"RPG Maker RTP to run properly. While the "
                                  L"game might run properly without installing "
                                  L"the RTP, it is recommended you install it "
                                  L"before playing.",
                            L"RPGLauncher", MB_ICONWARNING);
            }
            rtpPath[0] = 0;
        }
    }

    //Get the dll path
    dllPathSize = GetTempPathW(MAX_PATH, dllPath);
    if (dllPathSize >= MAX_PATH - STRLEN(DLL_NAME)) {
        error = ERR_INVALID_PATH;
        goto end;
    }
    wcscat(dllPath, DLL_NAME);
    dllPathSize += STRLEN(DLL_NAME);
    dllPathSize = (dllPathSize + 1) * sizeof(WCHAR);

    //Extract DLL
    if ((error = extract(hInstance, "DLL", dllPath)) != ERR_NONE)
        goto end;
	MoveFileExW(dllPath, NULL, MOVEFILE_DELAY_UNTIL_REBOOT);

#ifdef GUESS_ENCODING
	//Get the codepage if necessary
	if (codepage == 0) {
	    //Get the ldb path
	    Path ldbPath;
	    if (pathcat(ldbPath, gamePath, LDB_NAME)) {
	        error = ERR_INVALID_PATH;
            goto end;
        }

	    //Get the libguess path
	    Path libguessPath;
	    if (GetTempPathW(MAX_PATH, libguessPath) >= MAX_PATH - STRLEN(LIBGUESS_NAME)) {
	        error = ERR_INVALID_PATH;
	        goto end;
	    }
	    wcscat(libguessPath, LIBGUESS_NAME);

	    //Extract DLL
	    if ((error = extract(hInstance, "LIBGUESS", libguessPath)) != ERR_NONE)
	        goto end;

        //Get the codepage, free the library and delete it
	    codepage_init(libguessPath);
	    codepage = codepage_guessLdbEncoding(ldbPath);
	    codepage_free();
	    DeleteFileW(libguessPath);
	}
#endif

    //Create command line
    wcscpy(commandLine, L"\"");
    wcscat(commandLine, exePath);
    wcscat(commandLine, L"\"");
    if (optShowLogo)
        wcscat(commandLine, L" x");
    else
        wcscat(commandLine, L" TestPlay");
    if (optShowTitle)
        wcscat(commandLine, L" x");
    else
        wcscat(commandLine, L" HideTitle");
    if (optFullscreen)
        wcscat(commandLine, L" x");
    else
        wcscat(commandLine, L" Window");

    //Create environment variable block
    wsprintfW(env, L"%s=%s", ENV_SAVE_PATH, savePath);
    envSize = wcslen(env) + 1;
    wsprintfW(env + envSize, L"%s=%s", ENV_RTP_PATH, rtpPath);
    envSize += wcslen(env + envSize) + 1;
    wsprintfW(env + envSize, L"%s=%d", ENV_CODEPAGE, codepage);
    envSize += wcslen(env + envSize) + 1;
    env[envSize] = 0;

    //Spawn the RPG_RT.exe process and inject
    if (!CreateProcessW(exePath, commandLine, 0, 0, FALSE,
                        CREATE_SUSPENDED | CREATE_UNICODE_ENVIRONMENT,
                        env, gamePath, &si, &pi)) {
        error = ERR_LAUNCH;
        goto end;
    }

    if (!(dllPathRemote = VirtualAllocEx(pi.hProcess, NULL, dllPathSize,
                                         MEM_COMMIT, PAGE_READWRITE))) {
        error = ERR_INJECT;
        goto end;
    }

    if (!WriteProcessMemory(pi.hProcess, dllPathRemote, dllPath, dllPathSize,
                            NULL)) {
        error = ERR_INJECT;
        goto end;
    }

    if (!(kernel32 = LoadLibraryW(L"kernel32.dll"))) {
        error = ERR_INJECT;
        goto end;
    }

    if (!(pLoadLibraryW = GetProcAddress(kernel32, "LoadLibraryW"))) {
        error = ERR_INJECT;
        goto end;
    }

    if (!(thread = CreateRemoteThread(pi.hProcess, NULL, 0, pLoadLibraryW,
                                      dllPathRemote, 0, NULL))) {
        error = ERR_INJECT;
        goto end;
    }

    WaitForSingleObject(thread, INFINITE);
    DWORD codeRemote;
    if (GetExitCodeThread(thread, &codeRemote)) {
        if (codeRemote == 0) {
            error = ERR_INJECT;
            goto end;
        }
    } else {
        error = ERR_INJECT;
        goto end;
    }

end:
    //Cleanup
    LocalFree(argv);
    if (thread)
        CloseHandle(thread);
    if (kernel32)
        FreeLibrary(kernel32);
    if (dllPathRemote)
        VirtualFreeEx(pi.hProcess, dllPathRemote, 0, MEM_RELEASE);

    if (error == ERR_NONE) {
        ResumeThread(pi.hThread);
    } else {
        if(pi.hProcess)
            TerminateProcess(pi.hProcess, 1);
    }

    const WCHAR *msg = NULL;
    switch (error) {
    case ERR_UNKNOWN:
        msg = L"Unknown error.";
        break;
    case ERR_USAGE:
        msg = L"Incorrect command-line options specified.";
        break;
    case ERR_EXE_DNE:
        msg = L"RPG_RT.exe does not exist in the specified game path.";
        break;
    case ERR_ALREADY_RUNNING:
        msg = L"RPGLauncher is already running with another game.";
        break;
    case ERR_INVALID_PATH:
        msg = L"A specified path is too long or invalid.";
        break;
    case ERR_LAUNCH:
        msg = L"Could not launch RPG_RT.exe.";
        break;
    case ERR_INJECT:
        msg = L"Could not inject DLL into RPG_RT.exe.";
        break;
    }
    if (msg != NULL)
        MessageBoxW(0, msg, L"RPGLauncher 2.0", MB_ICONERROR);
    return error;
}
