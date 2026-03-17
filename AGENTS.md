# AGENTS.md

This file provides guidance to AI coding agents working in this repository.

## Project

C reference implementation of Compact Frame Format (CFF) — a binary framing protocol for delineating messages in byte streams, designed for microcontrollers. The entire library is two files: `src/cff.h` and `src/cff.c`.

## Build & Test Commands

```bash
# Run all tests (this is the default ceedling task)
ceedling test:all

# Run a single test file
ceedling test:cff_crc           # runs test/test_cff_crc.c
ceedling test:cff_parser        # runs test/test_cff_parser.c
ceedling test:cff_frame_builder
ceedling test:cff_integration

# Format code
rake format:all                 # apply clang-format
rake format:check               # dry-run check

# Build the usage example
cd example && mkdir -p build && cd build && cmake .. && cmake --build .
```

Prerequisites: Ruby 3.1+, Ceedling 1.0.1 (`gem install ceedling`), gcc, clang-format, CMake (for example only).

## Code Architecture

The library has four modules, all in `src/cff.c` with the public API in `src/cff.h`:

- **Ring Buffer** — Circular buffer for streaming byte ingestion. Supports append, consume, advance, peek, and preamble search with wrap-around.
- **CRC** — CRC-16/CCITT-FALSE (poly 0x1021, init 0xFFFF) with a lazily-initialized lookup table. Has variants for both linear buffers and ring buffers.
- **Frame Builder** — Constructs frames: writes preamble `[0xFA, 0xCE]`, auto-incrementing frame counter, payload size, header CRC, payload, and payload CRC.
- **Frame Parser** — `cff_parse_frame()` validates a single frame (preamble, header CRC, payload CRC). `cff_parse_frames()` iterates over a ring buffer invoking a callback per frame, recovering from frame loss by scanning for the next preamble.

All functions return `cff_error_en_t`. No dynamic allocation — callers provide buffers.

## Test Structure

Tests use the Unity framework via Ceedling. Test files are in `test/`, binary fixture data in `test/support/`. Compiler flags include `-Wall -Wextra -Werror`.

## Code Style

LLVM-based clang-format: 4-space indent, 120-char line limit. CI enforces formatting on `src/` files.
