#include "micro_aac/aac_encoder.h"
#include "micro_aac/aac_decoder.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

static int test_roundtrip_dm(uint32_t sample_rate, uint32_t channels, micro_aac_format_t format,
                             micro_aac_dec_downmix_t downmix, uint32_t out_channels) {
    printf("Testing roundtrip: rate=%u, channels=%u, format=%s, downmix=%d...\n",
           sample_rate, channels, (format == MICRO_AAC_FORMAT_ADTS) ? "ADTS" : "RAW", (int)downmix);

    micro_aac_config_t enc_cfg;
    micro_aac_config_init(&enc_cfg);
    enc_cfg.sample_rate = sample_rate;
    enc_cfg.num_channels = channels;
    enc_cfg.format = format;
    enc_cfg.object_type = MICRO_AAC_OBJECT_LOW;

    micro_aac_encoder_t *encoder = NULL;
    micro_aac_status_t status = micro_aac_encoder_create(&enc_cfg, &encoder);
    if (status != MICRO_AAC_OK || !encoder) {
        fprintf(stderr, "Failed to create encoder\n");
        return 1;
    }

    const uint8_t *asc_buf = NULL;
    uint32_t asc_len = 0;
    micro_aac_encoder_get_decoder_info(encoder, &asc_buf, &asc_len);

    micro_aac_dec_config_t dec_cfg;
    micro_aac_dec_config_init(&dec_cfg);
    dec_cfg.stream_format = (format == MICRO_AAC_FORMAT_ADTS) ? MICRO_AAC_DEC_STREAM_ADTS : MICRO_AAC_DEC_STREAM_RAW;
    dec_cfg.asc_buf = asc_buf;
    dec_cfg.asc_len = asc_len;
    dec_cfg.downmix = downmix;

    micro_aac_decoder_t *decoder = NULL;
    micro_aac_dec_status_t dec_status = micro_aac_decoder_create(&dec_cfg, &decoder);
    if (dec_status != MICRO_AAC_DEC_OK || !decoder) {
        fprintf(stderr, "Failed to create decoder\n");
        micro_aac_encoder_destroy(encoder);
        return 1;
    }

    uint32_t frame_samples = micro_aac_encoder_get_frame_samples(encoder);
    uint32_t max_out = micro_aac_encoder_get_max_output_bytes(encoder);

    int16_t *pcm_in = (int16_t *)malloc(frame_samples * channels * sizeof(int16_t));
    uint8_t *aac_buf = (uint8_t *)malloc(max_out);
    uint32_t dec_max_bytes = micro_aac_decoder_get_max_output_bytes(decoder);
    if (dec_max_bytes < 2048 * channels * sizeof(int16_t)) {
        dec_max_bytes = 2048 * channels * sizeof(int16_t);
    }
    int16_t *pcm_out = (int16_t *)malloc(dec_max_bytes);

    if (!pcm_in || !aac_buf || !pcm_out) {
        fprintf(stderr, "Allocation failed\n");
        return 1;
    }

    uint32_t bytes_written = 0;
    int frame_count = 0;

    // Encode multiple frames until encoder emits AAC frame
    while (bytes_written == 0 && frame_count < 10) {
        for (uint32_t i = 0; i < frame_samples * channels; i++) {
            double t = (double)(frame_count * frame_samples + i / channels) / enc_cfg.sample_rate;
            pcm_in[i] = (int16_t)(25000.0 * sin(2.0 * 3.1415926535 * 440.0 * t));
        }

        status = micro_aac_encoder_encode(encoder, pcm_in, frame_samples * channels, aac_buf, max_out, &bytes_written);
        frame_count++;
    }

    if (status != MICRO_AAC_OK || bytes_written == 0) {
        fprintf(stderr, "Encode failed: status=%d, bytes=%u\n", status, bytes_written);
        free(pcm_in);
        free(aac_buf);
        free(pcm_out);
        micro_aac_encoder_destroy(encoder);
        micro_aac_decoder_destroy(decoder);
        return 1;
    }

    printf("Encoded AAC frame: %u bytes\n", bytes_written);

    uint8_t *in_ptr = aac_buf;
    int32_t bytes_left = (int32_t)bytes_written;
    uint32_t samps_decoded = 0;

    dec_status = micro_aac_decoder_decode(decoder, &in_ptr, &bytes_left, pcm_out, &samps_decoded);
    printf("Decoder result: status=%d, decoded_samps=%u, bytes_left=%d\n", dec_status, samps_decoded, bytes_left);

    if (dec_status != MICRO_AAC_DEC_OK || samps_decoded == 0) {
        fprintf(stderr, "Decode failed\n");
        free(pcm_in);
        free(aac_buf);
        free(pcm_out);
        micro_aac_encoder_destroy(encoder);
        micro_aac_decoder_destroy(decoder);
        return 1;
    }

    micro_aac_dec_frame_info_t dec_info;
    micro_aac_decoder_get_frame_info(decoder, &dec_info);
    printf("Decoded frame info: sr=%u, ch=%u, samps=%u\n", dec_info.sample_rate, dec_info.num_channels, dec_info.output_samps);

    if (dec_info.sample_rate != sample_rate || dec_info.num_channels != out_channels) {
        fprintf(stderr, "Sample rate or channel count mismatch: expected %u/%u, got %u/%u\n",
                sample_rate, out_channels, dec_info.sample_rate, dec_info.num_channels);
        free(pcm_in);
        free(aac_buf);
        free(pcm_out);
        micro_aac_encoder_destroy(encoder);
        micro_aac_decoder_destroy(decoder);
        return 1;
    }

    free(pcm_in);
    free(aac_buf);
    free(pcm_out);
    micro_aac_encoder_destroy(encoder);
    micro_aac_decoder_destroy(decoder);
    return 0;
}

static int test_roundtrip(uint32_t sample_rate, uint32_t channels, micro_aac_format_t format) {
    return test_roundtrip_dm(sample_rate, channels, format, MICRO_AAC_DEC_DOWNMIX_NONE, channels);
}

int main(void) {
    printf("Starting micro_aac encoder + decoder test suite...\n");

    if (test_roundtrip(16000, 1, MICRO_AAC_FORMAT_ADTS) != 0) return 1;
    if (test_roundtrip(44100, 2, MICRO_AAC_FORMAT_ADTS) != 0) return 1;
    if (test_roundtrip(48000, 1, MICRO_AAC_FORMAT_RAW) != 0) return 1;
    if (test_roundtrip(48000, 2, MICRO_AAC_FORMAT_RAW) != 0) return 1;

    if (test_roundtrip_dm(44100, 2, MICRO_AAC_FORMAT_ADTS, MICRO_AAC_DEC_DOWNMIX_MONO, 1) != 0) return 1;
    if (test_roundtrip_dm(48000, 2, MICRO_AAC_FORMAT_RAW, MICRO_AAC_DEC_DOWNMIX_MONO, 1) != 0) return 1;
    if (test_roundtrip_dm(44100, 2, MICRO_AAC_FORMAT_ADTS, MICRO_AAC_DEC_DOWNMIX_STEREO, 2) != 0) return 1;

    printf("All decoder tests passed successfully!\n");
    return 0;
}
