# Server-Client Requirements Contract
**Version**: 1.0
**Status**: DRAFT

## Overview
This document defines the required changes to the ODB++ Server and Protocol to support the performance optimizations planned for the `OdbDesign3DClient`.

## 1. Batched Feature Streaming
**Requirement ID**: TR-01
**Priority**: High

### Description
Currently, `GetLayerFeaturesStream` streams individual `FeatureRecord` messages. This causes excessive overhead (header/framing) per small feature (like pads), leading to slow transfer rates for layers with 10k+ features.

### Required Change
The server must support sending **batches** of features in a single stream message.

#### Protocol Change
Modify `FeaturesFile.proto` (or equivalent) to add a batched response type:

```protobuf
// Existing
message FeatureRecord { ... }

// NEW: Container for batching
message FeatureRecordBatch {
    repeated FeatureRecord features = 1;
}
```

Modify the Service definition:
```protobuf
// Current
rpc GetLayerFeaturesStream(GetLayerFeaturesRequest) returns (stream FeatureRecord);

// Proposed (Option A: New RPC)
rpc GetLayerFeaturesBatchStream(GetLayerFeaturesRequest) returns (stream FeatureRecordBatch);

// Proposed (Option B: Modified RPC - Breaking Change)
// rpc GetLayerFeaturesStream(GetLayerFeaturesRequest) returns (stream FeatureRecordBatch);
```
*Recommendation*: Option A (New RPC) to maintain backward compatibility if needed, or Option B if we can break sync.

## 2. gRPC Compression Support
**Requirement ID**: TR-02
**Priority**: Medium

### Description
Large text/structure data should be compressed during transit.

### Required Change
- Ensure the Server is configured to accept and respect `grpc-encoding: gzip` headers.
- Ensure the Server can compress responses for `GetLayerFeaturesStream` (and the new batch stream).

## 3. Health Check & Metadata
**Requirement ID**: VS-01
**Priority**: Low

### Description
Client needs to know if the connected server supports these new optimization features.

### Required Change
- Update `HealthCheck` or introduce a `GetServerCapabilities` RPC that returns supported features (e.g., `["BATCH_STREAM", "GZIP"]`).
