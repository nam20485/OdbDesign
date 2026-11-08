# vcpkg GitHub Packages binary-cache outage — root cause & validated fix plan

> **Goal:** restore and durably protect the GitHub Packages NuGet feed layer of
> the vcpkg hybrid binary cache for the Linux and macOS CI lanes (Windows is
> collateral-damaged and is fixed by the same change).
> **Status:** root cause **empirically established and reproduced**; every
> solution in §5 was **validated on live GitHub runners** (ubuntu-24.04 +
> macos-15) or by direct API round-trips before being written down. POC
> evidence: branch `nam/vcpkg-cache-poc`, runs `42462664440`,
> `42463195934`, `42463795468` (deleted with the branch after this plan is
> executed).
> **Author:** ZCode agent, 2026-11-08, from live log forensics (2023-2026 CI
> runs), vcpkg-tool source (`binarycaching.cpp`), GitHub Packages API state,
> local feed probes, and 4 runner POC iterations.

---

## 1. TL;DR

The Unix feed writes have been failing since **2026-02-14** through **two
stacked, independent causes**, both misreported by nuget.exe as *"One or more
NuGet credential providers failed to authenticate"*, which sent three previous
fix attempts (`961ffc8`, `ff13c54`, the "Verify NuGet Feed Credentials" grep
step) chasing auth ghosts:

1. **RC1 — billing wall (2026-02-14 → 2026-11-08):** vcpkg cache packages
   pushed by a **user PAT default to PRIVATE**. The Feb-2026 windows+osx
   package sets consumed the personal account's Free-tier private-package
   storage quota; from 2026-02-14 every push — any OS, any token, any client —
   was rejected `403 twirp permission_denied: Account has reached its billing
   limit`. **Remediated during POC (validated):** the 37 stale private
   packages were deleted; pushes now succeed (runner-validated with the same
   PAT the CI uses).
2. **RC2 — brittle push-source URL:** `nuget.generated.config` sets
   `defaultPushSource = https://nuget.pkg.github.com/nam20485` (extensionless,
   v2-style). That endpoint **rejects pushes whose nuspec lacks repository
   linkage** with 401 "could not be authenticated" (measured on both OSes) —
   vcpkg's own nupkgs happen to carry a `<repository>` nuspec field
   (vcpkg-tool `binarycaching.cpp:3425-3429` embeds
   `$GITHUB_SERVER_URL/$GITHUB_REPOSITORY.git`) so they pass *today*, but the
   endpoint is the undocumented, picky one. The documented v3 push source
   (`…/nam20485/index.json`) accepts every payload with PAT or `GITHUB_TOKEN`,
   with or without the `<apiKeys>` section — **proven on both OSes**
   (P0/P1 fail vs P3/P4/P5 pass in §4).

Fix = switch the push source to `index.json` (robustness) + switch CI pushes
to `GITHUB_TOKEN` (packages then inherit the repo's **public** visibility →
storage is free; measured: a PAT push performed *today* during the POC
created its package **private** — RC1 refills if CI keeps the PAT) + replace
the grep-style "verify" step with a real push/restore round-trip tripwire.
§5 has the exact edits; §4 has the receipts.

---

## 2. Timeline (from feed API `updated_at` + CI run forensics)

| Date | Event | Evidence |
|---|---|---|
| 2023-10-20 | x64-linux set pushed **public** — feed writes worked | feed API: 80 `*_x64-linux` public packages, created 2023-10-20 |
| 2026-02-11/14 | windows + osx sets pushed — created **private** (PAT default). Last successful feed writes. | feed API `updated_at`: `libarchive_x64-windows` 2026-02-11, `openssl_arm64-osx` 2026-02-14 |
| 2026-02-14 → | **Every push 403s** on the private-package billing limit. nuget.exe wraps it as "credential providers failed to authenticate"; vcpkg logs it as a non-fatal note; jobs stay green. | PR-run logs 2026-09-22 → 2026-11-08: 22-42 push failures per Unix leg, every "Completed submission … to **1** binary cache(s)" |
| 2026-09-20 | `961ffc8` "repair": all platforms `readwrite`, mono `setApiKey` step. **Could not work** — pushes were 403-billing-blocked; the observed "No API Key could be found" was nuget's confusion, not the disease. | run `35722469087`: setApiKey step exit 0, pushes still fail |
| 2026-09-21 | `ff13c54` drops setApiKey ("generated config already persists everything"), adds the **grep-style** "Verify NuGet Feed Credentials" step — which passes while every write fails. | workflow text + green warm runs 2026-09-22 → 2026-11-02 with zero landed writes |
| 2026-11-08 | Investigation (this plan): local dotnet push reproduces the 403 verbatim; 37 private packages deleted; runner POC isolates RC2. | §3, §4 |

Why Windows looked healthy: its lane restores fully from the `actions/cache`
layer, so it never pushed after 2026-02-11 either — the feed has had **zero
writes from any platform for 9 months**, and the current-ABI
`x64-linux-dynamic` triplet (the Linux preset since the dynamic-triplet
switch) has **never** been in the feed at all.

## 3. Root-cause mechanics

**RC1 — private-package billing limit.** NuGet packages pushed to
`nuget.pkg.github.com/<user>` with a user PAT **default to private** and are
not repo-linked (no API exists to flip NuGet visibility — UI only; only
container packages have a REST visibility endpoint). Personal-account Free
plans include a small private-package storage allowance; the Feb-2026
windows+osx sets crossed it. Reproduced directly:

```
$ dotnet nuget push … --api-key <PAT> --source …/index.json
warn : twirp error permission_denied: Account has reached its billing limit…
error: Response status code does not indicate success: 403 (Forbidden).
```

Deleting the 37 stale private packages (2026-11-08, disclosed in §6) unblocked
writes — runner probe D0 (dotnet+PAT) exit 0 on **both** OSes in POC run
`42463795468`'s predecessors; the intermediate "still 403" readings within the
first hour were billing-accounting lag.

**RC2 — extensionless push-source URL.** vcpkg's NuGet push is
`nuget.exe push <nupkg> -SkipDuplicate -Timeout N -ConfigFile <cfg>` —
**no `-ApiKey`** (vcpkg-tool `src/vcpkg/binarycaching.cpp:781-790`; the only
key retry is a hardcoded Azure-DevOps special case at `:845-849`). Push auth
therefore rides the config. Our generated config points
`defaultPushSource` at the extensionless `https://nuget.pkg.github.com/nam20485`.
GH's service answers that endpoint with **401** for nupkgs without nuspec
repository linkage — verbatim runner capture (dummy probe package):

```
WARNING: Your request could not be authenticated by the GitHub Packages service.
  Unauthorized https://nuget.pkg.github.com/nam20485/ 269ms
  Response status code does not indicate success: 401 (Unauthorized).
```

…which nuget.exe reports as "credential providers failed to authenticate" and
vcpkg re-reports as its vendor-auth note. vcpkg's own nupkgs carry a
`<repository>` nuspec field (vcpkg-tool `binarycaching.cpp:3425-3429`) so
they survive this endpoint *for now* (E2E-A passes), but it remains the
undocumented path; the v3 `index.json` push source accepts every payload
(§4). The failure is URL-form-dependent and token-independent.

**Detection gap.** vcpkg treats a failed cache push as a warning; the workflow's
"Verify NuGet Feed Credentials (Non-Windows)" step greps the config file for
`ClearTextPassword`/`defaultPushSource` and passes unconditionally — the
canonical example of the grep-style pseudo-validation now banned in AGENTS.md
(user directive 2026-11-07).

## 4. POC validation matrix (branch `nam/vcpkg-cache-poc`)

Raw mono `nuget.exe` pushes (vcpkg's own fetched nuget under mono, real
runners, dummy package, `-SkipDuplicate`), POC runs `42462664440`/`42463195934`
(exit codes; identical on both OSes unless noted):

| # | Strategy | ubuntu | macos |
|---|---|---|---|
| P0 | **CI-replica config** (PAT, `<apiKeys>`, push URL **extensionless**) | 1 | 1 |
| P1 | GITHUB_TOKEN, extensionless URL | 1 | 1 |
| P2 | `mono nuget.exe push -ApiKey <PAT> -Source …/index.json` | 0 | 0 † |
| P3 | `-ApiKey <GITHUB_TOKEN> -Source …/index.json` | 0 | 0 |
| P4 | ConfigFile, PAT, **no** `<apiKeys>`, push URL `index.json` | 0 | 0 |
| P5 | ConfigFile, PAT + `<apiKeys>`, push URL `index.json` | 0 | 0 |
| D0 | `dotnet nuget push --api-key <PAT>` (billing gate probe) | 0 | 0 |
| D1 | `dotnet nuget push --api-key <GITHUB_TOKEN>` + package landed **public** | 0 | 0 |

† P2 failed once on mac in iteration 2 with the exact version already pushed
by ubuntu minutes earlier (duplicate); exit 0 on a fresh version in iteration
3. Not a mac-specific mechanism.

Conclusion of the raw matrix: **only the push-source URL form matters**
(extensionless → 401 on every token; `index.json` → success on every token);
`<apiKeys>` presence is irrelevant; mono is fully capable of pushing.

End-to-end through real `vcpkg` (build zlib → push → wipe all local state →
restore from feed as the **only** source, assert exit 0 + elapsed < 15 s so a
local rebuild cannot masquerade as a restore), iteration `42463795468` —
**all round-trips PASS on both OSes** (i.e. with quota cleared the feed layer
is functional again for real vcpkg workloads, even through the old
extensionless URL, thanks to vcpkg's nuspec repository field):

| E2E variant (vcpkg push + feed-only restore) | ubuntu | macos |
|---|---|---|
| A — CI-replica config (PAT, extensionless URL) | PASS, 6 s | PASS, 8 s |
| B — PAT, `index.json` push source | PASS, 5 s | PASS, 8 s |
| E — GITHUB_TOKEN, `index.json` push source | PASS, 5 s | PASS, 11 s |

Side-measurement from the same iteration: the mac `zlib_arm64-osx` package —
**created by the PAT variants** — landed **`visibility: private`** (since
deleted), while the GITHUB_TOKEN-created dummy package (D1) landed **public**.
PAT pushes re-privatize the feed package-by-package; GITHUB_TOKEN pushes do
not. This is the recurrence mechanism of RC1, measured live.

## 5. The fix (each step pre-validated above)

### 5.1 `nuget.generated.config`: push source → `index.json` (fixes RC2)

In `.github/workflows/cmake-multi-platform.yml` (the "Generate NuGet Config
for Binary Cache" step), `vcpkg-cache-warm.yml`, and `code-coverage.yml`
wherever the same heredoc exists:

- `<config><add key="defaultPushSource" value="${{ env.FEED_PUSH_URL }}" />`
  → value `${{ env.FEED_URL }}` (i.e. `https://nuget.pkg.github.com/nam20485/index.json`).
- The `<apiKeys>` entries can stay (P5 proves they are harmless) — keep them
  for the local-dev script which uses the same config shape.
- Same change in `scripts/setup-vcpkg-cache.ps1` / `.sh` and
  `docs/reference/vcpkg/localdev-gh-binary-cacheing.md` (FEED_PUSH_URL as a
  push source must not survive anywhere).

Validated by: P0/P1 fail vs P3/P4/P5 pass (both OSes); E2E round-trip §4.

### 5.2 CI pushes with `GITHUB_TOKEN` (prevents RC1 recurrence — the keystone)

Same-repo CI runs push with `${{ secrets.GITHUB_TOKEN }}` (password in
`packageSourceCredentials`, apiKeys values) instead of `VCPKG_PAT_TOKEN`:
workflow-created packages **inherit the repository's visibility** — OdbDesign
is public, so cache packages land **public → storage is free**, the private
quota can never refill from CI. GitHub documents this inheritance for
granular-permission registries (NuGet since 2022-12); vcpkg issue #28975
records the same PAT-private/GITHUB_TOKEN-public asymmetry; and the POC
**measured both directions live on 2026-11-08**: GITHUB_TOKEN-created dummy →
`public`, PAT-created `zlib_arm64-osx` → `private` (deleted afterwards).
Keeping the PAT would quietly re-privatize every package CI pushes and refill
the quota — this is not cosmetic.

- The workflow job already carries `permissions: packages: write`.
- Local dev (`scripts/setup-vcpkg-cache.*`) keeps the PAT (GITHUB_TOKEN
  doesn't exist locally); the localdev doc gains a note: PAT-pushed packages
  default private — flip to public in the UI once if you care about quota, or
  accept the small private footprint.
- Fork PRs: no `VCPKG_PAT_TOKEN` and a fork `GITHUB_TOKEN` — pushes
  non-fatally skip exactly as today (restore of public packages keeps working
  via the config's basic-auth read credentials).

### 5.3 Replace the grep verify step with a real round-trip tripwire

Delete "Verify NuGet Feed Credentials (Non-Windows)" (grep of the config —
banned pattern, zero behavioral coverage). In `vcpkg-cache-warm.yml` add a
post-install assertion step (this is the warm job's entire reason to exist —
a green run that wrote nothing is a failure):

1. After the warm `vcpkg install`, wipe the local default cache and
   buildtrees.
2. Re-run `vcpkg install` with `VCPKG_BINARY_SOURCES=clear;nugetconfig,<cfg>,read`
   as the **only** source; assert exit 0 **and** wall time < 15 s (a feed
   restore takes seconds; a local rebuild of the warmed set takes minutes —
   the timing gate is what makes exit-code-only honest).
3. Any failure fails the job loudly.

Validated by: the E2E harness in §4 is exactly this construction and
distinguishes PASS/FAIL correctly across four iterations (including catching
two harness bugs of its own — duplicate-version and missing-dir — proof the
assert has teeth).

### 5.4 Hygiene items riding the same PR

- `vcpkg-cache-warm.yml`: keep `mono-devel` install; warm jobs no longer need
  the PAT (GITHUB_TOKEN covers them) — drop `VCPKG_PAT_TOKEN` from warm.
- Delete branch `nam/vcpkg-cache-poc` + its workflow after the fix lands.
- Add the CDash/summary trick from the POC (probe-result table into
  `$GITHUB_STEP_SUMMARY`) if the warm job wants cheap observability.

### 5.5 Already executed during POC (receipts, not proposals)

- Deleted all 37 **private** vcpkg cache packages (`*_x64-windows` 18,
  `*_arm64-osx` 19; Feb-2026 ABI hashes, wrong-ABI dead weight, recreatable by
  any cold run). Public `*_x64-linux` set (2023) kept; the 4 private
  *container* packages from other projects (maxkernel-agent etc.) were **not**
  touched. Validated outcome: D0 exit 0 on both OSes — the billing gate is
  gone.
- Dummy POC packages (`odbdesign-cache-poc`) are deleted by the POC workflow
  itself (`if: always()` cleanup step).
- Deleted `zlib_arm64-osx` (created **private** by the POC's PAT variant during
  E2E — the very recurrence mechanism §5.2 eliminates). Private nuget count
  verified back to 0 after cleanup.

## 6. Rollout

1. Merge this plan's PR (docs + AGENTS.md ban only — no workflow changes yet).
2. Fix PR (branch `nam/vcpkg-binary-cache-fix`): §5.1-§5.4 as one PR to
   `nam20485`. Required checks on `nam20485` are analysis-only (no CMake
   builds) — expect Codacy/CodeQL/Analyze/dependency-review only.
3. First run after merge: `vcpkg-cache-warm` (workflow_dispatch it) must go
   green **through the new tripwire** — that run seeds the feed with current-ABI
   `x64-linux`, `x64-linux-dynamic`, `arm64-osx` packages as **public**.
4. Verify via API (not logs): `gh api /user/packages/nuget/zlib_arm64-osx`
   returns `visibility: public` and a current-baseline version; spot-check
   one cmake-multi-platform Unix leg shows the actions/cache layer falling
   through to NuGet restores on a cache-miss lane.
5. Expected one-time cost: macos lane's next cache-key miss rebuilds ~30 min
   once, then warm.

## 7. Risks / open items

- **Billing lag**: GH's private-storage accounting updates ~daily; if the
  account is re-checked before lag clears, a push may transiently 403. The
  tripwire (§5.3) makes this visible instead of silent.
- **Other private packages**: the 4 private containers from other projects
  still count against the personal quota. They fit (they pushed fine
  2026-08-29); the CI cache no longer depends on that headroom because CI
  packages are public-by-inheritance (§5.2).
- **nuget.exe URL-form strictness is service-side behavior** — the
  extensionless endpoint already rejects repository-less nupkgs (measured);
  vcpkg nuspecs pass it only via their embedded `<repository>` field. Moving
  to `index.json` (§5.1) removes the dependency on that undocumented
  behavior, and the tripwire (§5.3) catches any future endpoint tightening
  within one warm run (weekly).
