#include "micro_aac/aac_encoder.h"
#include "faac.h"

#include <stdlib.h>
#include <string.h>

#if defined(ESP_PLATFORM)
#include "esp_heap_caps.h"
#endif

struct micro_aac_encoder {
    faac_encoder *faac_handle;
    micro_aac_config_t config;
    faac_encoder_info info;
    const uint8_t *asc_buf;
    uint32_t asc_len;
};

void micro_aac_config_init(micro_aac_config_t *config) {
    if (!config) return;
    memset(config, 0, sizeof(*config));
    config->sample_rate = 16000;
    config->num_channels = 1;
    config->bit_rate = 0; // default rate
    config->format = MICRO_AAC_FORMAT_ADTS;
    config->object_type = MICRO_AAC_OBJECT_LOW;
    config->prefer_psram = true;
}

micro_aac_status_t micro_aac_encoder_create(const micro_aac_config_t *config, micro_aac_encoder_t **encoder_out) {
    if (!config || !encoder_out) {
        return MICRO_AAC_ERR_INVALID_ARG;
    }

    if (config->sample_rate == 0 || config->num_channels == 0 || config->num_channels > 2) {
        return MICRO_AAC_ERR_INVALID_ARG;
    }

    micro_aac_encoder_t *enc = NULL;

#if defined(ESP_PLATFORM)
    if (config->prefer_psram) {
        enc = (micro_aac_encoder_t *)heap_caps_malloc(sizeof(micro_aac_encoder_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    }
#endif
    if (!enc) {
        enc = (micro_aac_encoder_t *)malloc(sizeof(micro_aac_encoder_t));
    }

    if (!enc) {
        return MICRO_AAC_ERR_NO_MEM;
    }

    memset(enc, 0, sizeof(*enc));
    enc->config = *config;

    faac_params params;
    faac_status status = faac_params_init(&params, sizeof(params));
    if (status != FAAC_OK) {
        free(enc);
        return MICRO_AAC_ERR_ENCODER_INIT;
    }

    params.sample_rate = config->sample_rate;
    params.num_channels = config->num_channels;
    params.mpeg_version = FAAC_MPEG4;
    params.object_type = (config->object_type == MICRO_AAC_OBJECT_HE) ? FAAC_OBJ_HE_AAC_V1 : FAAC_OBJ_LOW;
    params.output_format = (config->format == MICRO_AAC_FORMAT_ADTS) ? FAAC_STREAM_ADTS : FAAC_STREAM_RAW;
    params.input_format = FAAC_INPUT_16BIT;

    if (config->bit_rate > 0) {
        params.bit_rate = config->bit_rate / config->num_channels;
    }

    status = faac_encoder_open(&params, &enc->faac_handle);
    if (status != FAAC_OK || !enc->faac_handle) {
        free(enc);
        return MICRO_AAC_ERR_ENCODER_INIT;
    }

    enc->info.struct_size = sizeof(faac_encoder_info);
    status = faac_encoder_get_info(enc->faac_handle, &enc->info);
    if (status != FAAC_OK) {
        faac_encoder_close(&enc->faac_handle);
        free(enc);
        return MICRO_AAC_ERR_ENCODER_INIT;
    }

    faac_encoder_asc(enc->faac_handle, &enc->asc_buf, &enc->asc_len);

    *encoder_out = enc;
    return MICRO_AAC_OK;
}

uint32_t micro_aac_encoder_get_frame_samples(micro_aac_encoder_t *encoder) {
    if (!encoder) return 0;
    return encoder->info.frame_samples;
}

uint32_t micro_aac_encoder_get_max_output_bytes(micro_aac_encoder_t *encoder) {
    if (!encoder) return 0;
    return encoder->info.max_output_bytes;
}

micro_aac_status_t micro_aac_encoder_encode(
    micro_aac_encoder_t *encoder,
    const int16_t *pcm_in,
    uint32_t in_samples,
    uint8_t *out_buf,
    uint32_t out_capacity,
    uint32_t *bytes_written
) {
    if (!encoder || !out_buf || !bytes_written) {
        return MICRO_AAC_ERR_INVALID_ARG;
    }

    if (out_capacity < encoder->info.max_output_bytes) {
        return MICRO_AAC_ERR_BUFFER_TOO_SMALL;
    }

    faac_status status = faac_encoder_encode(
        encoder->faac_handle,
        pcm_in,
        in_samples,
        out_buf,
        out_capacity,
        bytes_written
    );

    if (status != FAAC_OK) {
        return MICRO_AAC_ERR_ENCODE_FAILED;
    }

    return MICRO_AAC_OK;
}

micro_aac_status_t micro_aac_encoder_get_decoder_info(
    micro_aac_encoder_t *encoder,
    const uint8_t **asc_buf,
    uint32_t *asc_len
) {
    if (!encoder || !asc_buf || !asc_len) {
        return MICRO_AAC_ERR_INVALID_ARG;
    }

    *asc_buf = encoder->asc_buf;
    *asc_len = encoder->asc_len;
    return MICRO_AAC_OK;
}

void micro_aac_encoder_destroy(micro_aac_encoder_t *encoder) {
    if (!encoder) return;

    if (encoder->faac_handle) {
        faac_encoder_close(&encoder->faac_handle);
    }

    free(encoder);
}
