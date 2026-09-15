#include "micro_aac/aac_encoder.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

int main(void) {
    printf("Starting micro_aac test...\n");

    micro_aac_config_t config;
    micro_aac_config_init(&config);
    config.sample_rate = 16000;
    config.num_channels = 1;
    config.format = MICRO_AAC_FORMAT_ADTS;
    config.object_type = MICRO_AAC_OBJECT_LOW;

    micro_aac_encoder_t *encoder = NULL;
    micro_aac_status_t status = micro_aac_encoder_create(&config, &encoder);
    if (status != MICRO_AAC_OK || !encoder) {
        fprintf(stderr, "Failed to create encoder: %d\n", status);
        return 1;
    }

    uint32_t frame_samples = micro_aac_encoder_get_frame_samples(encoder);
    uint32_t max_out = micro_aac_encoder_get_max_output_bytes(encoder);

    printf("Encoder initialized: frame_samples=%u, max_output_bytes=%u\n", frame_samples, max_out);

    int16_t *pcm_in = (int16_t *)malloc(frame_samples * sizeof(int16_t));
    uint8_t *out_buf = (uint8_t *)malloc(max_out);

    if (!pcm_in || !out_buf) {
        fprintf(stderr, "Failed to allocate buffers\n");
        micro_aac_encoder_destroy(encoder);
        return 1;
    }

    // Generate 440 Hz sine wave for test
    for (uint32_t i = 0; i < frame_samples; i++) {
        double t = (double)i / config.sample_rate;
        pcm_in[i] = (int16_t)(32000.0 * sin(2.0 * 3.1415926535 * 440.0 * t));
    }

    uint32_t bytes_written = 0;
    status = micro_aac_encoder_encode(encoder, pcm_in, frame_samples, out_buf, max_out, &bytes_written);
    if (status != MICRO_AAC_OK) {
        fprintf(stderr, "Encoding failed: %d\n", status);
        free(pcm_in);
        free(out_buf);
        micro_aac_encoder_destroy(encoder);
        return 1;
    }

    printf("Successfully encoded frame, bytes written: %u\n", bytes_written);

    // Verify ADTS header if ADTS format was selected and bytes written > 0
    if (config.format == MICRO_AAC_FORMAT_ADTS && bytes_written >= 7) {
        // ADTS syncword is 0xFFF
        if (out_buf[0] == 0xFF && (out_buf[1] & 0xF0) == 0xF0) {
            printf("ADTS syncword verified: 0x%02X 0x%02X\n", out_buf[0], out_buf[1]);
        } else {
            fprintf(stderr, "Invalid ADTS syncword: 0x%02X 0x%02X\n", out_buf[0], out_buf[1]);
            free(pcm_in);
            free(out_buf);
            micro_aac_encoder_destroy(encoder);
            return 1;
        }
    }

    // Test flushing
    uint32_t flush_bytes = 0;
    status = micro_aac_encoder_encode(encoder, NULL, 0, out_buf, max_out, &flush_bytes);
    printf("Flushed encoder: status=%d, flush_bytes=%u\n", status, flush_bytes);

    free(pcm_in);
    free(out_buf);
    micro_aac_encoder_destroy(encoder);

    printf("Test passed!\n");
    return 0;
}
