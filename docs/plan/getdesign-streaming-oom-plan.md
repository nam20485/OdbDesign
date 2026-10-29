# GetDesign OOM — durable fix: streaming + disk-backed cache (DRAFT, 2026-10-24)

| | |
|---|---|
| Status | DRAFT — design captured for later execution ("at some point"). Not scheduled; not approved to build yet. |
| Problem | gRPC `GetDesign` OOMs the 10 Gi pod on the two large designs: `200-40628` measured ~10.1 GiB peak, `350-41017` estimated ~12.6 GiB (serialized `Design` = 1.34 GB). Node is 16 GB with ~6 Gi monitoring — **no capacity increase available** (and none wanted). |
| Constraint | REST is frozen (D5) and Crow 1.3.3 has no chunked/SSE anyway — **gRPC is the only sanctioned streaming surface**. Parse alone already fits (both designs load post-#600); it is the *serialization/serve* path that blows the budget. |

## Why it OOMs — the multiplier stack

Serving one big `GetDesign` today holds, simultaneously:

1. Parsed `FileArchive` (C++ object graph, ~2–3× archive size)
2. `ProductModel::Design` (built on top)
3. The **full protobuf `Design` message tree** (~2–4× wire size as C++ objects)
4. Serialized bytes (~1.34 GB for 350-41017)
5. The response-cache copy of the default flavour (**another full bytes copy**, D4/M2.3)

Items 3–5 exist only because the path materializes one monolithic message. That's the bug
shape: ~5–8× payload multiplier on a 1.34 GB payload. The durable fix is to stop building
items 3–5 monolithically — not to add RAM.

## Options considered

| Option | Verdict |
|---|---|
| **A. Server-streaming `GetDesignStream` + chunked construction** — build each chunk's submessage directly from the product model, write it to the wire, drop it; **never materialize the monolithic pb `Design`** | **RECOMMENDED — this is the fix.** Peak = items 1–2 + one chunk (~MBs). gRPC server-streaming is native; both clients already do streaming (3D client has streaming calls + `StreamTimeoutSeconds`). |
| B. Disk-backed cache: serialized stream written to a temp file on first fill, served by **mmap slices** — RAM cost becomes *page cache* (kernel-reclaimable under pressure) instead of anonymous heap | **Complement to A** — replaces item 5 entirely and gives the warm path zero-copy chunk serving. Not sufficient alone (item 3 still kills it). |
| C. Scoped/field-masked GetDesign (connectivity-only, no-geometry views) | Complementary for info-client-shaped reads; the 3D client needs full geometry eventually — doesn't remove the OOM. Cheap follow-on later, not the fix. |
| D. Cache size cap / eviction for huge designs | Needed regardless (a 1.34 GB heap-resident cached copy is indefensible on this node); subsumed by B. |
| E. More RAM / bigger node | Excluded by decision. |

## Recommended design (A+B)

### M6.1 — wire contract
`service.proto`: add `rpc GetDesignStream(GetDesignRequest) returns (stream DesignChunk)`.
`DesignChunk` = `oneof { Design header; ComponentBatch components; NetBatch nets; PackageBatch packages; ConnectivityBatch connectivity; FileModelBatch file_model_part; … }`, ordered so that **concatenating chunk payloads reproduces the exact monolithic `Design` bytes**. That ordering constraint is the correctness contract (below).

### M6.2 — server
Iterate the product model, emit batches (e.g. 1k components / one layer's features per chunk), constructing each batch's submessage fresh and releasing it after write. gRPC backpressure (slow client ⇒ server waits, not buffers). Dispatch rule: keep monolithic `GetDesign` for requests under a size threshold (estimated from model counts); over threshold, streaming only. Monolithic path also gets a hard guard (reject if estimate > budget) so the OOM becomes a clean `RESOURCE_EXHAUSTED` instead of a pod kill.

### M6.3 — cache (replaces the D4 hot-path for big designs)
First fill streams to the wire *and* to a temp file simultaneously; subsequent hits mmap the file and serve chunks as zero-copy slices. RAM = page cache (evictable), anonymous heap ≈ 0. Small designs keep the existing in-RAM cache unchanged.

### M6.4 — clients (F6-style fan-out)
Push updated protos to both clients; 3D client reassembles on read (streaming consume, existing timeout config), info client likewise. `FixtureCapture` gains `--stream` and can then capture real `connectivity.binpb` for the two large designs — retiring the test-only materializer stopgap from 3D PR #65.

### M6.5 — gates
- **Wire-equivalence:** reassembled stream == monolithic `GetDesign` bytes, byte-for-byte, on `sample_design` (the golden makes this exact).
- **Memory:** peak pod RSS ≤ ~4 GiB serving both large designs (measure, don't estimate).
- **Perf:** cold-stream time within ~1.5× monolithic on `sample_design`; warm (mmap) ≤ monolithic.
- Conformance/goldens unchanged (same bytes ⇒ same differentials).
- REST untouched (its default already clips fileModel; D5 freeze respected).

### Sequencing
Independent of all live work; best cut after the F4/F5 merges so the clients are stable
consumers first. Single server branch M6.1→M6.3 (PR), then client push-PRs (M6.4), then M6.5
benchmark evidence closes it. The size-threshold guard (M6.2 tail) can ship first as a
one-line stopgap if the OOM bites before the full build lands.

## M6.1 concrete chunk design (settled 2026-10-29, follows the FeatureRecordBatch precedent in service.proto:122)

`DesignChunk { oneof payload }`, emitted in this order; reassembly = assign scalars once, append repeated/map entries, place fileModel parts:

1. `DesignHeader` — productModel, name (+ the bounded FileArchive scalars: miscInfoFile, matrixFile, standardFontsFile, miscAttrListFile, symbolsDirectoriesByName, productName, fileName).
2. `StepSkeleton` — one per step (map key carried alongside): the full `StepDirectory` EXCEPT each layer's `FeaturesFile.repeated FeatureRecord features` (which is the 1.34 GB bulk). StepHdr/edaData/netlist/components etc. ride here; if a single step's eda-data proves unbounded in practice (> ~64 MB serialized), split it as a follow-up kind rather than widening this design now.
3. `FeatureBatch` — (stepName, layerName, repeated FeatureRecord): append into the addressed layer's features list; batch size target ~1–2k records. This mirrors `GetLayerFeaturesBatchStream` exactly.
4. `NetBatch` / `PackageBatch` / `ComponentBatch` / `PartBatch` — append to the Design-level repeated fields (only kinds actually present).
5. `ConnectivityBatch` — components then nets of the derived Connectivity message.
6. `NormalizedMapBatch` (only when `include_normalized_lists`): netsByName / packagesByName / componentsByName / partsByName map entries.

Gates unchanged (§M6.5); wire-equivalence checked by reassembling and comparing serialized bytes against the monolithic serialization on `sample_design`.
