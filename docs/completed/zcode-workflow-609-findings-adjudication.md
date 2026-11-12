# PR #609 review findings — verification & adjudication

| | |
|---|---|
| Source report | [`zcode-workflow-609-review-report.md`](zcode-workflow-609-review-report.md) (three-lens review, 16 findings: 1×P1, 15×P2) |
| Verified against | branch `nam/getdesign-streaming` @ `645a65c` (2026-11-01) |
| Method | every cited `file:line` re-read at HEAD; the report's grep evidence re-run; the contested P1 gate independently re-executed (3 fresh runs of `LargeDesign_350_41017_StreamWithinRssBudget`, binary confirmed current — `ninja: no work to do`); no code changed for this document |
| Structure | per finding: **Verification** (confirmed / corrected / refuted, with evidence), **Detail** (what is actually happening), **Recommendation** (fix or not, where the work should ride), **If not fixed** (concrete consequence) |

## Verdict summary

| # | Lens | Sev | Location | Claim | Verified | Recommendation |
|---|---|---|---|---|---|---|
| F1 | wire | P2 | `DesignStreamWalker.cpp:113-152` | step/layer sanitization collision diverges from monolith | ✅ confirmed | **FIX** (in #609) |
| F2 | wire | P2 | `Design.cpp:470-604` | estimator has no accuracy pin / is regression-silent in CI | ✅ confirmed (+report correction) | **FIX** (in #609) |
| F3 | wire | P2 | `OdbDesignServiceImpl.cpp:430` | `spool=aborted` logged when no spool was attempted | ✅ confirmed, **broader than reported** | **FIX** (in #609) |
| F4 | lifecycle | P2 | `Design.cpp:483` | "every serialized term counted" comment overstates | ✅ confirmed (with nuance) | **FIX** comment only (risk covered by F2 test) |
| F5 | lifecycle | P2 | `Design.cpp:501-502` | fonts bulk only in flat MISC allowance | ✅ confirmed as documented omission | **ACCEPT** (revisit trigger defined) |
| F6 | lifecycle | P2 | `StreamSpoolCache.cpp:437-444` | Windows orphan spool files escape byte accounting | ✅ confirmed, **4 sites not 1** | **DEFER** (Linux-only deployment) |
| F7 | lifecycle | P2 | `OdbDesignServiceImpl.cpp:375` | TOCTOU: stale design can publish under new generation | ✅ confirmed; cheap safe-direction fix exists | **FIX** (in #609) |
| F8 | lifecycle | P2 | `DesignStreamMemoryTests.cpp:273-283` | mixed indentation in `RunLargeDesignScenario` | ✅ confirmed (dup of F16b) | **FIX** (whitespace, batched) |
| F9 | tests | **P1** | `DesignStreamMemoryTests.cpp:382-383` | warm-peak marginal gate knife-edge | ✅ confirmed **worse: 3/3 fail locally** (reviewer: 1/5) | **FIX — top priority** (in #609) |
| F10 | tests | P2 | `OdbDesignServiceImpl.cpp:345-355,432-444` | truncated-stream INTERNAL + cancel paths untested | ✅ confirmed (grep: zero cancel tests) | **FIX** (in #609) |
| F11 | tests | P2 | `DesignStreamWalker.cpp:416,422` | `find()` on protobuf Maps — the #599 flake family | ✅ confirmed | **FIX** (in #609) |
| F12 | tests | P2 | `DesignStreamWalker.h:81-84` | reassembly rejection paths never driven negatively | ✅ confirmed (only success-path uses) | **FIX** (in #609) |
| F13 | tests | P2 | `GrpcServiceConfig.cpp:161-186` | new `design_streaming` config keys untested | ✅ confirmed (zero test refs) | **FIX** (in #609) |
| F14 | tests | P2 | `OdbDesignServiceImpl.cpp:395-402` | spool-I/O-failure degradation claim untested | ✅ confirmed (happy-path only) | **FIX** (cheap half; seam for the rest) |
| F15 | tests | P2 | `DesignStreamTests.cpp:506-520` | concurrent-fill test can hang instead of fail | ✅ confirmed exactly | **FIX** (one line) |
| F16 | tests | P2 | `DesignStreamMemoryTests.cpp:52` | stale "~4 GiB" comment; indent; `fix(ci)` scope | ✅ confirmed all three | **FIX** comment+indent (batched); commit title = note only |

**Tally: 13 fix (all ride open PR #609 as ≤3 batched commits, per the minimize-PR-count directive), 1 defer (F6), 1 accept (F5), 1 duplicate folded (F8≡F16b).**

---

## Cross-cutting corrections to the source report

Found while verifying; they change some conclusions:

1. **"Large-design archives absent locally" (wire lens, F2 evidence) is wrong.** Both archives exist at `new_designs/` (gitignored, repo root): `200-40628_Rev1_v7.tgz`, `350-41017_rev1_odbjob_v7.tgz` — `copyBigDesign` reads exactly that path (`DesignStreamMemoryTests.cpp:222`). The lifecycle and tests lenses both ran the benches; the wire lens could not. Consequence: an estimate-vs-actual measurement for the two OOM designs **is** collectable on this machine; F2's remediation is not blocked by fixture availability.
2. **F8 and F16(indent) are the same defect** — both describe the `RunLargeDesignScenario` indentation introduced by 5692a4f. One fix. Measured precisely: the comment block + signature + first block (`:266-281`) sit one tab deeper than the rest of the body (`:283+` is dedented one tab relative to its own first block).
3. **F6 undercounts its blast radius.** The unconditional `m_totalBytes -=` after a best-effort `remove` pattern exists at **four** sites, not one: `PublishEntry` (`StreamSpoolCache.cpp:437-444`), stale-generation drop in `TryOpenReader` (`:356-364`), `Drop` (`:462-464`), and `EvictOverBudgetLocked` (`:490-493`). Any fix must cover all four (or sweep at `InitializeDirectory`).
4. **F3's mislabel is broader than reported** — see the F3 detail: an *aborted* fill also logs `spool=committed`.

### P1 independent re-measurement (F9)

Three fresh runs (this machine, 2026-11-01, release binary current with 645a65c, ~78 GB RAM free):

| Run | parse-only KB | warm peak KB | threshold (parse + 1 GiB) KB | margin | cold peak KB | warm − cold |
|---|---|---|---|---|---|---|
| 1 | 4,310,892 | 5,393,784 | 5,359,468 | **−33,516 KB (FAIL)** | 4,981,008 | +412,776 KB |
| 2 | 4,309,964 | 5,358,680 | 5,358,540 | **−140 KB (FAIL)** | 4,973,252 | +385,428 KB |
| 3 | 4,312,616 | 5,472,856 | 5,361,192 | **−111,664 KB (FAIL)** | 5,019,872 | +452,984 KB |

All three runs fail `DesignStreamMemoryTests.cpp:382` (the warm marginal gate); every other warm assertion passes (chunks 6895 == 6895, wire bytes equal, warm ~7.8 s < cold ~11.3 s, absolute < 6 GiB). Run 2 fails by **140 KB** — 0.01% of the budget. The reviewer's 1-failure-in-5 (passing margins as thin as 4.4 MB) plus this 3/3 means the gate is effectively red-on-arrival when opted in on the current machine state; it cannot serve as M6.5 evidence as calibrated. The warm-vs-cold delta (+0.38–0.45 GiB here; +0.55–0.65 GiB in the reviewer's runs — the two passes share one VmHWM reset) is consistent, unexplained, and per the test's own comment (`:375-379`) would mean the spool reader is buffering rather than streaming.

---

## Wire-contract lens

### F1 — step/layer sanitization-collision divergence — **P2, CONFIRMED — FIX**

**Claim.** If two layer (or step) names in one design sanitize to the same UTF-8 key, the reassembled stream diverges from the monolith (duplicated features, or `ApplyDesignChunk` false).

**Verification.**
- Monolith is last-wins map assignment for both levels: `FileArchive.cpp:294` (`stepsbyname[ToUtf8(key)] = *to_protobuf()`) and `StepDirectory.cpp:255-264` (`layersbyname[ToUtf8(key)] = ...`, both skeleton and full branches).
- Walker emits a `FeatureBatch` for **every raw (step, layer)** with sanitized addressing and no collision filter (`DesignStreamWalker.cpp:113-152`); `StepSkeleton` chunks are applied last-wins (`:404`), matching the monolith — only the feature-batch path diverges.
- `kFeatures` application: unknown step → false (`:416-420`), unknown layer → false (`:422-426`), else **append** into the single surviving entry (`:428-431`).
- Design-level ByName maps *do* mirror the monolith's first-wins guard exactly (`ProtobufMapHelpers.h:17-31` vs `StreamNormalizedMap`, `DesignStreamWalker.cpp:26-37`) — the divergence is confined to the step/layer addressing path, as reported.
- The contract claims are unconditional in two places: `service.proto:29-33` ("Reassembling the chunks ... reproduces the exact monolithic GetDesign bytes") and `service.proto:156-157` ("the reassembled message serializes to the exact bytes of the monolithic GetDesign response").

**Detail.** Mechanism with two colliding raw layer names in one step: the monolith keeps only the last raw layer's records (earlier entry overwritten at map assignment); the stream delivers the surviving layer's skeleton (last-wins, consistent) but then appends feature batches of **both** raw layers into that one entry → the reassembled message contains the shadowed layer's features plus the survivor's, and byte-equality breaks. With colliding *step* names it is worse: the shadowed step's feature batches address the surviving step and either append into a same-named layer (silent duplication) or fail the lookup (`applyOk=false`, `DesignStreamTests.cpp:230-233` surfaces it as a malformed-sequence failure). The collision class is real, not hypothetical: `ProtobufMapHelpers.h:9-16` documents it (five undefined CP1252 slots → U+FFFD, plus repaired-key collisions), which is precisely why the design-level maps have the first-wins guard. No fixture exhibits it — but per this repo's own spec rule, corpus absence is not a format guarantee; ODB++ names are archive directory names and hostile encodings are possible in the wild.

**Recommendation.** **Fix, riding PR #609.** Two parts, both small:
1. Walker collision handling: in `Walk`, precompute the surviving raw steps/layers per sanitized key (std::map iteration order makes "last" well-defined and identical on both paths) and skip `FeatureBatch` emission for shadowed raw entries. ~20 lines, confined to the two loops at `:96-152`.
2. Scope the two `service.proto` contract comments to collision-free keys regardless (the claim as written is checkable-false).
This is the root-cause fix (matching the monolith), not a comment-only stopgap — and it must land **before M6.4 clients** copy the reassembly recipe out of the proto comments.

**If not fixed.** Exotic-name designs get silently corrupted geometry (duplicated features) or hard reassembly failures, with the proto contract promising byte-exactness the whole time; M6.4 bakes the recipe into two client codebases, after which fixing the server recipe means coordinating a three-repo change instead of one.

### F2 — `EstimateWireSizeBytes` is load-bearing but regression-silent — **P2, CONFIRMED — FIX**

**Claim.** The estimator is the sole dispatch input for the OOM guard and streaming split; no test references it, and nothing pins its accuracy — a term-dropping regression passes CI silently.

**Verification.**
- `grep -rn EstimateWireSizeBytes OdbDesignTests/` → **zero matches** (re-run; confirms the report).
- It is the sole input at all three dispatch sites: `OdbDesignServiceImpl.cpp:229` (GetDesign guard, request flavour), `:377-378` (stream spooling decision, default flavour), `DesignCache.cpp:1094-1095` (background pre-serialization skip).
- The only CI coverage is `MonolithicGetDesign_RejectsOverBudgetDesign_WithResourceExhausted` (`DesignStreamTests.cpp:369-393`), which uses `setMonolithicWireBudget(1)` (`:375`) — it passes for **any nonzero estimate** and pins nothing about magnitude.
- Real-budget separation is proven only by the opt-in bench (`DesignStreamMemoryTests.cpp:340`), which CI never runs (`ODB_DESIGN_STREAM_BENCH` gated, `:64`).
- Report correction: the OOM-design archives are **present locally** (`new_designs/`), so estimate-vs-actual can be recorded for exactly the designs the guard exists for.

**Detail.** The failure mode the guard exists to prevent (5692a4f's own commit message: pod OOM on the 10 GiB node) returns *silently* if a future edit drops a term: the estimate for an OOM design slides under the 256 MiB budget (`DesignCache.h:761`), GetDesign builds the monolithic tree, and the pod dies at serve time — CI green throughout. The margin is unknown: nothing records how far above the budget the two OOM designs' estimates actually sit today, so nobody can say whether a single-term drop is fatal. The estimator is a pure function of container sizes (`Design.cpp:512-603`) — deterministic, cheap to evaluate in a test, and directly comparable against `SerializeToString().size()` on any corpus design.

**Recommendation.** **Fix, riding PR #609** (test-only):
1. A CI test on `sample_design` + `rigidflex`: `EstimateWireSizeBytes(flavour)` ≥ c × actual serialized size (c ≈ 0.5 — the coefficients are deliberately generous; the pin is against *term disappearance*, not precision), both flavours. Uses only fixtures CI already has.
2. Extend the opt-in bench to print `estimate` alongside the existing `[BENCH]` phases for the two large designs, so the headroom above 256 MiB gets recorded in M6.5 evidence (the "how far above" question answered once and for all).

**If not fixed.** The OOM guard's calibration is unverifiable; the next refactor of `Design.cpp` (M6.6 re-bases cache charging on this same estimate — plan line 80) can silently invalidate it, and the first signal is a production pod kill on the exact designs this PR was cut for.

### F3 — `spool=` log mislabel — **P2, CONFIRMED AND BROADER — FIX**

**Claim.** For an `include_normalized_lists=true` request of an over-budget design, the done-log prints `spool=aborted` although no spool write was ever attempted.

**Verification.** `OdbDesignServiceImpl.cpp:430`: `(spooling ? " spool=committed" : (overBudget ? " spool=aborted" : ""))`, while `spooling` is initialized at `:381` as `!includeNormalizedLists && overBudget && BeginWrite(...)` — the flavour gate is in `spooling` but not in the printed fallback. Confirmed.
**Additionally (not in the report):** `spooling` is only ever set to false by the Append-failure branch (`:401`); the Commit/Abort decision at `:416-423` does not touch it. So a fill that **aborts** — client cancelled mid-walk, or generation bumped under it — still logs `spool=committed` (`spooling` remains true at `:430`). The label is wrong in both directions: "aborted" when nothing was attempted (normalized flavour), "committed" when the spool was actually aborted.

**Detail.** Cosmetic to the wire, but this is the ops line used to debug exactly the situations the PR is about (warm-path misses, invalidation races). A mislabel here sends an operator hunting for a spool-write failure that never happened — or trusting a spool that was dropped.

**Recommendation.** **Fix, riding PR #609** — track what actually happened (e.g. an enum: none-attempted / attempted-open-failed / aborted / committed) and print that. ~5 lines, log-only, zero wire impact.

**If not fixed.** Misleading diagnostics in the OOM/invalidation debugging path, including actively false "committed" claims for aborted publishes; costs real time in the first production incident involving the spool.

---

## Lifecycle lens

### F4 — "every serialized term below is counted" overstates — **P2, CONFIRMED (with nuance) — FIX comment**

**Claim.** Several actually-serialized terms are uncounted: ByName-map duplication of net/package/pin records, per-record propertyrecords inside NetRecord/PackageRecord, FeatureGroupRecord internals.

**Verification.** All cited serialization sites check out: `EdaDataFile.cpp:300` (`netrecordsbyname`), `:309` (`packagerecordsbyname`), `:1599` (`pinrecordsbyname` — pin records ride the wire twice inside every PackageRecord), `:132` / `:1603` (per-record `propertyrecords`), `:198-210` (FeatureGroupRecord embeds `featureidrecords` + `propertyrecords`, charged only `PROPERTY_RECORD_WIRE_BYTES=64` at `Design.cpp:548-549`). Base coefficients `NET_WIRE_BYTES=128` / `PACKAGE_WIRE_BYTES=256` (`Design.cpp:487-488`) are applied once per record in the base sum (`:580-590`).
Nuance: the coefficient comment at `Design.cpp:481-484` *deliberately* amortizes multi-placement ("a net two-to-three places (~128 B)"), so this is calibration-by-coefficient, not an oversight per se — but the blanket claim "every serialized term below is counted" (`:478-479`) is strictly false for the embedded-record terms above.

**Detail.** Dispatch behavior is currently safe with margin (the 200-40628 bench dispatches RESOURCE_EXHAUSTED far above budget; verified live by the review and by my F9 rerun — `status=8` with zero RSS movement). The issue is that the comment invites exactly the confidence F2 shows is unpinned: a reader trusting "every term counted" will not add tests, and the un-counted embedded terms are precisely the ones a future format change can inflate (see also the appendix note on FeatureGroupRecord — its wire content is murkier than even this finding assumed).

**Recommendation.** **Fix the comment** (one line: "dominant bulk counted; embedded per-record propertyrecords and ByName-map duplication amortized into coefficients") **and let F2's measurement test carry the safety argument.** No coefficient change warranted now — doubling coefficients would change dispatch for designs between the old and new estimates without evidence any real design sits there.

**If not fixed.** Nothing breaks at runtime; the risk is human — the comment's false precision deflects the exact pinning work F2 calls for, and the next reader of `Design.cpp` inherits an unwarranted guarantee.

### F5 — fonts bulk only in the flat MISC allowance — **P2, CONFIRMED — ACCEPT as documented**

**Claim.** `StandardFontsFile` bulk is covered only by `MISC_DESIGN_ALLOWANCE_BYTES` (256 KiB) + per-step allowance, not per-record, though the review ask listed fonts.

**Verification.** `StandardFontsFile.cpp:295-300` serializes `m_characterblocks`; `:325-330` serializes per-block `m_lineerecords`/`m_linerecords` (unbounded lists). `Design.cpp:506-510` documents the allowance rationale; no font term exists in the counting block (`:512-578`). Confirmed.

**Detail.** Standard-fonts files define glyph line segments per character block — in practice bounded (a "standard" font has ~100-ish characters × ~tens of lines). Both OOM designs are feature/component-dominated (the parse-RSS/wire ratios in the M6.6 plan, §1, are driven by features). The omission is disclosed in-code, which is the standard this repo holds for allowances.

**Recommendation.** **Accept as documented.** Define the revisit trigger now: if a corpus ever appears whose fonts file dominates, add a per-line-record term — and F2's measurement test is precisely the tripwire that would surface it (estimate badly undershooting actual on such a design).

**If not fixed.** A hypothetical fonts-heavy design could be under-estimated and served monolithically; with the MISC allowance and the real-world boundedness of standard fonts, this is theoretical until a corpus proves otherwise.

### F6 — Windows orphan spool files escape byte accounting — **P2, CONFIRMED (4 sites) — DEFER**

**Claim.** On last-committer-wins publish, if the loser's spool file cannot be removed (Windows, handle held by a warm reader), the orphan stays on disk but its bytes are already subtracted from `m_totalBytes` — and with a configured spool dir nothing ever reclaims it.

**Verification.** `PublishEntry` (`StreamSpoolCache.cpp:437-444`): best-effort `remove` then unconditional `m_totalBytes -=`. Same pattern at three more sites (stale-generation drop `:356-364`, `Drop` `:462-464`, eviction `:490-493`). The Windows failure mode is documented in-code (`:403-407`); `MakeStreamSpoolCache` constructs with `ownsDirectory=false` for a configured dir (`OdbDesignServiceImpl.cpp:152-156`), so the `remove_all` in `~StreamSpoolCache` (`:299-306`) never runs for it. POSIX unaffected (unlink-while-open keeps the reader's fd valid). Confirmed.

**Detail.** Consequence chain on Windows + `stream_spool_dir` set: every publish that displaces a still-being-read entry leaks one orphan file of unaccounted size; `spool_max_bytes` (the disk-protection knob) under-counts from then on; across process restarts the directory monotonically accumulates. On Linux (the actual deployment target — k3s per AGENTS.md) this cannot happen.

**Recommendation.** **Defer** with the fix shape written down for when it matters: re-add bytes when `remove` reports failure (all four sites), or sweep stale `*.spool` files at `InitializeDirectory`. Revisit trigger: any plan to serve from Windows. Not worth divergence/risk in #609 for a platform the service does not run on.

**If not fixed.** No effect on the Linux k3s deployment. On a hypothetical Windows deployment with a configured spool dir: unbounded disk growth invisible to `spool_max_bytes`.

### F7 — TOCTOU: stale design can publish under a new generation — **P2, CONFIRMED — FIX**

**Claim.** An invalidation landing between `GetDesign()` (`:320`) and `GetResponseGeneration()` (`:375`) lets a fill built from the stale Design publish under the NEW generation; the commit-time re-check (`:414-416`) cannot catch it because `generationAtStart` was read after the bump.

**Verification.** All four anchors confirmed: design fetch `:320`, generation read `:375`, re-check `:414-416`, publish `:418`. `GetResponseGeneration` is a plain read of the response store (`DesignCache.cpp:828-831`) — nothing correlates the two calls. Pre-existing before 5692a4f (which added the re-check and shrank the window from the whole fill to the fetch-to-start gap). The report's suggested precedent is real and verified: `CommitResponsePayloads` re-validates design-object identity under the cache lock at `DesignCache.cpp:1142-1149` — the spool publish path has no equivalent.

**Detail.** Why it matters more than "narrow": the poisoned outcome is a spool keyed to the *current* generation built from the *previous* design content. It does not self-heal — the generation matches, so warm serves replay stale content until the *next* invalidation. The window is the gap between two adjacent calls (sub-millisecond typical), and the trigger is a re-upload/invalidation racing a cold fill — rare, but this service's invalidation path exists precisely because re-uploads happen.
Cheap correct fix available: read `generationAtStart` **before** `GetDesign()` (hoist `:375` above `:320`). Then any invalidation between generation-read and publish trips the re-check → fill aborts → next request refills. The abort-on-race direction is safe (the current client still gets its streamed content; only the publish is skipped) and it is a strict strengthening of the existing check. Optionally add the identity re-validation mirroring `CommitResponsePayloads` for belt-and-braces.

**Recommendation.** **Fix, riding PR #609** — the hoist is a two-line reorder with a safe failure mode; the identity check can ride the same commit if desired. This is a correctness (data-freshness) hole, not hygiene, and M6.4 clients will assume re-upload ⇒ new content on the next request.

**If not fixed.** A rare re-upload race leaves warm clients permanently served stale design content (until the next re-upload), undermining the invalidation contract the test at `DesignStreamTests.cpp:455-463` asserts — that test passes only because the window is tiny, not because the hole is closed.

### F8 — mixed indentation in `RunLargeDesignScenario` — **P2, CONFIRMED — FIX (batched whitespace)**

**Claim.** 5692a4f introduced mixed indentation — signature/first block one level deeper than the rest of the body.

**Verification.** Measured: comment block + signature + first block (`DesignStreamMemoryTests.cpp:266-281`) at one deeper level than the body from `:283` onward (e.g. `:272` signature at 2 tabs vs `MakeServeEnv` at `:101` also 2 tabs — but `:283-303` statements at 2 tabs where `:274-281` are at 3). Compiles and runs; purely visual. Duplicate of the F16 indent item — count as one defect.

**Recommendation.** **Fix as a whitespace-only hunk batched into the next commit touching this file** (with F16's comment fix). Never a standalone PR (minimize-PR-count directive).

**If not fixed.** Persistent friction/visual noise in the file that contains the M6.5 gates; no functional cost.

---

## Tests-and-gates lens

### F9 — warm-peak RSS marginal gate is knife-edge — **P1, CONFIRMED AND WORSE — FIX (top priority)**

**Claim.** The 5692a4f warm gate (`EXPECT_LE(warmPeakKb, parseOnlyPeakKb + kServeMarginalBudgetKb)`, `DesignStreamMemoryTests.cpp:382-383`) fails intermittently for 350-41017 (reviewer: 1/5 runs, passing margins as thin as 4.4 MB; PR-body evidence implies ~21 MB margin), and the warm replay consistently peaks ~0.55-0.65 GiB above the cold walk — which by the test's own comment means "the spool reader is buffering instead of streaming".

**Verification — independently reproduced and amplified.** My three fresh runs all **fail** at `:382` (see table above): over by 33.5 MB, 140 KB, and 109 MB; every other warm assertion passes each time. Combined with the reviewer's 5 runs: 4 failures in 8 attempts on this machine, passing margins ≤ ~283 MB and typically < 10 MB. The absolute gate (`:380-381`, 6 GiB) holds everywhere (max observed 5.47 GiB) — the pod-fit claim is fine; it is specifically the *marginal* gate that is miscalibrated against reality. The warm−cold delta is reproduced in every run (+0.38–0.45 GiB over cold in mine). Reader code inspection (`StreamSpoolCache.cpp:107-140`): `Next()` reads one length-prefixed frame at a time into a transient string — no obvious whole-file buffering at the spool layer; the delta's source is genuinely unexplained by code reading (candidates: gRPC send-path buffering differences between the two serving loops, allocator behavior across the two sequential warm replays sharing one VmHWM reset, per-message serialization buffers at ~195 KB average frames × 6895).

**Detail.** Two distinct problems are bundled here:
1. **Gate calibration:** `parseOnlyPeakKb + 1 GiB` was set from cold-serve behavior; measured warm behavior sits at or above that line. A gate that fails ≥ half the time (or passes by 140 KB) protects nothing — it will be either ignored or widened reflexively, and then real regressions (actual reader buffering) slip through.
2. **Possible real finding:** the consistent +0.4-0.65 GiB warm-over-cold delta. If the warm path is genuinely buffering, the M6.5 "warm = page-cache replay, anonymous heap ≈ 0" story (plan §M6.3) is not what's implemented, and it eats pod headroom on every warm serve.

**Recommendation.** **Fix in PR #609, in this order:**
1. Investigate the warm delta first (heap profile / `malloc_trim` instrumentation around the warm loop) — if it is buffering, that is the root-cause fix and the gate likely settles without recalibration. Per the no-stopgaps directive, do not simply raise the constant and move on.
2. Recalibrate from measurements either way: the natural form is `warm ≤ coldPeak + measured slack` (the warm path is strictly a subset of the cold path's work plus file I/O — comparing it to itself is robust; comparing it to the parse baseline inherits two independent variances). Record the calibration data in M6.5 evidence.
Until then, the warm marginal gate should not be cited as M6.5 evidence (the PR body's "warm 5.09 GiB, ~21 MB margin" is exactly the fragility in question).
Not CI-blocking in the mechanical sense (the bench is opt-in; CI stays green), but it blocks the *evidence claim* and should land before M6.4 clients make warm-path assumptions.

**If not fixed.** The M6.5 memory gate for warm serving is noise — unusable as a regression tripwire; a genuine warm-path buffering regression (the thing the comment says the gate exists to catch) would be indistinguishable from the baseline flake; and if the +0.5 GiB is real buffering, every warm serve of a 1.34 GB design carries avoidable pod risk that M6.6's budget arithmetic (plan line 81: "~1 GiB serve marginal" headroom) is silently consuming.

### F10 — truncated-stream INTERNAL + cancellation paths untested — **P2, CONFIRMED — FIX**

**Claim.** The 5692a4f truncated-stream fix (sink failure without client cancel → INTERNAL, warm and cold) ships with no regression test; the client-cancellation path (`IsCancelled` → OK) is equally untested.

**Verification.** `grep -rn "IsCancelled\|Cancel()\|->Cancel" OdbDesignTests/*.cpp` → **zero** hits (re-run). The INTERNAL branches at `OdbDesignServiceImpl.cpp:345-355` (warm write failure), `:358-364` (warm spool read failure), `:432-444` (cold truncation) are unreachable from the current suite; the OK-on-cancel returns (`:339-344`, `:387-390` via sink false + `:445`) likewise. Confirmed.

**Detail.** These branches are the *entire point* of 5692a4f's rationale — "chunk count is the only completion signal M6.4 clients have". Nothing pins that contract: a refactor that reintroduces return-OK-on-truncation (the pre-fix bug shape) passes every current test, and M6.4 clients would silently accept truncated designs as complete. The cancel path matters for the same reason from the other side (cancelled clients must not log/error as failures, and aborted fills must not publish — the publish-abort at `:420-423` is only reachable via cancellation or generation bump).

**Recommendation.** **Fix, riding PR #609** (tests only):
- Cancel-mid-stream: client context `Cancel()` after N reads → assert final status OK and `EntryCount()==0` when the fill was spooling (covers the sink-abort → Abort-not-Commit chain too).
- Warm read failure: after a committed spool, truncate/corrupt the spool file on disk, then request → assert INTERNAL + spool dropped (`:358-364`).
- Cold writer failure without cancel is hard to stage in-process (needs `writer->Write` to fail without IsCancelled); accept as assert-by-inspection or revisit with a seam — do not block on it.

**If not fixed.** The PR's central client-facing guarantee (never a clean trailer on a truncated stream) is unguarded; the most likely future regression is also the most damaging one — silent partial designs in M6.4 clients.

### F11 — `find()` on protobuf Maps in `ApplyDesignChunk` — **P2, CONFIRMED — FIX**

**Claim.** `ApplyDesignChunk` calls `.find()` on `stepsbyname`/`layersbyname` — the exact lookup pattern #599 diagnosed as an address-layout-dependent protobuf-Map bug (present under iteration, absent under `find()`, ~40% of runs on affected binaries, unroot-caused), and which this same PR codifies as forbidden.

**Verification.** `DesignStreamWalker.cpp:416` (`stepsByName->find(batch.step_name())`), `:422` (`layersByName->find(batch.layer_name())`) — on generated protobuf Map fields. The convention text is at `ProtobufSerializationTests.cpp:260-268` ("Never find()/at() a protobuf Map in tests; iterate"), including the full #599 diagnosis (never root-caused; ASan/UBSan silent; not corruption/dual-runtimes/stale gencode). Confirmed.

**Detail.** Two honest counterpoints, and why fix anyway: (a) the convention as written scopes itself to tests, and (b) the gates exercise these two calls thousands of times per run with zero observed misses — if the #599 failure rate applied here, `DesignStreamTest.*` would flake constantly, and it doesn't. But #599 was never root-caused, so "it hasn't happened here" is not evidence it can't; these two calls sit on the critical path of every wire-equivalence gate (a miss flips `applyOk=false` and reds the suite with the flake family this PR itself documented); and M6.4 will replicate this function client-side where the same rule should apply — the reference implementation ought to model it. The maps are tiny (a handful of steps; tens of layers per step): a linear scan is free.

**Recommendation.** **Fix, riding PR #609** — replace both `find()` calls with linear scans (or carry the step/layer pointers from the skeleton pass). Mechanical, ~10 lines, removes a documented-unexplained flake class from the gates' own path. Consider a one-line note in the `service.proto` reassembly comment that client implementations should prefer iteration (C# map lookups have their own history).

**If not fixed.** Standing low-probability flake source in the PR's own gates, with a diagnosis cost already measured in "four CI runs and a log hunt" the last time this family appeared (#599); each future occurrence will be re-investigated from scratch because the code contradicts the convention doc.

### F12 — reassembly rejection paths never driven negatively — **P2, CONFIRMED — FIX**

**Claim.** The malformed/out-of-order rejection paths (FeatureBatch before header, unknown step/layer, `PAYLOAD_NOT_SET`) are implemented but never tested; every test asserts `applyOk==true`.

**Verification.** `ApplyDesignChunk` false-branches: `kStep` without filemodel (`:399-402`), `kFeatures` without filemodel (`:410-413`), unknown step (`:417-420`), unknown layer (`:423-426`), `PAYLOAD_NOT_SET` (`:489-491`). Test uses: only `DesignStreamTests.cpp:230,321,352`, all on well-formed walker output. Confirmed by grep and by reading all three sites.

**Detail.** This is the corrupt/hostile-input safety property for M6.4: clients will feed `ApplyDesignChunk`-equivalents bytes from a network, and the reference implementation's documented contract ("Returns false on a malformed sequence", `DesignStreamWalker.h:82-84`) is the spec they'll copy. An accidental future relaxation (e.g. "helpfully" creating a missing step) would pass every current green test while breaking the safety property — and would also silently change wire-equality behavior for genuinely malformed input.

**Recommendation.** **Fix, riding PR #609** — three cheap unit tests against a hand-built chunk sequence: features-before-header → false; FeatureBatch naming an undelivered step/layer → false; empty chunk (payload not set) → false. No fixtures needed.

**If not fixed.** The documented malformed-input contract is unverified; the most likely regression (tolerant reassembly) is invisible to the suite and corrupts the M6.4 client story rather than failing a gate.

### F13 — new `design_streaming` config keys untested — **P2, CONFIRMED — FIX**

**Claim.** The new keys (enabled, feature_batch_size 100-10000 clamp, spool_dir, spool_max_bytes 0→UINT64_MAX sentinel) have no test coverage, unlike sibling clamps.

**Verification.** Parsing at `GrpcServiceConfig.cpp:161-189`: enabled (`:166-169`), inline clamp `std::max(100, std::min(10000, value))` (`:171-175`), spool_dir passthrough (`:177-180`), 0→max sentinel (`:182-188`). `grep -n "design_streaming\|spool\|DesignStreaming" OdbDesignTests/GrpcServiceConfigTests.cpp` → **zero** hits (re-run), while that file tests the sibling keepalive/thread-pool/batch clamps. Confirmed.

**Detail.** These keys run the OOM-guard machinery's knobs: `feature_batch_size` bounds peak per-chunk memory; `spool_max_bytes` is the disk-protection budget (0-as-unlimited is the classic sentinel to silently break — an operator setting 0 expecting "no spooling" or "disabled" gets *unlimited*); `enabled=false` is the kill-switch. A regression in any of them (bounds change, sentinel inversion, clamp removal) currently passes CI.

**Recommendation.** **Fix, riding PR #609** — extend `GrpcServiceConfigTests` with: clamp below 100 / above 10000 / in-range; enabled true+false; `spool_max_bytes: 0` → UINT64_MAX; nonzero passthrough; spool_dir string passthrough. Follows the file's existing clamp-test pattern exactly.

**If not fixed.** Operator-facing knobs of the memory-safety machinery can regress silently; the nastiest realistic one is the 0-sentinel flipping meaning (bounded ↔ unlimited disk) with a green suite.

### F14 — spool-I/O-failure degradation untested — **P2, CONFIRMED — FIX (cheap half)**

**Claim.** The PR-body claim "spool I/O failure degrades to unspooled streaming" (`spoolWriter.Append` failure → Abort + continue, `OdbDesignServiceImpl.cpp:395-402`) has no test.

**Verification.** Code path confirmed at `:395-402`. Test-side `BeginWrite`/`Append` usage: only the happy path (`DesignStreamTests.cpp:510/528` concurrent test, plus the service-level spool tests); no failure injection anywhere. Confirmed.

**Detail.** This is the "the wire response is never failed by the disk" promise — the resilience property that keeps over-budget designs servable when the spool disk hiccups (ENOSPC being the expected real-world trigger). Asymmetric difficulty worth being explicit about: **BeginWrite-failure** is easy to stage (point `stream_spool_dir` at an unusable path → `m_usable=false` → `BeginWrite` false → `spooling=false`); **Append-failure mid-fill** is genuinely hard without a seam (on POSIX an open fd keeps succeeding through unlink/chmod/ENOSPC-on-other-files), so the honest scope is the BeginWrite half now plus a documented seam decision for Append.

**Recommendation.** **Fix the cheap half, riding PR #609**: end-to-end test with an unusable spool dir asserting the stream completes OK with byte-identical output and no published entry. Record Append-failure as covered-by-inspection unless a failure-injection seam is added later (do not contort the Writer API just for this).

**If not fixed.** A regression that fails the RPC on spool trouble — breaking over-budget serving exactly when disk is unhealthy, i.e. the moment the degradation matters — passes the suite.

### F15 — concurrent-fill test can hang instead of fail — **P2, CONFIRMED — FIX (one line)**

**Claim.** If `BeginWrite` fails on either thread before the barrier, `fillsReady` never reaches 2 and both threads spin forever.

**Verification.** Exact lines confirmed: `DesignStreamTests.cpp:510-513` early-returns on `BeginWrite` failure **without** incrementing `fillsReady`; `:516-519` spins `while (fillsReady.load() < 2) yield();` with no deadline. The other thread (whose `BeginWrite` succeeded) spins forever; `join`s never return. Confirmed.

**Detail.** Trigger plausibility: the test constructs `StreamSpoolCache cache(dir, ...)` over a `temp_directory_path`-based dir; any failure making that directory unusable (`InitializeDirectory` → `m_usable=false`) turns every `BeginWrite` false. The failure mode is the worst kind: not a red test but a **wedged binary** — in CI that is a job timeout (60 min) with no diagnostic, versus a 1-second red with a message.

**Recommendation.** **Fix, riding PR #609** — increment `fillsReady` unconditionally before the `BeginWrite` early-return (one line), or bound the barrier with a deadline + `FAIL`. The increment is strictly better (a `BeginWrite` failure then also produces a deterministic assertion failure via the entry/replay checks).

**If not fixed.** The concurrency gate the P1a fix shipped becomes a potential CI hang on any temp-dir environment problem — an opaque 60-minute timeout replacing a clear failure, exactly the failure shape (#599's "Subprocess aborted with no diagnostic") this repo has already paid for once.

### F16 — docs/style nits (stale "~4 GiB" comment, indent, `fix(ci)` scope) — **P2, CONFIRMED — FIX comment + indent; note the rest**

**Claim.** (a) File-header says cold-serve "must stay within the ~4 GiB budget" but the gate constant is 6 GiB (`DesignStreamMemoryTests.cpp:52` vs `:303`); (b) indentation (dup of F8); (c) commit 6a8bfd1's subject says `fix(ci)` but touches no CI file.

**Verification.** (a) Confirmed — `:52` says "~4 GiB budget"; `kAbsoluteBudgetKb = 6 GiB` at `:303`; the plan doc §M6.5 also says "~4 GiB" (line 50) while the PR body discloses 6 GiB — so the doc chain (plan → header comment → constant) is internally inconsistent and only the constant is true. (b) Confirmed (see F8). (c) Confirmed — `git show --stat 6a8bfd1`: StepDirectory.cpp + DesignStreamMemoryTests.cpp + ProtobufSerializationTests.cpp, no CI file. The report's own position (note only, no history rewrite) is correct and mandatory here: rewriting published commits violates the repo's no-rebase rules.

**Recommendation.** **Fix (a) and (b)** — update the header comment to the actual gates (6 GiB absolute + 1 GiB marginal over parse baseline) and re-indent with F8's hunk; also correct the plan doc's "~4 GiB" M6.5 line to the implemented 6 GiB while at it (one line, same batch). **(c) note-only** — record in the PR conversation that 6a8bfd1's scope label is off; never rewrite it.

**If not fixed.** The next person calibrating the memory gates reads "~4 GiB" in two places and 6 GiB in the code — exactly the class of stale-documentation trap this repo's swagger-copy discipline exists to prevent; the plan doc keeps promising a gate (4 GiB, both designs) that was never implemented.

---

## Appendix — new observations from this verification pass (not in the source report)

Out of the report's scope; recorded so they are not lost. Neither affects PR #609's diff (both verified pre-existing / untouched by the range `88be296..645a65c`).

1. **`FeatureGroupRecord::to_protobuf` appears to append `PropertyRecord` payloads into the `featureIdRecords` field** (`EdaDataFile.cpp:204-210`: the property loop calls `add_featureidrecords()` then `CopyFrom(*propertyRecord->to_protobuf())`). The proto defines them as distinct types (`edadatafile.proto:162-168`: `propertyRecords = 2`, `featureIdRecords = 3`), so the untyped `Message::CopyFrom` overload resolves at runtime — a type-confusion/UB hazard on release builds *if ever executed with a non-empty property list*. The suite is green, so no current fixture reaches it; the hunk predates the branch (introduced in `3271995`). Worth a one-line fix (`add_propertyrecords()`) + a tiny unit test in a follow-up — and it slightly strengthens F4: FeatureGroupRecord wire content is even less covered by the 64 B coefficient than the report assumed.
2. **The default-flavour wire-equivalence gate uses raw `EXPECT_EQ` on ~21 MB strings** (`DesignStreamTests.cpp:282-283`) — the exact pattern the file's own `BytesEqual` helper comment (`:78-81`) warns constructs a quadratic diff hang. Failure-path-only hazard (a real byte divergence wedges the test instead of reporting the offset); the sibling gates all use `BytesEqual`. One-line swap.
