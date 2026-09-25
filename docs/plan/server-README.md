# Server-Team Hand-off

**From:** OdbDesign 3D client team (this repo) · **Date:** 2026-09-10
**Purpose:** everything below is **server-repo responsibility** — collected here so
it can be handed over wholesale. The client team is not planning this work.

## Items (from `server-issues.md` unless noted)

| Item | What | Status |
|---|---|---|
| NI15 | Redoc page at `/redoc`, maximally annotated OpenAPI definitions | Open |
| NI16 | Publish generated coverage-report HTML to the GH Pages site on deploy, per environment branch (dev/staging/release…) | Open — **confirm target repo**: if it is *this* client repo's Pages site, it stays with us; the branch list suggests the server repo's environments |
| gRPC TLS | Secure the gRPC channel (currently plaintext `:50051` behind Tailscale) | Open — options in `grpc-auth-options-and-dev-plan.md` |
| Unified auth | One mechanism beyond HTTP Basic, covering gRPC + REST: per-user/per-resource authorization, interactive + device flow | Open — options in `grpc-auth-options-and-dev-plan.md` |
| Server-side optimizations | General review; specific idea: `RequestLoadDesign(Async)` that starts a file-archive load server-side and returns immediately, so a follow-up `GetDesign` for the same design is instant | Open |
| Alternative transport | Optional SSE/WebSocket channel, language-agnostic, with graceful client fallback (app must function when the channel is absent or fails) | Open, exploratory |
| gRPC perf phases 2–4 | Arena allocation, compression tuning, and the rest of the Feb optimization plan | Open — full file-by-file status in `server-grpc-optimization-remaining-work.md` (includes an unmerged `opt-phase2` branch that already implements S-1..S-3) |
| Matrix JSON default-field omission | `/filemodels/<design>/matrix/matrix` serializes via proto3-JSON, which omits default-valued fields: `type` is absent for SIGNAL layers even though `TYPE=SIGNAL` is authored in the raw matrix (verified on Turbot, sample_design, designodb_rigidflex); `context`/`polarity` never appear (0-valued). Optional client-anticipated fields `side`/`stackIndex`/`depthMm` are not in `matrixfile.proto` at all — `stackIndex` could be synthesized from row order; `depthMm` has no ODB++ source | Advisory — client self-heals (absent `type` ⇒ `Signal`) and treats the metadata as optional hints (NI22, `../plan/completed/physical-board-3d-view/plan.md`); always-print serialization + optional stack fields would improve the API for any consumer. **Update 2026-09-11:** the client now consumes `polarity` (feature + matrix layer, `dev/solid-board`) — the omission is harmless (absent ⇒ `Positive`), always-print would just make the JSON self-describing |
| **Turbot `GetDesign` returns corrupt protobuf (BLOCKER)** | Deployed server (post-#590+#591 image, pod 7d old at diagnosis) deterministically returns structurally invalid protobuf for `GetDesign(Turbot)` in **both** flavors (`include_normalized_lists` true and false). Client: `InvalidProtocolBufferException` ("invalid wire type" / "end-group tag did not match" / "completed reading a message while more data was available") → `Unavailable` after retries; load never starts. Server log (all deterministic, 4/4 attempts): cold path reports `ByteSizeLong()=103,824,384` — **exactly 99 MiB**, a truncation signature; the M1.4 cached-bytes path logs `Cached design bytes failed to parse for "Turbot"; falling back to full load` — the server cannot re-parse its own cached bytes either. Discriminators: `designodb_rigidflex` `GetDesign` = 102,393,582 bytes (natural size, not MiB-quantized) parses fine end-to-end — so neither transport, k3s LB, compression, nor the 250 MB limits are the cause; REST `/designs/Turbot` returns 176,455,937 bytes of valid JSON — the in-memory model is intact; no protobuf UTF-8 warnings (sanitizer active, ruling out the rigidflex CP1252 class). Suspect: `FileArchive::to_protobuf` / serialized-cache write path for Turbot-scale content produces a self-inconsistent tree (a field length that lies → quantized byte size + structurally invalid serialization). Diagnosed 2026-09-21, evidence: client logs `/tmp/r1/turbot-default-*.log` (session), server `kubectl logs` window 22:04–22:06 that day | **BLOCKER for Turbot via gRPC** (sample_design and rigidflex unaffected). Needs server-repo root-cause; suggested entry points: `ProductModel::Design::to_protobuf(bool)` → `FileModel::Design::FileArchive::to_protobuf`, `DesignCache` pre-serialization worker (`SerializeAsString`), and why `ByteSizeLong()` pins at exactly 99 MiB |

## Client-side touchpoints (for the server team to be aware of — not requirements)

We will provide the actual client-side requirements **as/when each item is
implemented**; this list only says where the client is affected so nothing lands
unannounced:

- **TLS / unified auth** — client gRPC channel construction (`GrpcChannelFactory`)
  and the REST `BasicAuthHandler` (env-provided Basic credentials today). Any
  scheme change needs a client config surface + credential flow; device flow
  would be new client UI.
- **`RequestLoadDesignAsync`** — a new client RPC call at load start (fire-and-forget
  or awaited); trivial to add when the API shape is fixed.
- **SSE/WebSocket transport** — the client transport layer sits behind
  `IOdbDesignClient`; a second channel is a client-side project, budget it with us.
- **Perf phases 2–4** — transparent to the client except message-size alignment
  (client is 150 MB, matching the server's 250 MB ceiling — already fine).

## Pointers

- Server repo: `nam20485/OdbDesign` (locally `/home/nam20485/src/github/nam20485/OdbDesign`; deployed in the k3s cluster, context `k3s-debian13vm`).
- Client transport backlog (our side): `docs/plan/opt/client-grpc-optimization-remaining-work.md`.
- Client integration notes / known-resolved issues: `AGENTS.md` in this repo.
