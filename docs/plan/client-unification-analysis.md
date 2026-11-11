# Client Unification Analysis — Info Client + 3D Viewer

- **Date:** 2026-11-07 (analysis) · 2026-11-08 (user feedback folded in; decisions locked)
- **Status:** Decided — locked decisions in §9; execution not started
- **Scope:** Architectural options for the two out-of-process client apps, `odbdesign-3d-client-prototype` (3D viewer) and `Odbdesign-info-client-india79-b` (info client), and their relationship to the planned `odbdesign-protoc` unified C++/C# contract library.
- **All facts below verified against the three checkouts this session** (2026-11-07). LOC excludes `obj/`, `bin/`, `.delta/worktrees/`.

---

## 1. Executive summary

**Recommendation (adopted 2026-11-08): Option 2 — phased shared-library extraction (Option 3 only until Phase 0 lands; Option 1 reviewed and retired — §9 decision 5).**

The codebase has already chosen this direction in miniature and just hasn't executed it: the info client's `src/OdbDesign.ProductModel` csproj explicitly declares itself *"Portable library intended for reuse across OdbDesign clients (eventually a NuGet package) … every consumer (info client, and later the 3D client) uses one product-model implementation"*, and `plan_docs/Cross-Probing-Design.md` (rev 2) already plans a second shared library, `OdbDesign.CrossProbing`. Option 2 is finishing a decision that is already written down, and it is the direct on-ramp to the `odbdesign-protoc` vision. Option 1 (merge the apps) is technically feasible — both are Avalonia 12 / .NET 10 — but it couples release trains and crash domains while leaving the actual hard part (the duplicated contract/product-model core) unsolved; it has been reviewed and **retired** (§9 decision 5). Option 2 is therefore the committed target architecture, its shape fixed by the §9 decisions: NuGet on GH Packages from day one, `odbdesign-protoc` as sole owner of the protos and all codegen, a proto-invisible interface contract as the **destination — staged, not day-one** (§9 decision 6: the transitional generated-type surface ships first and is retired slice by slice), the server repo as a consumer, and a C++ shared library to complete the vision.

---

## 2. Current state (verified facts)

### 2.1 The two apps

| | 3D viewer (`odbdesign-3d-client-prototype`) | Info client (`Odbdesign-info-client-india79-b`) |
|---|---|---|
| Framework | Avalonia 12.0.4, .NET 10, Ab4d.SharpEngine 4.0.9594 (Vulkan) | Avalonia 12.0.3, .NET 10, TreeDataGrid |
| Role | 3D board rendering from `Design.fileModel`; component inspector | 11 tab views: components, nets, stackup, drill tools, packages, parts, pins, vias, step hierarchy, symbols, EDA data |
| Transport | gRPC (7 of 8 RPCs) **+** REST for design/step/layer listing only (`RestDesignListService`, 472 LOC) | gRPC for **all** design reads (`DesignService`, 1,290 LOC); REST is a frozen 5-endpoint control plane (Refit, 162 LOC; decision D5) |
| Handwritten src | ~18.8K C# + 857 axaml | ~8.2K C# + 1,485 axaml |
| Tests | 88 files, ~1,010 tests, 24K LOC, 85% line-coverage CI gate | 18 files, 4.5K LOC + Testcontainers integration suite |
| CI | build+test matrix, **no publish workflow yet** (packaging plan still draft) | build+test 3-OS matrix **+ self-contained publish artifacts** (win/linux/osx-x64) |
| Maturity | prototype by name, production-shaped by practice | stable, shipped binaries |

Both apps are desktop Avalonia on the same .NET generation — **a merge would face process/release-coupling problems, not technology problems.**

### 2.2 Cross-linking today: cross-probing (half-built, currently inert)

The user-visible "cross-linking" feature is the cross-probe mechanism:

- Info client has `src/OdbDesignInfoClient.Services/CrossProbeService.cs` (193 LOC) — a **named-pipe client** to `"OdbDesignViewerPipe"` (hardcoded), duplex newline-delimited JSON: outbound `select`/`highlight` actions, inbound selection events.
- **The 3D viewer has no pipe server at all** (grep-verified: no `NamedPipeServerStream`, no IPC of any kind), and the info client's inbound `SelectionReceived` event has **zero subscribers**.
- So the linkage is **one-directional and inert end-to-end today** — a client talking to a pipe nobody serves.
- The rev-2 design (`plan_docs/Cross-Probing-Design.md`, status Proposed, 2026-09-14) specifies the real thing: viewer-hosted pipe server, versioned JSON envelope (`protocolVersion`, echo suppression, design-match gating), and — notably — **a shared `OdbDesign.CrossProbing` library owning the contract** (not yet created).
- No deep links, URL schemes, or `Process.Start` inter-launching exists in either app.

### 2.3 Shared / duplicated asset inventory

| Asset | 3D viewer | Info client | Notes |
|---|---|---|---|
| Vendored protos | 29 files, 1,382 LOC (`src/OdbDesign3DClient.Core/Protos/`) | 29 files, 1,382 LOC (`protoc/`) | **Byte-identical to each other and to the server canonical set today** (diff-verified this session). Sync is 100% manual. Grpc.Tools 2.80.0 codegen at build in both (≈62K LOC generated, not checked in). |
| `ComponentDetailBuilder` | 480 LOC | 453 LOC (`src/OdbDesign.ProductModel/`) | Near-twins. The **refDes case-sensitivity fix had to be applied twice** (documented in `component-connectivity-client-followups.md` F7a). |
| Conformance fixtures (M3.3) | `ConnectivityConformanceFixture` + capture tooling (394 LOC) | `ProductModelConformanceTests` (467 LOC) + golden fixture | Golden files **deliberately published into both repos** — same truth, two homes. |
| gRPC channel setup | `GrpcChannelFactory` (82 LOC): HTTP/2 keepalive, pooling, gzip, 250 MB ceiling | `ConnectionService` (301 LOC) | Info client carries a **documented TODO** for exactly the keepalive/pooling work the 3D client already solved (`docs/opt/client-grpc-optimization-handoff.md`). |
| Stream fallback + reassembly | `DesignStreamReassembler` (171 LOC, faithful port of server `DesignStreamWalker.cpp`) + `ResourceExhausted` fallback | **none — digests full monolithic `GetDesign` into client memory** | Server-side OOM work (`getdesign-streaming-oom-plan.md`, 10 GiB pod ceiling) means the info client **will** need this path for large designs — a port is otherwise inevitable. |
| REST auth | `BasicAuthHandler` (30 LOC) | `AuthHeaderHandler` (40 LOC) + `BasicAuthService` | Same `ODBDESIGN_REST_USERNAME/PASSWORD` convention in both, by agreement rather than by code. |
| Product-model reader | (renders directly from protobuf; `ComponentDetailBuilder` only) | `ProductModelReader` (204 LOC) | Only in info client. |

### 2.4 Drift state and history

- **Today: zero drift.** All three copies byte-identical; both clients synced within the last ~10 days (info PR #27, 3D PR #67).
- **Mechanism: none.** Sync is manual goodwill. The handoff docs record the info client's copy being **stale by all four then-current `service.proto` features** (missing `GetStandardFonts`, `RequestLoadDesign`, `LoadStatus`, `include_normalized_lists`). `AGENTS.md` carries a standing warning: *"verify a client's vendored `service.proto` before assuming it knows about a new RPC."* That warning existing at all is the cost of Option 3 speaking.

### 2.5 Forces already in motion

- **REST is frozen (decision D5); all design content reads are gRPC** in both clients. Any future sharing conversation is gRPC-shaped.
- **M2 auth** (Keycloak → JWT, `server-issues-implementation-plan.md`) will require token plumbing in **both** clients — auth handling is about to become a second duplicated subsystem if nothing is extracted.
- **`odbdesign-protoc` does not exist yet** (`gh repo view` 404, verified). The vision — central protos + C++ server service lib + C# client object-model/service-client libs consumed by all three apps — has no repo, no pipeline, no consumers.
- **3D PR #67 is still open** and contains the reassembler + freshest proto sync the SDK would lift.

---

## 3. Option 1 — Merge into one application

Combine both apps into a single Avalonia app retaining all functionality (grids + 3D in one workbench).

### Pros

- **Deletes the cross-probe problem.** The inert pipe, the entire rev-2 protocol spec, echo suppression, versioning — all collapse into an in-app event aggregator. Selection sync becomes a `WeakEventMessenger` subscription. This is the strongest argument for merging: the only genuinely hard shared-runtime problem the two apps have goes away.
- **Proto/data-model duplication eliminated *within* the merged app** — one vendored proto set, one `ComponentDetailBuilder`.
- Same stack (Avalonia 12 / .NET 10 both), so it is feasible; commercial EDA tools (Altium, KiCad) do ship grid + 3D in one workbench.
- One repo, one CI, one install for users.

### Cons

- **The hard part isn't the UI, it's the core — and merging doesn't extract it.** A merged app still has `MainViewModel` (3,101 LOC) + `MainWindow.axaml.cs` (1,554 LOC) next to 11 tab ViewModels; you would still want internal module boundaries, i.e. you end up building the Option-2 SDK *inside* the monolith first, under worse conditions.
- **Crash-domain and resource coupling.** A Vulkan/SharpEngine driver fault or a 3.3 GiB design load takes down data browsing with it; GPU-heavy sessions tax grid work.
- **Release-train coupling.** Info client has a working 3-OS self-contained publish pipeline; 3D client's packaging is still a draft plan. One train means the slowest cadence governs both, and an info-only fix ships a 3D binary.
- **Test-gate collision.** 3D's 85% line-coverage gate applied over a merged codebase including info's thinner coverage; ~28K combined test LOC to reconcile.
- **Workflow regression risk.** Two independent windows on two monitors is how EDA engineers actually work (layout next to BOM/property grids); a single-window app must reinstate multi-window — and then needs cross-window selection sync again, which looks a lot like the protocol you just deleted.
- **Abandons work mid-design:** rev-2 cross-probe spec + planned `OdbDesign.CrossProbing` lib, and reduces the `odbdesign-protoc` consumer set from three apps to two.
- **Large merge (~27K src LOC across two repos, two CIs) with zero immediate user-facing gain.**

### Verdict

**Rejected and retired** *(user decision 2026-11-08: "not considered a feasible approach in current form" — §9 decision 5)*. The analysis stands independently of that call: merging couples release and failure domains before the shared core exists and doesn't deliver the one thing that pays off in every scenario (extracted contracts + product model). The merged-workbench idea is off the roadmap in any form.

---

## 4. Option 2 — Refactor to shared libraries (phased)

Extract the duplicated contract/product-model/service-client core into shared libraries consumed by both apps — and, once the C++ half lands, by the **server** as well (§9 decision 3) — keeping the apps separate. Phased so each step is independently valuable and abortable.

### Pros

- **Finishes the direction the code already declares.** `OdbDesign.ProductModel`'s own description names the 3D client as its future consumer; `Cross-Probing-Design.md` already specifies `OdbDesign.CrossProbing` as a shared lib; M3.3 conformance fixtures already assume one shared golden truth. This is execution of a decision, not a new architecture.
- **Structurally eliminates proto drift** — the standing AGENTS.md trap — by replacing hand-synced copies with one codegen source. Compare: today's drift detection is "a human remembers to diff."
- **Retires measured double-maintenance:** refDes fix applied twice; info's keepalive TODO re-doing 3D's solved `GrpcChannelFactory`; conformance fixtures republished into two repos; the 171-LOC reassembler otherwise getting ported by hand when the info client meets large designs.
- **M2 auth implemented once** (token acquisition/refresh/headers in the SDK) instead of twice.
- **Direct on-ramp to `odbdesign-protoc`**: the C# SDK *is* the C# half of that vision; the repo, codegen pipeline, versioning and packaging created here are exactly what the C++ half would join.
- **Apps keep their cadences, gates, publish pipelines, and crash isolation.** No user-facing change, no release coupling.
- **The committed end state** (Option 1 retired): the phased SDK *is* the target architecture — C# NuGet first (Phase 1), C++ shared lib to complete it (Phase 3) — not a hedge against a future merge.

### Cons

- **Cross-repo choreography.** A proto change becomes: server repo → SDK sync/release → two client version bumps (versus today's "edit server, hand-copy twice," which is worse but *feels* lighter).
- **Versioning and packaging overhead.** Semver discipline, pinning policy, package hosting/auth in two CIs. Private-package billing is a live concern in this ecosystem (the 3D client's org already had an artifacts/packages billing saga); hosting choice must be deliberate (NuGet on GH Packages vs git submodule vs plain source consumption — see open questions).
- **One-time convergence cost.** The two `ComponentDetailBuilder` variants and two conformance suites must be reconciled into one implementation that passes both apps' expectations; ~4–5K LOC moves; both apps' PRs land in a coordinated window.
- **A shared lib can become a dumping ground** ("common") if boundaries aren't enforced — needs an explicit API surface rule (transport + contract + product model + cross-probe contract; *not* ViewModels, not rendering).

### Phased roadmap

| Phase | Content | Size / cost | Value even if we stop after |
|---|---|---|---|
| **0. Drift tripwire** | CI check in each client repo: diff vendored protos against the server repo at a pinned ref; fail on drift. Same pattern as `swagger-spec-configmap-sync.yml` in the server repo. | Hours. No architecture change. | Silent drift becomes loud drift — the single cheapest risk reduction available. |
| **1. C# client SDK** (`odbdesign-protoc` repo, C# half) | Create the repo and **move the protos in — it becomes the canonical, sole owner of all protoc messages + service definitions and all codegen** (§9 decision 3). C# half: `OdbDesign.Client` (channel factory, `OdbDesignGrpcClient`, reassembler, capability probes, auth handlers + env conventions, health) + `OdbDesign.ProductModel` moved from info repo (reader + converged `ComponentDetailBuilder` + conformance fixtures + capture tooling), **published to NuGet on GH Packages from day one** (§9 decision 1). **The SDK also publishes the generated contract types as a transitional public surface, namespaces unchanged** (`Odb.Lib.Protobuf.*`, same service client — §9 decision 6), so each client cutover is "delete local protos + Grpc.Tools, add PackageReference" — **zero renderer/converter churn**; the proto-free object model ships only where it already exists (ProductModel/Connectivity). Both clients consume the package and delete their vendored proto dirs. The server repo keeps a **vendored, tripwire-guarded copy** of the protos until Phase 3 replaces its gencode (it cannot consume the C++ half yet). | ~4–5K LOC extracted/converged (unchanged — decision 6 keeps object-model expansion out of the first cut); 1–2 weeks agent-assisted across 3 coordinated PRs (SDK repo + 2 client cutovers). | Duplication gone; auth once; streaming available to both; `odbdesign-protoc` exists with its first two consumers and owns the contract. |
| **2. `OdbDesign.CrossProbing`** | Implement per rev-2 design: pipe contract, versioned envelope, echo suppression; viewer-side pipe server + wire the existing info-side client (give `SelectionReceived` subscribers). | Days–1 week. | The cross-linking feature actually works end-to-end for the first time. |
| **3. C++ half of `odbdesign-protoc`** (**committed** — no longer optional; §9 decisions 1 & 3) | C++ **shared library** built from the canonical protos, **initially exposing the generated message/service types** (§9 decision 6): the server's code keeps compiling against the same types it uses today — new provenance, not a new API — and **deletes `OdbDesignLib/protoc/` + its own gencode**. Consumed by the server as a **vcpkg port from a registry hosted inside the SDK repo** (`vcpkg-configuration.json` + baseline pin — no submodules, §9 decision 7). Proto-invisible C++ headers follow later as Phase 4 slices. | Build/packaging migration, not an API migration (risks in §8). | Single codegen source across all three apps; the `odbdesign-protoc` vision functionally complete; the server repo is a pure consumer with minimal regression surface. |
| **4. Object-model expansion & proto-type retirement** (staged, driver-based — §9 decision 6) | Expand the SDK's proto-free object model slice by slice (the 3D renderers' features/geometry surface is the largest), each slice deleting the corresponding transitional generated-type exposure. Ratchet: consumer references to the transitional surface never increase — enforced by a compiled architecture test (e.g. NetArchTest on assembly reference graphs against a frozen, shrinking allowlist). | Slices ride feature work (M2 auth, cross-probe rev-2, future viewer work); no big-bang pass. | Full §9-decision-3 destination: protobuf invisible behind the interface contract, everywhere. |

### Verdict

**Adopt.** Highest ratio of standing benefit to risk, aligned with every documented plan, prerequisite for both competing end states, and executable in abortable phases.

---

## 5. Option 3 — Leave as-is

### Pros

- Zero cost and zero coordination today; both apps green; protos currently byte-identical.
- Prototypes keep maximum velocity; no versioning discipline required of anyone.

### Cons

- **The drift mechanism is unchanged** — manual goodwill, historically failed (4-feature gap), detected only by a human remembering the AGENTS.md warning.
- **Every upcoming item pays the duplication tax twice:** cross-probe rev-2 contract (implemented twice with no shared owner — the exact conditions that produced proto drift); M2 auth plumbing ×2; info-client streaming path (hand-port of the reassembler); fixture republication ×2; keepalive/pooling re-implementation.
- **The cited cross-linking feature stays inert** — status quo doesn't maintain it, it freezes it half-built (client-side pipe with no server, events with no subscribers).
- Contradicts the codebase's own declared direction (§2.5) at zero savings.

### Verdict

**Reject as a strategy; acceptable only as a holding pattern until Phase 0 lands** (which is deliberately hours of work, so the holding pattern should be short).

---

## 6. Comparison matrix

| Criterion | 1. Merge apps | 2. Shared libs (phased) | 3. Status quo |
|---|---|---|---|
| Eliminates proto drift | Within app only | **Yes, structurally** | No |
| Kills double maintenance | Yes (by deletion) | **Yes** | No — compounds |
| Delivers working cross-probe | By deletion (in-app events) | Yes (Phase 2, shared contract) | No (stays inert) |
| Release independence | ✗ coupled | **✓** | ✓ |
| Crash isolation | ✗ shared | **✓** | ✓ |
| Upfront cost | High (~27K LOC merge, CI consolidation, gates) | Medium, phased (Phase 0: hours; Phase 1: 1–2 wks) | Zero |
| Ongoing cost | Medium (one big train) | Low-medium (version bumps) | High (×2 everything) |
| `odbdesign-protoc` alignment | Partial (3→2 consumers) | **Direct on-ramp (all phases)** | None |
| User-facing gain | One install | None (by design) | None |
| No-regret property | ✗ | **✓ — committed target architecture** | ✗ |

---

## 7. Recommendation and rationale

**Option 2, phased — starting with Phase 0 immediately.**

1. **The evidence is already in the repos.** A library explicitly named for cross-client reuse exists (`OdbDesign.ProductModel`), a second shared lib is specified (`OdbDesign.CrossProbing`), golden fixtures are deliberately mirrored into both test suites, and the refDes fix was applied twice by hand. The system has voted; only the extraction is missing.
2. **Duplication cost is measured, not hypothetical** (§2.3), and every scheduled workstream (M2 auth, info-client streaming, cross-probe rev-2, M6.6-era large-design handling) multiplies it.
3. **Option 1's one genuine prize — killing the cross-probe protocol — is achievable without merging** (Phase 2), while its costs (release coupling, crash coupling, gate collision, workflow regression) are permanent.
4. **Option 2 is the end state, not a hedge:** with Option 1 retired, the phased SDK is the committed target architecture — C# NuGet first (Phase 1), C++ shared lib to complete it (Phase 3).
5. **Sequencing guards:** land 3D PR #67 first (it carries the reassembler and freshest proto sync Phase 1 lifts); do Phase 0 in parallel — it is independent and takes hours.

---

## 8. Risks & mitigations (Option 2)

| Risk | Mitigation |
|---|---|
| SDK becomes an unbounded "common" dumping ground | Explicit surface rule in the repo README: transport, contract, product model, cross-probe contract. ViewModels/rendering/UI never enter. Code-review gate. |
| Version-skew choreography annoys (SDK bump ×2 clients) | Phase 0 tripwire generalized to watch SDK↔server; clients pin exact versions; breaking proto changes are already rare (contract settled 2026-10-29). |
| Packaging/billing complexity (private NuGet auth in 2 CIs, GH Packages costs) | **Decided: private NuGet packages on GH Packages from day one** (§9 decisions 1 & 8). Budget: ~2–5 MB per version across the two packages; the 500 MB Free-plan quota ≈ 100–250 versions — a scheduled pruning job (REST `DELETE` package-version; keep last N + anything a consumer still pins) keeps storage flat indefinitely, and an account with no payment method/spending limit hard-caps at the included quota rather than silently billing overage. Cross-account consumption: nam20485-owned private packages ← intel-agency CI via a classic PAT (`read:packages` — fine-grained PATs don't work on this feed) stored as an intel-agency secret, plus a package-access grant in package settings. Deterministic versioning (semver + CI run number). |
| Server-side C++ migration (Phase 3): protobuf runtime version skew between shared lib and server; dual descriptor pools already caused one release-only SIGABRT | Build the shared lib against the same vcpkg baseline as the server (`4f6d4ae8`, protobuf 6.33.4#2); dynamic protobuf runtime on both sides (the `linux-dynamic-*` lesson); a round-trip contract test in server CI before the old gencode is deleted. |
| Canonical moves before the server can consume it (Phase 1 → 3 gap) | Invert the Phase 0 tripwire at Phase 1: every consumer repo — **including the server until Phase 3** — carries a vendored copy diffed against the SDK repo's canonical set, failing CI on drift. |
| Convergence conflicts between the two `ComponentDetailBuilder` variants | Both conformance suites (3D's + info's, incl. golden fixtures) become the SDK's gate; the merged implementation must pass both unchanged before either app cutover merges. |
| Coordinated-cutover window stalls | Cutovers are per-app and independent (SDK additive first, then app switchover PRs); no lockstep merge required. |
| Transitional proto-typed surface invites new dependencies — the Phase 4 migration stalls and never completes | Ratchet enforced by a compiled architecture test (e.g. NetArchTest asserting consumer assemblies reference no transitional SDK namespaces beyond a frozen allowlist); the allowlist only ever shrinks; each slice is scheduled onto feature work that already touches that area. |

---

## 9. Decisions (user feedback, locked 2026-11-08)

1. **Distribution: NuGet immediately.** The C# SDK ships as NuGet packages on GH Packages from day one — not deferred to a third consumer. The **C++ deliverable must be a shared library** (not static, not header-only, not source-inclusion).
2. **Repo placement: `nam20485/odbdesign-protoc`** (name free — verified). Cross-account consequence (owners verified this session): `intel-agency/odbdesign-3d-client-prototype` consumes a package owned by `nam20485` — explicit package-access grant + CI credential required.
3. **Canonical proto home: the new repo owns all protoc messages and service definitions and is solely responsible for code generation.** The server repo is a *consumer* of the SDK — it consumes the protoc messages and service definitions from the SDK, exactly as the clients do. **Consumers will not even know the object model and services were created from protos** — protobuf is an implementation detail hidden behind the SDK's interface contract. Consequences: protos move in Phase 1; the server's C++ consumption lands with the Phase 3 shared lib; until then the server carries a tripwired vendored copy.
4. **Timing: as proposed** — Phase 1 lands after 3D PR #67 and before M2 auth, so auth is implemented once, in the SDK.
5. **Option 1 (merged workbench): retired.** Not considered a feasible approach in current form; removed from the roadmap.
6. **Proto-invisibility is staged, not day-one** *(round-2 feedback, 2026-11-08 — resolves the former "reach of the proto-invisible surface" open question)*. The destination stands (decision 3), but the initial implementation **ships the generated contract types as a transitional public surface** (namespaces unchanged) — the 3D client keeps consuming proto-generated objects directly in Phase 1, and the server's C++ shared lib (Phase 3) initially exposes the generated types too. Wrappers replace the transitional surface **slice by slice** in Phase 4, with a ratchet: the transitional surface never grows. Rationale: the info client's object model covers only the Connectivity/design-summary slice (~840 LOC) — a day-one invisible surface would require mirroring the whole fileModel read surface *and* re-typing the 3D render pipeline (the most perf/memory-sensitive code in either app), roughly tripling the first cut to 10–15K+ LOC with a perf re-validation workstream. Staging keeps Phase 1 at ~4–5K LOC, leaves the renderer untouched, and makes Phase 3 a provenance swap rather than an API migration. What staging does **not** give up: contract ownership and drift elimination come from single-source codegen, not from invisibility — the SDK publishes the generated types themselves, so there is still exactly one codegen source from day one.
7. **C++ delivery: vcpkg registry — no submodules** *(round-3 feedback, 2026-11-08)*. The SDK repo hosts its own vcpkg registry (`registry/` dir + portfile building from the repo's release tags); the server consumes via `vcpkg-configuration.json` + baseline pin. Why this beats the alternatives: a vcpkg port rides the **existing vcpkg binary cache** (GH Packages feed) so the ~60K LOC gencode compiles once per SDK version, not in every CI run — FetchContent recompiles it every run and CI deliberately has no compiler cache; submodules are vetoed; prebuilt release binaries lose to source-building on cross-platform shared-lib compatibility. Side benefit: the SDK builds inside the server's own vcpkg toolchain/triplet world (x64-linux-dynamic / x64-windows), which structurally guarantees the protobuf-runtime alignment this plan already demands (§8). FetchContent at a pinned tag remains acceptable as a Phase 3 bootstrap before the port exists.

8. **Packages stay private** *(round-4 feedback, 2026-11-08 — closes the final open question)*. Affordability confirmed: ~2–5 MB per version across the two packages against the 500 MB Free-plan quota ≈ 100–250 versions of headroom, and REST version pruning (keep last N + anything a consumer still pins) keeps it flat for as long as needed. The wire contract stays unexposed, consistent with the pre-launch commercialization posture. If usage ever approaches the cap anyway, flipping visibility public later remains a one-settings-click fallback — billing follows current visibility, not history. Consequence: the cross-account credential (nam20485 classic PAT `read:packages` + package-access grant for intel-agency CI, §8) is on the Phase 1 critical path.

**No open questions remain** — all resolved through round 4 (2026-11-08). The plan is execution-ready per the §4 roadmap.
