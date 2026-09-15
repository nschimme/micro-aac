#include "micro_aac/aac_encoder.h"
#include <stdio.h>
#include <math.h>

#if defined(ESP_PLATFORM)
#include "esp_system.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#endif

extern "C" void app_main(void) {
    printf("--- AAC Encoder Benchmark ---\n");

    micro_aac_config_t config;
    micro_aac_config_init(&config);
    config.sample_rate = 16000;
    config.num_channels = 1;
    config.format = MICRO_AAC_FORMAT_ADTS;
    config.object_type = MICRO_AAC_OBJECT_LOW;

    micro_aac::AACEncoder encoder;
    micro_aac_status_t status = encoder.init(config);
    if (status != MICRO_AAC_OK) {
        printf("Failed to initialize AAC encoder: %d\n", status);
        return;
    }

    uint32_t frame_samples = encoder.get_frame_samples();
    uint32_t max_out = encoder.get_max_output_bytes();
    printf("Encoder ready. Frame samples: %u, Max output bytes: %u\n", frame_samples, max_out);

    int16_t *pcm_buf = (int16_t *)malloc(frame_samples * sizeof(int16_t));
    uint8_t *out_buf = (uint8_t *)malloc(max_out);

    if (!pcm_buf || !out_buf) {
        printf("Failed to allocate buffers!\n");
        return;
    }

    for (uint32_t i = 0; i < frame_samples; i++) {
        pcm_buf[i] = (int16_t)(16000.0 * sin(2.0 * 3.1415926535 * 440.0 * (double)i / config.sample_rate));
    }

    uint32_t bytes_written = 0;
    status = encoder.encode(pcm_buf, frame_samples, out_buf, max_out, &bytes_written);
    if (status == MICRO_AAC_OK) {
        printf("Encoded 1 frame successfully: %u bytes written\n", bytes_written);
    } else {
        printf("Encoding failed: %d\n", status);
    }

    free(pcm_buf);
    free(out_buf);
    printf("Benchmark complete.\n");
}
