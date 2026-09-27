# Sanitizer builds — finding memory errors without guessing

Three presets exist for this: `linux-asan` (configure), `linux-asan` (build), `linux-asan` (test).
They are for diagnosing, not for shipping — sanitizer builds are several times slower and are
not part of CI.

## When to reach for it

Use it when a test or the server does something that cannot happen under defined behaviour:
a process `abort()` with no diagnostic, a container that reports an entry present when iterated
but absent when looked up, results that differ between two runs of the same binary with nothing
changed in between. Those are the signatures of uninitialized memory, use-after-free, or
out-of-bounds writes. Reading code will not find them; a sanitizer will name the exact line.

This is the tool for the class of bug behind `fix(filemodel): initialize POD record members
before protobuf serialization` (#597), where an uninitialized `bool` reached the wire as a
non-0/1 byte and desynced a whole protobuf stream.

## How to run it

```bash
export VCPKG_ROOT=/home/nam20485/src/github/microsoft/vcpkg   # not set in every shell
export PATH="$HOME/.local/bin:$PATH"

# One-time: the preset has its own build dir and needs the vcpkg tree. In an agent
# session the vcpkg registry fetch is blocked, so reuse an existing install instead:
mkdir -p out/build/linux-asan
ln -sfn ../linux-dynamic-release/vcpkg_installed out/build/linux-asan/vcpkg_installed

VCPKG_MANIFEST_INSTALL=OFF cmake --preset linux-asan
VCPKG_MANIFEST_INSTALL=OFF cmake --build --preset linux-asan --target OdbDesignTests

# Run one test, or loop a flaky one until it reproduces:
ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=print_stacktrace=1 \
  out/build/linux-asan/OdbDesignTests/OdbDesignTests --gtest_filter='StepHdrFileTests.*'
```

`VCPKG_MANIFEST_INSTALL=OFF` is required in agent sessions because the shell guard blocks
vcpkg's internal `git fetch` into `~/.cache/vcpkg/registries/git`. The symlink above is what
makes that viable — without a `vcpkg_installed` tree, configure cannot find protobuf/gRPC.

## Reading the output

- `ERROR: AddressSanitizer: heap-use-after-free` / `heap-buffer-overflow` /
  `stack-use-after-return` — the first stack frame **in our code** is the bug. Frames below it
  in protobuf, gRPC or the STL are usually the victim, not the cause.
- `runtime error: ...` with `print_stacktrace=1` — undefined behaviour UBSan caught: signed
  overflow, null dereference, misaligned access, **loading an invalid value from a bool/enum**.
  That last one is exactly the #597 failure mode and is the most likely thing to show up here.
- `-fno-sanitize-recover=all` is set, so UBSan aborts on the first error instead of continuing.
  That is deliberate: a corrupted run produces misleading later failures.

`detect_leaks=0` is set above because protobuf and gRPC keep intentional long-lived
allocations; leak reports drown the signal. Drop it when you are specifically hunting leaks.

## Caveats

- The preset inherits `linux-dynamic-debug`, so it uses the `x64-linux-dynamic` triplet and one
  shared protobuf runtime. Do not switch it to a static triplet: two protobuf copies produce
  their own `SIGABRT` on descriptor-pool collision, which looks like a sanitizer finding and
  is not one.
- vcpkg's dependencies are **not** built with sanitizers. Errors originating inside them can be
  false positives; anything pointing at our own files is real.
- Expect roughly an order of magnitude slowdown and 2–3× memory. Run one test, not the suite.
