---
name: Critical Issues Fix Plan
overview: ""
todos: []
---

# Critical Issues Fix Plan

## Overview

The client team has identified three critical issues from their latest integration test run. This plan addresses the server-side issues and documents client-side issues.

## Critical Issues Identified

| Issue | Severity | Server-Side? | Status |
|-------|----------|--------------|--------|
| **Message Size Limit Too Small** | 🔴 CRITICAL | ✅ Yes | ❌ BLOCKING |
| **Batch Streaming Not Used** | 🔴 CRITICAL | ⚠️ Partial | ⏳ Investigation Needed |
| **Component Height Fix Not Working** | 🔴 CRITICAL | ❌ No | 📝 Client-Side Issue |

---

## Issue 1: Server Message Size Limit Too Small 🔴 **CRITICAL**

### Problem

**Profile response from `GetDesign` RPC is 116MB, but server limit is 100MB:**
- Profile response: 116,196,777 bytes (116.2 MB)
- Server limit: 104,857,600 bytes (100.0 MB exactly)
- **Result**: `ResourceExhausted` error - "SERVER: Sent message larger than max (116196777 vs. 104857600)"

**Affected Operations**:
- `GetDesign` RPC (returns full design including profile file)
- Profile file loading fails
- Component bodies loading fails (also part of GetDesign response)

### Root Cause

The `GetDesign` RPC returns the entire `ProductModel.Design` protobuf message, which includes:
- Profile file (116MB for this design)
- EDA data (component bodies)
- All step/layer metadata

The 100MB limit we configured is insufficient for large designs.

### Solution

**Increase server message size limit to 150MB** to accommodate large design responses with margin.

### Implementation

**File**: `OdbDesignServer/config.json`

**Change**:
```json
{
  "grpc": {
    "max_receive_message_size_mb": 150,
    "max_send_message_size_mb": 150,
    ...
  }
}
```

**Files Modified**:
- `OdbDesignServer/config.json` - Update limits to 150MB

**Client Coordination Required**:
- Client team needs to update their `MaxReceiveMessageSize` and `MaxSendMessageSize` to 150MB (157,286,400 bytes)

### Acceptance Criteria

- [ ] Server config updated to 150MB
- [ ] Server restarted with new config
- [ ] `GetDesign` RPC succeeds for `designodb_rigidflex/cellular_flip-phone`
- [ ] Profile file loads successfully
- [ ] Component bodies load successfully
- [ ] No `ResourceExhausted` errors

### Verification

**Test Command**:
```bash
# Verify config updated
cat OdbDesignServer/config.json | grep max_send_message_size_mb
# Expected: "max_send_message_size_mb": 150

# Restart server and test
# Client should successfully load design without ResourceExhausted errors
```

### Notes and Long-Term Considerations

**1. Why GetDesign Returns Entire Design Message?**

The `GetDesign` RPC returns the complete `ProductModel.Design` protobuf message, which includes:
- `FileArchive` (contains all step/layer metadata)
  - `StepDirectory` (per step)
    - `profilefile` (FeaturesFile - board outline/profile, 116MB for this design)
    - `edadatafile` (EdaDataFile - component bodies/package data)
    - `layersByName` (all layer metadata)
- `nets`, `packages`, `components`, `parts` (product model data)

**Current Client Usage**: Client calls `GetDesign` once to get all design metadata, then uses specific RPCs (`GetLayerFeaturesStream`, `GetLayerSymbols`) for layer data.

**Optimization Opportunity**: 
- Consider splitting `GetDesign` into smaller RPCs:
  - `GetDesignMetadata` (name, product model, step/layer names only)
  - `GetStepProfile` (profile file only - could be streamed)
  - `GetStepEdaData` (EDA data only - could be streamed)
- This would reduce initial load size and allow streaming for large data

**2. What is the Profile File?**

The profile file (`StepDirectory.profilefile`) is a `FeaturesFile` containing the board outline/profile geometry. For `designodb_rigidflex/cellular_flip-phone`, it's 116MB of feature records (contours, arcs, lines) that define the board shape.

**3. Is 150MB Too Large? Long-Term Solution Needed**

**Yes, 150MB is a band-aid solution.** Better approaches:

**Option A: Stream Profile File** (Recommended):
- Create `GetStepProfileStream` RPC that streams profile features in batches
- Similar to `GetLayerFeaturesBatchStream` but for profile
- Client reassembles profile from stream
- No message size limit issues

**Option B: Chunk GetDesign Response**:
- Split `GetDesign` into multiple chunks
- Client requests chunks sequentially
- More complex but maintains single-RPC model

**Option C: Lazy Loading**:
- `GetDesign` returns metadata only
- Client requests profile/EDA data separately when needed
- Better memory efficiency

**Immediate Action**: Increase to 150MB as temporary fix, but **plan streaming solution for next sprint**.

### Long-Term Solution Plan

**Phase 1** (Immediate): Increase limit to 150MB - **BLOCKING FIX**
**Phase 2** (Next Sprint): Implement `GetStepProfileStream` RPC - **PROPER SOLUTION**
**Phase 3** (Future): Consider splitting `GetDesign` into smaller RPCs - **OPTIMIZATION**

---

## Issue 2: Batch Streaming Not Being Used 🔴 **CRITICAL**

### Problem

**Client is not using batch streaming despite:**
- ✅ Server has batch streaming implemented and enabled (`config.json` has `"enabled": true`)
- ✅ Client has `GetLayerFeaturesBatchStreamAsync()` implemented
- ✅ Client checks `SupportsBatchStreamingAsync()` before streaming
- ❌ **No evidence in logs that batch streaming was used**

**Expected Log Pattern** (if batch streaming working):
```
[Debug] Server supports batch streaming
[Debug] Using batch streaming for layer: layer-1
[Debug] Batch stream progress: 4000 features received for layer-1
[Info] Completed batch streaming features: 11868 for layer-1 in 15.22s
```

**Actual Log Pattern** (individual streaming):
```
[Info] Streaming features... received 8000 so far for layer-1
[Info] Completed streaming features: 11868 for layer-1 in 15.22s
```

### Root Cause Analysis

**Possible Causes**:

1. **Client Capability Check Failing** (Most Likely):
   - `SupportsBatchStreamingAsync()` test request to `__test__` design returns `NOT_FOUND` instead of `UNIMPLEMENTED`
   - Capability check catches exception and defaults to `false`
   - Client falls back to individual streaming

2. **Server Feature Flag Not Applied**:
   - Server config not loaded correctly
   - Feature flag check in code returns false

3. **Protobuf Mismatch**:
   - Client protobuf definitions don't match server's
   - `GetLayerFeaturesBatchStream` RPC not recognized

### Investigation Steps

**Server-Side Verification**:

1. **Verify Server Configuration**:
   ```bash
   # Check config.json
   cat OdbDesignServer/config.json | grep -A 3 batch_streaming
   # Expected: "enabled": true
   ```

2. **Verify Server Code**:
   - Check `OdbDesignServer/Services/OdbDesignServiceImpl.cpp` line 208
   - Verify feature flag check: `if (!m_config->enable_batch_streaming)`
   - Verify it returns `UNIMPLEMENTED` when disabled, batches when enabled

3. **Test Server Directly**:
   - Call `GetLayerFeaturesBatchStream` with real design/step/layer
   - Verify server returns batches (not `UNIMPLEMENTED`)

**Client-Side Investigation** (Client Team):

1. **Add Debug Logging** to `SupportsBatchStreamingAsync()`:
   ```csharp
   _logger.LogInformation("Checking if server supports batch streaming...");
   // ... existing code ...
   catch (RpcException ex)
   {
       _logger.LogWarning("Batch streaming capability check failed: StatusCode={StatusCode}, Message={Message}", 
           ex.StatusCode, ex.Message);
   }
   ```

2. **Change Test Request Strategy**:
   - Instead of `__test__` design (returns `NOT_FOUND`), use actual design
   - Or check for `UNIMPLEMENTED` vs any other error code

3. **Verify Protobuf Sync**:
   - Regenerate client protobuf definitions from server `.proto` files
   - Ensure `GetLayerFeaturesBatchStream` RPC is defined

### Server-Side Actions

**If Server Issue**:
- Verify config loading
- Verify feature flag is checked correctly
- Add server-side logging to verify batch streaming is enabled

**If Client Issue**:
- Document findings for client team
- Provide guidance on capability check fix

### Acceptance Criteria

- [ ] Server configuration verified (`batch_streaming.enabled = true`)
- [ ] Server code verified (feature flag check working)
- [ ] Server returns batches (not `UNIMPLEMENTED`) when called directly
- [ ] Client capability check fixed (if server-side issue)
- [ ] Client uses batch streaming for large layers
- [ ] Performance improves 5-10x for large layers

---

## Issue 3: Component Height Fix Not Working 🔴 **CRITICAL**

### Problem

**1,626 "Height not found" warnings still appearing despite:**
- ✅ Code updated to check `attributeLookupTable` first
- ✅ Server correctly parses and serializes `attributeLookupTable`
- ❌ **No "Found component height from attributeLookupTable" debug logs**

### Root Cause Analysis

**This is a CLIENT-SIDE issue**, but we should investigate server-side data:

**Possible Causes**:

1. **Server Not Populating `attributeLookupTable`**:
   - Server serialization issue
   - `componentsFile.AttributeLookupTable` is null or empty

2. **AttributeNames Not Populated**:
   - `componentsFile.AttributeNames` is null or empty
   - Can't find index of `.comp_height` attribute

3. **Data Genuinely Missing**:
   - ODB++ files don't have `.comp_height` attribute declared
   - Components don't have attribute ID strings with height values

### Server-Side Investigation

**Add Server-Side Logging** (if needed):

1. **Verify Server Serialization**:
   - Add logging in `GetDesign` or component serialization
   - Verify `attributeLookupTable` is populated
   - Verify `AttributeNames` includes `.comp_height`

2. **Inspect ODB++ Files**:
   - Check if `.comp_height` attribute is declared
   - Check if components have attribute ID strings

### Server-Side Actions

**If Server Issue**:
- Fix serialization to populate `attributeLookupTable`
- Ensure `AttributeNames` includes all attributes

**If Data Quality Issue**:
- Document that ODB++ files lack height data
- Provide guidance to client team

### Acceptance Criteria

- [ ] Server serialization verified (if server issue)
- [ ] ODB++ files inspected (if data quality issue)
- [ ] Root cause identified
- [ ] Fix implemented (if server-side)
- [ ] Client team notified (if client-side or data quality)

---

## Implementation Priority

### Phase 1: Critical Fix (Immediate)

1. **Increase Message Size Limit to 150MB** 🔴 **HIGHEST PRIORITY**
   - Blocks profile and component loading
   - Quick fix (config change + restart)
   - **Estimated Time**: 5 minutes

### Phase 2: Investigation (This Week)

2. **Investigate Batch Streaming** ⚠️ **HIGH PRIORITY**
   - Performance impact (10x slower without it)
   - Requires investigation (server + client)
   - **Estimated Time**: 2-4 hours

3. **Investigate Component Height** ⚠️ **HIGH PRIORITY**
   - Visual accuracy issue
   - May be data quality issue
   - **Estimated Time**: 1-2 hours
   - **Note**: Changed to HIGH PRIORITY per review comments

---

## Files to Modify

1. **`OdbDesignServer/config.json`** - Increase message size limits to 150MB
2. **`docs/plan/render-components2/debug-output-issues/server-implementation-handoff.md`** - Update with findings
3. **`docs/plan/render-components2/debug-output-issues/CRITICAL-ISSUES-RESPONSE.md`** - Create response document

---

#