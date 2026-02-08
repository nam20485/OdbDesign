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
| 2.2  | Compression Level Configuration | 🟡 MEDIUM | Small (1-2 hours) |

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
Add configurable compression level (1-9) to the gRPC server, controllable via `config.json`.

### Why
- Compression is already enabled (GZIP, from initial fix), but uses the default level (6)
- Different environments benefit from different levels:
  - **LAN**: Level 1-3 → faster, lower CPU, still decent compression
  - **WAN**: Level 7-9 → better compression, higher CPU
  - **Balanced**: Level 6 → current implicit default

### Approach

#### Step 1: Add `compression_level` to `GrpcServiceConfig`

```cpp
// GrpcServiceConfig.h
int compression_level = 6;  // gzip level 1-9, default 6

// Clamping: min=1, max=9
```

#### Step 2: Parse from `config.json`

```json
{
  "grpc": {
    "compression": {
      "enabled": true,
      "level": 6
    }
  }
}
```

#### Step 3: Apply in `OdbDesignServerApp.cpp`

```cpp
if (loadResult.config->compression_enabled)
{
    builder.SetDefaultCompressionAlgorithm(GRPC_COMPRESS_GZIP);
    builder.SetDefaultCompressionLevel(
        static_cast<grpc_compression_level>(loadResult.config->compression_level));
    std::cout << "gRPC compression: gzip level "
              << loadResult.config->compression_level << std::endl;
}
```

> **Note**: The gRPC C++ API uses `grpc_compression_level` enum (NONE=0, LOW=1, MED=2, HIGH=3) rather than raw 1-9 zlib levels. The implementation needs to map the user-facing 1-9 level to the appropriate `grpc_compression_level` value, or use `SetDefaultCompressionAlgorithm` with channel arguments for finer control. This needs verification during implementation.

### Files to Modify

| File | Change |
|------|--------|
| `OdbDesignServer/Config/GrpcServiceConfig.h` | Add `compression_level` field |
| `OdbDesignServer/Config/GrpcServiceConfig.cpp` | Parse `compression.level` from JSON, clamp 1-9 |
| `OdbDesignServer/OdbDesignServerApp.cpp` | Apply compression level to builder |
| `OdbDesignServer/config.json` | Add `level` to `compression` section |
| `OdbDesignTests/GrpcServiceConfigTests.cpp` | Add tests for compression level parsing/clamping |

### Risk: LOW
- Simple config addition following established patterns (same as thread pool/keepalive)
- No behavioral change for existing deployments (default stays at 6)
- **No client changes needed**

### Acceptance Criteria
- [ ] `compression_level` field in `GrpcServiceConfig` with default 6
- [ ] JSON parsing with clamping (1-9 range)
- [ ] Compression level applied to gRPC `ServerBuilder`
- [ ] Startup log shows configured compression level
- [ ] Tests for compression level parsing and boundary clamping
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
