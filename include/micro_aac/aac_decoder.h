#ifndef MICRO_AAC_AAC_DECODER_H
#define MICRO_AAC_AAC_DECODER_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    MICRO_AAC_DEC_OK = 0,
    MICRO_AAC_DEC_ERR_INVALID_ARG = -1,
    MICRO_AAC_DEC_ERR_NO_MEM = -2,
    MICRO_AAC_DEC_ERR_INIT_FAILED = -3,
    MICRO_AAC_DEC_ERR_DECODE_FAILED = -4,
    MICRO_AAC_DEC_ERR_UNDERFLOW = -5,
    MICRO_AAC_DEC_ERR_BAD_HEADER = -6,
    MICRO_AAC_DEC_ERR_BUFFER_TOO_SMALL = -7
} micro_aac_dec_status_t;

typedef enum {
    MICRO_AAC_DEC_STREAM_RAW = 0,
    MICRO_AAC_DEC_STREAM_ADTS = 1
} micro_aac_dec_stream_format_t;

typedef enum {
    MICRO_AAC_DEC_DOWNMIX_NONE = 0,   /* Preserve native channel layout (default) */
    MICRO_AAC_DEC_DOWNMIX_MONO = 1,   /* Downmix all channels to mono */
    MICRO_AAC_DEC_DOWNMIX_STEREO = 2  /* Downmix surround to 2-channel stereo */
} micro_aac_dec_downmix_t;

typedef struct {
    micro_aac_dec_stream_format_t stream_format; /* ADTS (default) or RAW */
    bool prefer_psram;                           /* If true on ESP32, attempt PSRAM/SPIRAM allocation */
    const uint8_t *asc_buf;                      /* AudioSpecificConfig buffer (required for RAW, optional for ADTS) */
    uint32_t asc_len;                            /* Length of asc_buf */
    micro_aac_dec_downmix_t downmix;             /* Channel downmix mode (default: none) */
} micro_aac_dec_config_t;

typedef struct {
    uint32_t sample_rate;    /* Output sample rate (Hz) */
    uint32_t num_channels;   /* Channel count (1=mono, 2=stereo) */
    uint32_t bit_rate;       /* Bitrate in bits/sec (if known) */
    uint32_t output_samps;   /* Output PCM samples generated in last decode call (total across channels) */
    uint8_t  profile;        /* AAC profile/object type */
    uint32_t frame_samples;  /* Samples per channel per frame (typically 1024, or 2048 with SBR) */
    uint32_t max_output_bytes; /* Maximum possible output PCM bytes for a frame */
} micro_aac_dec_frame_info_t;

typedef struct micro_aac_decoder micro_aac_decoder_t;

/**
 * Initialize default AAC decoder configuration.
 */
void micro_aac_dec_config_init(micro_aac_dec_config_t *config);

/**
 * Create a new AAC decoder instance.
 */
micro_aac_dec_status_t micro_aac_decoder_create(const micro_aac_dec_config_t *config, micro_aac_decoder_t **decoder_out);

/**
 * Decode an AAC audio frame (ADTS or RAW AAC) into 16-bit PCM samples.
 *
 * @param decoder Pointer to decoder instance.
 * @param in_buf Pointer to pointer of input AAC bitstream buffer (updated upon consumption).
 * @param bytes_left Pointer to remaining bytes in input buffer (updated upon consumption).
 * @param pcm_out Output buffer for 16-bit signed PCM samples.
 * @param samps_written Output pointer to total PCM samples written across all channels.
 */
micro_aac_dec_status_t micro_aac_decoder_decode(
    micro_aac_decoder_t *decoder,
    uint8_t **in_buf,
    int32_t *bytes_left,
    int16_t *pcm_out,
    uint32_t *samps_written
);

/**
 * Decode an AAC audio frame with explicit output capacity in bytes.
 *
 * @param decoder Pointer to decoder instance.
 * @param in_buf Pointer to pointer of input AAC bitstream buffer (updated upon consumption).
 * @param bytes_left Pointer to remaining bytes in input buffer (updated upon consumption).
 * @param pcm_out Output buffer for 16-bit signed PCM samples.
 * @param out_cap_bytes Capacity of pcm_out in bytes.
 * @param samps_written Output pointer to total PCM samples written across all channels.
 */
micro_aac_dec_status_t micro_aac_decoder_decode_ex(
    micro_aac_decoder_t *decoder,
    uint8_t **in_buf,
    int32_t *bytes_left,
    int16_t *pcm_out,
    uint32_t out_cap_bytes,
    uint32_t *samps_written
);

/**
 * Find ADTS syncword in stream buffer.
 * Returns byte offset to syncword, or negative if not found.
 */
int32_t micro_aac_decoder_find_syncword(const uint8_t *buf, uint32_t buf_len);

/**
 * Get information about the stream / last decoded AAC frame.
 */
micro_aac_dec_status_t micro_aac_decoder_get_frame_info(
    micro_aac_decoder_t *decoder,
    micro_aac_dec_frame_info_t *info
);

/**
 * Get maximum required output PCM buffer size in bytes.
 */
uint32_t micro_aac_decoder_get_max_output_bytes(micro_aac_decoder_t *decoder);

/**
 * Flush internal decoder buffers/states.
 */
void micro_aac_decoder_flush(micro_aac_decoder_t *decoder);

/**
 * Destroy the AAC decoder instance and free resources.
 */
void micro_aac_decoder_destroy(micro_aac_decoder_t *decoder);

#ifdef __cplusplus
}

namespace micro_aac {

class AACDecoder {
public:
    AACDecoder() : decoder_(nullptr) {}
    ~AACDecoder() { destroy(); }

    micro_aac_dec_status_t init(const micro_aac_dec_config_t &config) {
        destroy();
        return micro_aac_decoder_create(&config, &decoder_);
    }

    void destroy() {
        if (decoder_) {
            micro_aac_decoder_destroy(decoder_);
            decoder_ = nullptr;
        }
    }

    micro_aac_dec_status_t decode(uint8_t **in_buf, int32_t *bytes_left, int16_t *pcm_out, uint32_t *samps_written) {
        return micro_aac_decoder_decode(decoder_, in_buf, bytes_left, pcm_out, samps_written);
    }

    micro_aac_dec_status_t decode(uint8_t **in_buf, int32_t *bytes_left, int16_t *pcm_out, uint32_t out_cap_bytes, uint32_t *samps_written) {
        return micro_aac_decoder_decode_ex(decoder_, in_buf, bytes_left, pcm_out, out_cap_bytes, samps_written);
    }

    micro_aac_dec_status_t get_frame_info(micro_aac_dec_frame_info_t *info) {
        return micro_aac_decoder_get_frame_info(decoder_, info);
    }

    uint32_t get_max_output_bytes() const {
        return micro_aac_decoder_get_max_output_bytes(decoder_);
    }

    void flush() {
        if (decoder_) micro_aac_decoder_flush(decoder_);
    }

    micro_aac_decoder_t *get_handle() const { return decoder_; }

private:
    micro_aac_decoder_t *decoder_;
};

} // namespace micro_aac

#endif /* __cplusplus */

#endif /* MICRO_AAC_AAC_DECODER_H */
