#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#define PACKAGE "micro-aac"
#define PACKAGE_VERSION "0.1.0"

#ifndef MAX_CHANNELS
#define MAX_CHANNELS 2
#endif

#ifndef FAAC_SBR_DECIMATION
#define FAAC_SBR_DECIMATION 1
#endif

#ifndef FAAC_STATS
#define FAAC_STATS 0
#endif
