# micro-aac

Wrapper library for FAAC AAC audio encoder and Helix AAC audio decoder optimized for ESP32, ESP-IDF, PlatformIO, and ESPHome.

## Features

- **AAC Encoder & Decoder**: Complete AAC encoding (FAAC) and fixed-point decoding (Helix AAC).
- **Dual Support**: Formatted for both ESP-IDF components (`idf_component.yml`) and PlatformIO (`library.json`).
- **PSRAM Awareness**: Automatic memory placement preferences for ESP32 SPIRAM/PSRAM.
- **Flexible Stream Output**: Configurable ADTS stream format or raw AAC stream output with AudioSpecificConfig metadata.
- **C and C++ API**: Clean C function interfaces and modern C++ `micro_aac::AACEncoder` / `micro_aac::AACDecoder` wrappers.

## Installation

### ESP-IDF

Add `micro-aac` to your project's `main/CMakeLists.txt` or `idf_component.yml`.

### PlatformIO

In `platformio.ini`:
```ini
lib_deps =
    https://github.com/esphome-libs/micro-aac.git
```

## Encoder Usage (C++)

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

## Decoder Usage (C++)

```cpp
#include "micro_aac/aac_decoder.h"

micro_aac_dec_config_t dec_config;
micro_aac_dec_config_init(&dec_config);

micro_aac::AACDecoder decoder;
if (decoder.init(dec_config) == MICRO_AAC_DEC_OK) {
    uint8_t *in_ptr = aac_data;
    int32_t bytes_left = aac_data_len;
    uint32_t samps_decoded = 0;

    decoder.decode(&in_ptr, &bytes_left, pcm_out_buffer, &samps_decoded);
}
```

## License

This project wrapper is licensed under Apache-2.0.
The FAAC encoder component is licensed under LGPL-2.1-or-later.
The Helix AAC decoder component is licensed under RPSL-1.0 / RCSL.
