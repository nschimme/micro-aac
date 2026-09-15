#ifndef MICRO_AAC_AAC_ENCODER_H
#define MICRO_AAC_AAC_ENCODER_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    MICRO_AAC_OK = 0,
    MICRO_AAC_ERR_INVALID_ARG = -1,
    MICRO_AAC_ERR_NO_MEM = -2,
    MICRO_AAC_ERR_ENCODER_INIT = -3,
    MICRO_AAC_ERR_ENCODE_FAILED = -4,
    MICRO_AAC_ERR_BUFFER_TOO_SMALL = -5
} micro_aac_status_t;

typedef enum {
    MICRO_AAC_FORMAT_RAW = 0,
    MICRO_AAC_FORMAT_ADTS = 1
} micro_aac_format_t;

typedef enum {
    MICRO_AAC_OBJECT_LOW = 2,   /* AAC Low Complexity (LC) */
    MICRO_AAC_OBJECT_HE = 5     /* High Efficiency AAC (HE-AAC) */
} micro_aac_object_type_t;

typedef struct {
    uint32_t sample_rate;        /* Input/output sample rate (Hz), e.g., 16000, 44100, 48000 */
    uint32_t num_channels;       /* Channels count: 1 (mono) or 2 (stereo) */
    uint32_t bit_rate;           /* Bitrate per channel in bits/sec (e.g., 64000). 0 = default */
    micro_aac_format_t format;   /* MICRO_AAC_FORMAT_ADTS or MICRO_AAC_FORMAT_RAW */
    micro_aac_object_type_t object_type; /* MICRO_AAC_OBJECT_LOW (LC) or MICRO_AAC_OBJECT_HE */
    bool prefer_psram;           /* If true and on ESP32, attempt PSRAM/SPIRAM allocation */
} micro_aac_config_t;

typedef struct {
    uint32_t sample_rate;
    uint32_t num_channels;
    uint32_t frame_length;      /* Total ADTS frame size in bytes including header */
    uint8_t  profile;           /* AAC Profile / Object type (0=Main, 1=LC, 2=SSR) */
} micro_aac_adts_header_t;

typedef struct micro_aac_encoder micro_aac_encoder_t;

/**
 * Initialize default AAC encoder configuration.
 */
void micro_aac_config_init(micro_aac_config_t *config);

/**
 * Create a new AAC encoder instance.
 */
micro_aac_status_t micro_aac_encoder_create(const micro_aac_config_t *config, micro_aac_encoder_t **encoder_out);

/**
 * Get required input samples per channel per frame (typically 1024 for LC AAC).
 */
uint32_t micro_aac_encoder_get_frame_samples(micro_aac_encoder_t *encoder);

/**
 * Get maximum output buffer size needed for one encoded frame.
 */
uint32_t micro_aac_encoder_get_max_output_bytes(micro_aac_encoder_t *encoder);

/**
 * Get priming delay in samples/channel for gapless playback tagging.
 */
uint32_t micro_aac_encoder_get_delay(micro_aac_encoder_t *encoder);

/**
 * Parse an ADTS header from an encoded frame buffer (at least 7 bytes).
 */
micro_aac_status_t micro_aac_adts_parse_header(const uint8_t *adts_buf, uint32_t buf_len, micro_aac_adts_header_t *out_hdr);

/**
 * Encode PCM audio samples (16-bit signed PCM, interleaved if stereo) into AAC frame.
 */
micro_aac_status_t micro_aac_encoder_encode(
    micro_aac_encoder_t *encoder,
    const int16_t *pcm_in,
    uint32_t in_samples,
    uint8_t *out_buf,
    uint32_t out_capacity,
    uint32_t *bytes_written
);

/**
 * Retrieve AudioSpecificConfig (decoder specific info) if in raw format.
 */
micro_aac_status_t micro_aac_encoder_get_decoder_info(
    micro_aac_encoder_t *encoder,
    const uint8_t **asc_buf,
    uint32_t *asc_len
);

/**
 * Destroy the AAC encoder instance and free resources.
 */
void micro_aac_encoder_destroy(micro_aac_encoder_t *encoder);

#ifdef __cplusplus
}

namespace micro_aac {

class AACEncoder {
public:
    AACEncoder() : encoder_(nullptr) {}
    ~AACEncoder() { destroy(); }

    micro_aac_status_t init(const micro_aac_config_t &config) {
        destroy();
        return micro_aac_encoder_create(&config, &encoder_);
    }

    void destroy() {
        if (encoder_) {
            micro_aac_encoder_destroy(encoder_);
            encoder_ = nullptr;
        }
    }

    uint32_t get_frame_samples() const {
        return encoder_ ? micro_aac_encoder_get_frame_samples(encoder_) : 0;
    }

    uint32_t get_max_output_bytes() const {
        return encoder_ ? micro_aac_encoder_get_max_output_bytes(encoder_) : 0;
    }

    uint32_t get_delay() const {
        return encoder_ ? micro_aac_encoder_get_delay(encoder_) : 0;
    }

    micro_aac_status_t encode(const int16_t *pcm_in, uint32_t in_samples, uint8_t *out_buf, uint32_t out_capacity, uint32_t *bytes_written) {
        return micro_aac_encoder_encode(encoder_, pcm_in, in_samples, out_buf, out_capacity, bytes_written);
    }

    micro_aac_status_t get_decoder_info(const uint8_t **asc_buf, uint32_t *asc_len) {
        return micro_aac_encoder_get_decoder_info(encoder_, asc_buf, asc_len);
    }

    micro_aac_encoder_t *get_handle() const { return encoder_; }

private:
    micro_aac_encoder_t *encoder_;
};

} // namespace micro_aac

#endif /* __cplusplus */

#endif /* MICRO_AAC_AAC_ENCODER_H */
