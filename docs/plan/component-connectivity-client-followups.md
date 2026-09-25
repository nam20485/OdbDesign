# Component connectivity — client follow-up work

| | |
|---|---|
| Purpose | The remaining client-side work after the unblocked prep was implemented 2026-09-15, listed short with a few lines each, for a future planning/implementation pass. Reader needs no prior context beyond the linked docs. |
| Parent docs | [component-connectivity.md](component-connectivity.md) (design/decisions) · [component-connectivity-implementation-plan.md](component-connectivity-implementation-plan.md) (milestones) · [component-connectivity-client-handoff.md](component-connectivity-client-handoff.md) (the contract clients follow — §5 migration, §6 landmine, §8 defects) |
| State caveat | Written against **uncommitted working trees** (F1). Re-verify with `git status`/`git diff` in each repo before planning from this. Corrected 2026-09-25: the goldens are no longer untracked — they are committed on this branch (`35ed14a`) and pending in PR #595. |

## Already done, awaiting commit (verified 2026-09-15)

Both clients' `ComponentDetailBuilder.cs`: the netlist→component join is **ordinal-only** now (the `byId`/`byIdAnySide` dicts, their population, and both probes in `ResolveComponent` are deleted, with a remarks block pinning why), and the design-object name dicts use `StringComparer.Ordinal`. Verified by execution: 3D suite 1154/1154 + full sln build; info suite 47/47 (3 of them new) + full sln build. Files touched:

| Repo | Files |
|---|---|
| `odbdesign-3d-client-prototype` | `src/OdbDesign3DClient.Core/Services/Implementations/ComponentDetailBuilder.cs` (562→550); `tests/OdbDesign3DClient.Core.Tests/Services/ComponentDetailBuilderTests.cs` (fixtures rerigged to ordinals; `Build_NeverResolvesComponentNumberAgainstId` added; `…CrossSideIdCollision…` → `…CrossSideOrdinalCollision…`) |
| `Odbdesign-info-client-india79-b` | `src/OdbDesign.ProductModel/ComponentDetailBuilder.cs` (532→519); `src/OdbDesign.ProductModel/ComponentDetailIndex.cs` (case fix here too — see F7a); `tests/OdbDesignInfoClient.Tests/ProductModel/ComponentDetailBuilderTests.cs` (**new**, 3 tests) |

## Open items

**F1 — Branch, commit, PR the above (both repos).** *Do first.* Nothing else in those repos can proceed cleanly over a dirty tree. Cannot be done from an OdbDesign-rooted session: the daemon shell guard blocks all git commands outside the session's own repo (even read-only), so run it from a session rooted in each client repo, or by hand. Draft message for both, adapt to each repo's log style: `fix(model): resolve netlist component numbers by ordinal only; make refDes keys case-sensitive` — body cites handoff §6/§8. Note in the PRs: these deletions permanently disarm the §6 deploy landmine, so they should deploy **before** any server build with populated `id` reaches the same environment.

**F2 — M3.3 conformance harness (both clients).** *Next after F1.* Publish the M0.3 golden files (`OdbDesignTests/Fixtures/Connectivity/*.golden.json` from `scripts/gen-connectivity-golden.py` — committed on OdbDesign branch `nam/connectivity-fixture-harness` as `35ed14a`, pending in **PR #595** — must merge to `nam20485` first) into both clients as test data; each asserts its `refDes → (pin, net)` view equals the golden. Include the zero-connectivity designs (`Panel-g7162`: 0/3,078 pins connected; `200-40628`: 0/129,698) as *legitimate-empty* cases per handoff §8b. The plan requires this to exist before the joiner-deletion PRs land.

**F3 — Info client REST→gRPC transport switch.** *Largest remaining item; its own PR, reviewable alone.* Design reads go through `src/OdbDesignInfoClient.Services/Api/IOdbDesignRestApi.cs` + `Api/Dtos/`; `Connectivity` is gRPC-only (D5), so this gates F4/F5 for the info client. gRPC codegen is already wired (`OdbDesign.ProductModel.csproj` from `:32`); sequence per handoff §5b: switch transport → refresh `protoc/` (currently missing all four current `service.proto` features) → delete hand-mirrored DTOs as they die.

**F4 — Full joiner deletion (both clients).** *Preparable, shippable only after server Phase 2.* What remains of the private join after today's cuts: the `(side, Id)` keys (`netsByComponent`, `byKey`, `ComponentDetailIndex.ByKey`), `ResolveComponent`'s ordinal fallback, `ResolvePinNumber` (`toeprintNumber→pinNumber`), and `BuildNetNameLookup`'s Index-vs-ordinal double-claim. Deleting it before `Connectivity` exists just empties the inspector (plan M0.5) — draft the PRs against the F2 harness, merge when M2.1 ships.

**F5 — `Connectivity` consumption rebuild (both clients).** *Blocked on M2.1/M2.2 merge.* ~50-line lookup over `Connectivity.components` keyed `refDes` (`StringComparer.Ordinal`); net tab from `netMembers`; branch on `Connection.Kind`, never on `-1`/`4294967295`/`$NONE$`. Field numbers are provisional until the proto actually merges (handoff §7) — do not code against the §6 text before then.

**F6 — Vendored-proto upkeep + sync check.** 3D: refresh `src/OdbDesign3DClient.Core/Protos/` when `Connectivity` lands in the server protos (drift claim inherited from design doc §4.4 — not independently measured). Info: folded into F3. There is deliberately **no** sync check: D7 (decided 2026-09-15) dropped M0.2 — the server owns the protos and pushes updates to clients, so every refresh is manual by design, not by omission. The durable fix is the future `odbdesign-model-libs` repo.

**F7 — Judgment calls surfaced today, deliberately not changed.**
- **a)** The plan docs say "all four name dictionaries" fold case; the info client actually had **six** sites across **two** files — its `ComponentDetailIndex.cs` (`Empty.ByName`, `NetsByName`) mirrors 3D's inline record but was outside the plan's scan. Both were fixed, but the plan/design docs should be corrected on the next pass.
- **b)** 3D `ComponentInspectorViewModel` keeps its own `OrdinalIgnoreCase` `_netsByName` (UI-layer net lookup, `:16/:44/:75`) — decide whether to align with the builder's Ordinal choice or document why the UI differs.
- **c)** Info `ProductModelReader.cs:163` groups by `PartName` OrdinalIgnoreCase — part names are not refDes, left alone; revisit only if the spec's case rules are asserted for parts too.
- **d)** Not touched anywhere: case-folding on HTTP headers, layer names/colors, `VALUE`/`BOM` attribute-name matching — legitimate today.

## Order at a glance

```text
F1 (commit) ──► F2 (harness, after goldens merge) ──► F4 (joiner deletion; MERGE only after M2.1)
        └────► F3 (info transport) ─────────────────► F5 (Consumption rebuild; after M2.1) ──► F4/F5 ship
F6 continuous · F7 = small decisions to close in any of the above PRs
```

Server-side gates (Phase 1 `id` population, Phase 2 `Connectivity`, M0.2, M0.3 merge) live in the implementation plan — this list is only what the client repos owe.
