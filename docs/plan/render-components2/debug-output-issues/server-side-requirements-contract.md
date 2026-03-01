# Server-Side Requirements Contract
**Version**: 1.0  
**Date**: January 2025  
**Status**: Ready for Server Team Implementation  
**Purpose**: Comprehensive requirements document for server-side changes needed to resolve client-side issues identified in debug output analysis

---

## Executive Summary

This document specifies required server-side changes to resolve client-side issues identified during debug output analysis. All requirements are prioritized and include detailed implementation guidance, verification checklists, and test cases.

### Priority Classification

- **HIGH PRIORITY**: Blocks functionality or causes frequent errors
- **MEDIUM PRIORITY**: Improves performance or enables optimizations
- **LOW PRIORITY**: Optional enhancements

### Impact Assessment

| Requirement | Priority | Impact | Effort | Dependencies |
|------------|----------|--------|--------|--------------|
| Layer Units in Metadata | HIGH | Prevents precondition failures | Medium | None |
| Message Size Limits | HIGH | Unblocks large design loading | Low | None |
| gRPC Compression | HIGH | Improves transfer efficiency | Low | None |
| Batch Streaming Enable | MEDIUM | Performance optimization | Low | Already implemented |

---

## 1. Problem 2: Layer Units Precondition (HIGH PRIORITY)

### Current Issue

Client receives `FailedPrecondition` errors when requesting layer symbols for layers without units set. This occurs specifically on `comp_+_top` and `comp_+_bot` layers (component placement layers).

**Error Message**: `Grpc.Core.RpcException: Status(StatusCode="FailedPrecondition", Detail="layer units not set")`

**Frequency**: 4 occurrences per design load (2 layers × 2 designs)

**Root Cause**: Server requires layer units to be set before retrieving layer symbols, but client cannot determine units without making the request first.

### Requirement 2.1: Include Units in Layer Metadata Responses

**Priority**: HIGH  
**Impact**: Prevents precondition failures, eliminates need for separate units query

#### Required Change

Include `units` field in layer metadata responses. This can be implemented in one of two ways:

**Option A: Add Units to GetLayerSymbols Response** (Recommended)
- Modify `GetLayerSymbolsResponse` to always include units
- Units should be available from `featuresFile.GetUnits()`
- Include units even if empty (client will handle gracefully)

**Option B: Create GetLayerInfo Endpoint** (Alternative)
- Create new `GetLayerInfo` RPC that returns layer metadata including units
- Client can query layer info before requesting symbols
- More flexible for future metadata additions

#### Implementation Details

**Server-Side Code Location**: `OdbDesignServer/Services/OdbDesignServiceImpl.cpp`

**Current Implementation** (Lines 346-350):
```cpp
const auto& featuresFile = layerIt->second->GetFeaturesFile();
auto unitsInfo = normalize_units(featuresFile.GetUnits());
if (!unitsInfo.ok)
{
    return { grpc::StatusCode::FAILED_PRECONDITION, unitsInfo.error };
}
```

**Required Change**:
- Extract units from `featuresFile.GetUnits()` before precondition check
- Include units in response message
- Remove or modify precondition check to allow empty units (client will default to "mm")

**Protobuf Changes** (if Option A):
```protobuf
message GetLayerSymbolsResponse {
  int32 sym_num_base = 1;
  string units = 2;  // Already exists - ensure always populated
  double units_to_mm = 3;  // Already exists - ensure always populated
  repeated SymbolName symbol_names = 4;
}
```

**Note**: `units` and `units_to_mm` fields already exist in `GetLayerSymbolsResponse`. The requirement is to ensure they are **always populated** even when ODB++ files lack units metadata.

#### Verification Checklist

- [ ] Units field populated in all `GetLayerSymbolsResponse` messages
- [ ] Units field populated even when ODB++ file lacks units metadata (use empty string or default)
- [ ] No `FailedPrecondition` errors for `comp_+_top` and `comp_+_bot` layers
- [ ] Test with designs that have units metadata
- [ ] Test with designs that lack units metadata
- [ ] Verify backward compatibility (existing clients continue working)

#### Test Cases

1. **Test Case 2.1.1: Component Layers with Units**
   - Load design with `comp_+_top` and `comp_+_bot` layers
   - Request symbols for both layers
   - Verify units are included in responses
   - Verify no `FailedPrecondition` errors

2. **Test Case 2.1.2: Component Layers without Units**
   - Load design where component layers lack units metadata
   - Request symbols for both layers
   - Verify units field is populated (empty string or default)
   - Verify no `FailedPrecondition` errors

3. **Test Case 2.1.3: Regular Layers**
   - Load design with regular feature layers
   - Request symbols for multiple layers
   - Verify units are included in all responses
   - Verify no regressions

### Requirement 2.2: SetLayerUnits API (Optional Enhancement)

**Priority**: LOW  
**Impact**: Provides retry mechanism fallback

#### Description

Optional API to allow client to set units before requesting symbols. This is only needed if Requirement 2.1 cannot be implemented immediately.

#### Implementation Details

**New RPC**:
```protobuf
rpc SetLayerUnits(SetLayerUnitsRequest) returns (SetLayerUnitsResponse);

message SetLayerUnitsRequest {
  string design_name = 1;
  string step_name = 2;
  string layer_name = 3;
  string units = 4;  // "mm", "inch", etc.
}

message SetLayerUnitsResponse {
  bool success = 1;
  string message = 2;
}
```

**Use Case**: Client retry logic fallback if units not in metadata

**Note**: This requirement is **optional** and only needed if Requirement 2.1 cannot be implemented. Recommendation is to implement Requirement 2.1 instead.

---

## 2. Problem 4: Message Size and Compression (HIGH PRIORITY)

### Current Issue

Client receives `ResourceExhausted` errors when loading large designs. Operations affected:
- `GetStepProfileAsync()` - Profile file loading
- `GetStepEdaDataAsync()` - Component bodies loading

**Error Message**: `Grpc.Core.RpcException: Status(StatusCode="ResourceExhausted", Detail="Received message exceeds the maximum configured message size.")`

**Frequency**: 3 occurrences (with retries) for `designodb_rigidflex/cellular_flip-phone`

**Root Cause**: gRPC message size limits are too small for large designs.

### Requirement 4.1: Verify Message Size Limits

**Priority**: HIGH  
**Impact**: Unblocks large design loading

#### Current Client Configuration

- **MaxReceiveMessageSize**: 100MB
- **MaxSendMessageSize**: 100MB
- **Location**: `GrpcChannelFactory.cs` (Lines 29-30)

#### Required Server Configuration

- Server must support at least **100MB** message size limits
- Verify server gRPC channel configuration matches client
- Document server-side limits in configuration

#### Implementation Details

**Server-Side Configuration**:
- Verify gRPC server channel options include:
  ```cpp
  grpc::ChannelArguments args;
  args.SetMaxReceiveMessageSize(100 * 1024 * 1024);  // 100 MB
  args.SetMaxSendMessageSize(100 * 1024 * 1024);     // 100 MB
  ```

**Configuration File**: Add to server configuration file (e.g., `config.json`):
```json
{
  "grpc": {
    "max_receive_message_size_mb": 100,
    "max_send_message_size_mb": 100
  }
}
```

#### Verification Checklist

- [ ] Server message size limits configured to 100MB+
- [ ] Test with large design (`designodb_rigidflex/cellular_flip-phone`)
- [ ] Verify `GetStepProfileAsync()` succeeds
- [ ] Verify `GetStepEdaDataAsync()` succeeds
- [ ] No `ResourceExhausted` errors for designs < 100MB
- [ ] Document limits in server configuration

#### Test Cases

1. **Test Case 4.1.1: Large Profile File**
   - Load `designodb_rigidflex/cellular_flip-phone` design
   - Request profile file
   - Verify successful load (no `ResourceExhausted` error)
   - Measure response size

2. **Test Case 4.1.2: Large EDA Data File**
   - Load `designodb_rigidflex/cellular_flip-phone` design
   - Request EDA data file
   - Verify successful load (no `ResourceExhausted` error)
   - Measure response size

3. **Test Case 4.1.3: Very Large Design**
   - Test with design approaching 100MB limit
   - Verify graceful handling if limit exceeded
   - Document maximum supported design size

### Requirement 4.2: Verify gRPC Compression Support

**Priority**: HIGH  
**Impact**: Improves transfer efficiency, reduces bandwidth

#### Current Client Configuration

- **Compression Provider**: Gzip
- **Compression Level**: Optimal
- **Location**: `GrpcChannelFactory.cs` (Lines 31-34)

#### Required Server Configuration

- Accept `grpc-encoding: gzip` headers
- Compress responses when requested
- Support compression for both individual and batch streams

#### Implementation Details

**Server-Side Configuration**:
- Enable gRPC compression in server channel options
- Accept compressed requests
- Compress responses when client requests compression

**Reference**: `server_requirements.md` TR-02 (already documented)

#### Verification Checklist

- [ ] Server accepts `grpc-encoding: gzip` headers
- [ ] Server compresses responses when requested
- [ ] Compression works for individual feature streams
- [ ] Compression works for batch feature streams
- [ ] Measure bandwidth reduction (should be 30-70% for text data)
- [ ] Verify no performance degradation

#### Test Cases

1. **Test Case 4.2.1: Compression Negotiation**
   - Connect client with compression enabled
   - Verify server accepts compressed requests
   - Verify server compresses responses
   - Check `grpc-encoding` headers in network trace

2. **Test Case 4.2.2: Bandwidth Measurement**
   - Load large design with compression disabled
   - Measure bandwidth used
   - Load same design with compression enabled
   - Measure bandwidth used
   - Verify reduction (target: 30-70% for ODB++ data)

3. **Test Case 4.2.3: Performance Impact**
   - Measure response time with compression
   - Measure response time without compression
   - Verify compression overhead is acceptable (< 10% increase)

---

## 3. Problem 2.4: Batch Streaming Feature Flag (MEDIUM PRIORITY)

### Current Status

Server implementation is **COMPLETE** but feature-flagged. Client is ready to use batch streaming once enabled.

**Reference**: `server_implementation_verification.md` - Implementation verified complete

### Requirement 4.3: Enable Batch Streaming Feature Flag

**Priority**: MEDIUM  
**Impact**: Performance optimization (10-100x improvement for large layers)

#### Required Change

Enable `enable_batch_streaming` feature flag in server configuration.

#### Implementation Details

**Configuration File**: `OdbDesignServer/config.json`

**Required Configuration**:
```json
{
  "grpc": {
    "batch_streaming": {
      "enabled": true,
      "batch_size": 500
    }
  }
}
```

**Configuration Options**:
- `enabled`: `true` to enable batch streaming, `false` to disable (returns `UNIMPLEMENTED`)
- `batch_size`: Number of features per batch (range: 100-1000, default: 500)

#### Verification Checklist

- [ ] Feature flag enabled in configuration
- [ ] `GetLayerFeaturesBatchStream` returns batches (not `UNIMPLEMENTED`)
- [ ] Test with layers of various sizes (< 1k, 1k-10k, 10k-100k, > 100k features)
- [ ] Verify batch sizes match configuration
- [ ] Measure performance improvement vs individual streaming
- [ ] Verify no regressions in existing functionality

#### Test Cases

1. **Test Case 4.3.1: Basic Batch Streaming**
   - Enable batch streaming feature flag
   - Request batch stream for layer with 5k features
   - Verify batches received (should be ~10 batches of 500 features)
   - Verify all features received correctly

2. **Test Case 4.3.2: Performance Comparison**
   - Load layer with 10k+ features using individual streaming
   - Measure time to complete
   - Load same layer using batch streaming
   - Measure time to complete
   - Verify improvement (target: 5-10x faster)

3. **Test Case 4.3.3: Batch Size Configuration**
   - Test with `batch_size = 100`
   - Test with `batch_size = 500` (default)
   - Test with `batch_size = 1000`
   - Verify batches match configured size
   - Measure performance impact of different batch sizes

4. **Test Case 4.3.4: Feature Flag Toggle**
   - Test with `enabled = false` (should return `UNIMPLEMENTED`)
   - Test with `enabled = true` (should work normally)
   - Verify graceful fallback on client side

---

## 4. Verification Checklist for Server Team

### Pre-Production Verification

**Critical Requirements** (Must Pass):
- [ ] Layer units included in metadata responses (Requirement 2.1)
- [ ] Message size limits configured to 100MB+ (Requirement 4.1)
- [ ] gRPC compression enabled and tested (Requirement 4.2)
- [ ] Test with `designodb_rigidflex/cellular_flip-phone` design
- [ ] No `FailedPrecondition` errors for component layers
- [ ] No `ResourceExhausted` errors for large designs

**Performance Requirements** (Should Pass):
- [ ] Batch streaming feature flag enabled (Requirement 4.3)
- [ ] Performance benchmarks show improvement with batch streaming
- [ ] Compression reduces bandwidth by 30-70%

**Optional Requirements** (Nice to Have):
- [ ] SetLayerUnits API implemented (Requirement 2.2)

### Testing Requirements

**Test Design**: `designodb_rigidflex/cellular_flip-phone`
- This design triggers all identified issues
- Use for comprehensive verification

**Test Scenarios**:
1. Load complete design (all layers)
2. Load component layers specifically (`comp_+_top`, `comp_+_bot`)
3. Load profile file
4. Load EDA data file
5. Test batch streaming with large layers

---

## 5. Implementation Timeline Recommendations

### Phase 1: Critical Fixes (Week 1)
1. **Requirement 2.1**: Include units in layer metadata (2-3 days)
2. **Requirement 4.1**: Verify message size limits (1 day)
3. **Requirement 4.2**: Verify compression support (1 day)

**Total**: 4-5 days

### Phase 2: Performance Optimization (Week 2)
1. **Requirement 4.3**: Enable batch streaming (1 day)
2. Performance testing and validation (2-3 days)

**Total**: 3-4 days

### Phase 3: Optional Enhancements (Future)
1. **Requirement 2.2**: SetLayerUnits API (if needed)

---

## 6. Client-Side Coordination

### Client Readiness

**Already Implemented**:
- ✅ gRPC compression enabled (client-side ready)
- ✅ 100MB message size limits configured
- ✅ Batch streaming client code ready (pending server enablement)
- ✅ Retry logic for `FailedPrecondition` errors

**Pending Server Changes**:
- ⏳ Layer units in metadata (Requirement 2.1)
- ⏳ Server message size limits (Requirement 4.1)
- ⏳ Server compression support (Requirement 4.2)
- ⏳ Batch streaming feature flag (Requirement 4.3)

### Integration Testing

Once server changes are implemented:
1. Client team will verify all requirements
2. Run integration tests with `designodb_rigidflex/cellular_flip-phone`
3. Measure performance improvements
4. Document results

---

## 7. References

### Related Documents

- **Debug Output Analysis**: `debug_output_analysis_and_solutions_8f80fb45.plan.md`
- **Action Plan**: `debug-output-action-plan.md`
- **Server Implementation Verification**: `server_implementation_verification.md`
- **Server Requirements**: `server_requirements.md`
- **Component Height Fix**: `component_height_root_cause_fix_025743a4.plan.md`

### Server-Side Source Code Locations

- **gRPC Service**: `OdbDesignServer/Services/OdbDesignServiceImpl.cpp`
- **Configuration**: `OdbDesignServer/Config/GrpcServiceConfig.h` & `.cpp`
- **Config File**: `OdbDesignServer/config.json`
- **Protobuf Definitions**: `OdbDesignServer/protoc/grpc/service.proto`

### Client-Side Source Code Locations

- **gRPC Client**: `src/OdbDesign3DClient.Core/Services/Implementations/OdbDesignGrpcClient.cs`
- **gRPC Channel Factory**: `src/OdbDesign3DClient.Core/Services/Implementations/GrpcChannelFactory.cs`
- **Protobuf Definitions**: `src/OdbDesign3DClient.Core/Protos/grpc/service.proto`

---

## 8. Contact & Support

**For Questions**:
- Review related documents listed in References section
- Check server-side source code locations
- Coordinate with client team for integration testing

**Document Version**: 1.0  
**Last Updated**: January 2025  
**Status**: Ready for Server Team Implementation

---

## Appendix A: Error Message Reference

### FailedPrecondition Error
```
Grpc.Core.RpcException: Status(StatusCode="FailedPrecondition", Detail="layer units not set")
```
**Occurrence**: Component layers (`comp_+_top`, `comp_+_bot`)  
**Fix**: Requirement 2.1

### ResourceExhausted Error
```
Grpc.Core.RpcException: Status(StatusCode="ResourceExhausted", Detail="Received message exceeds the maximum configured message size.")
```
**Occurrence**: Large designs (`designodb_rigidflex/cellular_flip-phone`)  
**Fix**: Requirement 4.1

---

## Appendix B: Configuration File Template

**File**: `OdbDesignServer/config.json`

```json
{
  "grpc": {
    "max_receive_message_size_mb": 100,
    "max_send_message_size_mb": 100,
    "compression": {
      "enabled": true,
      "algorithm": "gzip"
    },
    "batch_streaming": {
      "enabled": true,
      "batch_size": 500
    }
  }
}
```

---

**End of Document**
