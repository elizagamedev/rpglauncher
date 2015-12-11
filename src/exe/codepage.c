#ifdef GUESS_ENCODING

#include "common.h"

#include <libguess.h>

HMODULE libguess = NULL;
char *(*_libguess_determine_encoding)(const char *inbuf, int length, const char *region) = NULL;

static int getEncoding(const char *str)
{
    const static char *regions[] = {
        GUESS_REGION_JP,
        GUESS_REGION_CN,
        GUESS_REGION_TW,
        GUESS_REGION_KR,
        GUESS_REGION_RU,
        GUESS_REGION_AR,
        GUESS_REGION_TR,
        GUESS_REGION_GR,
        GUESS_REGION_HW,
        GUESS_REGION_PL,
        GUESS_REGION_BL,
	};

	//Use libguess to guess encoding
	int length = strlen(str);
	const char *encStr = 0;
	for (size_t i = 0; i < ARRAY_SIZE(langs); ++i) {
	    if ((encStr = _libguess_determine_encoding(str, length, regions[i])) != NULL)
	        break;
	}
	if (encStr == NULL)
	    return 0;

	//Convert libguess result to microsoft codepage
	if (!strcmp(encStr, "ISO-2022-JP") || !strcmp(encStr, "SJIS")) {
	    return 932; //japanese
	} else if (!strcmp(encStr, "GB2312") || !strcmp(encStr, "ISO-2022-CN")) {
        return 936; //chinese
	} else if (!strcmp(encStr, "BIG5") || !strcmp(encStr, "ISO-2022-TW")) {
	    return 950; //chinese (traditional)
	} else if (!strcmp(encStr, "EUC-KR") || !strcmp(encStr, "ISO-2022-KR")) {
	    return 949; //korean
	} else if (!strcmp(encStr, "CP1256") || !strcmp(encStr, "ISO-8859-6")) {
	    return 1256; //arabic
	} else if (!strcmp(encStr, "CP1251")) {
	    return 1251; //russian
	} else if (!strcmp(encStr, "CP1253") || !strcmp(encStr, "ISO-8859-7")) {
	    return 1253; //greek
	} else if (!strcmp(encStr, "CP1255") || !strcmp(encStr, "ISO-8859-8-I")) {
	    return 1255; //hebrew
	} else if (!strcmp(encStr, "CP1250") || !strcmp(encStr, "ISO-8859-2")) {
	    return 1250; //polish
	} else if (!strcmp(encStr, "CP1254") || !strcmp(encStr, "ISO-8859-9")) {
	    return 1254; //turkish
	} else if (!strcmmp(encStr, "CP1257") || !strcmp(encStr, "ISO-8859-13")) {
	    return 1257; //baltic
	} else {
	    return 1252; //western
	}
}

int codepage_guessLdbEncoding(const Path filename)
{
    return 0;
}

BOOL codepage_init(const Path filename)
{
    if (!(libguess = LoadLibraryW(filename)))
        return FALSE;
    if (!(_libguess_determine_encoding = (void*)GetProcAddress(libguess, "libguess_determine_encoding")))
        return FALSE;
    return TRUE;
}

void codepage_free()
{
    FreeLibrary(libguess);
}

#endif
