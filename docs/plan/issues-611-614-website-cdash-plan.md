# Master Plan: Website, API Docs & CI Reporting Infrastructure (#611–#614)

**Status: DECIDED — all open decisions (D1–D7) resolved 2026-11-07 (§6). Ready for execution; no repo changes made yet.**

| | |
|---|---|
| Issues | [#611 Add CDash support](https://github.com/nam20485/OdbDesign/issues/611) · [#612 Refactor GH pages site](https://github.com/nam20485/OdbDesign/issues/612) · [#613 Expand doxygen API docs](https://github.com/nam20485/OdbDesign/issues/613) · [#614 naked apex domain 404](https://github.com/nam20485/OdbDesign/issues/614) |
| Date | 2026-11-07 |
| Proposed branch | `nam/docs-cdash-website` (from `nam20485`, PR → `nam20485`, merge commit only) |
| Repo changes | M1 + M2 + M3 in **one PR** (batch, per minimize-PR directive); M4 (#614) is DNS-provider work with **no repo change** |

All four issues are one surface: the public documentation/reporting hub for the project —
what it documents (API docs, #613), how it is organized (#612), where CI results are
reported (CDash, #611), and how visitors reach it (domain, #614).

---

## 1. Verified current state (evidence)

### 1.1 CDash (#611)

- `CTestConfig.cmake` already exists (commit `3be542d`, on `nam20485`): project name
  `OdbDesign`, submit URL `https://my.cdash.org/submit.php?project=OdbDesign`, nightly
  start 00:00 UTC. CDash project created at <https://my.cdash.org/projects/2272>.
- `CMakeLists.txt:68` calls `enable_testing()` but **never `include(CTest)`** — so no
  `DartConfiguration.tcl` is generated in the build tree, and CTest dashboard actions
  (`ctest -T …` / `-D Experimental`) cannot run. `CTestConfig.cmake` is currently inert.
- `.github/workflows/cmake-multi-platform.yml:409` runs a plain
  `ctest --test-dir ./out/build/<preset>/OdbDesignTests …` (gating step, junit via
  dorny). Nothing submits anywhere.
- ✅ **Resolved 2026-11-07 (was the M3 blocker):** the CDash project is now **public** —
  `https://my.cdash.org/index.php?project=OdbDesign` returns 200 anonymously (404
  before the visibility flip), which also confirms the project name is exactly
  `OdbDesign` (matching `CTEST_PROJECT_NAME`/submit URL). Instance runs CDash 5.5.2.
  Probe results against `submit.php`: **open submissions** (an untokened POST passes
  auth and advances to field validation — no `CDASH_TOKEN` secret needed), and the
  endpoint **requires `project`/`build`/`stamp`/`site`/… as POST form fields — the
  `?project=` query string is ignored**. CTest ≥ 3.26 (when CDash moved to Laravel)
  sends exactly those fields; every submitting environment here qualifies (local
  CMake 3.31.6; CI runner images newer). So `CTestConfig.cmake`'s submit URL works
  as-is. End-to-end confirmation happens in M3 validation (§4).

### 1.2 GH Pages site (#612)

- `jekyll-gh-pages.yml` is the **only** Pages publisher (one deployment per repo). It
  runs on: push to `release`, weekly Monday 06:00 UTC schedule, manual dispatch.
  Scheduled runs execute the default branch's workflow file but force-checkout
  `release` (`:49`), so **site content only changes when it lands on `release`**.
- Build: Jekyll (`pages-themes/minimal`) over `./docs`, plus Doxygen HTML copied to
  `_site/api`, plus per-branch coverage HTML downloaded from the latest successful
  `code-coverage.yml` run into `_site/coverage/<branch>/` (development, release).
- `docs/index.md` is already a simple landing page but links only API + coverage.
  Published tree = all of `docs/` except `completed/`, two `plan/` files, one
  `reference/` file (`docs/_config.yml` excludes). `docs/README.md`, `BUILD.md`,
  `README-SETUP.md`, `TESTING.md`, `CONTRIBUTING.md`, `SECURITY.md`,
  `CODE_OF_CONDUCT.md`, `one_pager.md`, `plan/`, `reference/`, `reviews/` all publish.
- `_config.yml` excludes exist because **`{{` in published docs breaks the Pages
  build** (Liquid) — a known failure mode with no PR-time guard today (the workflow
  doesn't run on PRs at all).

### 1.3 Doxygen (#613)

- `OdbDesignLib/doxygen.conf`: `INPUT = ./OdbDesignLib` only; `OUTPUT_DIRECTORY = docs`,
  `HTML_OUTPUT = api` → served at `/api` (linked from `index.md` and 3× from
  `docs/README.md` as `https://source.odbdesignserver.com/api`).
- `EXCLUDE = OdbDesignLib/ProtoBuf` is **stale** — no such directory exists. The real
  proto dir is `OdbDesignLib/protoc/` and holds only `.proto` files (invisible to
  `FILE_PATTERNS`); generated `.pb.h/.pb.cc` exist only in the build tree (verified by
  find). No generated-code exclusion problem in practice.
- Not in input today: `OdbDesignServer/` (Controllers, Services, server app, REST/gRPC),
  `OdbDesignApp/`, `Utils/`, `OdbDesignTests/` (incl. `Fixtures/`).
- Doxygen runs only inside the Pages workflow (Ubuntu apt doxygen 1.9.8, matches the
  conf's Doxyfile version). `EXTRACT_ALL = NO`, `WARN_AS_ERROR = NO`.

### 1.4 Apex domain (#614)

DNS is on **easyDNS** (`dns1/2/3.easydns.*`). Measured 2026-11-07/08:

| Name | Record | Target | Result |
|---|---|---|---|
| `odbdesignserver.com` | A ×4 | `216.239.32.21` `.34` `.36` `.38` — **Google `ghs`** (legacy App Engine mapping, not GitHub) | http → 301 → `www` (`Server: ghs`), https → no TLS served |
| `www.odbdesignserver.com` | CNAME | `odbdesignserver.com.` | follows apex to Google → **404** (the issue's symptom) |
| `source.odbdesignserver.com` | CNAME | `nam20485.github.io.` | GitHub Pages, **200 OK** |

- ⚠️ **The mechanism proposed in the issue (point apex at `www`, alias `www` →
  `source`) will not work as written.** Verified empirically: probing GitHub's Pages
  IPs with `SNI: odbdesignserver.com` returns GitHub's **"Site not found · GitHub
  Pages" 404**. GitHub Pages binds exactly **one** custom domain (`source.`), and its
  auto-redirect only covers the apex↔www pair *when one of that pair is the configured
  domain*. CNAME/ALIAS records pointing apex+www at Pages therefore end in 404s, not
  redirects. The fix must be **HTTP-level redirects** (easyDNS "Web Forwards"), not
  DNS aliasing — see §2.4.
- (Issue-title typo noted for the record: `odbdsignserver.com` → means
  `odbdesignserver.com`.)

---

## 2. Design

### 2.1 M1 — Expand Doxygen to all projects (#613)

One **unified** Doxygen run for the whole repo (single HTML tree at `/api`, path and
all existing links unchanged).

`OdbDesignLib/doxygen.conf` changes:

| Tag | From | To |
|---|---|---|
| `PROJECT_NAME` | `"OdbDesignLib"` | `"OdbDesign"` |
| `PROJECT_BRIEF` | `OdbDesign ODB++ Parsing Library` | `OdbDesign ODB++ Parser — C++ library, gRPC/REST server, CLI, tests` |
| `INPUT` | `./OdbDesignLib` | `./OdbDesignLib ./OdbDesignServer ./OdbDesignApp ./Utils ./OdbDesignTests` |
| `EXCLUDE` | `OdbDesignLib/ProtoBuf` (stale, no-op) | *(empty — nothing to exclude; `.proto` files are invisible to `FILE_PATTERNS`)* |
| `EXTRACT_ALL` | `NO` | `YES` — the new projects (especially tests) carry few doc comments; `EXTRACT_ALL=YES` documents all members and silences undocumented-member warnings. Reference-style docs, not narrative docs. |
| `PREDEFINED` | *(empty)* | GoogleTest macro shims so test files parse as functions instead of hundreds of bogus `TEST_F` entities: `TEST(s,n)=void s##_##n()`, same for `TEST_F`/`TEST_P`/`TYPED_TEST`, and `INSTANTIATE_TEST_SUITE_P(...)=` (tune against actual warnings during implementation) |

Notes:
- Cross-references between projects work naturally (server/tests use library types);
  namespaces (`Odb::Lib::…`) group the tree.
- `OdbDesignTests/Fixtures/` is included — it is the documented test-fixture API
  (`FileArchiveLoadFixture` etc.).
- Workflow change: none required — `jekyll-gh-pages.yml:55` already runs
  `doxygen ./OdbDesignLib/doxygen.conf` from the repo root; only the conf changes.
- Future (out of scope): a PR-time doxygen-warning gate.

### 2.2 M2 — GH Pages site restructure (#612)

All changes are content-only under `docs/` plus one workflow addition.

**a) `docs/index.md` — the required simple front page.** Six primary links exactly as
the issue lists them, then a short "More documentation" section for what already
exists (nothing is removed from the published site):

```markdown
# OdbDesign
A free open source C++ library & REST/gRPC API for parsing ODB++ Design archives.

## Project
- [README](README.md) — features, building, running, integration
## Reference & reports
- [API reference](api/) — Doxygen for all projects (library, server, app, utils, tests)
- [Test coverage](coverage/) — per-branch HTML reports
- CDash dashboard — <https://my.cdash.org/index.php?project=OdbDesign>
- [Dependencies & SBOM](dependencies.md) — third-party packages, dependency & SBOM reports
## Community
- [Contact](contact.md) — email, chat, discussions, consulting
## More documentation
Build · Setup · Testing · Contributing · Security · Code of Conduct · one-pager · internal plan/review docs
```

**b) New `docs/contact.md`.** Consolidate the contact surface currently buried in
`docs/README.md` ("Need Help?" + "Contact" sections): email (maintainer), GitHub
issues/discussions, LinkedIn, Gitter room, website, Google-Calendar scheduling embed
(the raw-HTML embed works under kramdown), consulting note. `docs/README.md` keeps its
own section and gains a link to the page.

**c) New `docs/dependencies.md`** — the "SBOM, dependency, third-party SW packages"
link:

- Runtime third-party table (Crow, protobuf, gRPC, libarchive, OpenSSL, zlib, …) with
  pinned versions from `vcpkg.json`/`vcpkg-configuration.json` and upstream links —
  seed from the README "Credit" section, keep both pointing at one story.
- Build tooling: CMake, vcpkg (manifest mode, pinned baseline).
- Reports: GitHub dependency graph (fed by vcpkg `dependencygraph` integration),
  "Export SBOM" (SPDX, Insights → Dependency graph), the `sbom-generate-submit.yml`
  workflow (SPDX submission to the dependency API), `dependency-review.yml` (PR gate),
  Docker Scout, CodeQL, Codacy, OpenSSF Scorecard badge (already embedded in README).
- License note: AGPL-3.0 + commercial licensing pointer.

v1 is a curated static page (zero moving parts, no artifact plumbing) — the decided
scope (D7). The v2 extension (extend `sbom-generate-submit.yml` to upload the SPDX
JSON as an artifact and have the Pages workflow publish it at `/sbom/`, same pattern
as the coverage tree) is deferred to future work (§7).

**d) `jekyll-gh-pages.yml` — PR build-validation lane** (decided, D6):
add `pull_request: branches: [nam20485, development, staging, main, release]`; the
`build` job then runs on PRs (Doxygen + Jekyll build) but
`Add per-branch coverage reports` and the `deploy` job get
`if: github.event_name != 'pull_request'`. This is the first-ever PR-time guard
against the known `{{`-breaks-Liquid site failure (a bad doc would otherwise only be
discovered at promotion-to-release time).

### 2.3 M3 — CDash integration (#611)

Goal: every push build on the five CI branches submits its configure/build/test
results from all three matrix legs to <https://my.cdash.org/projects/2272>, without
adding any risk to required checks.

1. **`CMakeLists.txt:68`** — replace `enable_testing()` with `include(CTest)` (which
   calls it). Generates `DartConfiguration.tcl` from `CTestConfig.cmake` in every
   build tree; prerequisite for any `ctest -T` dashboard action.
2. **`cmake-multi-platform.yml` — Configure step** gains stable dashboard identity:
   `-DCTEST_SITE=gha-${{ matrix.os }}` and
   `-DCTEST_BUILD_NAME=${{ github.ref_name }}-${{ matrix.preset }}` (the pattern the
   Mastering CMake/CDash chapter the issue links documents). Without these, CDash
   groups builds under random runner hostnames.
3. **New step `Submit Test Results to CDash`** (after `Report Test Results`):

   ```yaml
   - name: Submit Test Results to CDash
     # Push builds only, on the 5 CI branches. PR submissions would spam the
     # dashboard and fork PRs cannot carry the secret anyway.
     if: github.event_name == 'push'
     # Observability sink: a CDash outage or bad submission must never fail a
     # required status check.
     continue-on-error: true
     env:
       CTEST_CONFIGURATION_TYPE: Release   # multi-config (VS) legs build Release
     run: >
       ctest --test-dir ./out/build/${{ matrix.preset }}
       -T Test -T Submit
   ```

   - The **gating test step is left untouched** (`OdbDesignTests` subdir, junit, dorny)
     — zero risk to required checks. The submit step re-runs the suite once from the
     build **root** (where `DartConfiguration.tcl` lives; test set is identical since
     only `OdbDesignTests` registers tests — confirm with `ctest -N`). Cost ≈ the
     suite's ~4 min per leg, in parallel lanes.
   - `CTEST_CONFIGURATION_TYPE=Release` avoids the multi-config hazard where a naive
     `-D Experimental` build step would rebuild everything in Debug on the VS leg.
   - If the standalone `-T Test -T Submit` form lands in the wrong/missing CDash
     group, fall back to `-T Start -T Test -T Submit`; last resort `-D Experimental`
     (re-runs configure+build as no-ops). Acceptance criterion: builds visible under
     the expected site/build names, per-platform, on the project dashboard.
   - **No submission token needed** — open submissions confirmed by probe (§1.1);
     the `ctest -S` driver + `CDASH_TOKEN` secret contingency is moot.
4. **`code-coverage.yml`**: no change — PR submissions and CDash coverage submissions
   are both deferred per D4.
5. **CDash-side configuration** (issue step 3; user, logged in to my.cdash.org):
   - ~~Confirm name/public~~ — done 2026-11-07 (§1.1). Remaining: decide build groups
     (default `Experimental` is fine; optionally per-branch groups), test-time
     thresholds, and turn off email notifications (or point at the maintainer inbox)
     before the first real submission wave.
6. **Validate** (issue step 2): after the PR merges to `nam20485` (push trigger
   includes it), confirm three per-platform builds appear on the dashboard and tests
   match counts; then re-check after the promotion merges so `development`/`release`
   builds appear too.

### 2.4 M4 — Apex + www → source redirect (#614)

**No repo change.** All work is in easyDNS + validation. Canonical stays
`source.odbdesignserver.com` (matches the issue's stated end state and keeps
`_config.yml`'s `url:` and every existing `…/api` link working).

Recommended (primary) — **easyDNS Web Forwards (HTTP 301)**:

1. Delete the four legacy Google `ghs` A records on `odbdesignserver.com` and the
   `www` CNAME to the apex.
2. Add Web Forwards: `odbdesignserver.com` → `https://source.odbdesignserver.com`,
   `www.odbdesignserver.com` → `https://source.odbdesignserver.com` (301).
   easyDNS serves the forwards (including HTTPS on the forwarded names — its
   forwarders provision certificates for zones hosted there; confirm in the easyDNS
   panel during execution).
3. Leave `source` exactly as-is (CNAME → `nam20485.github.io`, GitHub Pages cert).

Rejected alternative (for the record): flip the Pages custom domain to the apex and
let GitHub's apex↔www auto-redirect work natively — but then `source.` itself 404s
(unbound hostname, §1.4 evidence), breaking every existing `source.odbdesignserver.com`
link unless *it* gets a web-forward too, and inverts the canonical domain against the
issue's intent. Also rejected: pure CNAME/ALIAS aliasing — proven to end in GitHub
"Site not found" 404s.

Validation (after DNS change):

```bash
dig +short A odbdesignserver.com                 # easyDNS forwarder IPs (not 216.239.x)
dig +short CNAME www.odbdesignserver.com         # forwarder, not apex
curl -sI http://odbdesignserver.com              # 301 → https://source.odbdesignserver.com
curl -sI https://odbdesignserver.com             # 301 → source (valid TLS)
curl -sI https://www.odbdesignserver.com         # 301 → source (valid TLS)
curl -sI https://source.odbdesignserver.com      # 200, unchanged
```

---

## 3. Sequencing & rollout

```text
M1 (#613)  doxygen.conf expansion        ──┐
M2 (#612)  docs/ restructure + PR lane   ──┼─ one branch nam/docs-cdash-website → PR → nam20485 (merge commit)
M3 (#611)  CTest + workflow submit step  ──┘
M4 (#614)  easyDNS web forwards           — parallel, DNS-only, no PR
```

- Recommended order inside the PR: M1 → M2 → M3 (site links to `/api` content M1
  produces and to the CDash URL M3 confirms).
- Merge: `gh pr merge --merge --admin` — the nam20485 ruleset's required checks are
  static-analysis only (no CMake builds for docs/workflow-only changes); `Analyze
  (actions)` is the applicable gate for the workflow edits.
- **Deploy reality:** the live site updates only when the content reaches `release`
  (push or the Monday 06:00 UTC weekly run). CDash, by contrast, starts receiving
  submissions immediately after the merge (push trigger already includes `nam20485`).
  Plan live acceptance of the site at the next promotion; validate everything else
  pre-merge locally (§4).
- Issues close when: #613 = expanded API live at `/api`; #612 = new front page live;
  #611 = builds flowing on the dashboard + project configured/public; #614 = both
  hostnames 301-ing to `source.` with valid TLS.

## 4. Validation plan (pre-merge, local)

- **M1**: `sudo apt install doxygen graphviz` → `doxygen ./OdbDesignLib/doxygen.conf`
  from repo root → `ls docs/api/` and grep the generated HTML for entities from each
  project (e.g. `OdbServerAppBase`, `ArchiveExtractor`, `FileArchiveLoadFixture`);
  sanity-check the GoogleTest macro shims (no flood of `TEST_F` members in the index).
- **M2**: build the site exactly as CI does — run the
  `ghcr.io/actions/jekyll-build-pages` image (same one `actions/jekyll-build-pages`
  wraps) over `./docs` with the generated `docs/api` in place; confirm `_site/`
  contains the new pages and that adding a doc quoting `{{ }}` fails only where the
  excludes don't cover it (i.e. the PR lane would catch it).
- **M3**: configure any preset → `include(CTest)` produced
  `out/build/<preset>/DartConfiguration.tcl`; `ctest -N` from the build root shows the
  full suite; `ctest --test-dir out/build/<preset> -T Test -T Submit` locally against
  the project once it is public → build appears at my.cdash.org/projects/2272.
- **M4**: the dig/curl battery in §2.4.

## 5. Risks & mitigations

| Risk | Mitigation |
|---|---|
| ~~CDash project private / misnamed~~ | **Resolved 2026-11-07** — public, name confirmed `OdbDesign`, open submissions (§1.1) |
| Submit params must be POST fields (query string ignored on CDash 5.5.2) | CTest ≥ 3.26 sends them; local 3.31.6, CI runners newer; proven end-to-end in M3 validation |
| CDash outage or bad submission fails CI | Submit step is `continue-on-error` + push-only; gating path untouched |
| Doubled test time on push builds (~4 min/leg) | Parallel matrix lanes; acceptable; single-pass `-T Test` fold-in noted as future optimization |
| VS multi-config leg rebuilding in Debug | Design avoids configure/build stages (`-T Test -T Submit` only) + `CTEST_CONFIGURATION_TYPE=Release` |
| Doxygen warning/noise explosion from 4 new projects | `EXTRACT_ALL=YES` + macro shims; iterate on the local run; `WARN_AS_ERROR` stays off |
| `{{` Liquid breakage ships to the site | PR build-validation lane (D6) — currently nothing checks this before release |
| easyDNS forwarders' HTTPS behavior unproven | Verified in-panel during M4; fallback documented (canonical flip is the last resort, with its own web-forward for `source.`) |
| Site changes silently lag behind merges | Rollout note in §3; weekly Monday run self-heals stale branches |

## 6. Decisions

All seven open decisions were resolved 2026-11-07, each accepting the recommendation:

| # | Question | Decision |
|---|---|---|
| D1 | Unified Doxygen tree vs per-project output sections | **Unified** — one run, one `/api` tree (§2.1) |
| D2 | `EXTRACT_ALL` | **YES** — reference-style docs covering all members (§2.1) |
| D3 | Publish internal `plan/` + `reviews/` on the public site? | **Keep publishing** + organize from the front page; `_config.yml` exclude-list unchanged (§2.2a) |
| D4 | CDash submission scope | **Push-only**, 5 CI branches; PR submissions and CDash coverage submissions **deferred** (§2.3) |
| D5 | #614 mechanism | **easyDNS web forwards**, `source.` stays canonical; GitHub-native apex flip rejected (§2.4) |
| D6 | PR build-validation lane in `jekyll-gh-pages.yml` | **Yes** — add it (§2.2d) |
| D7 | Live SPDX SBOM publishing at `/sbom/` | **Defer to v2** — v1 is the curated static page (§2.2c) |

## 7. Out of scope / future

CDash coverage & nightly submissions; per-branch/versioned API docs; doxygen warning
gate in CI; SBOM live publishing (D7); site search/nav theme upgrade; moving the Pages
publish trigger off `release`.

## 8. Work-item checklist

- [x] M3.0 CDash UI: project made **public**; name confirmed exactly `OdbDesign`; submissions **open** (no token); CDash 5.5.2 wants submit params as POST fields — satisfied by CTest ≥ 3.26 (§1.1)
- [ ] M1 doxygen.conf: `INPUT`/`PROJECT_NAME`/`BRIEF`/`EXTRACT_ALL`/`PREDEFINED`, drop stale `EXCLUDE`
- [ ] M1 local doxygen run + per-project spot-checks
- [ ] M2 `docs/index.md` restructure (6 required links + secondary nav)
- [ ] M2 new `docs/contact.md`; link from `docs/README.md`
- [ ] M2 new `docs/dependencies.md`
- [ ] M2 `jekyll-gh-pages.yml` PR lane + guards (D6)
- [ ] M3 `CMakeLists.txt` `include(CTest)`
- [ ] M3 workflow: configure-step `CTEST_SITE`/`CTEST_BUILD_NAME` + push-only submit step
- [ ] M3 local submit validation; dispatch-triggered CI validation after merge
- [ ] PR `nam/docs-cdash-website` → `nam20485` (merge commit; `Analyze (actions)` gate)
- [ ] M4 easyDNS: remove Google records, add two 301 web forwards; run validation battery
- [ ] Post-promotion: verify `/api` expansion + new front page live; close #611–#614
