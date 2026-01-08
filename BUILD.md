# Building voip-utility

This document provides detailed instructions for building voip-utility from source.

## Quick Start

If you have all dependencies installed:

```bash
meson setup build
ninja -C build
```

## Dependencies Overview

| Dependency | Version | Required | Notes |
|------------|---------|----------|-------|
| GCC/Clang | 7+ | Yes | C11 support required |
| Meson | 0.50+ | Yes | Build system |
| Ninja | 1.8+ | Yes | Build backend |
| **PJSIP/PJSUA** | 2.x | Yes | Must be built from source (see below) |
| **OpenSSL** | **3.0+** | Yes | **Must match PJSIP build** |
| ALSA | 1.x | Yes | Audio device support |
| FFTW3 | 3.x | Yes | FFT for audio analysis (single precision) |
| cJSON | 1.x | Yes | JSON parsing |
| UUID | 2.x | Yes | Unique identifiers |
| libsrtp2 | 2.x | Yes | SRTP support |
| Opus | 1.x | Optional | Opus codec support |
| WebRTC Audio | 0.3+ | Optional | Echo cancellation |

## Critical: OpenSSL Version Requirement

**PJSIP must be compiled against the same OpenSSL version used during linking.**

### The Problem

If you see this error:
```
/usr/bin/ld: libpj-*.a(ssl_sock_ossl.o): undefined reference to `SSL_get1_peer_certificate'
```

This means:
- PJSIP static libraries were built against **OpenSSL 3.0+**
- Your system has **OpenSSL 1.1.x** (or earlier)

The function `SSL_get1_peer_certificate` was introduced in OpenSSL 3.0.0 (September 2021), replacing the deprecated `SSL_get_peer_certificate`.

### Solutions

**Option 1: Upgrade OpenSSL to 3.0+** (Recommended)

On Ubuntu 22.04+ / Debian 12+:
```bash
sudo apt install libssl-dev
openssl version  # Should show 3.0.x or higher
```

On older systems, you may need to build OpenSSL 3.x from source or use a PPA.

**Option 2: Rebuild PJSIP against your OpenSSL version**

If you must use OpenSSL 1.1.x, rebuild PJSIP from source against your system's OpenSSL (see PJSIP Build Instructions below).

## System Package Installation

### Ubuntu 22.04+ / Debian 12+ (OpenSSL 3.0)

```bash
# Build tools
sudo apt install build-essential meson ninja-build pkg-config

# Required dependencies
sudo apt install libssl-dev libfftw3-dev libcjson-dev libasound2-dev \
                 uuid-dev libsrtp2-dev

# Optional dependencies
sudo apt install libopus-dev libwebrtc-audio-processing-dev
```

### Ubuntu 20.04 / Debian 11 (OpenSSL 1.1)

These systems ship with OpenSSL 1.1.x. You have two options:

**Option A: Install OpenSSL 3.0 from source**
```bash
# Install OpenSSL 3.0 to /usr/local
wget https://www.openssl.org/source/openssl-3.0.13.tar.gz
tar xf openssl-3.0.13.tar.gz
cd openssl-3.0.13
./config --prefix=/usr/local --openssldir=/usr/local/ssl
make -j$(nproc)
sudo make install
sudo ldconfig

# Verify
/usr/local/bin/openssl version
```

**Option B: Use system OpenSSL and rebuild PJSIP**

See PJSIP Build Instructions below.

### Fedora 36+ / RHEL 9+

```bash
sudo dnf install gcc meson ninja-build pkg-config
sudo dnf install openssl-devel fftw3-devel cjson-devel alsa-lib-devel \
                 libuuid-devel libsrtp-devel
sudo dnf install opus-devel webrtc-audio-processing-devel  # Optional
```

### Arch Linux

```bash
sudo pacman -S base-devel meson ninja
sudo pacman -S openssl fftw cjson alsa-lib util-linux-libs libsrtp
sudo pacman -S opus webrtc-audio-processing  # Optional
```

## PJSIP Build Instructions

PJSIP must be built from source as most distributions don't package it.

### Download PJSIP

```bash
git clone https://github.com/pjsip/pjproject.git
cd pjproject
git checkout 2.15  # or latest stable tag
```

### Configure PJSIP

Create a custom configuration file:

```bash
cat > pjlib/include/pj/config_site.h << 'EOF'
/* Custom PJSIP configuration */
#define PJMEDIA_HAS_VIDEO 0
#define PJ_HAS_IPV6 1
#define PJMEDIA_AUDIO_DEV_HAS_ALSA 1
#define PJMEDIA_HAS_SRTP 1

/* Codec configuration */
#define PJMEDIA_HAS_OPUS_CODEC 1
#define PJMEDIA_HAS_G711_CODEC 1
#define PJMEDIA_HAS_G722_CODEC 1
#define PJMEDIA_HAS_GSM_CODEC 1
#define PJMEDIA_HAS_SPEEX_CODEC 1
#define PJMEDIA_HAS_ILBC_CODEC 1
EOF
```

### Build PJSIP

```bash
# Configure (adjust paths if using custom OpenSSL)
./configure --prefix=/usr/local \
    --enable-shared=no \
    --with-ssl=/usr \
    --with-opus \
    --disable-video

# If using OpenSSL 3.0 installed to /usr/local:
# ./configure --prefix=/usr/local \
#     --enable-shared=no \
#     --with-ssl=/usr/local \
#     --with-opus \
#     --disable-video

# Build
make dep
make -j$(nproc)

# Install
sudo make install
```

### Verify PJSIP Installation

```bash
# Check libraries exist
ls /usr/local/lib/libpj*.a

# Check headers
ls /usr/local/include/pjsua-lib/pjsua.h

# Verify OpenSSL linkage (should show your OpenSSL version)
strings /usr/local/lib/libpj-*.a | grep -i openssl | head -5
```

## Building voip-utility

### Standard Build

```bash
cd voip-utility
meson setup build
ninja -C build
```

### Build with Custom PJSIP Location

If PJSIP is installed to a non-standard location:

```bash
# Edit meson.build line 39 to change pjsip_libdir
# Or create a cross-file:

cat > custom-paths.ini << EOF
[built-in options]

[properties]
pjsip_libdir = '/path/to/pjsip/lib'
EOF

meson setup build --native-file=custom-paths.ini
ninja -C build
```

### Build Options

```bash
# Debug build
meson setup build -Dbuildtype=debug
ninja -C build

# Release build with optimizations
meson setup build -Dbuildtype=release
ninja -C build

# With tests
meson setup build -Dtests=true
ninja -C build
```

### Verify Build

```bash
# Run the binary
./build/voip-utility --help

# Check linked libraries
ldd ./build/voip-utility
```

## Troubleshooting

### Error: `undefined reference to 'SSL_get1_peer_certificate'`

**Cause:** PJSIP was built against OpenSSL 3.0+ but system has OpenSSL 1.1.x.

**Fix:** Either:
1. Upgrade to OpenSSL 3.0+ and reinstall libssl-dev
2. Rebuild PJSIP against your system's OpenSSL version

Check your OpenSSL version:
```bash
openssl version
pkg-config --modversion openssl
```

### Error: `cannot find -lpjsua-x86_64-pc-linux-gnu`

**Cause:** PJSIP not installed or architecture suffix mismatch.

**Fix:**
1. Install PJSIP to /usr/local (see above)
2. Check the actual library names and update meson.build arch_suffix

```bash
# Find actual architecture suffix
ls /usr/local/lib/libpjsua*.a
# Example output: libpjsua-arm-unknown-linux-gnueabihf.a
# Update meson.build line 40: arch_suffix = '-arm-unknown-linux-gnueabihf'
```

### Error: `fftw3f not found`

**Cause:** FFTW single-precision library not installed.

**Fix:**
```bash
# Debian/Ubuntu
sudo apt install libfftw3-dev

# Fedora
sudo dnf install fftw3-devel
```

### Error: `alsa not found`

**Cause:** ALSA development headers not installed.

**Fix:**
```bash
# Debian/Ubuntu
sudo apt install libasound2-dev

# Fedora
sudo dnf install alsa-lib-devel
```

### Error: `libsrtp2 not found`

**Cause:** SRTP library not installed.

**Fix:**
```bash
# Debian/Ubuntu
sudo apt install libsrtp2-dev

# Fedora
sudo dnf install libsrtp-devel
```

## Docker Build Environment

For reproducible builds, use Docker:

```dockerfile
FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    build-essential meson ninja-build pkg-config git \
    libssl-dev libfftw3-dev libcjson-dev libasound2-dev \
    uuid-dev libsrtp2-dev libopus-dev

# Build PJSIP
RUN git clone --depth 1 --branch 2.15 https://github.com/pjsip/pjproject.git /pjproject
WORKDIR /pjproject
RUN ./configure --prefix=/usr/local --enable-shared=no --with-ssl=/usr --disable-video && \
    make dep && make -j$(nproc) && make install

# Build voip-utility
WORKDIR /app
COPY . .
RUN meson setup build && ninja -C build
```

Build with:
```bash
docker build -t voip-utility-builder .
docker run --rm -v $(pwd)/build:/app/build voip-utility-builder
```

## Version Compatibility Matrix

| voip-utility | PJSIP | OpenSSL | Ubuntu | Debian | Fedora |
|-------------|-------|---------|--------|--------|--------|
| 0.1.x | 2.14-2.15 | 3.0+ | 22.04+ | 12+ | 36+ |
| 0.1.x | 2.14-2.15 | 1.1.x | 20.04* | 11* | 35* |

*Requires rebuilding PJSIP against system OpenSSL.

## Cross-Compilation

For embedded systems (e.g., Yocto), use the Yocto recipe or create a meson cross-file. Contact maintainers for cross-compilation support.
