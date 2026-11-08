# Commercialization Readiness — Execution Plan

*Engineering execution plan for [OdbDesign_Commercialization_Readiness_Guide.md](OdbDesign_Commercialization_Readiness_Guide.md) Steps 3–4 — public repo prep + private commercial overlay. Approved 2026-11-07.*

## Scope decisions (user, 2026-11-07)

- **Scope:** repo readiness (guide Step 3) **+** private commercial overlay (guide Step 4). Business collateral (Keiron reply, pricing one-pager) deliberately out of scope this round.
- **SDK distribution:** **private delivery only** — SDK archives are never public release assets; they are delivered per-customer after NDA/evaluation agreement.
- **Design consequence:** the public repo stays 100% free of license-enforcement code (an AGPL user could strip it anyway). All enforcement lives in the private overlay repo via its own server executable and watermarked builds.

## Verified repo state (audit 2026-11-06/07, HEAD `59ed1cd`)

- Zero `install()` rules anywhere; only dangling `$<INSTALL_INTERFACE:include>` placeholders (`OdbDesignLib/CMakeLists.txt:35`, `Utils/CMakeLists.txt:11`).
- Release archives are binaries-only (no headers, no Windows `.lib` import libs) — not compilable-against as an SDK.
- `OdbDesignLib/OdbDesign.h` is an umbrella header; `App/OdbAppBase.h` → `RequestAuthenticationBase.h` → `Utils/crow_win.h` pulls Crow into every customer compile. No parser-only entry point exists.
- ~30 public headers transitively require the 26 generated `.pb.h` files (they land in `CMAKE_CURRENT_BINARY_DIR`; no `PROTOC_OUT_DIR` set).
- `Utils` is a separate SHARED library publicly linked by `OdbDesign`.
- `BasicRequestAuthentication.cpp` is Basic-Auth-only (`odb:plusplus` defaults). No license/watermark/expiry mechanism in code.
- No `COMMERCIAL-LICENSE.md`, `THIRD_PARTY_NOTICES`, CLA/DCO. SPDX SBOM workflow submits to GitHub's dependency API only — nothing attached to deliveries.
- Fixture governance already sound: public `nam20485/OdbDesignTestData` holds only `designodb_rigidflex*` + IPC-2581 files; proprietary designs (200-40628, 350-41017, Panel-g7162, Turbot) are local-only. `sample_design.tgz` is local-only (`designs/`, gitignored).
- 100% copyright = `@nam20485` (`.github/CODEOWNERS`) — proprietary overlay + commercial relicensing of the same code is legally clean; CLA locks the chain of title before any external PR.
- `OdbDesignServer/main.cpp` is a 12-line thin shell over `OdbDesignServerApp`; server glue (OdbServerAppBase, OdbDesignServerApp, Controllers, Services) compiles into the executable target only.
- `OdbServerAppBase.cpp:129` hardcodes gRPC bind to `"0.0.0.0:"` regardless of `args().bindAddress()` (REST honors it at line 207).

## Phase 0 — Plan docs

This file + the readiness guide, committed to `nam20485` (direct push; require-PR rule removed there). `ctest_configuration.txt` left untracked (unwired CDash boilerplate — user to decide).

## Phase 1 — Public repo readiness

One branch `nam/commercial-readiness` cut from `nam20485`, one PR, **merge commit**.

1. **Server library split** — `OdbDesignServer/CMakeLists.txt`: new `OdbDesignServerLib` STATIC library (all sources except `main.cpp`); `OdbDesignServer` executable becomes the thin main linking it. Behavior unchanged; preserves `file(COPY config.json)` / templates handling. Unblocks the private overlay (links `OdbDesignServerLib`) and improves testability.
2. **gRPC bind fix** — `OdbServerAppBase.cpp:129`: use `args().bindAddress()` instead of hardcoded `"0.0.0.0"` (default unchanged; eval builds can then be loopback-locked on both REST and gRPC).
3. **CMake `install()` rules** — `install(TARGETS ...)` for `OdbDesign`, `Utils`, `OdbDesignServer`(+`Lib`) with RUNTIME/LIBRARY/ARCHIVE destinations; install public headers preserving structure (`FileModel/`, `ProductModel/`, `App/`, root public headers, Utils public headers) + the 26 generated `.pb.h` from the binary dir; Crow headers (BSD-3) ship in `thirdparty/crow/` so the SDK header surface is self-contained. Optional if cheap: minimal `OdbDesignConfig.cmake` + `installPresets`.
4. **Parser-only entry point** — new `OdbDesignLib/OdbDesignParser.h` umbrella (FileModel + ProductModel + parse_error + invalid_odb_error; zero `App/`, zero Crow) + GoogleTest smoke test that includes **only** that header, loads `sample_design.tgz`, builds ProductModel (skips gracefully without `ODB_TEST_DATA_DIR`).
5. **Compliance files**
   - `scripts/generate-third-party-notices.sh` walking `vcpkg_installed/<triplet>/share/*/copyright` → `THIRD_PARTY_NOTICES.txt` (commit generated output for the 5 pinned direct deps; private packager regenerates full transitive set at build time).
   - `COMMERCIAL-LICENSE.md` at root — **price-free** dual-license overview (pricing is a live business decision): AGPL-3.0 default, commercial for closed-source, 4-step onboarding (scoping → NDA/eval → order schedule → delivery), support-tier summary, contact. Link from `docs/README.md` + `docs/one_pager.md`.
   - `.github/CLA.md` (Apache-style ICLA + CCLA with explicit commercial relicensing grant) + `docs/CONTRIBUTING.md` update. CLA Assistant enablement = user browser step (documented).
6. **Eval sample + hygiene** — publish `sample_design.tgz` (public-domain) from local `designs/` to `nam20485/OdbDesignTestData`; fix `OdbDesignApp/OdbDesignApp.cpp:46,50` hardcoded OneDrive path (arg/env lookup); optional: `sbom-generate-submit.yml` uploads SPDX JSON as workflow artifact.

**Verification:** linux-dynamic-release + linux-debug presets build green; `ctest` presets green; `cmake --install` layout check (`include/ lib/ bin/`); out-of-tree consumer smoke (20-line `g++` main, only `OdbDesignParser.h`, no Crow flags); Windows leg via CI `x64-release`.

## Phase 2 — Private commercial overlay (new private repo `nam20485/OdbDesign-Commercial`)

A branch in the public repo is not private (all refs fetchable) — separate repo required. Superbuilds upstream via git submodule pinned to `nam20485`; reuses vcpkg baseline/overrides + GH Packages vcpkg binary cache.

1. **`LicenseGuard/` static lib** (C++17, OpenSSL EVP already in the vcpkg tree) — Ed25519 verify of canonical-JSON `license.json` (sorted keys, no whitespace): `license_id, customer, product, edition, issued_at, expires_at, loopback_only, features[]`; embedded public key; checks signature → expiry → edition/features. GoogleTest: valid / expired / tampered / wrong key / feature gate.
2. **`OdbDesignCommercialServer`** (overlay-owned main) — links public `OdbDesignServerLib` + `LicenseGuard`. Startup: verify license (`ODBDESIGN_LICENSE_FILE` env) → exit non-zero **before binding** on failure; `loopback_only` forces/validates `127.0.0.1` for REST + gRPC (relies on Phase-1 bind fix) → construct `OdbDesignServerApp` → `Run()`.
3. **Watermark layer** (OEM production DLL — no runtime check, guide Option 1) — generated `Provenance.cpp` embedding customer_id / license_id / build timestamp; private CMake injects `-DODBDESIGN_CUSTOMER_ID=...` etc.; symbol strip (MSVC no-debug release; Linux `strip --strip-unneeded`, private symbols copy retained).
4. **Tooling + packaging**
   - `tools/make-license.(sh|ps1)` — Ed25519 signer (openssl CLI; private key in password manager / repo secret, never committed) → `license.json` with base64 signature.
   - `.github/workflows/package-commercial.yml` — manual dispatch (inputs: customer, edition, expiry); matrix `x64-release` + `linux-dynamic-release`; `cmake --install` → per-customer zip (include/ lib/ bin/, DLLs + `.lib` + server + vcpkg runtime DLLs, regenerated THIRD_PARTY_NOTICES, SPDX SBOM, license.json, sample_design.tgz, SDK README) → sha256 + GPG-sign → private artifact (short retention) for manual delivery. Nothing public touched.
   - Runbook README: key generation/rotation, cutting an eval build, cutting a watermarked OEM SDK, delivery checklist.

**Verification:** LicenseGuard tests green; Linux end-to-end: valid license → binds 127.0.0.1, `/healthz` 200; expired/tampered → refuses before binding; `loopback_only` + non-loopback attempt → refuses; `strings` shows watermark UUID; `nm` confirms stripped symbols.

## Phase 3 — User-side checklist (documented only)

CLA Assistant enablement; licensor entity decision (guide: The Intel Agency now, WA LLC at $50k ARR); Keiron reply + pricing one-pager (out of scope this round); M2 auth roadmap unaffected.

## Notes & risks

- Legally clean: 100% copyright ownership verified; CLA locks chain of title before any external PR is accepted.
- Effort: Phase 1 ≈ 1–2 days, Phase 2 ≈ 2–3 days (within 35 hr/wk cap).
- Pricing intentionally absent from `COMMERCIAL-LICENSE.md` — live user decision per the guide.
- One PR for all of Phase 1 (batching directive); Phase 2 lands directly on the private repo default branch.
