# Program status — server + 3D client + info client (2026-10-21)

| | |
|---|---|
| Purpose | Full status report across the three main projects: what was recently completed, what is in flight, what remains, and what is next — with a single dependency-ordered view of all three. Companion to the epic-specific [component-connectivity-status-2026-10-21.md](component-connectivity-status-2026-10-21.md). |
| Verified | 2026-10-21 against origin, all three repos fetched; local checkouts fast-forwarded and every PR/issue claim re-checked via `gh` (the 3D checkout arrived **7 commits behind** and was FF'd before analysis — the stale-local failure mode strikes again). |
| Repo heads at verification | OdbDesign `nam20485` @ `99d9f2c` · 3D client `development` @ `ba940e5` · info client `origin/development` @ `ca4359c` |
| Cluster | k3s unreachable from this machine today (Tailscale-only); deployment statements below are from records, not live probes. |

## TL;DR

- **Server:** connectivity Phase 1 is one green, mergeable, unreviewed PR away from done (**#603**). Phase 2 (`connectivity.proto` + always-on derive) has **not started** and is the sole blocker for both clients.
- **3D client:** all epic prep done (F1, F2); rendering stabilised through the chirality family. Waiting on server M2.1. One **unfiled** new bug (mirrored silkscreen text on Turbot).
- **Info client:** all epic prep done (F1, F2 **and** F3 — design reads now gRPC, `Api/Dtos/` deleted). Waiting on server M2.1.
- **The whole epic is: merge #603 → execute M2.1–M2.4 → push protos to both clients (F6) → client F4/F5s.**

---

## Interleaved dependency view (all three projects, dependency order)

Legend: `[S]` server OdbDesign · `[3D]` 3D client · `[IC]` info client. `✓` merged · `▶` open / in flight · `○` not started.

```text
═══ DONE — dependency order (date) ═════════════════════════════════════════════════════════

 ✓ [S]  #594      09-15  Phase 0: plan + swagger foreign keys + spec distribution (M0.1, M0.7)
 ✓ [3D] F1        09-20  stop resolving components by Id (1a1e49e6)      ┐ deploy landmine
 ✓ [S]  #596/#597 09-22  vcpkg cache repair · Turbot corrupt-protobuf fix│ disarmed BEFORE
 ✓ [S]  #595      09-27  M0.3 golden harness + M0.5 client handoff      │ the wire change
 ✓ [3D] #51–#56   09-25→ chip passives, Undefined pins, chirality #53/#55,│ (both clients
                        component search                                 │ stopped Id-
 ✓ [IC] F1  #22   10-15  parity rebuild, stops resolving by Id           ┘ resolve first)
 ✓ [S]  #600      10-15  parse fix (absent attrs ≠ error) — deployed to k3s 10-15 (1bf2f7a)
 ✓ [3D] #57       10-05  arc-sweep chirality fix ("magnetic field" rings)
 ✓ [3D] F2  #59/#60 10-15 conformance harness + captures, all four designs
 ✓ [IC] F3  #23   10-15  design reads → gRPC, Api/Dtos/ deleted, protos refreshed
 ✓ [IC] F2  #24   10-17  conformance harness vs server golden
 ✓ [S]  #601      10-18  M1.1+M1.2+M1.3: typed `id` fields on wire, "ID" map key deleted
 ✓ [S]  #602/#604/#605 10-18  swagger-configmap-sync CI repairs (argv, SHA, regenerated map)
 ✓ [3D] #61 + ba940e5 / [IC] ca4359c  10-18  both clients carry everything to their trunks

═══ IN FLIGHT ══════════════════════════════════════════════════════════════════════════════

 ▶ [S]  #603      M1.4 delete never-populated componentRecordsByName
                     — all 17 checks green, MERGEABLE (branch behind base), no review yet
                     MERGING THIS CLOSES PHASE 1

═══ NEXT, DEPENDENCY ORDER — everything below waits on the gate ════════════════════════════

 ○ [S]  M2.1      connectivity.proto (field 12, Design msg) + always-on derive   ◄── THE GATE
                     (CMake reconfigure required — protos are globbed; D4: no flag, rides
                      the cached default flavour)
 ○ [S]  M2.2      $NONE$/-1 → Connection.Kind semantics absorbed server-side
 ○ [S]  M2.3      warm-path cache assertion (connectivity present in cached bytes)
 ○ [S]  M2.4      size gate: ≤150 KB on sample_design, ≤10% fileModel on legacy design
                     (also measures the M1.2 payload delta; merge gate for Phase 2)
        │  Phase 2 merges
        ├──► [3D]+[IC]  F6  server push-PRs refresh both clients' vendored protos (D7)
        ├──► [3D]  F4/F5 (plan M3.1) delete the 562-line joiner in ComponentDetailBuilder.cs,
        │             rebuild ComponentDetailIndex over Connectivity.components keyed refDes
        └──► [IC]  F4/F5 (plan M3.2) delete the 531-line joiner, consume Connectivity
                      (+ refDes case handling fix, shared hazard with 3D)
 ○ [S]  Phase 4 debt: M4.4 delete dead BuildPlacementsFromEdaDataFile · M4.1 only if it
                     kills include_normalized_lists · M4.2 stays deferred · M4.3 dropped

═══ PARALLEL TRACKS — no dependency on the epic, can interleave anytime ═══════════════════

 ▶ [3D]  UNFILED BUG: mirrored silkscreen text on Turbot (logo + refdes labels render
        backwards; screenshots staged untracked in docs/plan/issues/mirrored_text/)
 ○ [3D]  #11 hit testing & selection on board features
 ○ [IC]  epics #3–#8 (phases 1–6) · #10 REST JSON deserialization (verify vs #23 gRPC switch)
 ○ [S]  #599 StepHdrFile intermittent (~42%) · auth epic #504/#505 (Keycloak→JWT) ·
        WebSocket events (server-issues M3.2) · M0.6 SwaggerUI image spec sync ·
        Argo CD GitOps execution (#568) · GetDesign OOM on 200-40628 / 350-41017
        (~10.1 / ~12.6 GiB peaks vs 10 Gi pod — the sizing constraint M2.4 must respect)
```

Read the diagram bottom-up for "what unblocks what": everything in the clients' epic future hangs off `[S] M2.1`; nothing client-side can usefully start before it.

---

## 1. Server — OdbDesign (this repo)

**Position: Phase 1 ≈ 90% (one open PR), Phase 2 not started. Branch `nam20485` @ `99d9f2c`, clean vs origin (only local-doc churn uncommitted).**

### Recently completed

| When | What |
|---|---|
| 09-15 → 09-27 | Phase 0 complete: #594 (plan, swagger foreign keys, spec distribution fixes), #595 (M0.3 golden fixture harness + M0.5 client handoff). |
| 09-22 | #597 Turbot GetDesign corrupt-protobuf blocker (uninit POD record members serialized raw — the release-only SIGABRT class); #596 vcpkg binary-cache CI repair. |
| 10-15 | #600 parse fix: absent attribute string means "no attributes", not a parse error — this is what made the connectivity-designs corpus loadable; deployed to k3s 10-15 (image `1bf2f7a`). |
| 10-18 | **#601 = M1.1+M1.2+M1.3**: record UIDs parsed into typed `id` fields (components, features, eda-data NET `id=7`/PKG `id=13` proto fields), all 8 `;ID=` call sites rewired, `attributeLookupTable["ID"]` deleted atomically, swagger rewritten same commit. |
| 10-18 | #602 / #604 / #605: swagger-configmap-sync workflow repairs (argv overflow → `--input` payload; SHA export; regenerated ConfigMap for the #601 annotations) + in-repo documentation of the workflow's 409 limitation (its auto-commit cannot land on `nam20485` under the ruleset; drift heals via PR). |

### In flight

- **#603 (M1.4)** — delete the never-populated `componentRecordsByName`. All 17 status checks green, `MERGEABLE` (head behind base — "Update branch" before merging), no review yet. Merging it **closes Phase 1**.

### What remains

- **Phase 2 (the gate for both clients):** M2.1 `connectivity.proto` (field 12 on `Design`) + always-on derive from `m_nets[].GetPinConnections()` in both `to_protobuf` flavours (CMake reconfigure required — protos are globbed); M2.2 `$NONE$`/`-1` → `Connection.Kind` semantics; M2.3 warm-path cache assertion; M2.4 size benchmark (also discharges the one outstanding M1.2 measurement).
- **Phase 4 debt:** M4.4 (delete dead `BuildPlacementsFromEdaDataFile`), M4.1 conditional, M4.2 explicitly deferred (REST freeze), M4.3 dropped.
- **Engineering issues:** #599 StepHdrFile intermittent key-miss (~42% of runs) — the only open engineering issue; #518 CodeQL workflow; auth epic (#504 JWT, #505 endpoint authz → Keycloak per server-issues plan); WebSocket events (server-issues M3.2); #517 GraphQL; #506 IPC-2581; #490 gRPC migration/perf.
- **Ops debt:** Argo CD GitOps execution (#568) not started; M0.6 (sync spec copy #3 into the SwaggerUI image repo + republish — only a bare `docker run` still serves the stale copy); no record of a `nam20485` image publish + k3s rollout after #601 (last confirmed cluster roll is #600 on 10-15 — safe now that the landmine is disarmed, but the next deploy should simply carry Phase 1 complete).
- **Known limit:** GetDesign on `200-40628` / `350-41017` OOMs the 10 Gi pod (measured ~10.1 / est ~12.6 GiB peaks on the 16 GB node with ~6 Gi monitoring) — the sizing constraint M2.4 must respect.

### What's next

1. Review + merge **#603** (`gh pr merge --merge` after "Update branch").
2. Cut the Phase 2 branch from `nam20485`, start **M2.1** (new proto → CMake reconfigure → derive → swagger annotation in the same PR; keep the swagger-configmap sync 409 note in mind for the #2 regeneration).
3. Then M2.2 → M2.3 → M2.4 as the merge gate; on merge, execute the client fan-out (F6 → F4/F5).

## 2. 3D client — odbdesign-3d-client-prototype

**Position: epic prep COMPLETE (F1, F2); waiting on server M2.1. Branch `development` @ `ba940e5` (FF'd today from 7 behind; screenshots + `.zcodeignore` untracked).**

### Recently completed

| When | What |
|---|---|
| 09-25 → 09-27 | #51 Tier-1 M2 chip passives (C/L/D, KiCad-sampled palette); #52 real-ODB++ pins are `Undefined` electrical-type fix (chips were all-gray); #53 scene chirality — board was vertically mirrored vs industry top-view ("user was right since day 1"); #55 pads + fallback arcs chirality (on-demand layers offset by a board width); #56 component search in the inspector (live-filtered refdes dropdown). |
| 10-05 | #57 arc-sweep chirality fix — huge-radius arcs rendered giant complementary "magnetic field" rings. Plus `5d8accf`: the client-side copy of the connectivity implementation plan + task specs (`docs/component-connectivity-implementation-plan.md`, `docs/connectivity-task.md`). |
| 10-15 | **F2 done**: #59 conformance harness against the server goldens; #60 captures completing all four designs. |
| 10-18 | #61 carried everything to `main`; `ba940e5` merged main → development. |

### In flight / newly observed

- **Unfiled bug — mirrored silkscreen text on Turbot:** the staged screenshots (`docs/plan/issues/mirrored_text/turbot_top_sst.png`, `turbot_bottom_ssb.png`) show the Turbot logo and refdes labels rendered backwards on silkscreen. Same *family* as the chirality fixes (#53/#55/#57) but distinct: text rendering, not geometry. Needs an issue filed (the staged directory already matches the repo's issue-evidence convention).
- Housekeeping: the `weird-magnetic-field…png` screenshot is evidence for already-fixed #57 (commit or drop); `.zcodeignore` is untracked.

### What remains

- **F6** — vendored proto refresh, delivered as a server push-PR when `Connectivity` lands (D7).
- **F4/F5 (plan M3.1)** — delete the private join in `ComponentDetailBuilder.cs` (562 lines: `byId`/`byIndex`/`byIndexAnySide` maps, `ResolveComponent`, the `netsByComponent` inversion loop, `ResolvePinNumber`); rebuild `ComponentDetailIndex` as a lookup over `Connectivity.components` keyed `refDes` (also removes the `(BoardSide, uint Id)` key that caused the component collapse); keep `NetDetail` sourced from `netMembers`. Includes the shared refDes case-handling fix. **Blocked on M2.1.**
- **#11** — hit testing and selection on board features (open issue, independent of the epic).

### What's next

1. File the mirrored-silkscreen-text issue with the staged screenshots (can proceed now, parallel to the epic).
2. Otherwise idle on the epic until server M2.1 merges → take the F6 push-PR, then F4/F5 drafted against the F2 harness.

## 3. Info client — Odbdesign-info-client-india79-b

**Position: epic prep COMPLETE (F1, F2 **and** F3 — the furthest along of the two clients). Waiting on server M2.1. Local checkout still sits on the merged `feature/f2-conformance-harness` branch; `origin/development` @ `ca4359c` has everything.**

### Recently completed

| When | What |
|---|---|
| 10-15 | **F1 via #22** — parity rebuild of design-data handling (hardened shared product-model fetch, gated live test, revoked-certificate restore fix). |
| 10-15 | **F3 via #23** — design reads switched from REST to gRPC; vendored protos refreshed; `Api/Dtos/` **deleted**; per-step parse single-flight restored (no degraded cached step rows). `IOdbDesignRestApi.cs` remains only for non-design reads. This was the prerequisite for the client ever seeing `Connectivity` (D5: not built for REST). |
| 10-17 | **F2 via #24** — connectivity conformance harness against the server golden, with capture-manifest provenance cross-checking and review findings addressed. |
| 10-18 | `ca4359c` merged `nam20485` → `development`, picking up #22/#23/#24. |

### What remains

- **F6** — vendored proto refresh via server push-PR (same D7 mechanism as the 3D client).
- **F4/F5 (plan M3.2)** — delete the private join in `src/OdbDesign.ProductModel/ComponentDetailBuilder.cs` (531 lines, same deletion shape as the 3D client) + consume `Connectivity` in `DesignService.cs`/remaining API surface; refDes case handling fix shared with the 3D client. **Blocked on M2.1.**
- Backlog epics **#3–#8** (phases 1–6: foundation → core visualization → connectivity layer → extended viz → cross-probing/IPC → testing/optimization/release) and **#10** (REST JSON deserialization for component/net data — likely partially superseded by the #23 gRPC switch; re-validate before scheduling).

### What's next

1. Housekeeping: switch the local checkout to `development` (the feature branch is fully merged).
2. Re-validate #10 against the post-#23 transport split; prune or re-scope it.
3. Idle on the epic until server M2.1 → take F6, then F4/F5 drafted against the F2 harness.

---

## Cross-cutting facts (verified 2026-10-21)

- **Deploy landmine — disarmed.** Both clients stopped resolving components by `Id` (3D 09-20, info #22 10-15) *before* the server populated `id` on the wire (#601, 10-18). No merged client branch still resolves by `Id`; a server rollout carrying #601+ is safe.
- **Vendored-proto drift is process-managed, not tool-managed.** Both clients vendor protos; nothing checks sync (M0.2 dropped in favour of D7 push-PRs). Until M2.1's push-PR lands, assume client protos know nothing of `Connectivity`.
- **Swagger spec copies:** #1 (repo) is authoritative and #605-annotated; #2 (ConfigMap) regenerated and committed; #3 (SwaggerUI image repo) still stale pending M0.6 — only a bare `docker run` serves it (compose bind-mounts #1).
- **Verification discipline:** all three repos were fetched + FF'd before this report; two of the three arrived stale. Always `git fetch` + FF + `gh`-verify before believing any status claim — including this one next time.

**Scoreboard: one open PR (#603) closes server Phase 1; one unstarted phase (M2.1–M2.4) unblocks both clients' F4/F5/F6; both clients are otherwise ready and idle by design.**

---

## Completion update — 2026-10-29 (fresh sweep, all claims re-verified against origin)

**The component-connectivity epic is COMPLETE.** Final merges: 3D client **#65** (`0430d86` — F4/F5 + the folded gitignore/trufflehog-fix commits), info client **#26** (`8aa2b95`), server **#608** M4.4 (`e3b44a9`). Server owns connectivity end-to-end; both clients consume `Connectivity` with refDes-keyed indexes and zero private joins; M4.4's dead alternative path is deleted. Phase 4 residue is decision-based only (M4.1 conditional, M4.2 frozen with REST). Trufflehog sweep validated in production by the 2026-10-26 weekly cron (success). Housekeeping: 3D `development` FF-carried; info `main` awaits the next "Development" carry PR (#25 + #26); server has one new dependabot PR (#607, vcpkg baseline 2026.10.16). Open non-epic items: 3D #64 mirrored silkscreen, 3D #11 hit-testing, info #10 (stale vs #23), Kilo credits. Next arcs: GetDesign streaming (M6, see [getdesign-streaming-oom-plan.md](getdesign-streaming-oom-plan.md)), cross-probing (info epic #7), DRC groundwork.
