# Client Team Debugging Guidance - Critical Issues Investigation

**Date**: January 2025  
**Purpose**: Provide deterministic logging and debugging guidance to identify root causes of three critical issues  
**Status**: 🔴 **ACTION REQUIRED** - Add logging and investigate

---

## Overview

This document provides specific logging guidance to help identify root causes of three critical issues identified in the latest integration test run. Add the suggested logging, run tests, and share logs with server team for analysis.

---

## Issue 1: Batch Streaming Not Being Used 🔴 **CRITICAL**

### Problem Summary

Client is not using batch streaming despite server having it enabled. No evidence in logs that batch streaming was attempted or used.

### Root Cause Investigation - Required Logging

#### 1. Add Logging to `SupportsBatchStreamingAsync()` Method

**File**: `src/OdbDesign3DClient.Core/Services/Implementations/OdbDesignGrpcClient.cs`

**Location**: Method `SupportsBatchStreamingAsync()` (around lines 316-365)

**Add Logging**:

```csharp
public async Task<bool> SupportsBatchStreamingAsync(CancellationToken cancellationToken = default)
{
    _logger.LogInformation("[BatchStream] Starting batch streaming capability check...");
    
    try
    {
        // Log the test request details
        var testRequest = new GetLayerFeaturesRequest
        {
            DesignName = "__test__",
            StepName = "__test__",
            LayerName = "__test__"
        };
        
        _logger.LogDebug("[BatchStream] Test request: Design={Design}, Step={Step}, Layer={Layer}", 
            testRequest.DesignName, testRequest.StepName, testRequest.LayerName);
        
        // ... existing code to call GetLayerFeaturesBatchStream ...
        
        _logger.LogInformation("[BatchStream] Capability check succeeded - server supports batch streaming");
        return true;
    }
    catch (RpcException ex)
    {
        _logger.LogWarning("[BatchStream] Capability check failed: StatusCode={StatusCode}, Detail={Detail}", 
            ex.StatusCode, ex.Status.Detail);
        
        // Log specific error codes
        if (ex.StatusCode == StatusCode.Unimplemented)
        {
            _logger.LogInformation("[BatchStream] Server returned UNIMPLEMENTED - batch streaming not available");
        }
        else if (ex.StatusCode == StatusCode.NotFound)
        {
            _logger.LogWarning("[BatchStream] Server returned NOT_FOUND for test design - this may cause false negative");
            _logger.LogInformation("[BatchStream] Suggestion: Try with real design/step/layer instead of __test__");
        }
        else
        {
            _logger.LogWarning("[BatchStream] Unexpected error code: {StatusCode}", ex.StatusCode);
        }
        
        return false;
    }
    catch (Exception ex)
    {
        _logger.LogError(ex, "[BatchStream] Unexpected exception during capability check");
        return false;
    }
}
```

#### 2. Add Logging to Layer Loading Decision Point

**File**: `src/OdbDesign3DClient.Core/ViewModels/MainViewModel.cs` (or wherever layer loading happens)

**Location**: Where `SupportsBatchStreamingAsync()` result is used to decide streaming method

**Add Logging**:

```csharp
// Before calling SupportsBatchStreamingAsync
_logger.LogInformation("[LayerLoad] Checking batch streaming support before loading layer {LayerName}", layerName);

bool supportsBatch = await _grpcClient.SupportsBatchStreamingAsync(cancellationToken);

_logger.LogInformation("[LayerLoad] Batch streaming support result: {SupportsBatch} for layer {LayerName}", 
    supportsBatch, layerName);

if (supportsBatch)
{
    _logger.LogInformation("[LayerLoad] Using BATCH streaming for layer {LayerName}", layerName);
    // Call GetLayerFeaturesBatchStreamAsync
}
else
{
    _logger.LogInformation("[LayerLoad] Using INDIVIDUAL streaming for layer {LayerName} (batch not supported)", layerName);
    // Call GetLayerFeaturesStreamAsync
}
```

#### 3. Add Logging to Batch Stream Processing

**File**: `src/OdbDesign3DClient.Core/Services/Implementations/OdbDesignGrpcClient.cs`

**Location**: `GetLayerFeaturesBatchStreamAsync()` method

**Add Logging**:

```csharp
public async Task GetLayerFeaturesBatchStreamAsync(...)
{
    _logger.LogInformation("[BatchStream] Starting batch stream for Design={Design}, Step={Step}, Layer={Layer}", 
        designName, stepName, layerName);
    
    int totalFeatures = 0;
    int batchCount = 0;
    
    await foreach (var batch in call.ResponseStream.ReadAllAsync(cancellationToken))
    {
        batchCount++;
        int batchSize = batch.Features.Count;
        totalFeatures += batchSize;
        
        _logger.LogDebug("[BatchStream] Received batch #{BatchNum}: {BatchSize} features (total: {Total})", 
            batchCount, batchSize, totalFeatures);
        
        // Process batch...
    }
    
    _logger.LogInformation("[BatchStream] Completed batch stream: {TotalFeatures} features in {BatchCount} batches for layer {LayerName}", 
        totalFeatures, batchCount, layerName);
}
```

### Verification Checklist

After adding logging, verify:

- [ ] `SupportsBatchStreamingAsync()` logs show capability check result
- [ ] If `false`, logs show why (UNIMPLEMENTED, NOT_FOUND, or other error)
- [ ] Layer loading logs show which streaming method is used
- [ ] If batch streaming is used, logs show batch reception
- [ ] If individual streaming is used, logs show why batch wasn't used

### Expected Log Patterns

**If Batch Streaming Works**:
```
[BatchStream] Starting batch streaming capability check...
[BatchStream] Capability check succeeded - server supports batch streaming
[LayerLoad] Batch streaming support result: True for layer layer-1
[LayerLoad] Using BATCH streaming for layer layer-1
[BatchStream] Starting batch stream for Design=designodb_rigidflex, Step=cellular_flip-phone, Layer=layer-1
[BatchStream] Received batch #1: 500 features (total: 500)
[BatchStream] Received batch #2: 500 features (total: 1000)
...
[BatchStream] Completed batch stream: 11868 features in 24 batches for layer layer-1
```

**If Capability Check Fails**:
```
[BatchStream] Starting batch streaming capability check...
[BatchStream] Capability check failed: StatusCode=NotFound, Detail=Design not found: __test__
[BatchStream] Server returned NOT_FOUND for test design - this may cause false negative
[LayerLoad] Batch streaming support result: False for layer layer-1
[LayerLoad] Using INDIVIDUAL streaming for layer layer-1 (batch not supported)
```

---

## Issue 2: Component Height Fix Not Working 🔴 **CRITICAL**

### Problem Summary

1,626 "Height not found" warnings still appearing. No "Found component height from attributeLookupTable" debug logs, suggesting the fix isn't working.

### Root Cause Investigation - Required Logging

#### 1. Add Comprehensive Logging to `GetHeight()` Method

**File**: `src/OdbDesign3DClient.Core/Services/Implementations/ComponentToShapeConverter.cs`

**Location**: `GetHeight()` method (around lines 78-100)

**Add Logging**:

```csharp
private float GetHeight(ComponentsFile.Types.ComponentRecord comp, 
                        EdaDataFile.Types.PackageRecord pkg, 
                        ComponentsFile componentsFile, 
                        float scale)
{
    _logger.LogDebug("[CompHeight] GetHeight called for Component={CompName}, Package={PkgRef}", 
        comp.CompName, comp.PkgRef);
    
    // Log componentsFile state
    if (componentsFile == null)
    {
        _logger.LogWarning("[CompHeight] componentsFile is NULL for Component={CompName}", comp.CompName);
    }
    else
    {
        _logger.LogDebug("[CompHeight] componentsFile provided for Component={CompName}", comp.CompName);
        
        // Log AttributeNames
        if (componentsFile.AttributeNames == null)
        {
            _logger.LogWarning("[CompHeight] AttributeNames is NULL for Component={CompName}", comp.CompName);
        }
        else
        {
            _logger.LogDebug("[CompHeight] AttributeNames count: {Count} for Component={CompName}", 
                componentsFile.AttributeNames.Count, comp.CompName);
            
            // Log all attribute names (first 10 to avoid spam)
            var attrNames = componentsFile.AttributeNames.Take(10).ToList();
            _logger.LogDebug("[CompHeight] AttributeNames (first 10): {Names}", 
                string.Join(", ", attrNames));
            
            // Check for .comp_height specifically
            int compHeightIndex = -1;
            for (int i = 0; i < componentsFile.AttributeNames.Count; i++)
            {
                if (string.Equals(componentsFile.AttributeNames[i], ".comp_height", StringComparison.OrdinalIgnoreCase))
                {
                    compHeightIndex = i;
                    _logger.LogDebug("[CompHeight] Found .comp_height at index {Index} for Component={CompName}", 
                        i, comp.CompName);
                    break;
                }
            }
            
            if (compHeightIndex < 0)
            {
                _logger.LogWarning("[CompHeight] .comp_height NOT FOUND in AttributeNames for Component={CompName}", 
                    comp.CompName);
            }
        }
        
        // Log AttributeLookupTable
        if (comp.AttributeLookupTable == null)
        {
            _logger.LogWarning("[CompHeight] Component AttributeLookupTable is NULL for Component={CompName}", 
                comp.CompName);
        }
        else
        {
            _logger.LogDebug("[CompHeight] Component AttributeLookupTable count: {Count} for Component={CompName}", 
                comp.AttributeLookupTable.Count, comp.CompName);
            
            if (comp.AttributeLookupTable.Count > 0)
            {
                // Log first few keys (to see what's in there)
                var keys = comp.AttributeLookupTable.Keys.Take(5).ToList();
                _logger.LogDebug("[CompHeight] AttributeLookupTable keys (first 5): {Keys}", 
                    string.Join(", ", keys));
            }
            else
            {
                _logger.LogWarning("[CompHeight] AttributeLookupTable is EMPTY for Component={CompName}", 
                    comp.CompName);
            }
        }
    }
    
    // Existing attributeLookupTable lookup code with enhanced logging
    if (componentsFile != null && componentsFile.AttributeNames != null)
    {
        int compHeightIndex = -1;
        for (int i = 0; i < componentsFile.AttributeNames.Count; i++)
        {
            if (string.Equals(componentsFile.AttributeNames[i], ".comp_height", StringComparison.OrdinalIgnoreCase))
            {
                compHeightIndex = i;
                break;
            }
        }

        if (compHeightIndex >= 0 && comp.AttributeLookupTable != null)
        {
            string indexKey = compHeightIndex.ToString();
            _logger.LogDebug("[CompHeight] Looking up index key '{IndexKey}' in AttributeLookupTable for Component={CompName}", 
                indexKey, comp.CompName);
            
            if (comp.AttributeLookupTable.TryGetValue(indexKey, out string? heightValue))
            {
                _logger.LogDebug("[CompHeight] Found heightValue='{HeightValue}' for Component={CompName}", 
                    heightValue, comp.CompName);
                
                if (string.IsNullOrWhiteSpace(heightValue))
                {
                    _logger.LogWarning("[CompHeight] heightValue is NULL or WHITESPACE for Component={CompName}", 
                        comp.CompName);
                }
                else if (float.TryParse(heightValue, out var heightFromAttr))
                {
                    _logger.LogInformation("[CompHeight] ✅ SUCCESS: Found height={Height} from attributeLookupTable for Component={CompName}, Package={PkgRef}", 
                        heightFromAttr, comp.CompName, comp.PkgRef);
                    return heightFromAttr * scale;
                }
                else
                {
                    _logger.LogWarning("[CompHeight] Failed to parse heightValue='{HeightValue}' as float for Component={CompName}", 
                        heightValue, comp.CompName);
                }
            }
            else
            {
                _logger.LogWarning("[CompHeight] Index key '{IndexKey}' NOT FOUND in AttributeLookupTable for Component={CompName}", 
                    indexKey, comp.CompName);
            }
        }
        else
        {
            if (compHeightIndex < 0)
            {
                _logger.LogWarning("[CompHeight] .comp_height index not found in AttributeNames for Component={CompName}", 
                    comp.CompName);
            }
            if (comp.AttributeLookupTable == null)
            {
                _logger.LogWarning("[CompHeight] AttributeLookupTable is null for Component={CompName}", 
                    comp.CompName);
            }
        }
    }
    
    // Fallback to PropertyRecords (existing code)
    // ... existing PropertyRecords lookup ...
    
    // Default fallback
    _logger.LogWarning("[CompHeight] ❌ FAILED: Height not found for Component={CompName} (Package {PkgRef}). Defaulting to 1.0", 
        comp.CompName, comp.PkgRef);
    return 1.0f * scale;
}
```

#### 2. Add Logging to Component Data Deserialization

**File**: Wherever `ComponentsFile` is deserialized from protobuf

**Add Logging**:

```csharp
// After deserializing ComponentsFile from GetDesign response
_logger.LogDebug("[CompHeight] Deserialized ComponentsFile: AttributeNames count={Count}, Components count={CompCount}", 
    componentsFile.AttributeNames?.Count ?? 0, 
    componentsFile.Components?.Count ?? 0);

// Log a sample component's AttributeLookupTable
if (componentsFile.Components != null && componentsFile.Components.Count > 0)
{
    var sampleComp = componentsFile.Components[0];
    _logger.LogDebug("[CompHeight] Sample component '{CompName}': AttributeLookupTable count={Count}", 
        sampleComp.CompName, 
        sampleComp.AttributeLookupTable?.Count ?? 0);
    
    if (sampleComp.AttributeLookupTable != null && sampleComp.AttributeLookupTable.Count > 0)
    {
        var sampleKeys = sampleComp.AttributeLookupTable.Keys.Take(5).ToList();
        _logger.LogDebug("[CompHeight] Sample component AttributeLookupTable keys: {Keys}", 
            string.Join(", ", sampleKeys));
    }
}
```

### Verification Checklist

After adding logging, verify:

- [ ] Logs show `componentsFile` is not null
- [ ] Logs show `AttributeNames` is populated (and includes `.comp_height`)
- [ ] Logs show `AttributeLookupTable` is populated for components
- [ ] Logs show lookup attempts and results
- [ ] If lookup fails, logs show why (missing index, empty value, parse failure)

### Expected Log Patterns

**If Fix Works**:
```
[CompHeight] GetHeight called for Component=MTG1, Package=0
[CompHeight] componentsFile provided for Component=MTG1
[CompHeight] AttributeNames count: 5 for Component=MTG1
[CompHeight] Found .comp_height at index 0 for Component=MTG1
[CompHeight] Component AttributeLookupTable count: 2 for Component=MTG1
[CompHeight] Looking up index key '0' in AttributeLookupTable for Component=MTG1
[CompHeight] Found heightValue='0.001378' for Component=MTG1
[CompHeight] ✅ SUCCESS: Found height=0.001378 from attributeLookupTable for Component=MTG1, Package=0
```

**If AttributeLookupTable Empty**:
```
[CompHeight] GetHeight called for Component=MTG1, Package=0
[CompHeight] Component AttributeLookupTable is EMPTY for Component=MTG1
[CompHeight] ❌ FAILED: Height not found for Component=MTG1 (Package 0). Defaulting to 1.0
```

**If AttributeNames Missing**:
```
[CompHeight] GetHeight called for Component=MTG1, Package=0
[CompHeight] AttributeNames is NULL for Component=MTG1
[CompHeight] ❌ FAILED: Height not found for Component=MTG1 (Package 0). Defaulting to 1.0
```

---

## Issue 3: Server Message Size Limit Too Small 🔴 **CRITICAL**

### Problem Summary

Profile response from `GetDesign` RPC is 116MB, but server limit is 100MB, causing `ResourceExhausted` errors.

### Root Cause Investigation - Required Logging

#### 1. Add Logging to `GetDesign` Call

**File**: `src/OdbDesign3DClient.Core/Services/Implementations/OdbDesignGrpcClient.cs`

**Location**: `GetDesignAsync()` method

**Add Logging**:

```csharp
public async Task<ProductModel.Design> GetDesignAsync(string designName, CancellationToken cancellationToken = default)
{
    _logger.LogInformation("[GetDesign] Requesting design: {DesignName}", designName);
    
    try
    {
        var request = new GetDesignRequest { DesignName = designName };
        
        // Log client message size limits
        _logger.LogDebug("[GetDesign] Client MaxReceiveMessageSize: {Size} bytes ({SizeMB} MB)", 
            _channel.MaxReceiveMessageSize, 
            _channel.MaxReceiveMessageSize / (1024 * 1024));
        
        var response = await Client.GetDesignAsync(request, cancellationToken: cancellationToken);
        
        // Calculate response size (approximate)
        int responseSize = response.CalculateSize();
        double responseSizeMB = responseSize / (1024.0 * 1024.0);
        
        _logger.LogInformation("[GetDesign] ✅ Success: Received design {DesignName}, Response size: {Size} bytes ({SizeMB:F2} MB)", 
            designName, responseSize, responseSizeMB);
        
        return response;
    }
    catch (RpcException ex) when (ex.StatusCode == StatusCode.ResourceExhausted)
    {
        _logger.LogError(ex, "[GetDesign] ❌ ResourceExhausted error for design {DesignName}: {Detail}", 
            designName, ex.Status.Detail);
        
        // Parse error detail to extract size information
        if (ex.Status.Detail.Contains("larger than max"))
        {
            _logger.LogError("[GetDesign] Message size exceeded server limit. Detail: {Detail}", ex.Status.Detail);
        }
        
        throw;
    }
    catch (Exception ex)
    {
        _logger.LogError(ex, "[GetDesign] Unexpected error requesting design {DesignName}", designName);
        throw;
    }
}
```

#### 2. Add Logging to Profile File Extraction

**File**: Wherever profile file is extracted from `GetDesign` response

**Add Logging**:

```csharp
// After GetDesign response received
var design = await _grpcClient.GetDesignAsync(designName, cancellationToken);

_logger.LogDebug("[Profile] Extracting profile file from design response...");

if (design.FileModel?.Steps != null)
{
    foreach (var step in design.FileModel.Steps)
    {
        _logger.LogDebug("[Profile] Step: {StepName}", step.Name);
        
        if (step.ProfileFile != null)
        {
            // Calculate profile file size
            int profileSize = step.ProfileFile.CalculateSize();
            double profileSizeMB = profileSize / (1024.0 * 1024.0);
            
            _logger.LogInformation("[Profile] Profile file found for step {StepName}: Size={Size} bytes ({SizeMB:F2} MB)", 
                step.Name, profileSize, profileSizeMB);
            
            if (profileSizeMB > 100)
            {
                _logger.LogWarning("[Profile] ⚠️ Profile file size ({SizeMB:F2} MB) exceeds 100MB limit!", profileSizeMB);
            }
        }
        else
        {
            _logger.LogWarning("[Profile] Profile file is NULL for step {StepName}", step.Name);
        }
    }
}
```

### Verification Checklist

After adding logging, verify:

- [ ] Logs show client message size limits
- [ ] Logs show `GetDesign` response size
- [ ] If `ResourceExhausted` occurs, logs show error details
- [ ] Logs show profile file size if extracted successfully

### Expected Log Patterns

**If Message Size OK**:
```
[GetDesign] Requesting design: designodb_rigidflex
[GetDesign] Client MaxReceiveMessageSize: 104857600 bytes (100 MB)
[GetDesign] ✅ Success: Received design designodb_rigidflex, Response size: 116196777 bytes (110.78 MB)
[Profile] Profile file found for step cellular_flip-phone: Size=116196777 bytes (110.78 MB)
```

**If ResourceExhausted**:
```
[GetDesign] Requesting design: designodb_rigidflex
[GetDesign] Client MaxReceiveMessageSize: 104857600 bytes (100 MB)
[GetDesign] ❌ ResourceExhausted error for design designodb_rigidflex: SERVER: Sent message larger than max (116196777 vs. 104857600)
[GetDesign] Message size exceeded server limit. Detail: SERVER: Sent message larger than max (116196777 vs. 104857600)
```

---

## General Logging Best Practices

### Log Levels

- **Information**: Important state changes, method entry/exit, capability checks
- **Debug**: Detailed data inspection, intermediate values, lookup attempts
- **Warning**: Non-fatal issues, fallbacks, missing data
- **Error**: Exceptions, failures, critical issues

### Log Prefixes

Use consistent prefixes for easy filtering:
- `[BatchStream]` - Batch streaming related
- `[CompHeight]` - Component height related
- `[GetDesign]` - GetDesign RPC related
- `[Profile]` - Profile file related
- `[LayerLoad]` - Layer loading related

### Log Filtering

After adding logs, filter by prefix to focus on specific issues:

```bash
# Filter batch streaming logs
grep "\[BatchStream\]" debug-output.txt

# Filter component height logs
grep "\[CompHeight\]" debug-output.txt

# Filter GetDesign logs
grep "\[GetDesign\]" debug-output.txt
```

---

## Next Steps

1. **Add Logging**: Implement logging as specified above
2. **Run Test**: Execute integration test with `designodb_rigidflex/cellular_flip-phone`
3. **Capture Logs**: Save full debug output to file
4. **Share Logs**: Send logs to server team for analysis
5. **Analyze**: Review logs to identify root causes

---

## Questions to Answer

After adding logging, the logs should answer:

### Batch Streaming:
- ✅ Does `SupportsBatchStreamingAsync()` return `true` or `false`?
- ✅ If `false`, what error code is returned (UNIMPLEMENTED, NOT_FOUND, other)?
- ✅ Is the capability check using `__test__` design causing false negatives?
- ✅ Does the client actually call `GetLayerFeaturesBatchStreamAsync()` when capability is `true`?

### Component Height:
- ✅ Is `componentsFile` null or populated?
- ✅ Is `AttributeNames` populated and does it include `.comp_height`?
- ✅ Is `AttributeLookupTable` populated for components?
- ✅ What index is `.comp_height` at?
- ✅ Does the lookup succeed but value is empty/invalid?
- ✅ Is the data missing from ODB++ files or from server serialization?

### Message Size:
- ✅ What is the actual `GetDesign` response size?
- ✅ What are the client message size limits?
- ✅ Does the error occur on client or server side?
- ✅ What is the profile file size specifically?

---

**Document Version**: 1.0  
**Last Updated**: January 2025  
**Status**: Ready for Client Team Implementation
