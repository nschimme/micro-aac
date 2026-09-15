# micro-aac Development Guide

ESP-IDF component wrapping the FAAC AAC audio encoder with PSRAM support for ESP32 and ESPHome.

## Project Structure

```text
lib/faac/             # Upstream FAAC encoder (git submodule)
cmake/                # CMake build helpers and configuration template
include/micro_aac/    # Public API headers
src/                  # micro-aac C/C++ wrapper implementation
examples/             # ESP-IDF / PlatformIO benchmark examples
tests/                # Host CTest suite
```

## Build & Test Commands

### Host Build & CTest

```bash
mkdir -p build && cd build
cmake ..
make
ctest --output-on-failure
```

### ESP-IDF Benchmark Example

```bash
cd examples/encode_benchmark
idf.py set-target esp32s3
idf.py build
```
