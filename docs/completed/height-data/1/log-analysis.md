# Component Height Log Analysis

**Date**: January 14, 2026  
**Session**: Matching client/server debug logs  
**Server Log**: `docs/height-data/1/server-debug-output.txt`  
**Client Log**: `docs/height-data/1/client-debug-output.txt` (empty - client-side logging not yet implemented)

---

## Executive Summary

### Components Analyzed
- **R70** (Package 48, designodb_rigidflex)
- **R56** (Package 48, designodb_rigidflex)
- **N5** (Package 26, designodb_rigidflex)
- **MTG1** (Package 0, sample_design)
- **TP5** (Package 160, designodb_rigidflex; Package 2, sample_design)
- **J11** (Package 1, sample_design; Package 18, sample_design)

### Data Flow Stages Verified
1. **PARSE_START**: Raw ODB++ attribute ID string
2. **PARSE_RESULT**: Parsed AttributeLookupTable and AttributeNames array
3. **SERIALIZE**: AttributeLookupTable before protobuf serialization
4. **GRPC_RESPONSE**: AttributeLookupTable in gRPC response after serialization

### Key Findings

✅ **Server correctly preserves height data** for components that have it in the ODB++ file  
✅ **Server correctly omits height data** for components that don't have it (as per ODB++ spec)  
✅ **No data loss** occurs through parsing → serialization → gRPC transmission  
⚠️ **Index mapping varies** by component type - height is not always at index 0

---

## Per-Component Analysis

### R70 (Package 48, designodb_rigidflex)

**Status**: ✅ Height data present and preserved throughout pipeline

#### Timeline

**PARSE_START** (06:18:16.863):
```
attrIdString=";0=0.010236,1=1,2=9,3=0;ID=3929"
```

**PARSE_RESULT** (06:18:16.863):
```
AttributeNames=[0:".comp_height", 1:".comp_mount_type", 2:".comp_type2", 3:".package_type"]
AttributeLookupTable={"0":"0.010236", "1":"1", "2":"9", "3":"0", "ID":"3929"}
.comp_height@index0=YES
key"0"_exists=YES
height_value="0.010236"
```

**SERIALIZE** (06:32:48.636, repeated multiple times):
```
AttributeNames=[0:".comp_height", 1:".comp_mount_type", 2:".comp_type2", 3:".package_type"]
AttributeLookupTable={"0":"0.010236", "1":"1", "2":"9", "3":"0", "ID":"3929"}
height_value="0.010236"
```

**GRPC_RESPONSE** (06:32:53.063, repeated multiple times):
```
AttributeNames=[0:".comp_height", 1:".comp_mount_type", 2:".comp_type2", 3:".package_type"]
AttributeLookupTable={"0":"0.010236", "1":"1", "2":"9", "3":"0", "ID":"3929"}
height_value="0.010236"
```

#### Height Extraction
- **Index**: 0
- **AttributeName**: `.comp_height`
- **Lookup Key**: `"0"`
- **Value**: `"0.010236"` (0.010236 mm)
- **Consistency**: ✅ Same value through all stages

---

### R56 (Package 48, designodb_rigidflex)

**Status**: ✅ Height data present and preserved throughout pipeline

#### Timeline

**PARSE_START** (06:18:16.859):
```
attrIdString=";0=0.010236,1=1,2=9,3=0;ID=3896"
```

**PARSE_RESULT** (06:18:16.859):
```
AttributeNames=[0:".comp_height", 1:".comp_mount_type", 2:".comp_type2", 3:".package_type"]
AttributeLookupTable={"0":"0.010236", "1":"1", "2":"9", "3":"0", "ID":"3896"}
.comp_height@index0=YES
key"0"_exists=YES
height_value="0.010236"
```

**SERIALIZE** (06:32:48.635, repeated multiple times):
```
AttributeNames=[0:".comp_height", 1:".comp_mount_type", 2:".comp_type2", 3:".package_type"]
AttributeLookupTable={"0":"0.010236", "1":"1", "2":"9", "3":"0", "ID":"3896"}
height_value="0.010236"
```

**GRPC_RESPONSE** (06:32:53.062, repeated multiple times):
```
AttributeNames=[0:".comp_height", 1:".comp_mount_type", 2:".comp_type2", 3:".package_type"]
AttributeLookupTable={"0":"0.010236", "1":"1", "2":"9", "3":"0", "ID":"3896"}
height_value="0.010236"
```

#### Height Extraction
- **Index**: 0
- **AttributeName**: `.comp_height`
- **Lookup Key**: `"0"`
- **Value**: `"0.010236"` (0.010236 mm)
- **Consistency**: ✅ Same value through all stages

---

### N5 (Package 26, designodb_rigidflex)

**Status**: ✅ Correctly omits height data (not present in ODB++ file)

#### Timeline

**PARSE_START** (06:18:16.886):
```
attrIdString=";1=1,2=2,3=1;ID=4262"
```
**Note**: No `;0=` in the attribute string - height data is legitimately missing.

**PARSE_RESULT** (06:18:16.886):
```
AttributeNames=[0:".comp_height", 1:".comp_mount_type", 2:".comp_type2", 3:".package_type"]
AttributeLookupTable={"1":"1", "2":"2", "3":"1", "ID":"4262"}
.comp_height@index0=YES
key"0"_exists=NO ⚠️ MISSING_KEY_0
```

**SERIALIZE** (06:32:48.641, repeated multiple times):
```
AttributeNames=[0:".comp_height", 1:".comp_mount_type", 2:".comp_type2", 3:".package_type"]
AttributeLookupTable={"1":"1", "2":"2", "3":"1", "ID":"4262"}
key"0"_missing
```

**GRPC_RESPONSE** (06:32:53.063, repeated multiple times):
```
AttributeNames=[0:".comp_height", 1:".comp_mount_type", 2:".comp_type2", 3:".package_type"]
AttributeLookupTable={"1":"1", "2":"2", "3":"1", "ID":"4262"}
key"0"_missing
```

#### Height Extraction
- **Index**: 0 (AttributeNames indicates `.comp_height` should be at index 0)
- **Lookup Key**: `"0"` - **MISSING**
- **Expected Behavior**: Client should default to 1.0mm when key `"0"` is missing
- **Consistency**: ✅ Missing consistently through all stages (correct behavior)

---

### MTG1 (Package 0, sample_design)

**Status**: ✅ Height data present and preserved throughout pipeline

#### Timeline

**PARSE_START**: Not found in logs (component parsed before tracing started)

**SERIALIZE** (06:36:14.716, repeated multiple times):
```
AttributeNames=[0:".comp_height", 1:".comp_mount_type"]
AttributeLookupTable={"0":"0.001000", "1":"2", "ID":"61442"}
height_value="0.001000"
```

**GRPC_RESPONSE** (06:36:16.269, repeated multiple times):
```
AttributeNames=[0:".comp_height", 1:".comp_mount_type"]
AttributeLookupTable={"0":"0.001000", "1":"2", "ID":"61442"}
height_value="0.001000"
```

#### Height Extraction
- **Index**: 0
- **AttributeName**: `.comp_height`
- **Lookup Key**: `"0"`
- **Value**: `"0.001000"` (0.001 mm)
- **Consistency**: ✅ Same value through all stages

---

### TP5 (Package 160, designodb_rigidflex)

**Status**: ✅ Height data present and preserved throughout pipeline

#### Timeline

**PARSE_START** (06:18:16.854):
```
attrIdString=";0=0.001378,1=1;ID=4350"
```

**PARSE_RESULT** (06:18:16.854):
```
AttributeNames=[0:".comp_height", 1:".comp_mount_type", 2:".comp_type2", 3:".package_type"]
AttributeLookupTable={"0":"0.001378", "1":"1", "ID":"4350"}
.comp_height@index0=YES
key"0"_exists=YES
height_value="0.001378"
```

**SERIALIZE** (06:32:48.633, repeated multiple times):
```
AttributeNames=[0:".comp_height", 1:".comp_mount_type", 2:".comp_type2", 3:".package_type"]
AttributeLookupTable={"0":"0.001378", "1":"1", "ID":"4350"}
height_value="0.001378"
```

**GRPC_RESPONSE** (06:32:53.063, repeated multiple times):
```
AttributeNames=[0:".comp_height", 1:".comp_mount_type", 2:".comp_type2", 3:".package_type"]
AttributeLookupTable={"0":"0.001378", "1":"1", "ID":"4350"}
height_value="0.001378"
```

#### Height Extraction
- **Index**: 0
- **AttributeName**: `.comp_height`
- **Lookup Key**: `"0"`
- **Value**: `"0.001378"` (0.001378 mm)
- **Consistency**: ✅ Same value through all stages

---

### TP5 (Package 2, sample_design)

**Status**: ✅ Height data present and preserved throughout pipeline

#### Timeline

**PARSE_START**: Not found in logs (component parsed before tracing started)

**SERIALIZE** (06:36:14.717, repeated multiple times):
```
AttributeNames=[0:".comp_height", 1:".comp_mount_type"]
AttributeLookupTable={"0":"0.300000", "1":"2", "ID":"61540"}
height_value="0.300000"
```

**GRPC_RESPONSE** (06:36:16.269, repeated multiple times):
```
AttributeNames=[0:".comp_height", 1:".comp_mount_type"]
AttributeLookupTable={"0":"0.300000", "1":"2", "ID":"61540"}
height_value="0.300000"
```

#### Height Extraction
- **Index**: 0
- **AttributeName**: `.comp_height`
- **Lookup Key**: `"0"`
- **Value**: `"0.300000"` (0.3 mm)
- **Consistency**: ✅ Same value through all stages

---

### J11 (Package 1, sample_design)

**Status**: ✅ Correctly omits height data (not present in ODB++ file)

#### Timeline

**PARSE_START**: Not found in logs (component parsed before tracing started)

**SERIALIZE** (06:36:14.717, repeated multiple times):
```
AttributeNames=[0:".comp_height", 1:".comp_mount_type"]
AttributeLookupTable={"1":"1", "ID":"61481"}
key"0"_missing
```

**GRPC_RESPONSE** (06:36:16.269, repeated multiple times):
```
AttributeNames=[0:".comp_height", 1:".comp_mount_type"]
AttributeLookupTable={"1":"1", "ID":"61481"}
key"0"_missing
```

#### Height Extraction
- **Index**: 0 (AttributeNames indicates `.comp_height` should be at index 0)
- **Lookup Key**: `"0"` - **MISSING**
- **Expected Behavior**: Client should default to 1.0mm when key `"0"` is missing
- **Consistency**: ✅ Missing consistently through all stages (correct behavior)

---

### J11 (Package 18, sample_design)

**Status**: ⚠️ Height data present but at index 1, not index 0

#### Timeline

**PARSE_START** (06:37:21.779):
```
attrIdString=";89=2,1=0.430000;ID=4709"
```
**Note**: Height value is at index `1` (`;1=0.430000`), not index `0`.

**PARSE_RESULT** (06:37:21.779):
```
AttributeNames=[0:".comp_height_max", 1:".comp_height", 2:".comp_htol_plus", ...]
AttributeLookupTable={"1":"0.430000", "89":"2", "ID":"4709"}
.comp_height@index0=NO
key"0"_exists=NO
```
**Critical**: `.comp_height` is at **index 1**, not index 0. `.comp_height_max` is at index 0.

**SERIALIZE** (06:38:03.354, repeated multiple times):
```
AttributeNames=[0:".comp_height_max", 1:".comp_height", 2:".comp_htol_plus", ...]
AttributeLookupTable={"1":"0.430000", "89":"2", "ID":"4709"}
```
**Note**: Height value preserved at key `"1"`.

**GRPC_RESPONSE** (06:38:15.563, repeated multiple times):
```
AttributeNames=[0:".comp_height_max", 1:".comp_height", 2:".comp_htol_plus", ...]
AttributeLookupTable={"1":"0.430000", "89":"2", "ID":"4709"}
```
**Note**: Height value preserved at key `"1"`.

#### Height Extraction
- **Index**: 1 (NOT 0)
- **AttributeName**: `.comp_height` (at index 1)
- **Lookup Key**: `"1"` (NOT `"0"`)
- **Value**: `"0.430000"` (0.43 mm)
- **Consistency**: ✅ Same value through all stages
- **⚠️ Critical**: Client must check AttributeNames array to find which index corresponds to `.comp_height`, not assume index 0

---

## Data Flow Verification

### Summary Table

| Component | Package | Design | Height Present | Index | Key | Parse | Serialize | GRPC | Status |
|-----------|---------|--------|----------------|-------|-----|-------|-----------|------|--------|
| R70 | 48 | rigidflex | ✅ Yes | 0 | "0" | ✅ | ✅ | ✅ | ✅ Consistent |
| R56 | 48 | rigidflex | ✅ Yes | 0 | "0" | ✅ | ✅ | ✅ | ✅ Consistent |
| N5 | 26 | rigidflex | ❌ No | 0 | "0" (missing) | ❌ | ❌ | ❌ | ✅ Correct (missing) |
| MTG1 | 0 | sample | ✅ Yes | 0 | "0" | N/A | ✅ | ✅ | ✅ Consistent |
| TP5 | 160 | rigidflex | ✅ Yes | 0 | "0" | ✅ | ✅ | ✅ | ✅ Consistent |
| TP5 | 2 | sample | ✅ Yes | 0 | "0" | N/A | ✅ | ✅ | ✅ Consistent |
| J11 | 1 | sample | ❌ No | 0 | "0" (missing) | N/A | ❌ | ❌ | ✅ Correct (missing) |
| J11 | 18 | sample | ✅ Yes | **1** | **"1"** | ✅ | ✅ | ✅ | ⚠️ **Index 1, not 0** |

### Verification Results

✅ **No data loss detected**: All components that have height data in the ODB++ file retain it through all stages  
✅ **Correct omission**: Components without height data correctly omit key `"0"` throughout  
✅ **Protobuf serialization**: Correctly preserves AttributeLookupTable structure  
✅ **gRPC transmission**: Correctly transmits AttributeLookupTable to client

---

## Index Mapping Analysis

### Standard Case: Height at Index 0

**Most components** (R70, R56, TP5, MTG1) have height at index 0:
- `AttributeNames[0] = ".comp_height"`
- `AttributeLookupTable["0"] = "<height_value>"`

**Client extraction logic** (standard case):
```python
# Check if AttributeNames[0] is ".comp_height"
if len(attribute_names) > 0 and attribute_names[0] == ".comp_height":
    height_value = attribute_lookup_table.get("0")
    if height_value:
        return float(height_value)
    else:
        return 1.0  # Default height
```

### Edge Case: Height at Index 1

**Some components** (J11 Package=18, R70 Package=75, R56 Package=75) have height at index 1:
- `AttributeNames[0] = ".comp_height_max"`
- `AttributeNames[1] = ".comp_height"`
- `AttributeLookupTable["1"] = "<height_value>"`

**Root cause**: These components use extended attribute sets that include `.comp_height_max` at index 0, pushing `.comp_height` to index 1.

**Client extraction logic** (robust case):
```python
# Find index of ".comp_height" in AttributeNames array
height_index = None
for i, attr_name in enumerate(attribute_names):
    if attr_name == ".comp_height":
        height_index = i
        break

if height_index is not None:
    height_key = str(height_index)
    height_value = attribute_lookup_table.get(height_key)
    if height_value:
        return float(height_value)
    else:
        return 1.0  # Default height
else:
    return 1.0  # ".comp_height" not in AttributeNames
```

### Components with Extended Attributes

The following components use extended attribute sets (from logs):
- **J11 Package=18**: `.comp_height` at index 1
- **R70 Package=75**: `.comp_height` at index 1 (from parse log: `attrIdString=";98=9,116=0,89=1,1=0.020000"`)
- **R56 Package=75**: `.comp_height` at index 1 (from parse log: `attrIdString=";98=9,116=0,89=1,1=0.020000"`)

---

## Root Cause Identification

### Server-Side Analysis

✅ **Server is functioning correctly**:
- Parses ODB++ files correctly
- Preserves height data through serialization
- Transmits height data correctly via gRPC
- Correctly omits height data when not present in ODB++ file

### Client-Side Issue (Hypothesis)

Based on server logs showing correct data transmission, the issue is likely **client-side**:

1. **Incorrect index assumption**: Client may be assuming height is always at index 0
2. **Missing AttributeNames check**: Client may not be checking AttributeNames array to find `.comp_height` index
3. **Incorrect lookup key**: Client may be using hardcoded `"0"` instead of finding the correct index

### Recommended Client-Side Fix

**Use AttributeNames array to find `.comp_height` index**:

```python
def extract_component_height(component):
    """
    Extract component height from AttributeLookupTable.
    
    Args:
        component: Component protobuf message with attribute_names and attribute_lookup_table
        
    Returns:
        float: Component height in mm, or 1.0 if not found
    """
    # Find index of ".comp_height" in AttributeNames
    height_index = None
    for i, attr_name in enumerate(component.attribute_names):
        if attr_name == ".comp_height":
            height_index = i
            break
    
    if height_index is None:
        # ".comp_height" not in AttributeNames - use default
        return 1.0
    
    # Look up height value using index as string key
    height_key = str(height_index)
    height_value_str = component.attribute_lookup_table.get(height_key)
    
    if height_value_str:
        try:
            return float(height_value_str)
        except ValueError:
            return 1.0  # Invalid value - use default
    else:
        return 1.0  # Key missing - use default
```

---

## Conclusion

### Server Status
✅ **Server is functioning correctly** - no fixes needed

### Client Status
⚠️ **Client needs to implement robust height extraction** that:
1. Checks AttributeNames array to find `.comp_height` index
2. Uses the correct index as lookup key (not hardcoded `"0"`)
3. Handles missing height data gracefully (defaults to 1.0mm)

### Next Steps
1. **Client-side**: Implement robust height extraction using AttributeNames array
2. **Client-side**: Add logging to verify height extraction logic
3. **Client-side**: Test with components that have height at index 1 (J11 Package=18)
4. **Client-side**: Verify default height (1.0mm) for components without height data
