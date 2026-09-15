# micro-aac

Wrapper library for the FAAC AAC audio encoder optimized for ESP32 and ESP-IDF with PSRAM support and configurable settings.

## Features

- **FAAC AAC Encoder**: Real-time AAC audio encoding for ESP32 and embedded targets.
- **Dual Support**: Formatted for both ESP-IDF components (`idf_component.yml`) and PlatformIO (`library.json`).
- **PSRAM Awareness**: Automatic memory placement preferences for ESP32 SPIRAM/PSRAM.
- **Flexible Stream Output**: Configurable ADTS stream format or raw AAC stream output with AudioSpecificConfig metadata.
- **C and C++ API**: Clean C function interfaces and modern C++ `micro_aac::AACEncoder` wrapper.

## Installation

### ESP-IDF

Add `micro-aac` to your project's `main/CMakeLists.txt` or `idf_component.yml`.

### PlatformIO

In `platformio.ini`:
```ini
lib_deps =
    https://github.com/esphome-libs/micro-aac.git
```

## Quick Start (C++)

```cpp
#include "micro_aac/aac_encoder.h"

micro_aac_config_t config;
micro_aac_config_init(&config);
config.sample_rate = 16000;
config.num_channels = 1; // mono
config.format = MICRO_AAC_FORMAT_ADTS;
config.object_type = MICRO_AAC_OBJECT_LOW; // Low Complexity (LC) AAC

micro_aac::AACEncoder encoder;
if (encoder.init(config) == MICRO_AAC_OK) {
    uint32_t frame_samples = encoder.get_frame_samples();
    uint32_t max_out = encoder.get_max_output_bytes();

    // Prepare buffers and encode PCM...
    uint32_t bytes_written = 0;
    encoder.encode(pcm_data, frame_samples, out_buf, max_out, &bytes_written);
}
```

## Quick Start (C)

```c
#include "micro_aac/aac_encoder.h"

micro_aac_config_t config;
micro_aac_config_init(&config);
config.sample_rate = 16000;
config.num_channels = 1;

micro_aac_encoder_t *encoder = NULL;
if (micro_aac_encoder_create(&config, &encoder) == MICRO_AAC_OK) {
    uint32_t frame_samples = micro_aac_encoder_get_frame_samples(encoder);
    uint32_t max_out = micro_aac_encoder_get_max_output_bytes(encoder);

    uint32_t bytes_written = 0;
    micro_aac_encoder_encode(encoder, pcm_data, frame_samples, out_buf, max_out, &bytes_written);

    micro_aac_encoder_destroy(encoder);
}
```

## License

This project wrapper is licensed under Apache-2.0.
The underlying FAAC encoder is licensed under LGPL-2.1-or-later.
