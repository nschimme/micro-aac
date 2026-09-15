#ifndef FAAC_FASTMATH_H
#define FAAC_FASTMATH_H

#include <math.h>

#if defined(__XTENSA__) && defined(__XCC__)
/* Xtensa FPU intrinsics if using Xtensa compiler */
#include <xtensa/simd.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

static inline float faac_fast_inv_sqrt(float x) {
    union {
        float f;
        uint32_t i;
    } conv;
    float x2 = x * 0.5f;
    conv.f = x;
    conv.i = 0x5f3759df - (conv.i >> 1);
    conv.f = conv.f * (1.5f - (x2 * conv.f * conv.f));
    return conv.f;
}

static inline float faac_fast_sqrt(float x) {
    if (x <= 0.0f) return 0.0f;
    return x * faac_fast_inv_sqrt(x);
}

#ifdef __cplusplus
}
#endif

#endif /* FAAC_FASTMATH_H */
