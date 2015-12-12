#include "config.h"

Config config = {0};

BOOL config_init()
{
    // Get the save and RTP paths
    GetEnvironmentVariableW(ENV_SAVE_PATH, config.savePath, MAX_PATH);
    GetEnvironmentVariableW(ENV_RTP_PATH, config.rtpPath, MAX_PATH);

    // Codepage
    WCHAR szCodepage[32];
    if (GetEnvironmentVariableW(ENV_CODEPAGE,
                                szCodepage,
                                ARRAY_SIZE(szCodepage))
            >= ARRAY_SIZE(szCodepage))
        return FALSE;
    config.codepage = _wtoi(szCodepage);
    if (config.codepage == 0)
        config.codepage = 932; // Shift-JIS

    return TRUE;
}
