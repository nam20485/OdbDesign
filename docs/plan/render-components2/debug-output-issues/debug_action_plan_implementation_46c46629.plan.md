---
name: Debug Action Plan Implementation
overview: Implement client-side fixes from debug output analysis and create server-side requirements contract document for handoff to server dev team.
todos:
  - id: fix-component-height
    content: Update ComponentToShapeConverter.GetHeight() to check attributeLookupTable first, then fall back to PropertyRecords
    status: pending
  - id: implement-layer-units-retry
    content: Implement FailedPrecondition retry logic in GetLayerSymbolsAsync() to handle layer units not set errors
    status: pending
  - id: improve-substrate-fallback
    content: Enhance substrate fallback logic and add RenderErrorSubstrate() method for visual error indication
    status: pending
  - id: async-view-mode-switch
    content: Implement asynchronous view mode switching with loading indicators to prevent UI blocking
    status: pending
  - id: investigate-fps-property
    content: Research Ab4D SharpEngine API for correct FPS property names and built-in performance APIs
    status: pending
  - id: make-fps-optional
    content: Add EnableFpsMonitoring configuration setting and make FPS monitoring optional
    status: pending
    dependencies:
      - investigate-fps-property
  - id: create-server-requirements-doc
    content: Create comprehensive server-side requirements contract document with all required changes, verification checklists, and test cases
    status: pending
  - id: update-unimplemented-items
    content: "Add deferred optimizations (Problem 4: Solutions 4.3-4.6, Problem 5: Solutions 5.3-5.4) to unimplemented-items.md"
    status: pending
---

# Debug Action Plan Implementation

## Overview

This plan implements the client-side fixes from `debug-output-action-plan.md` and creates a comprehensive server-side requirements contract document for handoff to the server development team.

## Client-Side Implementation Tasks

### Problem 1: FPS Property Investigation (Client-Side Only)

**Files to Modify:**

- `src/OdbDesign3DClient.Core/Services/Implementations/PerformanceMonitor.cs` (or create if doesn't exist)
- `src/OdbDesign3DClient.Core/Configuration/OdbDesignSettings.cs`
- `src/OdbDesign3DClient.Desktop/appsettings.json`

**Tasks:**

1. Research Ab4D SharpEngine API documentation for FPS property names
2. Use reflection to inspect actual properties at runtime
3. Investigate SharpEngine's built-in performance APIs (events/callbacks)
4. Add `EnableFpsMonitoring` configuration setting
5. Make FPS monitoring optional/configurable
6. Wrap FPS property access in conditional checks

**Implementation Order:**

1. Solution 1.1: Verify correct property names via API docs and reflection
2. Solution 1.2: Investigate built-in performance APIs
3. Solution 1.3: Make FPS monitoring optional (quick mitigation)

### Problem 2: Layer Units Precondition Fix

**Files to Modify:**

- `src/OdbDesign3DClient.Core/Services/Implementations/OdbDesignGrpcClient.cs` (Lines 143-196)

**Tasks:**

1. Implement Solution 2.3: Handle `FailedPrecondition` gracefully with retry logic

   - Catch `FailedPrecondition` specifically in `GetLayerSymbolsAsync()`
   - Check if error message contains "layer units not set"
   - Query layer units using separate call (if API exists)
   - Set units on server (if API exists)
   - Retry symbol request
   - Improve error logging

**Note:** Solution 2.2 (server returning units with layer info) will be documented in server requirements contract.

### Problem 3: Component Height Fix

**Files to Modify:**

- `src/OdbDesign3DClient.Core/Services/Implementations/ComponentToShapeConverter.cs`

**Tasks:**

1. Update `GetHeight()` method to check `attributeLookupTable` first
2. Add `ComponentsFile` parameter to method signature
3. Find `.comp_height` index in `componentsFile.AttributeNames`
4. Check `comp.AttributeLookupTable` using index as key
5. Fall back to `PropertyRecords` for backward compatibility
6. Update call site to pass `components` parameter

**Reference:** See `component_height_root_cause_fix_025743a4.plan.md` for detailed implementation.

### Problem 4: Message Size Verification (Client-Side)

**Files to Verify:**

- `src/OdbDesign3DClient.Core/Services/Implementations/GrpcChannelFactory.cs` (Lines 27-35)

**Tasks:**

1. Verify 100MB message size limits are sufficient
2. Document current configuration for server team
3. Add server requirements for matching limits

**Note:** Server-side requirements will be documented in contract.

### Problem 5: View Mode Performance Optimization

**Files to Modify:**

- `src/OdbDesign3DClient.Core/ViewModels/MainViewModel.cs` (view mode switching logic)

**Tasks:**

1. Implement Solution 5.2: Asynchronous mode switching

   - Wrap mode switch in `Task.Run()` or background thread
   - Prevent UI blocking during transitions

2. Implement Solution 5.5: User experience enhancement

   - Add loading spinner during transitions
   - Show progress indicators

**Defer to Future:** Solutions 5.1, 5.3, 5.4 (add to `unimplemented-items.md`)

### Problem 6: Substrate Fallback Improvements

**Files to Modify:**

- `src/OdbDesign3DClient.Core/ViewModels/MainViewModel.cs` (Lines 865-933)

**Tasks:**

1. Improve fallback logic (Solution 6.1)

   - Verify layer content is loaded before attempting fallback
   - Add validation checks for `_layerNodes.Count == 0`
   - Improve error messages

2. Implement error rendering approach

   - Create `RenderErrorSubstrate()` method
   - Render red cross-hatch pattern for visual error indication
   - Show error in UI status bar
   - Log detailed error messages

**Note:** Solution 6.2 (fix root cause - message size) addressed in Problem 4.

## Server-Side Requirements Contract Document

**File to Create:**

- `docs/render-components/render-componentsd-finish/server-implementation/server-side-requirements-contract.md`

**Document Structure:**

### 1. Executive Summary

- Overview of required server changes
- Priority classification
- Impact assessment

### 2. Problem 2: Layer Units Precondition (HIGH PRIORITY)

**Requirement 2.1: Include Units in Layer Metadata Responses**

- **Current Issue:** Client receives `FailedPrecondition` when requesting symbols for layers without units set
- **Required Change:** Include `units` field in layer metadata responses
- **Affected Endpoints:**
  - `GetLayerInfo` (if exists) - add `units` field
  - OR modify `GetLayerSymbols` to include units in initial response
- **Implementation Details:**
  - Units should be available from `featuresFile.GetUnits()`
  - Include units in response even if empty (client will handle)
  - Cache units when layers are first loaded
- **Verification:**
  - Test with `comp_+_top` and `comp_+_bot` layers
  - Verify units are included in responses
  - Verify no `FailedPrecondition` errors occur

**Requirement 2.2: SetLayerUnits API (Optional Enhancement)**

- **Description:** Optional API to allow client to set units before requesting symbols
- **Priority:** Low (if Requirement 2.1 implemented)
- **Use Case:** Retry logic fallback if units not in metadata

### 3. Problem 4: Message Size and Compression (HIGH PRIORITY)

**Requirement 4.1: Verify Message Size Limits**

- **Current Client Configuration:** 100MB (`MaxReceiveMessageSize` and `MaxSendMessageSize`)
- **Required Server Configuration:**
  - Server must support at least 100MB message size limits
  - Verify server gRPC channel configuration matches client
  - Document server-side limits in configuration
- **Verification:**
  - Test with large designs (`designodb_rigidflex/cellular_flip-phone`)
  - Verify `GetStepProfileAsync()` succeeds
  - Verify `GetStepEdaDataAsync()` succeeds
  - No `ResourceExhausted` errors for designs < 100MB

**Requirement 4.2: Verify gRPC Compression Support**

- **Current Client Configuration:** Gzip compression enabled (`CompressionLevel.Optimal`)
- **Required Server Configuration:**
  - Accept `grpc-encoding: gzip` headers
  - Compress responses when requested
  - Support compression for both individual and batch streams
- **Verification:**
  - Test with compression enabled client
  - Verify server accepts compressed requests
  - Verify server compresses responses
  - Measure bandwidth reduction

**Reference:** `server_requirements.md` TR-02 (already documented)

### 4. Problem 2.4: Batch Streaming Feature Flag (MEDIUM PRIORITY)

**Requirement 4.3: Enable Batch Streaming Feature Flag**

- **Current Status:** Server implementation complete, feature-flagged
- **Required Change:** Enable `enable_batch_streaming` in server configuration
- **Configuration File:** `OdbDesignServer/config.json`
- **Required Configuration:**
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

- **Verification:**
  - Verify `GetLayerFeaturesBatchStream` returns batches (not `UNIMPLEMENTED`)
  - Test with layers of various sizes
  - Verify batch sizes match configuration
  - Measure performance improvement vs individual streaming

**Reference:** `server_implementation_verification.md` (already documented)

### 5. Verification Checklist for Server Team

**Pre-Production Verification:**

- [ ] Layer units included in metadata responses (Requirement 2.1)
- [ ] Message size limits configured to 100MB+ (Requirement 4.1)
- [ ] gRPC compression enabled and tested (Requirement 4.2)
- [ ] Batch streaming feature flag enabled (Requirement 4.3)
- [ ] Test with `designodb_rigidflex/cellular_flip-phone` design
- [ ] No `FailedPrecondition` errors for component layers
- [ ] No `ResourceExhausted` errors for large designs
- [ ] Performance benchmarks show improvement with batch streaming

### 6. Testing Requirements

**Test Cases:**

1. **Layer Units Test:**

   - Load `comp_+_top` and `comp_+_bot` layers
   - Verify units are included in responses
   - Verify no precondition failures

2. **Message Size Test:**

   - Load large design (`designodb_rigidflex/cellular_flip-phone`)
   - Verify profile file loads successfully
   - Verify EDA data loads successfully

3. **Compression Test:**

   - Enable compression on client
   - Verify server accepts compressed requests
   - Measure bandwidth reduction

4. **Batch Streaming Test:**

   - Enable batch streaming feature flag
   - Test with layers of various sizes
   - Verify performance improvement

## Implementation Order

### Phase 1: Client-Side Fixes (This Week)

1. Component Height Fix (Problem 3) - High priority, resolves 400+ warnings
2. Layer Units Retry Logic (Problem 2.3) - High priority, immediate fix
3. Substrate Error Rendering (Problem 6.1) - Medium priority, improves UX
4. View Mode Async Switching (Problem 5.2, 5.5) - Medium priority, improves UX

### Phase 2: Investigation Tasks (This Week)

1. FPS Property Investigation (Problem 1.1, 1.2, 1.3) - Medium priority, reduces log noise

### Phase 3: Server Requirements Document (This Week)

1. Create comprehensive server-side requirements contract
2. Document all server-side changes needed
3. Include verification checklists and test cases

### Phase 4: Server Coordination (Next Week)

1. Hand off requirements document to server team
2. Coordinate on implementation timeline
3. Verify server changes once implemented

## Files to Create/Modify

### New Files:

- `docs/render-components/render-componentsd-finish/server-implementation/server-side-requirements-contract.md`

### Modified Files:

- `src/OdbDesign3DClient.Core/Services/Implementations/ComponentToShapeConverter.cs`
- `src/OdbDesign3DClient.Core/Services/Implementations/OdbDesignGrpcClient.cs`
- `src/OdbDesign3DClient.Core/ViewModels/MainViewModel.cs`
- `src/OdbDesign3DClient.Core/Services/Implementations/PerformanceMonitor.cs` (or create)
- `src/OdbDesign3DClient.Core/Configuration/OdbDesignSettings.cs`
- `src/OdbDesign3DClient.Desktop/appsettings.json`
- `docs/render-components/render-componentsd-finish/unimplemented-items.md` (add deferred optimizations)

## Success Criteria

### Client-Side:

- ✅ Component height warnings resolved (400+ → 0)
- ✅ Layer units precondition errors handled gracefully
- ✅ Substrate errors render visually
- ✅ View mode switching doesn't block UI
- ✅ FPS monitoring optional/configurable

### Server-Side (Documentation):

- ✅ Comprehensive requirements contract created
- ✅ All server-side changes documented
- ✅ Verification checklists provided
- ✅ Test cases defined
- ✅ Ready for server team handoff