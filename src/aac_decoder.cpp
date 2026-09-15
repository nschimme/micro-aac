#include "micro_aac/aac_decoder.h"
#include "aacdec.h"

#include <stdlib.h>
#include <string.h>

#if defined(ESP_PLATFORM)
#include "esp_heap_caps.h"
#endif

struct micro_aac_decoder {
    HAACDecoder helix_handle;
    micro_aac_dec_config_t config;
};

void micro_aac_dec_config_init(micro_aac_dec_config_t *config) {
    if (!config) return;
    memset(config, 0, sizeof(*config));
    config->prefer_psram = true;
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

    dec->helix_handle = AACInitDecoder();
    if (!dec->helix_handle) {
        free(dec);
        return MICRO_AAC_DEC_ERR_INIT_FAILED;
    }

    *decoder_out = dec;
    return MICRO_AAC_DEC_OK;
}

micro_aac_dec_status_t micro_aac_decoder_decode(
    micro_aac_decoder_t *decoder,
    uint8_t **in_buf,
    int32_t *bytes_left,
    int16_t *pcm_out,
    uint32_t *samps_written
) {
    if (!decoder || !decoder->helix_handle || !in_buf || !*in_buf || !bytes_left || !pcm_out || !samps_written) {
        return MICRO_AAC_DEC_ERR_INVALID_ARG;
    }

    int left = (int)*bytes_left;
    int err = AACDecode(decoder->helix_handle, in_buf, &left, (short *)pcm_out);
    *bytes_left = left;

    if (err == ERR_AAC_NONE) {
        AACFrameInfo info;
        AACGetLastFrameInfo(decoder->helix_handle, &info);
        *samps_written = (uint32_t)info.outputSamps;
        return MICRO_AAC_DEC_OK;
    } else if (err == ERR_AAC_INDATA_UNDERFLOW) {
        *samps_written = 0;
        return MICRO_AAC_DEC_ERR_UNDERFLOW;
    } else if (err == ERR_AAC_INVALID_ADTS_HEADER || err == ERR_AAC_INVALID_ADIF_HEADER) {
        *samps_written = 0;
        return MICRO_AAC_DEC_ERR_BAD_HEADER;
    } else {
        *samps_written = 0;
        return MICRO_AAC_DEC_ERR_DECODE_FAILED;
    }
}

int32_t micro_aac_decoder_find_syncword(const uint8_t *buf, uint32_t buf_len) {
    if (!buf || buf_len < 7) return -1;
    return (int32_t)AACFindSyncWord((unsigned char *)buf, (int)buf_len);
}

micro_aac_dec_status_t micro_aac_decoder_get_frame_info(
    micro_aac_decoder_t *decoder,
    micro_aac_dec_frame_info_t *info
) {
    if (!decoder || !decoder->helix_handle || !info) {
        return MICRO_AAC_DEC_ERR_INVALID_ARG;
    }

    AACFrameInfo helix_info;
    AACGetLastFrameInfo(decoder->helix_handle, &helix_info);

    info->sample_rate = (uint32_t)helix_info.sampRateOut;
    info->num_channels = (uint32_t)helix_info.nChans;
    info->bit_rate = (uint32_t)helix_info.bitRate;
    info->output_samps = (uint32_t)helix_info.outputSamps;
    info->profile = (uint8_t)helix_info.profile;

    return MICRO_AAC_DEC_OK;
}

void micro_aac_decoder_flush(micro_aac_decoder_t *decoder) {
    if (!decoder || !decoder->helix_handle) return;
    AACFlushCodec(decoder->helix_handle);
}

void micro_aac_decoder_destroy(micro_aac_decoder_t *decoder) {
    if (!decoder) return;

    if (decoder->helix_handle) {
        AACFreeDecoder(decoder->helix_handle);
    }

    free(decoder);
}
