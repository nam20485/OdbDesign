# Add Linux FASTBuild generator presets + local tooling

## Context

`OdbDesign/CMakePresets.json` currently defines only Ninja-based Linux presets. The user wants additional presets using CMake's [FASTBuild generator](https://cmake.org/cmake/help/latest/generator/FASTBuild.html) (generates `fbuild.bff`; builds run through `fbuild` via the normal `cmake --build` flow).

Requirements confirmed by research:
- FASTBuild generator needs **CMake ≥ 4.2** and **FASTBuild (fbuild) v1.17+ on PATH**. Supported on Linux.
- This machine has **CMake 3.31.6 (apt)** and **no fbuild**, so tooling must be installed first (user approved).
- Prebuilt Linux binaries exist: `https://fastbuild.org/downloads/v1.20/FASTBuild-Linux-x64-v1.20.zip` (verified HTTP 200, ~1.3 MB). GitHub releases carry source only.
- pip has `cmake 4.4.3`; `pipx`/`pip install --user` places it in `~/.local/bin`.
- User chose: **mirror all 4 existing Linux variants** (debug, release, dynamic-debug, dynamic-release).

## Step 1 — Install tooling

1. FASTBuild v1.20:
   - `curl -Lo /tmp/FASTBuild-Linux-x64-v1.20.zip https://fastbuild.org/downloads/v1.20/FASTBuild-Linux-x64-v1.20.zip`
   - Unzip, copy `fbuild` (and `fbuildworker`) into `~/.local/bin/`, `chmod +x`.
   - Verify: `fbuild -version` → 1.20.
2. CMake ≥ 4.2:
   - `pipx install cmake` (fallback: `pip install --user cmake`) → `~/.local/bin/cmake` 4.4.3.
   - Verify `cmake --version` resolves to 4.4.3 (i.e. `~/.local/bin` precedes `/usr/bin` on PATH). Apt's 3.31.6 stays installed as fallback at `/usr/bin/cmake`.
   - Note: this changes the default `cmake` for local builds to 4.4.3 — fine for this project (`cmakeMinimumRequired` 3.21, vcpkg compatible).

## Step 2 — Edit `CMakePresets.json`

File: `/home/nam20485/src/github/nam20485/OdbDesign/CMakePresets.json`

### configurePresets (insert after `linux-dynamic-release`, line ~153)

- Hidden `linux-fastbuild-base`: `"inherits": "linux-base"`, `"generator": "FASTBuild"`. Everything else (binaryDir `out/build/${presetName}`, vcpkg toolchain, compilers, Linux condition) inherits from `linux-base` (lines 93–119).
- `linux-fastbuild-debug` — inherits `linux-fastbuild-base`, `CMAKE_BUILD_TYPE: Debug`.
- `linux-fastbuild-release` — inherits `linux-fastbuild-base`, `CMAKE_BUILD_TYPE: Release`.
- `linux-fastbuild-dynamic-debug` — inherits `linux-fastbuild-base`, `Debug` + `VCPKG_TARGET_TRIPLET: x64-linux-dynamic`.
- `linux-fastbuild-dynamic-release` — inherits `linux-fastbuild-base`, `Release` + `VCPKG_TARGET_TRIPLET: x64-linux-dynamic`.

Display names follow existing style ("Linux FASTBuild Debug", etc.).

### buildPresets (insert after `linux-dynamic-release`, line ~306)

Four matching presets, each `"inherits": "linux-base"` (the hidden Linux build base), `configurePreset` pointing at the same-named configure preset, `cleanFirst: false`, `configuration`, and a description — mirroring the existing linux-dynamic-* entries.

### testPresets (insert after `linux-dynamic-release`, line ~416)

Four matching presets, each `"inherits": "linux-base"` (hidden Linux test base) + `configurePreset`, mirroring existing entries.

No changes to `cmakeMinimumRequired`, `version`, Windows/macOS presets, or CI (new presets are opt-in; workflows name presets explicitly).

## Risks / known caveats

- Generator-override-via-inheritance (`linux-base` sets Ninja, child sets FASTBuild) is expected to work; if the CMake version in use rejects it, define `linux-fastbuild-base` standalone by duplicating `linux-base`'s fields with `generator: FASTBuild`.
- The FASTBuild generator documents limitations with directory-dependent custom commands (needs `SYMBOLIC`). This project uses protoc/gRPC codegen custom commands — a full build may surface incompatibilities. If so, report findings rather than hacking around them.

## Verification

1. `cmake --list-presets` shows the 4 new configure presets.
2. `cmake --preset linux-fastbuild-dynamic-debug` → configures, `out/build/linux-fastbuild-dynamic-debug/fbuild.bff` exists.
3. `cmake --build --preset linux-fastbuild-dynamic-debug` → build proceeds via fbuild (full success not guaranteed — see caveats; capture outcome).
4. `ctest --preset linux-fastbuild-dynamic-debug -R BasicAssertions` if the build completes.
5. Confirm no diff noise elsewhere in `CMakePresets.json` (JSON stays valid; `python3 -m json.tool` check).
