# Component connectivity — status snapshot 2026-10-14 + client engagement runbooks

| | |
|---|---|
| Purpose | Point-in-time status of the component-connectivity epic, **verified against origin** (GitHub PR/API state, not local branch tips), plus ready-to-run engagement runbooks and session prompts for both client repos. Supersedes [component-connectivity-status-2026-09-20.md](component-connectivity-status-2026-09-20.md) (true at its date; stale as shipped). |
| Living docs | [component-connectivity.md](component-connectivity.md) (design/decisions) · [component-connectivity-implementation-plan.md](component-connectivity-implementation-plan.md) (milestones) · [component-connectivity-client-handoff.md](component-connectivity-client-handoff.md) (client contract) · [component-connectivity-client-followups.md](component-connectivity-client-followups.md) (client F-list) |
| Lesson recorded | The 2026-09-20 and one 2026-10-14 analysis both went out stale because they ran against a **local** `nam/connectivity-fixture-harness` that was 8 commits behind its own remote, with dirty files duplicating already-merged content. Always compare local tips against `origin/` and check PR state via `gh` before status work. |

## Where each repo stands (verified 2026-10-14)

### OdbDesign (this repo) — on `nam20485`, clean, in sync with origin

* **Phase 0 COMPLETE.** #594 (M0.1 swagger FK annotations + M0.7 spec distribution) and **#595 MERGED 2026-09-27** (M0.3 golden harness + M0.5 client handoff + followups F1–F7 + prior status snapshot; carried a 33-finding review-resolution pass; docs committed in `58732fc`).
* **Decisions D1–D7 all resolved.** D3 = `attributeLookupTable["ID"]` deleted atomically with `id` population (no dual-write; M4.3 dropped). D4 = `Connectivity` always-on, no request flag (M0.4 resolved). D5 = REST frozen, gRPC-only consumption. D7 = server owns the protos → M0.2 CI drift check dropped (vendored-proto refresh happens as push PRs). M4.2 deferred (`58732fc`).
* Since #595 the repo moved to other work: #596 (vcpkg CI cache fixes), #597 (Turbot uninit-POD serialization fix), stephdr intermittent-failure investigation, linux-asan/fastbuild presets (direct `nam20485` pushes), #598 dependabot docker bump (open).
* **Server Phase 1 and Phase 2 NOT started** — that is the next work in this repo.

### `odbdesign-3d-client-prototype` (default branch `main`, very active)

* **F1 DONE and merged to main** — `1a1e49e6` (2026-09-20) "component join: ordinal-only resolution, never ComponentRecord.Id" plus follow-up `f57c03d0` (2026-09-23) pick-details fix. The §6 deploy landmine is disarmed for this client wherever its `main` runs.
* 10+ rendering PRs merged since 09-25 (chirality fixes, component search, Tier-1 M2 chip passives, undefined-pin electrical fix) — unrelated to connectivity but proof the repo is healthy.
* **F2 conformance harness NOT present** (no golden/conformance files anywhere in the tree). Unblocked since 2026-09-27 (goldens on `nam20485` via #595). **This is the 3D client's next item.**
* F4/F5 (full joiner deletion + `Connectivity` consumption) blocked on server Phase 2. F6 (vendored `src/OdbDesign3DClient.Core/Protos/` refresh) due when `Connectivity` lands in the protos.

### `Odbdesign-info-client-india79-b` (default branch `main` STALE since 2026-02-09)

* All current work lives on `feature/phase-3-parity` → **PR #22 OPEN** (base = the repo's own `nam20485` branch, last updated 2026-10-05, unmerged).
* **F1-equivalent DONE on that branch** — `09f84860` (2026-09-19) "re-key component joins on the canonical (side, ordinal) netlist foreign key". ⚠️ Until PR #22 merges, any info-client checkout/deployment from `main` still carries the live broken `(side, Id)` join — merging is part of disarming the landmine.
* The branch also carries a large parity rebuild (shared `OdbDesign.ProductModel` library over the gRPC FileModel, hierarchical tree grids, step/Pins/Vias/Symbols/EDA tabs, deep-link cross-probe) and its own connectivity task docs (`243c855a`, 2026-10-05).
* **F3 REST→gRPC NOT done** — `src/OdbDesignInfoClient.Services/Api/IOdbDesignRestApi.cs`, `Api/Dtos/`, `AuthHeaderHandler.cs`, `BasicAuthService.cs` all still present on the branch.

## Remaining sequence

```text
SERVER (OdbDesign):  Phase 1 (M1.1+M1.3 atomic → M1.2 → M1.4) ──► Phase 2 (M2.1 → M2.2 → M2.3 → M2.4 gate)
3D CLIENT:           F2 conformance harness [UNBLOCKED NOW] ──► (park until M2.1) F4/F5
INFO CLIENT:         merge PR #22 ──► F3 REST→gRPC (own PR) ──► F2 harness ──► (park until M2.1) F4/F5
DEBT:                M4.1, M4.2 (deferred), M4.4 · F6 vendored-proto refresh with each server proto change
```

---

## Engaging the client work — runbooks + prompts

### Mechanics that bite (read first)

* **The daemon shell guard blocks all git commands outside the session's own repo** (even read-only). Client-repo git work therefore needs a session rooted in that repo — or hands on a terminal. Cross-repo **file reads by absolute path do work**, so a client-rooted session can read the OdbDesign docs and goldens directly.
* **Merge commits only, never squash, never rebase** — every repo, every PR pair (standing directive; enforced server-side on the OdbDesign branches).
* Golden fixtures live at `/home/nam20485/src/github/nam20485/OdbDesign/OdbDesignTests/Fixtures/Connectivity/` — four files, compact JSON: `sample_design.golden.json` (813 comps, **2,811/2,811 pins connected**, fidelity full), `Panel-g7162-31800_odb.golden.json` (1,440 comps, 3,078 pins, **0 connected**, 517 nets, **88 ordinal/UID collisions on Top**, full), `200-40628_Rev1_v7.golden.json` (57,774 comps, 129,698 pins, **0 connected**, sampled), `350-41017_rev1_odbjob_v7.golden.json` (81,157 comps, **158,742 connected**, sampled). Shape: `aggregates{...}` + `components[{index, side, refDes, uid, pins[{name, netOrdinal, ordinal, pinNumber, subnetOrdinal}]}]`. Regenerate with `scripts/gen-connectivity-golden.py` in the OdbDesign repo; under D7, golden drift reaches clients as a push PR, not a re-fetch.
* **Zero connected pins is legitimate data** (handoff §8b), not a regression — the Panel and 200-40628 fixtures exist to pin that.

### 3D client — F2 conformance harness (next item, unblocked now)

Steps, to be executed by a session rooted in `/home/nam20485/src/github/nam20485/odbdesign-3d-client-prototype`:

1. Read the contract: `…/OdbDesign/docs/plan/component-connectivity-client-handoff.md` §5a/§5c/§8b and `…/component-connectivity-client-followups.md` F2.
2. Copy the four goldens into the repo as test data (suggest `tests/OdbDesign3DClient.Core.Tests/TestData/Connectivity/` — follow the repo's existing test-data conventions if any).
3. Inspect how `tests/OdbDesign3DClient.Core.Tests/Services/ComponentDetailBuilderTests.cs` sources `Design` data today, and extend that mechanism to carry the four real designs. **The per-design source data must come from the real designs** (a captured serialization of each design's `Design` protobuf checked in as test data, or a live-server integration path) — hand-built approximations would make the test prove nothing.
4. Write the conformance test: for each fixture, run `ComponentDetailBuilder` and assert its `refDes → (pin, net)` view equals the golden — keying `refDes` with `StringComparer.Ordinal`; treat `Panel` and `200-40628` as *legitimate-empty* (assert zero connected, all pins unconnected, no error path); use `sample_design` for the real 2,811-connection assertions and `350-41017` as the large connected case. Sampled-fidelity goldens: assert against the aggregates plus the deterministic subset they contain — read the `fidelity` field, don't assume `full`.
5. Full test suite green; open a PR into `main` noting it satisfies M3.3 from the OdbDesign plan and cites the golden provenance.

Prompt to paste into the new session:

```text
You are rooted in the odbdesign-3d-client-prototype repo. Task: implement the F2/M3.3
conformance harness — golden-file tests pinning our component/net view to server-derived
ground truth.

Read first (absolute paths, readable cross-repo):
- /home/nam20485/src/github/nam20485/OdbDesign/docs/plan/component-connectivity-client-handoff.md
  (§5a is what was already done to this repo; §5c defines this task; §8b defines
  legitimate-empty; §2 defines the identity model)
- /home/nam20485/src/github/nam20485/OdbDesign/docs/plan/component-connectivity-client-followups.md
  (item F2)
- Goldens: /home/nam20485/src/github/nam20485/OdbDesign/OdbDesignTests/Fixtures/Connectivity/*.golden.json
  (four files; compact JSON; inspect the shape and the "fidelity" field before coding)

Steps:
1. Copy the four goldens into this repo as test data (follow existing test-data
   conventions; suggest tests/OdbDesign3DClient.Core.Tests/TestData/Connectivity/).
2. Inspect how ComponentDetailBuilderTests currently source Design data for the builder.
   Extend that mechanism so the four real designs' data is available to tests — captured
   serializations checked in, or a documented live-server path. Do NOT hand-build
   approximate fixtures; the harness is only worth what its input data is worth.
3. Add ComponentDetailBuilderConformanceTests: for each fixture design, build through
   ComponentDetailBuilder and assert the refDes → (pin, net) view equals the golden.
   - refDes keys: StringComparer.Ordinal (case-significant, spec p.152).
   - sample_design: full connectivity assertions (2,811/2,811 connected).
   - Panel-g7162 and 200-40628: ZERO connected is legitimate — assert the all-unconnected
     board renders without error; do not treat empty net lists as failure.
   - Panel-g7162 additionally pins the ordinal/UID collision roster (88 on Top).
   - 350-41017: large connected design (158,742). Sampled goldens: assert the aggregates
     plus their deterministic subset; branch on the fidelity field.
4. Run the full test suite; everything green.
5. Open a PR into main (merge commit will be used; never squash/rebase). Title suggestion:
   "test: connectivity conformance harness against server golden fixtures".
   Body: cites the golden provenance (OdbDesign scripts/gen-connectivity-golden.py,
   OdbDesign PR #595), states M3.3 satisfied, notes the harness fails if anyone
   re-introduces a client-side join (that is its purpose).

Constraints: do not touch src/OdbDesign3DClient.Core/Protos/ (F6 comes when the server
ships Connectivity); do not delete the remaining ordinal joiner (F4 waits for server
Phase 2); do not interpret -1/4294967295/$NONE$ sentinels anywhere new.
```

### Info client — merge PR #22, then F3 (F2 after merge)

Steps, for a session rooted in `/home/nam20485/src/github/nam20485/Odbdesign-info-client-india79-b`:

1. **PR #22 first.** It carries the F1-equivalent (`09f84860`) plus the whole parity rebuild; nothing else in the repo should stack on an unmerged 10-week branch. Review state, address findings, update the branch from its base **with a merge** if stale, get it green, merge with a merge commit. Until it merges, `main` (February vintage) is the only clean checkout and it still contains the broken `(side, Id)` join.
2. **F3 next, its own PR, reviewable alone.** Read handoff §5b. Sequence: switch design reads from REST (`IOdbDesignRestApi.cs` + `Api/Dtos/`) to the gRPC path the repo already carries (`OdbDesign.ProductModel` already consumes the gRPC FileModel) → refresh the vendored `protoc/` from the server sources (`/home/nam20485/src/github/nam20485/OdbDesign/OdbDesignLib/protoc/` and `OdbDesignServer/protoc/grpc/service.proto` — the vendored copy is missing at least `GetStandardFonts`, `RequestLoadDesign`, `LoadStatus`, `include_normalized_lists`) → delete hand-mirrored DTOs as the switch kills them. Don't grow any hand-mirrored shape.
3. **F2 after the transport switch lands** — same harness shape as the 3D runbook above, now over gRPC-native data.

Prompt to paste into the new session (phase 1 — PR #22):

```text
You are rooted in the Odbdesign-info-client-india79-b repo. Task: get PR #22
(feature/phase-3-parity → nam20485) reviewed, green, and merged.

Context: main is stale since 2026-02-09; feature/phase-3-parity carries the parity
rebuild AND the component-join fix (09f84860, ordinal-keyed joins). Nothing else should
stack on an unmerged branch. The repo's own connectivity task docs are already on the
branch (added 2026-10-05) — read them plus:
/home/nam20485/src/github/nam20485/OdbDesign/docs/plan/component-connectivity-client-handoff.md (§5b)

Steps:
1. gh pr view 22 — check review state, CI, and whether the branch is stale vs its base.
2. Address open review findings; keep changes on the branch.
3. If stale, update by MERGING the base into the branch (never rebase, never squash).
4. Full build + test suite green.
5. Report ready-to-merge state. Merge uses a merge commit.

After merge, stop and report — F3 (REST→gRPC transport switch) is a separate PR that
gets its own session.
```

Prompt for the follow-up session (phase 2 — F3):

```text
You are rooted in the Odbdesign-info-client-india79-b repo, on a fresh branch cut from
the just-merged parity work. Task: F3 — switch design reads from REST to gRPC (own PR,
reviewable alone).

Read first:
- /home/nam20485/src/github/nam20485/OdbDesign/docs/plan/component-connectivity-client-handoff.md (§5b, §4)
- /home/nam20485/src/github/nam20485/OdbDesign/docs/plan/component-connectivity-client-followups.md (F3, F6)
- This repo's own connectivity task docs (on the branch).

Why: server Connectivity (D5) ships gRPC-only; REST is frozen at security-fixes-only.
While reads go through IOdbDesignRestApi.cs + Api/Dtos/, we can never see it.

Sequence (from handoff §5b — keep the order):
1. Refresh vendored protoc/ from the server sources BEFORE wiring new code:
   /home/nam20485/src/github/nam20485/OdbDesign/OdbDesignLib/protoc/*.proto and
   /home/nam20485/src/github/nam20485/OdbDesign/OdbDesignServer/protoc/grpc/service.proto.
   The vendored copy is missing at least GetStandardFonts, RequestLoadDesign, LoadStatus,
   include_normalized_lists. Verify by diff, don't assume.
2. Switch the design read path (DesignService.cs and callers) from
   src/OdbDesignInfoClient.Services/Api/IOdbDesignRestApi.cs to the gRPC client this repo
   already carries (OdbDesign.ProductModel consumes the gRPC FileModel today — reuse its
   channel/auth wiring).
3. Delete hand-mirrored Api/Dtos/ shapes as the switch makes them dead; do not grow any
   hand-mirrored shape. Auth handlers (AuthHeaderHandler/BasicAuthService) stay if the
   gRPC path uses them; delete only what is truly dead.
4. Full build + test suite green; add/adjust tests for the new read path.
5. Open a PR (merge commit semantics; never squash/rebase) noting: satisfies handoff §5b
   sequence; protoc refreshed (list the features gained); DTOs deleted (list them);
   unblocks F2 (conformance harness) and F5 (Connectivity consumption).

Constraints: do not build Connectivity consumption yet (server Phase 2 not shipped —
field numbers provisional); do not touch the remaining ordinal joiner beyond what the
transport switch requires (F4 waits for server Phase 2); branch on Connection.Kind, never
on -1/4294967295/$NONE$, when that work does arrive.
```

### What stays parked (both clients)

* **F4/F5** — full joiner deletion + `Connectivity` consumption rebuild: preparable as draft PRs against the F2 harness, **merge only after server M2.1**; deleting the joiner before `Connectivity` exists just empties the inspector (plan M0.5). Do not code against the §6 proto text before the real proto merges — field numbers are provisional.
* **F6** — vendored-proto refresh in the 3D client: due when `Connectivity` lands in the server protos; under D7 the server team push-PRs it.
* **Server gates** — Phase 1 (`id` population) and Phase 2 are OdbDesign-repo work; see the implementation plan.

### Deploy-landmine scoreboard

No server build with populated `id` may reach an environment whose running clients still resolve `componentNumber` by `Id`. 3D: disarmed on `main` (F1 merged 2026-09-20). Info: **disarmed only on the PR #22 branch** — merging #22 (and shipping anything that runs from it) completes the front-run. Server Phase 1 hasn't started, so nothing is imminent, but the info-client merge is the open half of this constraint.
