#include "util.h"
#include "file.h"

WCHAR *util_toWideAllocCP(const CHAR *str, int cp)
{
    if (str == NULL)
        return NULL;

    size_t size = MultiByteToWideChar(cp, 0, str, -1, NULL, 0);
    if (size > 0) {
        WCHAR *wstr = (WCHAR*)malloc(size * sizeof(WCHAR));
        MultiByteToWideChar(cp, 0, str, -1, wstr, size);
        return wstr;
    }
    return NULL;
}

static BOOL _getFile(Path dst, const WCHAR *root, const CHAR *file)
{
    Path wfile;
    if (util_toWide(wfile, file, MAX_PATH))
        return TRUE;
    if (file_concatPath(dst, root, wfile))
        return TRUE;
    if (file_exists(dst))
        return FALSE;
    if (util_toWideACP(wfile, file, MAX_PATH))
        return TRUE;
    if (file_concatPath(dst, root, wfile))
        return TRUE;
    if (file_exists(dst))
        return FALSE;
    return TRUE;
}

BOOL util_getPatchedPath(Path dst, const CHAR *src)
{
    //Save file
    const CHAR *ext = PathFindExtensionA(src) + 1;
    if (!stricmp(ext, "lsd")) {
        Path wsrc;
        if (util_toWide(wsrc, PathFindFileNameA(src), MAX_PATH))
            return TRUE;
        return file_concatPath(dst, config.savePath, wsrc);
    }

    //RTP file
    if (src[0] == FAKE_RTP_PATH[0])
        return _getFile(dst, config.rtpPath, src + STRLEN(FAKE_RTP_PATH));

    //Game file
    return _getFile(dst, L"", src);
}
