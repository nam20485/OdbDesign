# Component connectivity — status snapshot 2026-10-21

| | |
|---|---|
| Purpose | Re-verification of the 2026-10-14/10-17 status claims against origin, corrected to current state, per request. Supersedes [component-connectivity-status-2026-10-14.md](component-connectivity-status-2026-10-14.md). |
| Living docs | [component-connectivity.md](component-connectivity.md) · [component-connectivity-implementation-plan.md](component-connectivity-implementation-plan.md) · [component-connectivity-client-handoff.md](component-connectivity-client-handoff.md) · [component-connectivity-client-followups.md](component-connectivity-client-followups.md) |
| Verified | 2026-10-21 against origin (local `nam20485` fast-forwarded to `99d9f2c` after arriving **15 commits behind** — the stale-local-checkout failure mode again; every claim below was re-checked via `gh` on remote state). |

## 1. Verification verdict — prior claims vs now

| Claim as of 10-17 | Still true? | Current reality |
|---|---|---|
| Server Phase 1 NOT started | **NO — stale** | **#601 merged 2026-10-18**: the atomic M1.1+M1.3 commit, and it absorbed **M1.2** too — all 8 `;ID=` call sites wired to typed `id` homes (components, features, eda-data NET `id=7` / PKG `id=13` proto fields added), `"ID"` map key deleted atomically, swagger rewritten same-commit. Plan-doc status row updated in the PR. **M1.4 is open as #603.** |
| Server Phase 2 NOT started | Yes | No `connectivity.proto` exists; no M2.x branch. **This is what the info client is waiting on — correctly.** |
| Last finished = #600 parse-fix chain | **NO — stale** | Last finished = #601 (M1.1–M1.3) plus three swagger-configmap-sync CI fixes merged 10-18: #602 (argv overflow → `--input` payload), #604 (SHA export), #605 (regenerated ConfigMap for the #601 annotations; documents the workflow's 409 limitation). Docker publishes succeeded on development 10-18. |
| `feature-grpc/opt-phase2` = Feb-2026 gRPC work, name collision | Yes | Untouched since 2026-02-09. Still not connectivity Phase 2. |
| `development` carries 3 stray commits nam20485 lacks | **NO — resolved** | Back-merged; `development` vs `nam20485` is now 0 behind / 51 ahead (correct direction). |
| Info client: PR #22 open, F3 not done, REST layer present | **NO — stale** | **#22 MERGED 10-15** (parity rebuild + F1-equivalent). **#23 MERGED 10-15 = F3 DONE** ("Switch design reads to gRPC and refresh vendored protos") — `Api/Dtos/` is deleted; `IOdbDesignRestApi.cs` remains only for non-design reads. **#24 MERGED 10-17 = F2 DONE** (conformance harness vs server goldens). Repo's `main` current through 10-18. |
| 3D client: F2 owed, goldens absent | **NO — stale** | **F2 DONE**: PRs #59 (harness, 10-15), #60 (captures for all four designs, 10-15) merged; 6 golden/conformance files in tree; carried to `main` via #61 (10-18). |
| Info client waiting on server Phase 2 | Yes | Correct — its F4/F5 (joiner deletion + `Connectivity` consumption) need M2.1. |
| GetDesign OOMs the 10Gi pod on 200-40628 / 350-41017 (~10.1/~12.6 GiB peaks) | Assume yes (not re-measured) | No counter-evidence; still the sizing constraint M2.4 will hit. |
| Deploy landmine: server `id` population vs Id-resolving clients | **Resolved — disarmed both sides** | 3D F1 landed 09-20; info F1-equivalent landed with #22 on 10-15 — both **before** #601 merged 10-18. No merged client branch still resolves by `Id`. |

## 2. Current state per repo (verified 2026-10-21)

### OdbDesign — Phase 1 ~75% done, Phase 2 next

* **Phase 0 ✓** (#594, #595). **Phase 1**: M1.1+M1.2+M1.3 ✓ (**#601**); M1.4 **open as #603** ("delete the never-populated componentRecordsByName") — all visible CI legs green, `MERGEABLE`, no review yet. Remaining of M1.2: only the M2.4 payload measurement (per the plan doc's updated status row).
* **Phase 2 not started.** M2.1 (`connectivity.proto` + derive; needs CMake reconfigure — protos are globbed), M2.2 (`$NONE$`/`-1` → `Connection.Kind` semantics), M2.3 (warm-path cache assertion), M2.4 (size gate: ≤150 KB `sample_design`, ≤10% fileModel on the legacy design).
* Also open: #598 dependabot debian, #599 stephdr intermittent (~42%), dependabot action-bump pile (#520 #513 #512 #510 #467).
* CI note from #605: the swagger configmap sync workflow has a **409 limitation** documented in-repo — check its notes before relying on it during Phase 2 spec edits.

### 3D client (`odbdesign-3d-client-prototype`) — client work COMPLETE pending Phase 2

F1 ✓ (`1a1e49e6`), F2 ✓ (#59/#60, all four designs captured). Remaining: **F4/F5** (full joiner deletion + `Connectivity` consumption — blocked on M2.1) and **F6** (vendored proto refresh — server push-PR when `Connectivity` lands, per D7).

### Info client (`Odbdesign-info-client-india79-b`) — client work COMPLETE pending Phase 2

F1 ✓ (#22), **F3 ✓** (#23 — design reads on gRPC, protos refreshed, `Api/Dtos/` deleted), F2 ✓ (#24). Remaining: **F4/F5**, blocked on M2.1 — hence "waiting on server Phase 2".

## 3. What's next

1. **Merge #603 (M1.4)** — green and mergeable; needs review. Closes Phase 1.
2. **Start Phase 2** in this repo: cut the branch from `nam20485`, add `connectivity.proto` (field 12 on `design.proto`'s Design message per the contract; CMake reconfigure required), derive from `m_nets[].GetPinConnections()` in both `to_protobuf` flavours (D4 always-on — no flag, rides the cached default), then M2.2 semantics, M2.3 warm-path assertion, M2.4 size benchmark as the merge gate.
3. **When M2.1 merges:** push-PR both clients' vendored protos (F6), then their F4/F5 PRs (drafted against the F2 harnesses, which both repos now have).
4. Phase 4 debt afterwards (M4.1, M4.2 deferred, M4.4 delete dead `BuildPlacementsFromEdaDataFile`).
5. Unrelated but cheap: #598 dependabot, and the stephdr #599 investigation is still the only open engineering issue.

```text
Phase 1:  M1.1+M1.2+M1.3 ✓ (#601) ──► M1.4 (#603 open, green) ──► Phase 1 closed
Phase 2:  M2.1 (proto+derive, CMake reconfigure) ──► M2.2 ──► M2.3 ──► M2.4 size gate ──► merge
Clients:  F1 ✓ F2 ✓ (both repos) · F3 ✓ (info) ──► [M2.1 merges] ──► F6 proto push-PRs ──► F4/F5
```

**Scoreboard: the epic is one open PR (#603) + Phase 2 away from unblocking both clients.**
