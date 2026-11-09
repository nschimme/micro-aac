#include "micro_aac/aac_decoder.h"
#include "faad3.h"

#include <stdlib.h>
#include <string.h>

#if defined(ESP_PLATFORM)
#include "esp_heap_caps.h"
#endif

struct micro_aac_decoder {
    faad_decoder *faad_handle;
    micro_aac_dec_config_t config;
    faad_stream_info info;
    uint32_t last_output_samps;
};

void micro_aac_dec_config_init(micro_aac_dec_config_t *config) {
    if (!config) return;
    memset(config, 0, sizeof(*config));
    config->stream_format = MICRO_AAC_DEC_STREAM_ADTS;
    config->prefer_psram = true;
    config->asc_buf = NULL;
    config->asc_len = 0;
}

micro_aac_dec_status_t micro_aac_decoder_create(const micro_aac_dec_config_t *config, micro_aac_decoder_t **decoder_out) {
    if (!decoder_out) {
        return MICRO_AAC_DEC_ERR_INVALID_ARG;
    }

    micro_aac_decoder_t *dec = NULL;

#if defined(ESP_PLATFORM)
    if (config && config->prefer_psram) {
        dec = (micro_aac_decoder_t *)heap_caps_malloc(sizeof(micro_aac_decoder_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    }
#endif
    if (!dec) {
        dec = (micro_aac_decoder_t *)malloc(sizeof(micro_aac_decoder_t));
    }

    if (!dec) {
        return MICRO_AAC_DEC_ERR_NO_MEM;
    }

    memset(dec, 0, sizeof(*dec));
    if (config) {
        dec->config = *config;
    } else {
        micro_aac_dec_config_init(&dec->config);
    }

    faad_config f_cfg;
    faad_status st = faad_config_init(&f_cfg, sizeof(f_cfg));
    if (st != FAAD_OK) {
        free(dec);
        return MICRO_AAC_DEC_ERR_INIT_FAILED;
    }

    f_cfg.stream_format = (dec->config.stream_format == MICRO_AAC_DEC_STREAM_RAW) ? FAAD_STREAM_RAW : FAAD_STREAM_ADTS;
    f_cfg.output_format = FAAD_OUTPUT_16BIT;
    f_cfg.downmix_mode = FAAD_DOWNMIX_NONE;

    st = faad_decoder_open(&f_cfg, dec->config.asc_buf, dec->config.asc_len, &dec->faad_handle);
    if (st != FAAD_OK || !dec->faad_handle) {
        free(dec);
        return MICRO_AAC_DEC_ERR_INIT_FAILED;
    }

    dec->info.struct_size = sizeof(faad_stream_info);
    faad_decoder_get_info(dec->faad_handle, &dec->info);

    *decoder_out = dec;
    return MICRO_AAC_DEC_OK;
}

uint32_t micro_aac_decoder_get_max_output_bytes(micro_aac_decoder_t *decoder) {
    if (!decoder || !decoder->faad_handle) return 0;
    decoder->info.struct_size = sizeof(faad_stream_info);
    faad_decoder_get_info(decoder->faad_handle, &decoder->info);
    return decoder->info.max_output_bytes;
}

micro_aac_dec_status_t micro_aac_decoder_decode_ex(
    micro_aac_decoder_t *decoder,
    uint8_t **in_buf,
    int32_t *bytes_left,
    int16_t *pcm_out,
    uint32_t out_cap_bytes,
    uint32_t *samps_written
) {
    if (!decoder || !decoder->faad_handle || !in_buf || !*in_buf || !bytes_left || !pcm_out || !samps_written) {
        return MICRO_AAC_DEC_ERR_INVALID_ARG;
    }

    if (*bytes_left <= 0) {
        *samps_written = 0;
        return MICRO_AAC_DEC_ERR_UNDERFLOW;
    }

    decoder->info.struct_size = sizeof(faad_stream_info);
    faad_decoder_get_info(decoder->faad_handle, &decoder->info);

    uint32_t needed_cap = decoder->info.max_output_bytes;
    if (needed_cap == 0) {
        needed_cap = 2048 * 2 * sizeof(int16_t);
    }

    if (out_cap_bytes < needed_cap) {
        *samps_written = 0;
        return MICRO_AAC_DEC_ERR_BUFFER_TOO_SMALL;
    }

    uint32_t consumed = 0;
    uint32_t bytes_out = 0;
    uint32_t frame_flags = 0;

    faad_status st = faad_decode_frame(
        decoder->faad_handle,
        *in_buf,
        (uint32_t)*bytes_left,
        &consumed,
        pcm_out,
        out_cap_bytes,
        &bytes_out,
        &frame_flags
    );

    if (consumed > 0) {
        *in_buf += consumed;
        *bytes_left -= (int32_t)consumed;
    }

    if (st == FAAD_OK) {
        *samps_written = bytes_out / sizeof(int16_t);
        decoder->last_output_samps = *samps_written;
        decoder->info.struct_size = sizeof(faad_stream_info);
        faad_decoder_get_info(decoder->faad_handle, &decoder->info);
        return MICRO_AAC_DEC_OK;
    } else if (st == FAAD_ERR_NEED_MORE_DATA) {
        *samps_written = 0;
        return MICRO_AAC_DEC_ERR_UNDERFLOW;
    } else if (st == FAAD_ERR_OUTPUT_TOO_SMALL) {
        *samps_written = 0;
        return MICRO_AAC_DEC_ERR_BUFFER_TOO_SMALL;
    } else if (st == FAAD_ERR_SYNC_LOST) {
        *samps_written = 0;
        return MICRO_AAC_DEC_ERR_BAD_HEADER;
    } else {
        *samps_written = 0;
        return MICRO_AAC_DEC_ERR_DECODE_FAILED;
    }
}

micro_aac_dec_status_t micro_aac_decoder_decode(
    micro_aac_decoder_t *decoder,
    uint8_t **in_buf,
    int32_t *bytes_left,
    int16_t *pcm_out,
    uint32_t *samps_written
) {
    if (!decoder || !decoder->faad_handle) {
        return MICRO_AAC_DEC_ERR_INVALID_ARG;
    }

    decoder->info.struct_size = sizeof(faad_stream_info);
    faad_decoder_get_info(decoder->faad_handle, &decoder->info);

    uint32_t cap = decoder->info.max_output_bytes;
    if (cap < 2048 * 2 * sizeof(int16_t)) {
        cap = 2048 * 2 * sizeof(int16_t);
    }

    return micro_aac_decoder_decode_ex(decoder, in_buf, bytes_left, pcm_out, cap, samps_written);
}

int32_t micro_aac_decoder_find_syncword(const uint8_t *buf, uint32_t buf_len) {
    if (!buf || buf_len < 2) return -1;

    for (uint32_t i = 0; i + 1 < buf_len; i++) {
        if (buf[i] == 0xFF && (buf[i + 1] & 0xF0) == 0xF0) {
            return (int32_t)i;
        }
    }
    return -1;
}

micro_aac_dec_status_t micro_aac_decoder_get_frame_info(
    micro_aac_decoder_t *decoder,
    micro_aac_dec_frame_info_t *info
) {
    if (!decoder || !decoder->faad_handle || !info) {
        return MICRO_AAC_DEC_ERR_INVALID_ARG;
    }

    decoder->info.struct_size = sizeof(faad_stream_info);
    faad_decoder_get_info(decoder->faad_handle, &decoder->info);

    info->sample_rate = decoder->info.sample_rate;
    info->num_channels = decoder->info.channels;
    info->bit_rate = 0;
    info->output_samps = decoder->last_output_samps;
    info->profile = (uint8_t)decoder->info.object_type;
    info->frame_samples = decoder->info.frame_samples;
    info->max_output_bytes = decoder->info.max_output_bytes;

    return MICRO_AAC_DEC_OK;
}

void micro_aac_decoder_flush(micro_aac_decoder_t *decoder) {
    if (!decoder || !decoder->faad_handle) return;
    faad_decoder_flush(decoder->faad_handle);
    decoder->last_output_samps = 0;
}

void micro_aac_decoder_destroy(micro_aac_decoder_t *decoder) {
    if (!decoder) return;

    if (decoder->faad_handle) {
        faad_decoder_close(&decoder->faad_handle);
    }

    free(decoder);
}
