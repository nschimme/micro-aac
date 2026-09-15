#ifndef MICRO_AAC_DEFAULT_CONFIG_H
#define MICRO_AAC_DEFAULT_CONFIG_H

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

#ifndef HAVE_CONFIG_H
#define HAVE_CONFIG_H
#endif

#ifndef HELIX_CONFIG_H
#define HELIX_CONFIG_H
#endif

#ifndef USE_DEFAULT_STDLIB
#define USE_DEFAULT_STDLIB
#endif

#endif /* MICRO_AAC_DEFAULT_CONFIG_H */
