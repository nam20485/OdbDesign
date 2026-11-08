# Client Unification Analysis — Info Client + 3D Viewer

- **Date:** 2026-11-07
- **Status:** Proposed — awaiting free-form feedback before any execution
- **Scope:** Architectural options for the two out-of-process client apps, `odbdesign-3d-client-prototype` (3D viewer) and `Odbdesign-info-client-india79-b` (info client), and their relationship to the planned `odbdesign-protoc` unified C++/C# contract library.
- **All facts below verified against the three checkouts this session** (2026-11-07). LOC excludes `obj/`, `bin/`, `.delta/worktrees/`.

---

## 1. Executive summary

**Recommendation: Option 2 — phased shared-library extraction (Option 3 only until Phase 0 lands; Option 1 rejected for now, not forever).**

The codebase has already chosen this direction in miniature and just hasn't executed it: the info client's `src/OdbDesign.ProductModel` csproj explicitly declares itself *"Portable library intended for reuse across OdbDesign clients (eventually a NuGet package) … every consumer (info client, and later the 3D client) uses one product-model implementation"*, and `plan_docs/Cross-Probing-Design.md` (rev 2) already plans a second shared library, `OdbDesign.CrossProbing`. Option 2 is finishing a decision that is already written down, and it is the direct on-ramp to the `odbdesign-protoc` vision. Option 1 (merge the apps) is technically feasible — both are Avalonia 12 / .NET 10 — but it couples release trains and crash domains while leaving the actual hard part (the duplicated contract/product-model core) unsolved; if a merged "workbench" app is ever wanted, the SDK from Option 2 is precisely the core it would be built on. Option 2 is therefore the no-regret move.

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

**Reject for now.** Feasible but premature: it couples release and failure domains before the shared core exists, and it doesn't deliver the one thing that pays off in every scenario (extracted contracts + product model). Keep a merged "workbench" app as a *possible* post-SDK end state — after Option 2, a merge becomes mostly a UI-shell project instead of an architecture project.

---

## 4. Option 2 — Refactor to shared libraries (phased)

Extract the duplicated contract/product-model/service-client core into shared libraries consumed by both apps, keeping the apps separate. Phased so each step is independently valuable and abortable.

### Pros

- **Finishes the direction the code already declares.** `OdbDesign.ProductModel`'s own description names the 3D client as its future consumer; `Cross-Probing-Design.md` already specifies `OdbDesign.CrossProbing` as a shared lib; M3.3 conformance fixtures already assume one shared golden truth. This is execution of a decision, not a new architecture.
- **Structurally eliminates proto drift** — the standing AGENTS.md trap — by replacing hand-synced copies with one codegen source. Compare: today's drift detection is "a human remembers to diff."
- **Retires measured double-maintenance:** refDes fix applied twice; info's keepalive TODO re-doing 3D's solved `GrpcChannelFactory`; conformance fixtures republished into two repos; the 171-LOC reassembler otherwise getting ported by hand when the info client meets large designs.
- **M2 auth implemented once** (token acquisition/refresh/headers in the SDK) instead of twice.
- **Direct on-ramp to `odbdesign-protoc`**: the C# SDK *is* the C# half of that vision; the repo, codegen pipeline, versioning and packaging created here are exactly what the C++ half would join.
- **Apps keep their cadences, gates, publish pipelines, and crash isolation.** No user-facing change, no release coupling.
- **No-regret:** if the apps are ever merged (Option 1 later), the SDK is the core the merged app is built on; if they never merge, the SDK pays off continuously. It is the only option that is a prerequisite of both others.

### Cons

- **Cross-repo choreography.** A proto change becomes: server repo → SDK sync/release → two client version bumps (versus today's "edit server, hand-copy twice," which is worse but *feels* lighter).
- **Versioning and packaging overhead.** Semver discipline, pinning policy, package hosting/auth in two CIs. Private-package billing is a live concern in this ecosystem (the 3D client's org already had an artifacts/packages billing saga); hosting choice must be deliberate (NuGet on GH Packages vs git submodule vs plain source consumption — see open questions).
- **One-time convergence cost.** The two `ComponentDetailBuilder` variants and two conformance suites must be reconciled into one implementation that passes both apps' expectations; ~4–5K LOC moves; both apps' PRs land in a coordinated window.
- **A shared lib can become a dumping ground** ("common") if boundaries aren't enforced — needs an explicit API surface rule (transport + contract + product model + cross-probe contract; *not* ViewModels, not rendering).

### Phased roadmap

| Phase | Content | Size / cost | Value even if we stop after |
|---|---|---|---|
| **0. Drift tripwire** | CI check in each client repo: diff vendored protos against the server repo at a pinned ref; fail on drift. Same pattern as `swagger-spec-configmap-sync.yml` in the server repo. | Hours. No architecture change. | Silent drift becomes loud drift — the single cheapest risk reduction available. |
| **1. C# client SDK** (`odbdesign-protoc` repo, C# half) | Canonical proto sync + codegen; `OdbDesign.Client` (channel factory, `OdbDesignGrpcClient`, reassembler, capability probes, auth handlers + env conventions, health); `OdbDesign.ProductModel` moved from info repo (reader + converged `ComponentDetailBuilder` + conformance fixtures + capture tooling). Both apps cut over. | ~4–5K LOC extracted/converged; 1–2 weeks agent-assisted across 3 coordinated PRs (SDK repo + 2 client cutovers). | Duplication gone; auth once; streaming available to both; `odbdesign-protoc` exists with its first two consumers. |
| **2. `OdbDesign.CrossProbing`** | Implement per rev-2 design: pipe contract, versioned envelope, echo suppression; viewer-side pipe server + wire the existing info-side client (give `SelectionReceived` subscribers). | Days–1 week. | The cross-linking feature actually works end-to-end for the first time. |
| **3. C++ half of `odbdesign-protoc`** (optional, driver-based) | Server consumes generated C++ messages/service base from the central repo (vcpkg registry or submodule). | Larger; touches the healthy server build. | Only when a real driver exists (second C++ consumer, commercialization packaging). Do **not** do this speculatively. |

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
| No-regret property | ✗ | **✓ (prerequisite of both other end states)** | ✗ |

---

## 7. Recommendation and rationale

**Option 2, phased — starting with Phase 0 immediately.**

1. **The evidence is already in the repos.** A library explicitly named for cross-client reuse exists (`OdbDesign.ProductModel`), a second shared lib is specified (`OdbDesign.CrossProbing`), golden fixtures are deliberately mirrored into both test suites, and the refDes fix was applied twice by hand. The system has voted; only the extraction is missing.
2. **Duplication cost is measured, not hypothetical** (§2.3), and every scheduled workstream (M2 auth, info-client streaming, cross-probe rev-2, M6.6-era large-design handling) multiplies it.
3. **Option 1's one genuine prize — killing the cross-probe protocol — is achievable without merging** (Phase 2), while its costs (release coupling, crash coupling, gate collision, workflow regression) are permanent.
4. **Option 2 is the only no-regret move:** it is the C# half of `odbdesign-protoc` *and* the facilitator of any future merged workbench. The other two options each foreclose something; this one forecloses nothing.
5. **Sequencing guards:** land 3D PR #67 first (it carries the reassembler and freshest proto sync Phase 1 lifts); do Phase 0 in parallel — it is independent and takes hours.

---

## 8. Risks & mitigations (Option 2)

| Risk | Mitigation |
|---|---|
| SDK becomes an unbounded "common" dumping ground | Explicit surface rule in the repo README: transport, contract, product model, cross-probe contract. ViewModels/rendering/UI never enter. Code-review gate. |
| Version-skew choreography annoys (SDK bump ×2 clients) | Phase 0 tripwire generalized to watch SDK↔server; clients pin exact versions; breaking proto changes are already rare (contract settled 2026-10-29). |
| Packaging/billing complexity (private NuGet auth in 2 CIs, GH Packages costs) | Start with source-level consumption (git submodule or repo-relative project reference) — zero infrastructure; introduce NuGet only when a third consumer or commercialization demands it. Decide explicitly at Phase 1 kickoff (open question 1). |
| Convergence conflicts between the two `ComponentDetailBuilder` variants | Both conformance suites (3D's + info's, incl. golden fixtures) become the SDK's gate; the merged implementation must pass both unchanged before either app cutover merges. |
| Coordinated-cutover window stalls | Cutovers are per-app and independent (SDK additive first, then app switchover PRs); no lockstep merge required. |

---

## 9. Open questions (decide at Phase 1 kickoff)

1. **Distribution:** NuGet on GH Packages vs git submodule vs source inclusion — bearing in mind private-package billing/auth history (3D client org) and that only two consumers exist today. *Lean: submodule/source now, NuGet at third consumer.*
2. **Repo placement:** personal `nam20485/odbdesign-protoc` (name is free — verified), or hosted alongside whichever account owns the long-term product.
3. **Canonical proto home:** keep protos canonical in the OdbDesign server repo (SDK syncs from it, tripwire-enforced) vs move to `odbdesign-protoc` and have the server consume them. *Lean: server stays canonical until Phase 3 — the C++ build owns them today and shouldn't be destabilized speculatively.*
4. **Timing vs in-flight work:** 3D PR #67 merge, M2 auth start, M6.6 — Phase 1 slots cleanly after #67 and before M2 (so auth lands once, in the SDK).
5. **Long-term end state:** keep the merged "workbench" app (Option 1) on the roadmap as a post-SDK possibility, or retire the idea?
