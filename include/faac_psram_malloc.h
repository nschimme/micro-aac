#ifndef FAAC_MALLOC_H
#define FAAC_MALLOC_H

#include <stdlib.h>

#if defined(ESP_PLATFORM)
#include "esp_heap_caps.h"

static inline void *faac_psram_malloc(size_t size) {
#if defined(CONFIG_MICRO_AAC_USE_SPIRAM)
    void *ptr = heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (ptr) return ptr;
#endif
    return malloc(size);
}

static inline void *faac_psram_calloc(size_t n, size_t size) {
#if defined(CONFIG_MICRO_AAC_USE_SPIRAM)
    void *ptr = heap_caps_calloc(n, size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (ptr) return ptr;
#endif
    return calloc(n, size);
}

static inline void faac_psram_free(void *ptr) {
    free(ptr);
}

#define malloc(s) faac_psram_malloc(s)
#define calloc(n, s) faac_psram_calloc(n, s)
#define free(p) faac_psram_free(p)

#endif /* ESP_PLATFORM */

#endif /* FAAC_MALLOC_H */
