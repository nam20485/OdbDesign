# linux-dynamic-release Rollout Plan

> Implementation plan for replacing the `linux-release` preset/binaries with the
> `linux-dynamic-release` preset/binaries as the main Linux build.
> Source request: [`docs/linux-dynamic-release-plan-prompt.md`](./linux-dynamic-release-plan-prompt.md)

## Objective

Replace every *active* reference to the `linux-release` CMake preset (builds,
binaries, Docker images, CI workflows, artifact packaging, docs, helper
scripts) with the `linux-dynamic-release` preset, and validate end-to-end.

## Background

`linux-release` links vcpkg dependencies (protobuf in particular) **statically**
into both `libOdbDesign.so` and the `OdbDesignServer` executable. At runtime two
independent copies of the protobuf descriptor pool are loaded, which leads to
dereferencing uninitialized pointers and crashes (release-only `SIGABRT` on
`GET /filemodels/<design>/matrix/matrix`).

`linux-dynamic-release` sets `VCPKG_TARGET_TRIPLET=x64-linux-dynamic`, so
protobuf/gRPC/libarchive/zlib/etc. are linked as **shared** libraries and a
single runtime copy is used process-wide. Both presets (configure/build/test)
already exist in `CMakePresets.json`.

## Decision: what "replace" means

| Item | Decision |
|---|---|
| `linux-release` presets in `CMakePresets.json` | **Kept** but unused. `linux-mingw-w64-release` inherits from the configure preset, so removal would require rework with no benefit. |
| Historical docs (`plan_docs/*`, `docs/grpc/*` dev plans, `docs/workflow-optimization-analysis.md`, `docs/crash issue.txt`) | **Untouched** — they are records of past work. |
| Disabled workflows (`.github/workflows/disabled/*`) | **Updated** for consistency if ever re-enabled. |
| `code-coverage.yml` (uses `linux-debug`) | **Out of scope** — it never used `linux-release`, and staying on the static debug build keeps the coverage baseline comparable. |
| k8s manifests / compose files | **No change needed** — they consume the published image, which is rebuilt from the updated Dockerfiles/workflows. |

## Options considered

### Option A — Ship vcpkg shared libraries alongside the binaries (chosen)

Copy `vcpkg_installed/x64-linux-dynamic/lib/*.so*` (plus `ossl-modules/`) next
to the server binary in both the Docker runtime image and the CI artifact zip.
The existing `LD_LIBRARY_PATH=/OdbDesign/bin` env in both Dockerfiles already
makes them discoverable.

- **Pros:** simple, deterministic, no host-specific tooling; works identically
  in the full `Dockerfile`, `Dockerfile.prebuilt`, and local scripts; OpenSSL 3
  providers resolve because `ossl-modules/` sits next to `libcrypto.so`.
- **Cons:** ships a few libraries that may not be strictly required; artifact
  zips grow by roughly tens of MB (also affects public release downloads).

### Option B — `ldd`-filtered copy

Run `ldd` on the built binaries in the build stage and copy only the resolved
dependencies.

- **Pros:** minimal set of shipped libraries.
- **Cons:** extra build-stage scripting, fragile against loader differences,
  harder to audit; saves little in practice.

### Option C — `vcpkg export` a runtime package

Use `vcpkg export` to produce a standalone runtime bundle.

- **Pros:** "official" vcpkg mechanism.
- **Cons:** `vcpkg export` is poorly supported for dynamic Linux triplets,
  adds a slow extra step, and duplicates what a glob copy already achieves.

**Recommendation: Option A.** Simplicity and robustness win; the size cost is
acceptable.

## Changes

### Build & packaging (substantive)

1. **`Dockerfile`**
   - Pre-install matches the preset triplet:
     `vcpkg install --triplet x64-linux-dynamic` (the old static pre-install
     would build packages the dynamic build never uses).
   - `cmake --preset linux-dynamic-release` /
     `cmake --build --preset linux-dynamic-release`.
   - Runtime stage: copy the project binaries from
     `out/build/linux-dynamic-release/...` and add
     `COPY --from=build .../vcpkg_installed/x64-linux-dynamic/lib/*.so* ./bin/`
     plus the `ossl-modules/` directory.
2. **`Dockerfile.prebuilt`**
   - `COPY ./artifacts/*.so ./bin/` → `COPY ./artifacts/*.so* ./bin/` so
     versioned libraries (`libarchive.so.13`, `libprotobuf.so.33.x`, ...) ship.
3. **`.github/workflows/cmake-multi-platform.yml`**
   - Matrix preset: `linux-release` → `linux-dynamic-release`.
   - Manual vcpkg install: Linux passes `--triplet x64-linux-dynamic`;
     macOS keeps its default triplet (split into two conditional steps).
   - Linux artifact step: also copy the vcpkg shared libs into the artifact
     dir; zip glob `./*.so` → `./*.so*`.
4. **`.github/workflows/vcpkg-cache-warm.yml`**
   - `warm-linux` warms **both** triplets: static (still consumed by
     `code-coverage.yml`'s `linux-debug`) and dynamic (new; must be pushed to
     the GitHub Packages NuGet feed so future builds hit the cache).
5. **Disabled workflows** — `disabled/test-runtime.yml`, `disabled/codeql.yml`:
   preset matrix entry updated to `linux-dynamic-release`.
6. **Scripts** — `scripts/compress-artifacts.sh` (paths + vcpkg libs + zip
   glob), `scripts/compress-artifacts.ps1` (stale header comments),
   `scripts/run-codeql-local.ps1` (Linux build command).

### Docs

- `AGENTS.md`, `docs/BUILD.md`, `docs/README.md`, `scripts/README.md` — Linux
  examples switched to `linux-dynamic-release`, with a note that Linux release
  binaries now depend on the bundled vcpkg shared libraries.

## Verification

1. **Local build**: `cmake --preset linux-dynamic-release` +
   `cmake --build --preset linux-dynamic-release`.
2. **Tests**: `ctest` against `out/build/linux-dynamic-release` with
   `ODB_TEST_DATA_DIR` / `ODB_TEST_ENVIRONMENT_VARIABLE` set.
3. **Runtime smoke (host)**: run `OdbDesignServer` from the dynamic build dir;
   `curl /healthz/live`, then the previously crashing
   `GET /filemodels/<design>/matrix/matrix` endpoint.
4. **Prebuilt image simulation**: assemble `./artifacts` exactly like the
   workflow does (project `.so`s + `OdbDesignServer` + vcpkg `*.so*`),
   `docker build -f Dockerfile.prebuilt`, run the container with a test design
   mounted, and curl healthz + the matrix endpoint.
5. **Workflow YAML** sanity-checked after edits.
6. **CI gate**: push the branch, open a PR against `nam20485`; the
   `CMake Build Multi-Platform` + `Code Coverage` PR checks are the final
   validation. (`docker-publish` only runs post-merge on the tracked branches.)

## Migration notes / known impacts

- **First CI run** builds the dynamic-triplet vcpkg packages from source once
  (~one-time cost); the `vcpkg-cache-warm` job then pushes them to the feed.
- **Artifact / release zip size** grows (vcpkg shared libs are included).
- **`compose.local.yml` (`USE_VCPKG_CACHE=1`)** builds will cache-miss (the
  read-only feed only holds static packages until the warm job pushes dynamic
  ones). Workaround: build once with `USE_VCPKG_CACHE=0`, or wait for the
  warm job.
- Branch: `linux-dynamic-release-rollout`, PR base: `nam20485`.
