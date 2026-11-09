# micro-aac - Embedded AAC Wrapper

Embedded-focused wrapper for the FAAC AAC audio encoder and FAAD AAC audio decoder. Designed as an ESP-IDF component and PlatformIO library with PSRAM support for ESP32 and ESPHome.

## Features

- **AAC Encoder & Decoder**: Complete AAC encoding (FAAC) and decoding (FAAD3).
- **Dual Support**: Formatted for both ESP-IDF components (`idf_component.yml`) and PlatformIO (`library.json`).
- **PSRAM Awareness**: Automatic memory placement preferences for ESP32 SPIRAM/PSRAM using custom allocators.
- **Flexible Stream Format**: Configurable ADTS stream format or raw AAC stream output with AudioSpecificConfig metadata.
- **C and C++ API**: Clean C function interfaces and modern C++ `micro_aac::AACEncoder` / `micro_aac::AACDecoder` wrappers.
- **No Patent Risk**: Parametric Stereo (PS) encoding is disabled by default.

## Supported Targets

| Target | Real-Time Decode | Real-Time Encode | Notes |
|---|---|---|---|
| ESP32-S3 (240 MHz) | AAC-LC, HE-AAC v1, HE-AAC v2 | AAC-LC, HE-AAC v1 | Full FPU acceleration; PSRAM recommended for high workloads |
| Classic ESP32 (240 MHz) | AAC-LC, HE-AAC v1 | AAC-LC, HE-AAC v1 | FPU acceleration; limited internal RAM without PSRAM |
| ESP32-C6 (160 MHz) | No FPU | Voice only | Lacks hardware FPU; runs below real time on stereo |

See [Freeware Advanced Audio ESP32 Guide](https://freewareadvancedaudio.github.io/docs/esp32-aac) for extensive benchmarks and hardware guidance.

## Configuration

In ESP-IDF:
```bash
idf.py menuconfig
# Navigate to: Component config → micro-aac Configuration
```

Available Kconfig options:
- `MICRO_AAC_MAX_CHANNELS`: Maximum audio channels (default: 2, range 1-8).
- `MICRO_AAC_SBR_DECIMATION`: SBR spectral analysis decimation density (default: 1).
- `MICRO_AAC_USE_SPIRAM`: Prefer SPIRAM (PSRAM) for codec state and buffers (default: `y` when SPIRAM enabled).

## Installation

### ESP-IDF

Add `micro-aac` to your project's `main/CMakeLists.txt` or `idf_component.yml`.

### PlatformIO

In `platformio.ini`:
```ini
lib_deps =
    https://github.com/esphome-libs/micro-aac.git
```

## Usage Example

### Encoder Usage (C++)

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

### Decoder Usage (C++)

```cpp
#include "micro_aac/aac_decoder.h"

micro_aac_dec_config_t dec_config;
micro_aac_dec_config_init(&dec_config);
dec_config.stream_format = MICRO_AAC_DEC_STREAM_ADTS;

micro_aac::AACDecoder decoder;
if (decoder.init(dec_config) == MICRO_AAC_DEC_OK) {
    uint8_t *in_ptr = aac_data;
    int32_t bytes_left = aac_data_len;
    uint32_t samps_decoded = 0;

    // Output buffer should hold at least decoder.get_max_output_bytes()
    decoder.decode(&in_ptr, &bytes_left, pcm_out_buffer, &samps_decoded);
}
```

### Streaming ADTS playback

Network sources deliver ADTS in arbitrary pieces, so a partial frame at the end of the buffer is normal.
Keep a small buffer, top it up from the source, and feed it to the decoder:

```cpp
// buf/have: bytes received so far; `more` is true until the source is exhausted.
int32_t off = micro_aac_decoder_find_syncword(buf, have);   // skip junk, resync after loss
if (off > 0) { memmove(buf, buf + off, have - off); have -= off; }

uint8_t *p = buf;
int32_t left = have;
uint32_t samps = 0;
auto st = decoder.decode_ex(&p, &left, pcm, pcm_capacity_bytes, &samps);
if (st == MICRO_AAC_DEC_OK) {                 // consume what was used, play `samps` samples
    memmove(buf, p, left); have = left;
} else if (st == MICRO_AAC_DEC_ERR_UNDERFLOW && more) {
    // incomplete frame: read more bytes and call again
} else {                                      // corrupt frame: drop one byte and resync
    memmove(buf, buf + 1, --have);
}
```

`tests/test_adts_streaming.c` exercises this loop with chunk sizes from 1 to 4096 bytes, junk before the stream and a lost frame.
Home Assistant's ffmpeg proxy produces ADTS (`ffmpeg -f adts`), which this path decodes directly.

### Encoder

The encoder stays available for future use, such as reducing over-the-air data on the device (compare the Opus encoder in micro-opus).
It is compiled with the component but the linker drops it from firmware that never references it (a decode-only ESP32-S3 image built with ESP-IDF 5.5.1 contains no encoder symbols).
That decode-only image is roughly 62 KB of flash code and 15 KB of flash data larger than an empty app.

## Memory Usage

Codec state is allocated when the instance is created.
- **AAC-LC Decode (FAAD)**: ~49 KB heap
- **HE-AAC v1 Decode (FAAD)**: ~98 KB heap
- **AAC-LC Encode (FAAC)**: ~49 KB mono, ~90 KB stereo
- **HE-AAC v1 Encode (FAAC)**: ~83 KB mono, ~142 KB stereo

When `CONFIG_MICRO_AAC_USE_SPIRAM` is enabled, heap allocations automatically prefer PSRAM (`MALLOC_CAP_SPIRAM`), preserving internal SRAM.

## Testing

A host-based CTest suite lives in `tests/`:
```bash
mkdir -p build && cd build
cmake ..
make
ctest --output-on-failure
```

The ADTS fixtures in `tests/data/` are generated by `scripts/gen_adts_fixtures.sh` (needs `ffmpeg` and the FAAC frontend).

## License

- micro-aac wrapper code: [Apache License 2.0](LICENSE)
- FAAC encoder & FAAD decoder (`lib/faac/`): [LGPL-2.1-or-later](https://github.com/FreewareAdvancedAudio/faac/blob/master/COPYING)
