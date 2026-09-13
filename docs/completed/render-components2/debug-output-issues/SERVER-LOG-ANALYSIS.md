# Server Log Analysis - Two Issues Identified

**Date**: January 2025  
**Source**: `server-side.log`  
**Status**: 🔍 **ANALYSIS COMPLETE** - Root causes identified

---

## Issue 1: UTF-8 Validation Warnings ⚠️

### Problem

**Warning Message** (appears ~15 times):
```
E0000 00:00:1767961821.209815   47264 wire_format_lite.cc:610] String field 'Odb.Lib.Protobuf.PropertyRecord.value' contains invalid UTF-8 data when serializing a protocol buffer. Use the 'bytes' type if you intend to send raw bytes.
```

### Root Cause Analysis

**Protobuf Definition** (`OdbDesignLib/protoc/common.proto`):
```protobuf
message PropertyRecord {   
    optional string name = 1;
    optional string value = 2;  // ← Defined as 'string' (requires UTF-8)
    repeated double floatValues = 3;
}
```

**Parsing Code** (`OdbDesignLib/FileModel/Design/PropertyRecord.cpp` and `ComponentsFile.cpp`):
- PropertyRecord values are parsed directly from ODB++ file tokens (lines 532, 575)
- No UTF-8 validation or encoding conversion performed
- Values assigned directly: `pPropertyRecord->value = token;`

**Issue**: ODB++ files may contain:
- Binary data in property values
- Non-UTF-8 text (e.g., Windows-1252, ISO-8859-1)
- Special characters not valid in UTF-8

When protobuf serializes these values, it validates UTF-8 and generates warnings for invalid sequences.

### Impact Assessment

**Severity**: ⚠️ **LOW** (Non-blocking)

- **Functional Impact**: None - warnings don't prevent serialization
- **Performance Impact**: Minimal - warnings are logged but don't affect operation
- **Data Integrity**: Potential - invalid UTF-8 sequences may be corrupted or lost
- **Log Pollution**: Medium - ~15 warnings per design load

### Solution Options

**Option 1: Change Protobuf Field Type to `bytes`** (Recommended for binary data)
- **Pros**: Supports binary data, no UTF-8 validation
- **Cons**: Breaking change, requires client updates, loses string semantics
- **Effort**: Medium (protobuf change + client updates)

**Option 2: UTF-8 Validation and Sanitization** (Recommended for text data)
- **Pros**: Maintains string type, validates/sanitizes data
- **Cons**: May lose some data if truly binary
- **Effort**: Low (add validation in parsing/serialization)

**Option 3: Suppress Warnings** (Quick fix)
- **Pros**: Quick, no code changes
- **Cons**: Doesn't fix root cause, data may still be corrupted
- **Effort**: Very Low (protobuf flag or logging filter)

### Recommended Fix

**Short-term**: Option 3 - Suppress warnings (if data integrity is acceptable)
**Long-term**: Option 2 - Add UTF-8 validation and sanitization during parsing

**Implementation** (Option 2):
```cpp
// In PropertyRecord::to_protobuf() or parsing code
std::string sanitizedValue = value;
if (!IsValidUTF8(value))
{
    // Convert to UTF-8 or sanitize
    sanitizedValue = ConvertToUTF8(value); // or sanitize invalid sequences
    // Log warning for debugging
}
pPropertyRecordMessage->set_value(sanitizedValue);
```

### Verification

After fix:
- [ ] No UTF-8 warnings in server logs
- [ ] Property values serialize correctly
- [ ] Client receives valid UTF-8 strings
- [ ] No data loss for valid text values

---

## Issue 2: Excessive DesignCache Retrievals ⚠️

### Problem

**Observation**: `GetFileArchive()` called ~180 times in <3 minutes
- All calls return "Found. Returning from cache" (cache is working)
- Pattern: Many rapid successive calls

**Log Pattern**:
```
[01/09/26 04:27:04.889 DesignCache.cpp :49 INFO] Retrieving design "designodb_rigidflex" from cache...
[01/09/26 04:27:04.889 DesignCache.cpp :69 INFO] Found. Returning from cache.
[01/09/26 04:27:04.893 DesignCache.cpp :49 INFO] Retrieving design "designodb_rigidflex" from cache...
[01/09/26 04:27:04.893 DesignCache.cpp :69 INFO] Found. Returning from cache.
... (repeats ~180 times)
```

### Root Cause Analysis

**Code Locations Calling `GetFileArchive()`**:

1. **`GetDesign` RPC** (line 121):
   - Calls `m_designCache->GetDesign()` (not GetFileArchive)
   - Called once per client request

2. **`GetLayerFeaturesStream` RPC** (line 145):
   - Calls `m_designCache->GetFileArchive()` 
   - Called once per layer streaming request

3. **`GetLayerFeaturesBatchStream` RPC** (line 219):
   - Calls `m_designCache->GetFileArchive()`
   - Called once per layer batch streaming request

4. **`GetLayerSymbols` RPC** (line 331):
   - Calls `m_designCache->GetFileArchive()`
   - Called once per layer symbols request

**Analysis**:
- Client loads 43 layers
- Each layer requires: `GetLayerSymbols` + `GetLayerFeaturesStream` (or batch)
- **Expected calls**: 43 layers × 2 RPCs = ~86 calls minimum
- **Actual calls**: ~180 calls = ~4.2 calls per layer

**Possible Causes**:
1. **Multiple RPC calls per layer** (normal):
   - GetLayerSymbols (1 call)
   - GetLayerFeaturesStream (1 call)
   - Additional metadata requests?
   - Retries?

2. **Client making redundant requests** (optimization opportunity):
   - Requesting same layer multiple times
   - Not caching layer metadata client-side
   - Making separate requests for symbols and features when could batch

3. **Server implementation pattern** (idiosyncrasy):
   - Each RPC independently fetches FileArchive
   - No request-level caching within single client session
   - Multiple concurrent requests from same client

### Impact Assessment

**Severity**: ⚠️ **LOW-MEDIUM** (Performance concern, not blocking)

**Current Performance**:
- Cache lookup is fast (in-memory map lookup)
- No disk I/O (all "Found. Returning from cache")
- Overhead: ~180 map lookups + log messages

**Potential Optimization**:
- Reduce redundant cache lookups
- Cache FileArchive reference per request context
- Reduce logging verbosity for cache hits

### Optimization Opportunities

**Option 1: Request-Level Caching** (Recommended)
- Cache `FileArchive` reference in `ServerContext` or request handler
- Reuse within same request instead of multiple cache lookups
- **Benefit**: Reduces redundant lookups, maintains thread safety

**Option 2: Reduce Logging Verbosity**
- Change cache hit logging from INFO to DEBUG
- Only log cache misses or first access
- **Benefit**: Reduces log noise, minimal performance improvement

**Option 3: Client-Side Optimization**
- Client caches layer metadata after first request
- Batch multiple requests per layer
- **Benefit**: Reduces server requests, better client performance

**Option 4: Accept Current Pattern** (If performance acceptable)
- Cache lookups are fast (in-memory)
- No disk I/O overhead
- Simple, thread-safe implementation
- **Benefit**: No code changes needed

### Recommended Action

**Short-term**: Option 2 - Reduce logging verbosity (quick win)
**Medium-term**: Option 1 - Request-level caching (if performance becomes issue)
**Long-term**: Option 3 - Client-side optimization (best overall improvement)

### Implementation (Option 2 - Quick Fix)

**File**: `OdbDesignLib/App/DesignCache.cpp`

**Change** (Line 48-49):
```cpp
// Current (INFO level):
ss << "Retrieving design \"" << designName << "\" from cache... ";
loginfo(ss.str());

// Change to (DEBUG level):
ss << "Retrieving design \"" << designName << "\" from cache... ";
logdebug(ss.str());  // Only log at DEBUG level for cache hits
```

**Change** (Line 69):
```cpp
// Current (INFO level):
loginfo("Found. Returning from cache.");

// Change to (DEBUG level):
logdebug("Found. Returning from cache.");  // Only log at DEBUG level
```

**Keep INFO level for**:
- Cache misses (line 54: "Not found in cache, attempting to load from file...")
- Successful loads (line 63: "Loaded from file")

### Implementation Status

✅ **COMPLETE**: Cache hit logging changed from INFO to DEBUG level
- **File Modified**: `OdbDesignLib/App/DesignCache.cpp`
- **Lines Changed**: 49 (cache retrieval log), 69 (cache hit log)
- Cache hits now log at DEBUG level (reduces log noise)
- Cache misses and loads remain at INFO level (important events)

### Verification

After optimization:
- ✅ Cache hit logs moved to DEBUG level
- ✅ Cache miss/load logs remain at INFO level
- ⏳ Log file size reduction (verify on next run)
- ⏳ Performance impact measured (if implementing request-level caching)

---

## Summary

### Issue 1: UTF-8 Warnings
- **Root Cause**: ODB++ property values contain non-UTF-8 data
- **Impact**: Low (warnings only, no functional impact)
- **Fix**: Add UTF-8 validation/sanitization (long-term) or suppress warnings (short-term)

### Issue 2: Excessive Cache Retrievals
- **Root Cause**: Each RPC call independently fetches FileArchive (normal pattern)
- **Impact**: Low-Medium (performance concern, cache is working efficiently)
- **Fix**: Reduce logging verbosity (quick fix) or implement request-level caching (optimization)

### Recommendations

**Immediate Actions**:
1. ✅ **COMPLETE** - Reduce cache hit logging to DEBUG level
2. ✅ **COMPLETE** - Document UTF-8 issue for future fix (low priority)

**Future Optimizations**:
1. Add UTF-8 validation/sanitization for PropertyRecord values (when needed)
2. Consider request-level caching if performance becomes issue
3. Coordinate with client team on request batching

---

**Document Version**: 1.0  
**Last Updated**: January 2025  
**Status**: Analysis Complete
