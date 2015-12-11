#ifndef CONFIG_H
#define CONFIG_H

#include "common.h"

typedef struct {
    int codepage;
    Path savePath;
    Path rtpPath;
} Config;

extern Config config;

BOOL config_init();

#endif
