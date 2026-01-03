# Server-Client Requirements Contract v2.0
**Version**: 2.0  
**Status**: FINAL  
**Last Updated**: January 2025

## Overview & Goals

This document defines the required changes to the ODB++ Server and Protocol to support performance optimizations for the `OdbDesign3DClient`. The contract specifies protocol changes, implementation phases, backward compatibility strategy, performance targets, and testing requirements.

### Goals

1. **Reduce Stream Overhead**: Minimize per-message overhead for layers with 10k+ features
2. **Maintain Backward Compatibility**: Support both old and new protocols during migration
3. **Enable Capability Negotiation**: Allow clients to detect server capabilities
4. **Performance Target**: 10-100x reduction in stream overhead for large layers

---

## Protocol Changes

### 1. Batched Feature Streaming

**Requirement ID**: TR-01  
**Priority**: High  
**Breaking Change**: No (new RPC added)

#### Protocol Definition

Add to `service.proto`:

```protobuf
// Batch container for feature records to reduce stream overhead
message FeatureRecordBatch {
  repeated .Odb.Lib.Protobuf.FeaturesFile.FeatureRecord features = 1;
}

// Service definition addition
service OdbDesignService {
  // ... existing RPCs ...
  
  // Batched server-streaming call for high-performance streaming of layer features.
  // Sends features in batches to reduce stream overhead for layers with 10k+ features.
  // Client should use this when server supports it, falling back to GetLayerFeaturesStream if unavailable.
  rpc GetLayerFeaturesBatchStream(GetLayerFeaturesRequest) returns (stream FeatureRecordBatch);
}
```

#### Field Specifications

- **FeatureRecordBatch.features**: Array of `FeatureRecord` messages. Server should batch 100-1000 features per message (configurable, default 500).
- **GetLayerFeaturesRequest**: Reuses existing request message (no changes needed).

#### Semantics

- Server sends features in batches of configurable size (recommended: 500 features per batch).
- Empty batches are allowed (server may send empty batch to indicate end of stream).
- Client unwraps batches and processes individual features transparently.
- If server doesn't support batching, returns `Unimplemented` status code.

---

## Implementation Phases

### Phase 1: Server Implementation (Server Team)

**Duration**: 2-3 days  
**Dependencies**: None

#### Tasks

1. **Add Protobuf Definition**
   - Add `FeatureRecordBatch` message to `service.proto`
   - Add `GetLayerFeaturesBatchStream` RPC to service definition
   - Regenerate gRPC server code

2. **Implement Batch Streaming Logic**
   - Modify feature streaming logic to batch features
   - Add batch size configuration (default: 500 features)
   - Ensure empty batches are sent to indicate stream end

3. **Error Handling**
   - Return `Unimplemented` status if batching not yet available (for gradual rollout)
   - Handle cancellation tokens properly
   - Maintain existing error handling patterns

4. **Testing**
   - Unit tests for batch creation logic
   - Integration tests with mock client
   - Performance tests comparing batch vs individual streaming

#### Deliverables

- Updated `service.proto` with batch RPC
- Server implementation of `GetLayerFeaturesBatchStream`
- Server-side tests
- Performance benchmarks

---

### Phase 2: Client Implementation (Client Team)

**Duration**: 2-3 days  
**Dependencies**: Phase 1 complete (or can proceed in parallel with capability detection)

#### Tasks

1. **Update Protobuf Definitions** ✅ **COMPLETED**
   - Add `FeatureRecordBatch` message
   - Add `GetLayerFeaturesBatchStream` RPC
   - Regenerate gRPC client code

2. **Implement Batch Streaming Client** ✅ **COMPLETED**
   - Add `GetLayerFeaturesBatchStreamAsync()` method to `IOdbDesignClient`
   - Implement batch unwrapping logic in `OdbDesignGrpcClient`
   - Add progress logging for batch streams

3. **Capability Detection** ✅ **COMPLETED**
   - Implement `SupportsBatchStreamingAsync()` method
   - Cache capability check result per server endpoint
   - Graceful fallback to individual streaming

4. **Integration** ✅ **COMPLETED**
   - Update `MainViewModel` to detect capabilities
   - Use batch streaming when available
   - Fall back to individual streaming if unavailable

#### Deliverables

- ✅ Client implementation of batch streaming
- ✅ Capability detection logic
- ✅ Integration with MainViewModel
- ✅ Fallback handling

---

## Backward Compatibility

### Strategy

**Option A: New RPC (Recommended)** ✅ **IMPLEMENTED**

- Add new `GetLayerFeaturesBatchStream` RPC alongside existing `GetLayerFeaturesStream`
- Existing clients continue using `GetLayerFeaturesStream`
- New clients can use batch streaming when available
- **Status**: Implemented in client, awaiting server support

### Migration Path

1. **Server Rollout**:
   - Deploy server with both RPCs available
   - Monitor usage of batch vs individual streaming
   - Gradually migrate clients to batch streaming

2. **Client Migration**:
   - Clients detect server capabilities automatically
   - Use batch streaming when available
   - Fall back to individual streaming if unavailable
   - No client code changes needed for fallback

3. **Deprecation** (Future):
   - After all clients support batching, consider deprecating individual streaming
   - Provide migration window before removal

---

## Performance Targets & Metrics

### Targets

- **Stream Overhead Reduction**: 10-100x reduction for layers with 10k+ features
- **Latency Improvement**: 20-50% reduction in time to first feature for large layers
- **Throughput Improvement**: 30-80% increase in features/second for large layers

### Measurement

- **Baseline**: Measure current `GetLayerFeaturesStream` performance
- **Target**: Measure `GetLayerFeaturesBatchStream` performance
- **Metrics**:
  - Time to first feature
  - Total stream time
  - Features per second
  - Network bytes transferred
  - CPU usage during streaming

### Benchmark Scenarios

1. **Small Layer** (< 1k features): Minimal difference expected
2. **Medium Layer** (1k-10k features): 5-10x improvement expected
3. **Large Layer** (10k-100k features): 10-100x improvement expected
4. **Very Large Layer** (> 100k features): Maximum improvement expected

---

## Testing Requirements

### Server Tests

1. **Unit Tests**:
   - Batch creation with various feature counts
   - Empty batch handling
   - Batch size limits
   - Error handling

2. **Integration Tests**:
   - End-to-end batch streaming
   - Cancellation handling
   - Concurrent requests
   - Large layer streaming (10k+ features)

3. **Performance Tests**:
   - Compare batch vs individual streaming
   - Measure overhead reduction
   - CPU and memory usage

### Client Tests

1. **Unit Tests**: ✅ **COMPLETED**
   - Batch unwrapping logic
   - Capability detection
   - Fallback handling

2. **Integration Tests**:
   - End-to-end batch streaming with mock server
   - Fallback to individual streaming
   - Error handling

3. **Performance Tests**: ✅ **INFRASTRUCTURE COMPLETE**
   - Benchmark suite created
   - LoadLargeLayerBenchmark implemented
   - FeatureConversionBenchmark implemented

---

## Capability Negotiation

### Current Implementation ✅ **COMPLETED**

Client uses `SupportsBatchStreamingAsync()` method:

```csharp
public async Task<bool> SupportsBatchStreamingAsync(CancellationToken cancellationToken = default)
{
    // Try to call batch stream with test request
    // If Unimplemented status returned, server doesn't support it
    // Otherwise, cache result and return true
}
```

### Future Enhancement: GetServerCapabilities RPC

For more comprehensive capability negotiation, consider adding:

```protobuf
rpc GetServerCapabilities(GetServerCapabilitiesRequest) returns (GetServerCapabilitiesResponse);

message GetServerCapabilitiesResponse {
  repeated string supported_features = 1; // e.g., ["BATCH_STREAM", "GZIP", "COMPRESSION"]
  map<string, string> feature_versions = 2; // e.g., {"BATCH_STREAM": "1.0"}
}
```

**Status**: Not required for initial implementation. Current capability detection sufficient.

---

## Migration Guide

### For Server Developers

1. **Add Protobuf Definition**
   ```protobuf
   message FeatureRecordBatch {
     repeated .Odb.Lib.Protobuf.FeaturesFile.FeatureRecord features = 1;
   }
   
   rpc GetLayerFeaturesBatchStream(GetLayerFeaturesRequest) returns (stream FeatureRecordBatch);
   ```

2. **Implement Batch Logic**
   - Collect features into batches (default: 500 per batch)
   - Send batches via stream
   - Handle cancellation and errors

3. **Add Configuration**
   - Batch size: configurable (default: 500)
   - Enable/disable batching: for gradual rollout

4. **Test**
   - Unit tests for batch creation
   - Integration tests with client
   - Performance benchmarks

### For Client Developers

**Status**: ✅ **CLIENT IMPLEMENTATION COMPLETE**

Client automatically:
1. Detects server capabilities
2. Uses batch streaming when available
3. Falls back to individual streaming if unavailable
4. No code changes needed for existing clients

---

## gRPC Compression Support

**Requirement ID**: TR-02  
**Priority**: Medium  
**Status**: ✅ **CLIENT READY**

### Client Implementation ✅ **COMPLETED**

- Gzip compression enabled in `GrpcChannelFactory`
- Automatic negotiation with server
- No server changes required (server should accept compression headers)

### Server Requirements

- Accept `grpc-encoding: gzip` headers
- Compress responses when requested
- Support compression for both individual and batch streams

---

## Health Check & Metadata

**Requirement ID**: VS-01  
**Priority**: Low  
**Status**: Not Required (capability detection sufficient)

Current `HealthCheck` RPC is sufficient. Capability detection via `SupportsBatchStreamingAsync()` provides needed functionality without additional RPC.

---

## Summary

### Completed ✅

- ✅ Client-side batch streaming implementation
- ✅ Capability detection
- ✅ Graceful fallback handling
- ✅ Protobuf definitions updated
- ✅ Integration with MainViewModel
- ✅ Benchmark infrastructure
- ✅ gRPC compression enabled

### Pending ⏳

- ⏳ Server-side batch streaming implementation
- ⏳ Server-side performance testing
- ⏳ End-to-end integration testing
- ⏳ Performance validation

### Recommendations

1. **Server Team**: Implement `GetLayerFeaturesBatchStream` RPC as specified
2. **Testing**: Run performance benchmarks comparing batch vs individual streaming
3. **Rollout**: Gradual rollout with capability detection ensures smooth migration
4. **Monitoring**: Track usage of batch vs individual streaming to measure adoption

---

## References

- **Original Requirements**: `server_requirements.md`
- **Implementation Plan**: `implementation_plan.md`
- **Completed Work**: `completed-work.md`
- **Protobuf Definition**: `src/OdbDesign3DClient.Core/Protos/grpc/service.proto`
