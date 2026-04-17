# Phase 2: gRPC Performance Optimizations Plan

**Branch**: `feature/grpc`  
**PR**: [#498](https://github.com/nam20485/OdbDesign/pull/498)  
**Status**: ⏳ AWAITING APPROVAL  
**Prerequisites**: Phase 1 complete (thread safety, server config, keepalive) ✅

---

## Items Covered

| Item | Title | Priority | Estimated Effort |
|------|-------|----------|------------------|
| 2.1  | Enable Protobuf Arena in `.proto` files | 🟡 MEDIUM | Small (1-2 hours) |
| 1.3  | Protobuf Arena Allocation in streaming RPCs | 🔴 HIGH | Large (4-8 hours) |
| 2.2  | Compression Level Configuration (level-based, not zlib 1-9) | 🟡 MEDIUM | Small (1-2 hours) |

**Recommended execution order**: 2.1 → 1.3 → 2.2 (2.1 is a prerequisite for 1.3; 2.2 is independent)

---

## Item 2.1: Enable Protobuf Arena in `.proto` Files

### What
Add `option cc_enable_arenas = true;` to all `.proto` files and remove the stale `//option optimize_for = CODE_SIZE;` comment from each.

### Why
- **Prerequisite for item 1.3** — arena allocation in C++ code requires `cc_enable_arenas = true` in the proto definitions
- The existing `//option optimize_for = CODE_SIZE;` comments are misleading (they were already commented out, so `SPEED` is the default — which is correct)
- Cleanup reduces confusion for future developers

### Files to Modify (20+ `.proto` files)
All files under `OdbDesignLib/protoc/` and `OdbDesignServer/protoc/grpc/`:
- `attrlistfile.proto`, `color.proto`, `common.proto`, `component.proto`, `componentsfile.proto`
- `design.proto`, `enums.proto`, `featuresfile.proto`, `filearchive.proto`, `layerdirectory.proto`
- `matrixfile.proto`, `miscinfofile.proto`, `net.proto`, `netlistfile.proto`, `package.proto`
- `part.proto`, `pin.proto`, `pinconnection.proto`, `standardfontsfile.proto`, `stepdirectory.proto`
- `stephdrfile.proto`, `symbolname.proto`, `symbolsdirectory.proto`, `toolsfile.proto`, `via.proto`
- `service.proto` (gRPC service definition)

### Changes Per File
```proto
// BEFORE:
syntax = "proto3";
//option optimize_for = CODE_SIZE;

// AFTER:
syntax = "proto3";
option cc_enable_arenas = true;
```

### Impact
- Regenerated `.pb.h` / `.pb.cc` files will include arena support methods
- **No behavioral change** — arenas are enabled but not used until item 1.3
- **No client changes needed** — wire format is identical
- **Full rebuild required** after protoc regeneration

### Risk: LOW
- Wire-compatible change; existing tests should pass without modification
- May slightly increase generated code size (arena methods added)

### Acceptance Criteria
- [ ] All `.proto` files have `option cc_enable_arenas = true;`
- [ ] All stale `//option optimize_for = CODE_SIZE;` comments removed
- [ ] Full build succeeds (all 106+ targets)
- [ ] All existing tests pass

---

## Item 1.3: Protobuf Arena Allocation in Streaming RPCs

### What
Replace heap-allocated `to_protobuf()` calls in streaming RPC loops with arena-based allocation to eliminate per-message `malloc`/`free` overhead.

### Why
- Currently **every feature record** in a streaming response triggers a heap allocation via `std::unique_ptr<TPbMessage> to_protobuf()`
- For layers with 10,000+ features, this means 10,000+ `malloc`/`free` pairs per RPC call
- Arena allocation amortizes this: allocate one block, reuse for all messages, free once at end
- Expected **50-70% reduction** in allocation overhead, **20-30% faster** streaming

### Approach

#### Step 1: Add `to_protobuf_arena()` to `IProtoBuffable<T>`

Add an optional arena-based method to the template interface:

```cpp
// IProtoBuffable.h - Add new method alongside existing to_protobuf()
template<typename TPbMessage>
class IProtoBuffable : public Utils::IJsonable
{
public:
    virtual std::unique_ptr<TPbMessage> to_protobuf() const = 0;

    // Arena-based serialization: populate a pre-allocated message in-place
    // Default implementation delegates to to_protobuf() + Swap for backward compatibility
    virtual void to_protobuf(TPbMessage* pMessage) const
    {
        auto pTemp = to_protobuf();
        if (pTemp) pMessage->Swap(pTemp.get());
    }
    // ...
};
```

This keeps backward compatibility: classes that don't override `to_protobuf(TPbMessage*)` will fall back to heap allocation + swap.

#### Step 2: Implement `to_protobuf(TPbMessage*)` for Hot-Path Classes

Focus on classes used in streaming RPCs (highest allocation frequency):

| Class | Proto Message | Usage |
|-------|---------------|-------|
| `FeatureRecord` (in FeaturesFile) | `FeaturesFile::FeatureRecord` | `GetLayerFeaturesStream`, `GetLayerFeaturesBatchStream` |

The `FeatureRecord::to_protobuf(FeaturesFile::FeatureRecord* msg)` override should populate the pre-allocated message in-place without any `make_unique`/`new`.

#### Step 3: Update Streaming RPCs to Use Arenas

```cpp
// OdbDesignServiceImpl.cpp - GetLayerFeaturesStream
grpc::Status GetLayerFeaturesStream(...) {
    google::protobuf::Arena arena;

    for (const auto& featureRecord : featureRecords) {
        auto* msg = google::protobuf::Arena::CreateMessage<
            Odb::Lib::Protobuf::FeaturesFile::FeatureRecord>(&arena);

        featureRecord->to_protobuf(msg);  // In-place populate

        if (!writer->Write(*msg)) break;
    }
    // Arena deallocated once here — no per-iteration free
}
```

#### Step 4: Update Batch Streaming Similarly

Same pattern for `GetLayerFeaturesBatchStream` — create batch messages on the arena.

### Files to Modify

| File | Change |
|------|--------|
| `OdbDesignLib/IProtoBuffable.h` | Add `virtual void to_protobuf(TPbMessage*)` with default implementation |
| `OdbDesignLib/FileModel/Design/FeaturesFile.h` | Declare `FeatureRecord::to_protobuf(proto_msg*)` override |
| `OdbDesignLib/FileModel/Design/FeaturesFile.cpp` | Implement `FeatureRecord::to_protobuf(proto_msg*)` |
| `OdbDesignServer/Services/OdbDesignServiceImpl.cpp` | Use arena in `GetLayerFeaturesStream` and `GetLayerFeaturesBatchStream` |
| `OdbDesignServer/Services/OdbDesignServiceImpl.h` | Add `#include <google/protobuf/arena.h>` |

### Risk: MEDIUM
- Adding a virtual overload to `IProtoBuffable` is safe (default implementation preserves existing behavior)
- Arena messages have different lifetime rules — must not be referenced after arena destruction
- Hot-path change; needs careful testing with real design data

### Acceptance Criteria
- [ ] `IProtoBuffable` has arena-friendly `to_protobuf(TPbMessage*)` overload
- [ ] `FeatureRecord` has optimized in-place implementation
- [ ] `GetLayerFeaturesStream` uses arena allocation
- [ ] `GetLayerFeaturesBatchStream` uses arena allocation
- [ ] Full build succeeds
- [ ] All existing tests pass
- [ ] New test(s) verify arena-allocated messages serialize correctly

---

## Item 2.2: Compression Level Configuration

### What
Replace the boolean compression toggle with a string-based compression level config using gRPC's `grpc_compression_level` enum.

### gRPC C++ Compression API (Verified Against Headers)

The gRPC C++ API provides **two** server-side compression controls on `grpc::ServerBuilder`. They are **mutually exclusive** — `SetDefaultCompressionAlgorithm` overrides `SetDefaultCompressionLevel` when both are set.

| Method | Parameter Type | Purpose |
|--------|---------------|---------|
| `SetDefaultCompressionAlgorithm(algo)` | `grpc_compression_algorithm` | Forces a specific algorithm for all calls |
| `SetDefaultCompressionLevel(level)` | `grpc_compression_level` | Sets an abstract quality tier; gRPC selects the best algorithm based on what the peer accepts |

**`grpc_compression_algorithm` enum** (from `grpc/impl/compression_types.h`):

| Value | Constant | Meaning |
|-------|----------|---------|
| 0 | `GRPC_COMPRESS_NONE` | No compression |
| 1 | `GRPC_COMPRESS_DEFLATE` | Deflate |
| 2 | `GRPC_COMPRESS_GZIP` | Gzip |

**`grpc_compression_level` enum** (from `grpc/impl/compression_types.h`):

| Value | Constant | Meaning |
|-------|----------|---------|
| 0 | `GRPC_COMPRESS_LEVEL_NONE` | No compression |
| 1 | `GRPC_COMPRESS_LEVEL_LOW` | Favor speed over compression ratio |
| 2 | `GRPC_COMPRESS_LEVEL_MED` | Balanced |
| 3 | `GRPC_COMPRESS_LEVEL_HIGH` | Favor compression ratio over speed |

**Critical**: Raw zlib levels (1-9) are **not exposed** by the gRPC C++ public API. The internal zlib compression level is hardcoded to `Z_DEFAULT_COMPRESSION` (effectively level 6). The `grpc_compression_level` enum controls **algorithm selection priority** given the peer's accepted encodings — not the zlib compression ratio itself.

### Current State
The server calls `SetDefaultCompressionAlgorithm(GRPC_COMPRESS_GZIP)` with a `bool compression_enabled` toggle. This unconditionally forces gzip at zlib's default ratio. There is no level control.

### Recommended Approach

Switch from algorithm-based to **level-based** configuration. This is more flexible: it lets gRPC negotiate the best algorithm with each client based on the requested quality tier, rather than unconditionally forcing gzip.

**Config string → enum mapping**:

| Config Value | Maps To | Behavior |
|--------------|---------|----------|
| `"none"` | `GRPC_COMPRESS_LEVEL_NONE` | No compression (replaces `enabled: false`) |
| `"low"` | `GRPC_COMPRESS_LEVEL_LOW` | Light compression — favors speed |
| `"medium"` | `GRPC_COMPRESS_LEVEL_MED` | Balanced |
| `"high"` | `GRPC_COMPRESS_LEVEL_HIGH` | Best compression — favors ratio |

### Recommendation for Your Environment

Your deployment has two modes:
1. **On-prem / LAN** (2.5 GbE wired): bandwidth is abundant (~300 MB/s), latency is sub-millisecond
2. **Cloud / WAN** (clients across internet): bandwidth is limited (50-500 Mbps typical), latency is 10-100ms+

| Scenario | Recommended Level | Rationale |
|----------|-------------------|-----------|
| **LAN (2.5 GbE)** | `"none"` or `"low"` | At 2.5 Gbps, a 50 MB response transfers in ~160ms uncompressed. Compression CPU time (both server-side encode and client-side decode) likely exceeds the bandwidth savings — you're CPU-bound, not network-bound on a LAN this fast. |
| **Cloud / WAN** | `"high"` | Across the internet at typical 50-500 Mbps, compression cuts payload ~55-65%, directly reducing transfer time and cost. The CPU overhead is well worth it when bandwidth is the bottleneck. |
| **Mixed / Unknown** | `"medium"` | Safe middle ground — provides meaningful compression without excessive CPU overhead. |

**Default: `"high"`** — this is the safest production default because:
- Matches current behavior (the existing `SetDefaultCompressionAlgorithm(GRPC_COMPRESS_GZIP)` already applies full gzip)
- Most production deployments (on-prem k8s or cloud) will serve clients across the internet where compression matters most
- On-prem users with fast LANs can explicitly override to `"low"` or `"none"` in their `config.json`

### Implementation

#### Step 1: Replace `compression_enabled` in `GrpcServiceConfig`

```cpp
// GrpcServiceConfig.h — include the gRPC compression header
#include <grpc/impl/compression_types.h>

// Replace: bool compression_enabled = true;
// With:
grpc_compression_level compression_level = GRPC_COMPRESS_LEVEL_HIGH;
```

#### Step 2: Parse string from `config.json`

```json
{
  "grpc": {
    "compression": {
      "level": "high"
    }
  }
}
```

String-to-enum parsing with validation:
```cpp
// GrpcServiceConfig.cpp
static grpc_compression_level parseCompressionLevel(const std::string& value)
{
    if (value == "none")   return GRPC_COMPRESS_LEVEL_NONE;
    if (value == "low")    return GRPC_COMPRESS_LEVEL_LOW;
    if (value == "medium") return GRPC_COMPRESS_LEVEL_MED;
    if (value == "high")   return GRPC_COMPRESS_LEVEL_HIGH;
    // Unknown value → default to high (matches current gzip behavior)
    return GRPC_COMPRESS_LEVEL_HIGH;
}
```

#### Step 3: Replace `SetDefaultCompressionAlgorithm` with `SetDefaultCompressionLevel`

```cpp
// OdbDesignServerApp.cpp — replaces the existing compression block
if (loadResult.config->compression_level != GRPC_COMPRESS_LEVEL_NONE)
{
    builder.SetDefaultCompressionLevel(loadResult.config->compression_level);
    std::cout << "gRPC compression level: "
              << compressionLevelToString(loadResult.config->compression_level)
              << std::endl;
}
else
{
    std::cout << "gRPC compression: disabled" << std::endl;
}
```

#### Step 4: Remove `compression_enabled` boolean

The boolean becomes redundant — `level: "none"` means disabled. This simplifies the config surface from two fields to one.

### Files to Modify

| File | Change |
|------|--------|
| `OdbDesignServer/Config/GrpcServiceConfig.h` | Replace `bool compression_enabled` with `grpc_compression_level compression_level`; add `#include <grpc/impl/compression_types.h>` |
| `OdbDesignServer/Config/GrpcServiceConfig.cpp` | Add `parseCompressionLevel()` helper; parse `compression.level` string; remove `compression_enabled` parsing |
| `OdbDesignServer/OdbDesignServerApp.cpp` | Replace `SetDefaultCompressionAlgorithm(GRPC_COMPRESS_GZIP)` with `SetDefaultCompressionLevel()`; update startup log |
| `OdbDesignServer/config.json` | Replace `"compression": { "enabled": true }` with `"compression": { "level": "high" }` |
| `OdbDesignTests/GrpcServiceConfigTests.cpp` | Update tests: replace `compression_enabled` tests with level string parsing tests (`"none"`, `"low"`, `"medium"`, `"high"`, unknown fallback) |

### Risk: LOW
- Equivalent behavior for `"high"` (gzip at default ratio, same as current `GRPC_COMPRESS_GZIP`)
- Level-based approach is more idiomatic per gRPC documentation
- Simpler config surface (1 field instead of 2)
- **No client changes needed** — compression negotiation is transparent

### Acceptance Criteria
- [ ] `compression_level` uses `grpc_compression_level` enum type (not `int`, not 1-9)
- [ ] String config parsing: `"none"`, `"low"`, `"medium"`, `"high"` with unknown-value fallback to `"high"`
- [ ] `SetDefaultCompressionLevel()` used instead of `SetDefaultCompressionAlgorithm()`
- [ ] `compression_enabled` boolean removed (subsumed by `level: "none"`)
- [ ] Startup log shows human-readable compression level name
- [ ] Tests for all four level strings + unknown value fallback
- [ ] Full build succeeds
- [ ] All existing tests pass

---

## Implementation Order & Dependencies

```
2.1 Enable Arenas in .proto ──► 1.3 Arena Allocation in RPCs
                                        │
2.2 Compression Level Config ───────────┘ (independent, can be parallel)
```

**Recommended sequence**:
1. **2.1** first (small, prerequisite for 1.3)
2. **2.2** next (small, independent, quick win)
3. **1.3** last (largest scope, depends on 2.1)

## Total Estimated Effort

| Item | Effort | Risk |
|------|--------|------|
| 2.1  | 1-2 hours | LOW |
| 2.2  | 1-2 hours | LOW |
| 1.3  | 4-8 hours | MEDIUM |
| **Total** | **6-12 hours** | |

---

*Awaiting approval before proceeding with implementation.*
