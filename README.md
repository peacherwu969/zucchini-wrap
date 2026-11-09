# Standalone AOSP Zucchini for GCC/CMake

This overlay builds the AOSP `external/zucchini` source as:

- `libzucchini.a`
- `libzucchini.so`
- a small `zucchini` command-line program

It does **not** build Android, Soong, `libchrome`, `libbase`, `liblog`, or
`libcutils`.

The intended source is AOSP 17's `external/zucchini`. The build keeps DEX and
ELF support, and disables Windows PE and ZTF support.

## Why this cut is smaller

The AOSP project has three conceptual layers:

1. Buffer-level patch generation/application.
2. Chromium memory-mapped file wrappers.
3. Chromium command-line/logging infrastructure.

This overlay keeps layer 1, replaces layers 2 and 3 with a small standard C++
file wrapper, and supplies a narrow compatibility layer for the Chromium
`base` utilities still used by the algorithms.

The following AOSP files are deliberately not compiled:

- `mapped_file.cc`
- `zucchini_integration.cc`
- `zucchini_tools.cc`
- `main_utils.cc`
- `zucchini_commands.cc`
- `zucchini_main_aosp.cc`
- Windows and ZTF disassembler sources

## CentOS Stream 9 build

Install the ordinary host compiler and build tools:

```bash
sudo dnf install -y gcc gcc-c++ cmake make
```

Assume your existing AOSP checkout is `/data02/aosp`:

```bash
cd zucchini-standalone-overlay

./build-centos9.sh \
    /data02/aosp/external/zucchini \
    build
```

The equivalent explicit commands are:

```bash
cmake -S . -B build -G "Unix Makefiles" \
    -DCMAKE_BUILD_TYPE=Release \
    -DZUCCHINI_SOURCE_DIR=/data02/aosp/external/zucchini

cmake --build build --parallel "$(nproc)"
```

For a machine-specific build, add:

```bash
-DZUCCHINI_NATIVE_OPTIMIZE=ON
```

DEX is enabled by default. To make a DEX-only build:

```bash
cmake -S . -B build-dex -G "Unix Makefiles" \
    -DCMAKE_BUILD_TYPE=Release \
    -DZUCCHINI_SOURCE_DIR=/data02/aosp/external/zucchini \
    -DZUCCHINI_ENABLE_DEX=ON \
    -DZUCCHINI_ENABLE_ELF=OFF
```

## Command-line use

Generate an executable-aware patch:

```bash
build/zucchini gen old.dex new.dex update.zuc
```

Apply it:

```bash
build/zucchini apply old.dex update.zuc rebuilt.dex
```

Verify application without keeping another output file:

```bash
build/zucchini verify old.dex update.zuc new.dex
```

Generate a raw/no-disassembly Zucchini patch:

```bash
build/zucchini raw old.bin new.bin update.zuc
```

The command also accepts `-gen`, `-apply`, `-verify`, and `-raw`.

## Library use: simple C API

```c
#include <zucchini_standalone.h>

int rc = zucchini_generate_file("old.dex", "new.dex", "update.zuc");
if (rc != ZUCCHINI_STATUS_SUCCESS) {
    fprintf(stderr, "%s: %s\n",
            zucchini_status_string(rc),
            zucchini_last_error());
}
```

Link against either library:

```bash
g++ app.cc -I/path/to/install/include \
    -L/path/to/install/lib64 -lzucchini
```

Install into a private prefix:

```bash
cmake --install build --prefix "$HOME/.local/zucchini"
```

## Library use: original buffer API

The installed library also exports the original AOSP C++ interface:

```cpp
#include "components/zucchini/zucchini.h"

zucchini::EnsemblePatchWriter writer(old_view, new_view);
auto status = zucchini::GenerateBuffer(old_view, new_view, &writer);
```

For integration into your `imgabdiff`, this buffer API is preferable because
you already own the input buffers and avoid reading each DEX a second time.

## Memory behavior

The small standalone CLI reads `old`, `new`, and the patch into vectors. This
is deliberately simple. For normal APK DEX entries and native libraries, a
32 GB machine is ample. Your in-process caller should use the original buffer
API directly so the wrapper makes no extra input copies.

The patch format stores image sizes as 32-bit values, so this wrapper rejects
individual files larger than 4 GiB instead of allowing an internal checked cast
to abort.

## Patch compatibility

Keep patch generation and application on the same AOSP Zucchini revision.
AOSP's format line is version 1, while newer Chromium has made incompatible
format changes. Do not generate with AOSP 17 and apply with a current Chromium
or Mozilla binary, or the reverse.

## If the first compile reports another `base/...` include

AOSP branches occasionally differ by a small header dependency. List the
headers used by your exact checkout:

```bash
./scan-base-deps.sh /data02/aosp/external/zucchini
```

Compare that output with `compat/base`. Usually the adjustment is a tiny shim,
not a reason to bring in all of Chromium `base`.

## Validation status

The compatibility headers and their callback, checked-arithmetic, logging, and
erase helpers were syntax-tested with GCC in isolation. The complete link must
be validated against your exact AOSP checkout, which is not present in this
environment. The source list and public API are taken from the AOSP 17 project
layout rather than from current Chromium.
