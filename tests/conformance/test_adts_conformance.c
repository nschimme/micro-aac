#include "micro_aac/aac_encoder.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define SAMPLES_PER_FRAME 1024
#define NUM_FRAMES 10

int main(void) {
    printf("--- Running AAC ADTS Bitstream Conformance Test ---\n");

    micro_aac_config_t config;
    micro_aac_config_init(&config);
    config.sample_rate = 44100;
    config.num_channels = 2; // Stereo
    config.bit_rate = 128000;
    config.format = MICRO_AAC_FORMAT_ADTS;
    config.object_type = MICRO_AAC_OBJECT_LOW;

    micro_aac_encoder_t *encoder = NULL;
    micro_aac_status_t status = micro_aac_encoder_create(&config, &encoder);
    if (status != MICRO_AAC_OK || !encoder) {
        fprintf(stderr, "Failed to create encoder\n");
        return 1;
    }

    uint32_t frame_samples = micro_aac_encoder_get_frame_samples(encoder);
    uint32_t max_out = micro_aac_encoder_get_max_output_bytes(encoder);

    int16_t *pcm_in = (int16_t *)malloc(frame_samples * config.num_channels * sizeof(int16_t));
    uint8_t *out_buf = (uint8_t *)malloc(max_out);

    if (!pcm_in || !out_buf) {
        fprintf(stderr, "Allocation failed\n");
        micro_aac_encoder_destroy(encoder);
        return 1;
    }

    uint32_t total_encoded_bytes = 0;
    uint32_t total_frames = 0;

    for (int frame = 0; frame < NUM_FRAMES; frame++) {
        for (uint32_t i = 0; i < frame_samples * config.num_channels; i++) {
            double t = (double)(frame * frame_samples + i / config.num_channels) / config.sample_rate;
            pcm_in[i] = (int16_t)(20000.0 * sin(2.0 * 3.1415926535 * 1000.0 * t));
        }

        uint32_t bytes_written = 0;
        status = micro_aac_encoder_encode(encoder, pcm_in, frame_samples * config.num_channels, out_buf, max_out, &bytes_written);
        if (status != MICRO_AAC_OK) {
            fprintf(stderr, "Frame %d encode failed: %d\n", frame, status);
            free(pcm_in);
            free(out_buf);
            micro_aac_encoder_destroy(encoder);
            return 1;
        }

        if (bytes_written > 0) {
            micro_aac_adts_header_t hdr;
            status = micro_aac_adts_parse_header(out_buf, bytes_written, &hdr);
            if (status != MICRO_AAC_OK) {
                fprintf(stderr, "ADTS header validation failed for frame %d\n", frame);
                free(pcm_in);
                free(out_buf);
                micro_aac_encoder_destroy(encoder);
                return 1;
            }

            if (hdr.sample_rate != config.sample_rate || hdr.num_channels != config.num_channels) {
                fprintf(stderr, "Header parameter mismatch in frame %d\n", frame);
                free(pcm_in);
                free(out_buf);
                micro_aac_encoder_destroy(encoder);
                return 1;
            }

            total_encoded_bytes += bytes_written;
            total_frames++;
        }
    }

    printf("Conformance test passed: Encoded %u frames, Total %u bytes\n", total_frames, total_encoded_bytes);

    free(pcm_in);
    free(out_buf);
    micro_aac_encoder_destroy(encoder);
    return 0;
}
