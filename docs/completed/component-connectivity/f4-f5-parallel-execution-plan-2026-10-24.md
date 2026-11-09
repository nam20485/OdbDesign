# F4/F5 parallel execution plan — all three repos, interleaved (2026-10-24)

| | |
|---|---|
| Status | **COMPLETE — 2026-10-29 verified.** All three PRs MERGED: 3D client **#65** (`0430d86`, admin merge past the gate flake; carried gitignore + trufflehog-fix commits), info client **#26** (`8aa2b95`), server **#608** (`e3b44a9`). **The component-connectivity epic is COMPLETE** — server-owned Connectivity end-to-end, both clients consuming it, no private joins, conformance tests enforcing it. Phase 4 residue: M4.1 conditional-only, M4.2 deferred by decision. Post-merge: 3D `development` FF-carried to `0430d86`; info `main` awaits the next "Development" carry PR (#25/#26); trufflehog sweep validated by the 2026-10-26 weekly cron (success). |
| Scope | The epic's remaining client work (F4/F5 in both clients) + the one cheap server debt item (M4.4), run as three INDEPENDENT parallel tracks. No cross-track dependencies — any track can land in any order. |
| Execution | Parallel subagents (orchestrated): heavy codegen tracks get high reasoning, the small server deletion gets low. Each track = branch → implement → local gates → push → PR → review cycle. |

## Interleaved timeline with PR points

```text
NOW — all three gates are open; nothing depends on anything else
─────
Track A [3D CLIENT]  F4/F5 = plan M3.1 (the 562-line joiner)
  A0. housekeeping: commit staged mirrored-text screenshots, file the issue
  A1. branch  feat/connectivity-index        (base: main, tip = #62 F6 merge)
  A2. delete the private join in ComponentDetailBuilder.cs; rebuild
      ComponentDetailIndex over Connectivity.components keyed refDes
  A3. green: dotnet build 0 err · all tests (incl. conformance vs goldens)
  A4. PR ► intel-agency/odbdesign-3d-client-prototype #64 ── merge ──┐
                                                                     │
Track B [INFO CLIENT]  F4/F5 = plan M3.2 (the 531-line joiner)      │ 3D-side
  B1. branch  feature/f4f5-connectivity  (base: nam20485, post-#25) │ epic DONE
  B2. same deletion shape; consume Connectivity in DesignService;    │
      refDes case-handling fix (shared hazard with A2)              │
  B3. green: build · conformance vs goldens                          │
  B4. PR ► nam20485/Odbdesign-info-client-india79-b #26 ── merge ────┤
                                                                     │ BOTH merged =
Track C [SERVER]  M4.4 Phase-4 debt (tiny, deliberately parallel)   │ EPIC COMPLETE
  C1. branch  nam/m44-delete-dead-placements  (base: nam20485 @785ae26+)│
  C2. delete dead BuildPlacementsFromEdaDataFile (Design.cpp:521, Design.h:104)
  C3. green: linux-dynamic-release build + ctest                     │
  C4. PR ► nam20485/OdbDesign #607 ── merge ─────────────────────────┘

USER BUTTONS (not delegable)
  ⛁ #63 admin merge:  gh pr merge 63 --merge --admin   (all-green, gate flake)
  ⛁ approve/adjust this plan

AFTER A + B MERGE — the connectivity epic is complete; next arcs (separate plans):
  ► cross-probing between the two clients (info epic #7) — F4/F5 is its enabler
  ► DRC groundwork (the commercial target)
  ► server backlog: auth epic (#504/#505), WebSocket events (M3.2), #599 flake,
    Argo CD GitOps execution (#568), M0.6 swagger image spec sync
```

## Track A — 3D client F4/F5 (M3.1) · agent: high reasoning

**Base:** `main` @ the #62 merge (`f0b441a`). **Branch:** `feat/connectivity-index`.

Work, per the implementation plan §M3.1, file `src/OdbDesign3DClient.Core/Services/Implementations/ComponentDetailBuilder.cs` (562 lines):

- Delete: `byId`/`byIndex`/`byIdAnySide`/`byIndexAnySide` construction (`:98-103`), `ResolveComponent` (`:440-460`), the netlist inversion loop building `netsByComponent` (`:110-135`, `:172-178`), `ResolvePinNumber` (`:479-481`).
- Rebuild `ComponentDetailIndex` as a lookup over `Connectivity.components`, keyed `refDes` — this also deletes the `(BoardSide, uint Id)` key (`:14`/`:44`/`:238`) that caused the component-collapse bug.
- Keep `NetDetail` for the net tab, sourced from `netMembers`.
- Apply the refDes case-handling fix (§8 hazard 3; coordinate semantics with Track B — same fix both sides).
- Unconnected pins read from the roster (`Kind.UNCONNECTED`, net unset) — do not reconstruct from nets.

**Gates before PR:** `dotnet build` 0 errors; full `dotnet test` (last known: 1230/1230) including the #59/#60 conformance fixtures; grep-clean for the deleted symbols (no joiner resurrection). **PR #64**, merge `--merge`.

**Sub-task A0 (cheap, first):** commit the untracked `docs/plan/issues/mirrored_text/*.png` + `weird-magnetic-field*.png` evidence and file the mirrored-silkscreen-text issue (top/bottom silk mirrored on Turbot; distinct from #53/#55/#57 — text rendering, not geometry). `.zcodeignore`: commit or drop, agent's judgment.

## Track B — info client F4/F5 (M3.2) · agent: high reasoning

**Base:** `origin/nam20485` (post-#25 tip). **Branch:** `feature/f4f5-connectivity`.

Work, per §M3.2, file `src/OdbDesign.ProductModel/ComponentDetailBuilder.cs` (531 lines — same deletion shape: `:74`, `:96-108`, `:415-432`, `:448-453`) plus:

- Consume `Connectivity` in `DesignService.cs` (transport is already gRPC post-#23 — the only remaining REST use is non-design reads).
- Same refDes case-handling fix as Track A.
- Unconnected-pin semantics identical to Track A (roster-sourced, `Kind.UNCONNECTED`).

**Gates before PR:** build green on all legs; the #24 conformance harness passes against the goldens; grep-clean for deleted symbols. **PR #26**, merge `--merge`. Review-findings loop per repo convention (address, push, resolve threads).

## Track C — server M4.4 · agent: low reasoning (small, mechanical)

**Base:** `origin/nam20485` (current tip). **Branch:** `nam/m44-delete-dead-placements`.

- Delete the dead `BuildPlacementsFromEdaDataFile` — definition `Design.cpp:521`, declaration `Design.h:104`, zero call sites (verify by grep before deleting; if a call site has appeared since the plan was written, STOP and report instead of deleting).
- **Gates:** `cmake --preset linux-dynamic-release` build + `ctest --preset linux-dynamic-release` green (env: `VCPKG_ROOT` exported manually, `ODB_TEST_DATA_DIR`/`ODB_TEST_ENVIRONMENT_VARIABLE` from `~/.bashrc`).
- **PR #607** → `nam20485` base, merge `--merge`. Note: `nam20485` ruleset requires Codacy/CodeQL/Analyze checks only (no CMake legs) — but they must be green.

## Risks / notes

- **Vendored-proto freshness is fine** — both clients' protos were refreshed by F6 (#62/#25). No F6-style push-PR needed in these tracks.
- **Review churn is expected:** the client repos run their reviewer workflows per push; findings get addressed in-track (the established loop). Kilo-equivalent nitpicks on deleted-code diffs are cheap to satisfy.
- **refDes case-handling must match across A and B** — same semantics both sides, else the shared conformance fixtures can pass on one and fail on the other. The plan doc's §8 hazard 3 wording is the contract.
- **Server M4.4 agent must not touch anything else** — if the dead method's deletion ripens into a build error (include graph), fix the include, not the feature.
- #63 is NOT a blocker for any track (the gitleaks gate isn't in the required checks for these PRs; merge it whenever).
- All merges are `--merge` commits everywhere, per standing directive. No squashing, no rebasing, no "Update with rebase" — "Update branch" only.
