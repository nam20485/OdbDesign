# Component Height Investigation Guide

**Date**: January 2025  
**Status**: Ready for Execution  
**Test Components**: R70, R56, N5 (designodb_rigidflex); MTG1, TP5, J11 (sample_design)

---

## Overview

This guide explains how to run the component height investigation to identify where height data is being lost in the data flow from ODB++ parsing → protobuf serialization → gRPC transmission → client deserialization.

## Test Components

### designodb_rigidflex (comp_+_top layer)
- **R70** (Package 48): ✅ Has height data `;0=0.010236` → Should have `AttributeLookupTable["0"] = "0.010236"`
- **R56** (Package 48): ✅ Has height data `;0=0.010236` → Should have `AttributeLookupTable["0"] = "0.010236"`
- **N5** (Package 26): ❌ No height data `;1=1,2=2,3=1` (no `;0=`) → Should correctly omit key `"0"` and default to 1.0mm

### sample_design
- **MTG1** (Package 0): ✅ Has height data `;0=0.001000` → Should have `AttributeLookupTable["0"] = "0.001000"`
- **TP5** (Package 2): ✅ Has height data `;0=0.300000` → Should have `AttributeLookupTable["0"] = "0.300000"`
- **J11** (Package 1): ❌ No height data `;1=1` (no `;0=`) → Should correctly omit key `"0"` and default to 1.0mm

## ComponentHeightTracer Configuration

**Location**: `OdbDesignLib/FileModel/Design/ComponentHeightTracer.cpp`

**Configured Components**:
- R70, R56, N5 (designodb_rigidflex)
- MTG1, TP5, J11 (sample_design)
- First 20 failures (components missing height data)

**Logging Points**:
1. **Parse Start**: `[CompHeightTrace][PARSE_START]` - Logs raw attribute ID string from ODB++
2. **Parse Result**: `[CompHeightTrace][PARSE_RESULT]` - Logs parsed AttributeLookupTable and AttributeNames
3. **Serialization**: `[CompHeightTrace][SERIALIZE]` - Logs AttributeLookupTable before protobuf serialization
4. **gRPC Response**: `[CompHeightTrace][GRPC_RESPONSE]` - Logs AttributeLookupTable from protobuf after serialization

## Running the Investigation

### Step 1: Build the Server

```powershell
# Build with ComponentHeightTracer enabled (already compiled in)
cmake --build out/build/x64-debug --config Debug
```

### Step 2: Start the Server

```powershell
# Start OdbDesignServer
.\out\build\x64-debug\Debug\OdbDesignServer.exe
```

### Step 3: Request Designs via gRPC

Make gRPC requests for both designs:
- `designodb_rigidflex` (comp_+_top layer)
- `sample_design` (both top and bottom layers)

### Step 4: Analyze Logs

Search for `[CompHeightTrace]` entries in server logs:

```powershell
# Filter for component height traces
Select-String -Path "server.log" -Pattern "\[CompHeightTrace\]"
```

## Expected Log Patterns

### ✅ Component WITH Height Data (e.g., R70)

**Parse Start**:
```
[CompHeightTrace][PARSE_START] Component=R70, Package=48, attrIdString=";0=0.010236"
```

**Parse Result**:
```
[CompHeightTrace][PARSE_RESULT] Component=R70, Package=48, AttributeNames=[0:".comp_height", ...], AttributeLookupTable={"0":"0.010236"}, .comp_height@index0=YES, key"0"_exists=YES, height_value="0.010236"
```

**Serialization**:
```
[CompHeightTrace][SERIALIZE] Component=R70, Package=48, AttributeNames=[0:".comp_height", ...], AttributeLookupTable={"0":"0.010236"}, height_value="0.010236"
```

**gRPC Response**:
```
[CompHeightTrace][GRPC_RESPONSE] Component=R70, Package=48, AttributeNames=[0:".comp_height", ...], AttributeLookupTable={"0":"0.010236"}, height_value="0.010236"
```

### ❌ Component WITHOUT Height Data (e.g., N5)

**Parse Start**:
```
[CompHeightTrace][PARSE_START] Component=N5, Package=26, attrIdString=";1=1,2=2,3=1"
```

**Parse Result**:
```
[CompHeightTrace][PARSE_RESULT] Component=N5, Package=26, AttributeNames=[0:".comp_height", ...], AttributeLookupTable={"1":"1", "2":"2", "3":"1"}, .comp_height@index0=YES, key"0"_exists=NO ⚠️ MISSING_KEY_0
```

**Serialization**:
```
[CompHeightTrace][SERIALIZE] Component=N5, Package=26, AttributeNames=[0:".comp_height", ...], AttributeLookupTable={"1":"1", "2":"2", "3":"1"}, key"0"_missing
```

**gRPC Response**:
```
[CompHeightTrace][GRPC_RESPONSE] Component=N5, Package=26, AttributeNames=[0:".comp_height", ...], AttributeLookupTable={"1":"1", "2":"2", "3":"1"}, key"0"_missing
```

## Root Cause Analysis

### If Data is Lost at Parse Time

**Symptom**: `[CompHeightTrace][PARSE_RESULT]` shows `key"0"_exists=NO` for components WITH height data

**Fix**: Check `AttributeLookupTable::ParseAttributeLookupTable()` in `AttributeLookupTable.cpp`

### If Data is Lost at Serialization Time

**Symptom**: `[CompHeightTrace][SERIALIZE]` shows `key"0"_exists=YES` but `[CompHeightTrace][GRPC_RESPONSE]` shows `key"0"_missing`

**Fix**: Check `ComponentRecord::to_protobuf()` in `ComponentsFile.cpp` lines 228-231

### If Data is Lost During gRPC Transmission

**Symptom**: `[CompHeightTrace][GRPC_RESPONSE]` shows `key"0"_missing` but serialization was correct

**Fix**: Check protobuf message size limits or gRPC configuration

### If Data Reaches Client But Lookup Fails

**Symptom**: All logs show correct data, but client still defaults

**Fix**: Client-side debugging needed - verify:
1. Protobuf deserialization reads `attributeLookupTable` map correctly
2. Client checks if key `"0"` exists before lookup
3. Client uses correct key format (string `"0"`, not integer `0`)

## Verification Checklist

- [ ] R70: Parse shows `key"0"_exists=YES`, height_value="0.010236"
- [ ] R70: Serialize shows `height_value="0.010236"`
- [ ] R70: gRPC Response shows `height_value="0.010236"`
- [ ] R56: Parse shows `key"0"_exists=YES`, height_value="0.010236"
- [ ] R56: Serialize shows `height_value="0.010236"`
- [ ] R56: gRPC Response shows `height_value="0.010236"`
- [ ] N5: Parse shows `key"0"_exists=NO` (correct - no height data)
- [ ] N5: Serialize shows `key"0"_missing` (correct)
- [ ] N5: gRPC Response shows `key"0"_missing` (correct)
- [ ] MTG1: All stages show `height_value="0.001000"`
- [ ] TP5: All stages show `height_value="0.300000"`
- [ ] J11: All stages show `key"0"_missing` (correct - no height data)

## Next Steps After Investigation

1. **If server-side issue**: Fix parsing/serialization code
2. **If client-side issue**: Provide debugging data and fix guidance to client team
3. **If both**: Fix both sides
4. **Document**: Update plan with findings and resolution

## References

- Plan: `c:\Users\nmill\.cursor\plans\investigate_component_height_data_flow_issue_25500458.plan.md`
- Spec Confirmation: `docs/height-data/odb-spec-attribute-lookup-confirmation.md`
- ComponentHeightTracer: `OdbDesignLib/FileModel/Design/ComponentHeightTracer.cpp`
