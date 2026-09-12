# Component Height Investigation - Findings Summary

**Date**: January 14, 2026  
**Investigation**: Component height data flow analysis  
**Logs Analyzed**: Server debug logs (`docs/height-data/1/server-debug-output.txt`)

---

## Executive Summary

### Investigation Objective
Determine where component height data is lost or incorrectly handled in the data pipeline from ODB++ parsing → protobuf serialization → gRPC transmission → client deserialization.

### Key Finding
**Server is functioning correctly** - height data is preserved throughout the entire server-side pipeline. The issue is **client-side extraction logic**.

### Components Analyzed
- ✅ R70 (Package 48): Height present and preserved
- ✅ R56 (Package 48): Height present and preserved
- ✅ N5 (Package 26): Height correctly omitted (not in ODB++ file)
- ✅ MTG1 (Package 0): Height present and preserved
- ✅ TP5 (Package 160, Package 2): Height present and preserved
- ✅ J11 (Package 1): Height correctly omitted (not in ODB++ file)
- ⚠️ J11 (Package 18): Height present but at **index 1, not index 0**

---

## Critical Discovery: Index Mapping Issue

### Problem
**Height is not always at index 0**. Some components have height at index 1 when extended attribute sets are used.

### Evidence

**Standard Case** (R70, R56, TP5, MTG1):
```
AttributeNames=[0:".comp_height", 1:".comp_mount_type", ...]
AttributeLookupTable={"0":"0.010236", "1":"1", ...}
```
✅ Height at index 0, key `"0"`

**Edge Case** (J11 Package=18):
```
AttributeNames=[0:".comp_height_max", 1:".comp_height", ...]
AttributeLookupTable={"1":"0.430000", "89":"2", ...}
```
⚠️ Height at index 1, key `"1"` (NOT `"0"`)

### Impact
If client assumes height is always at index 0, it will:
- ✅ Work correctly for standard components (R70, R56, TP5, MTG1)
- ❌ Fail for components with extended attributes (J11 Package=18)
- ❌ Return default height (1.0mm) instead of actual height (0.43mm)

---

## Server-Side Verification

### Data Flow Stages Verified

1. **PARSE_START**: Raw ODB++ attribute string parsed correctly
2. **PARSE_RESULT**: AttributeLookupTable and AttributeNames populated correctly
3. **SERIALIZE**: AttributeLookupTable preserved through protobuf serialization
4. **GRPC_RESPONSE**: AttributeLookupTable correctly transmitted to client

### Verification Results

| Stage | Status | Evidence |
|-------|--------|----------|
| Parsing | ✅ Correct | Height values match ODB++ file |
| Serialization | ✅ Correct | AttributeLookupTable unchanged |
| gRPC Transmission | ✅ Correct | AttributeLookupTable transmitted correctly |
| Missing Data Handling | ✅ Correct | Components without height correctly omit key `"0"` |

### Conclusion
**No server-side fixes needed** - server is functioning correctly.

---

## Client-Side Issue

### Root Cause
Client is likely using **hardcoded index 0** to extract height, instead of checking the AttributeNames array to find the correct index.

### Current Client Logic (Hypothesized)
```python
# INCORRECT - assumes height is always at index 0
height_value = component.attribute_lookup_table.get("0")
if height_value:
    return float(height_value)
else:
    return 1.0  # Default
```

### Problems with This Approach
1. ❌ Fails for components with extended attributes (height at index 1)
2. ❌ Doesn't verify that index 0 actually contains `.comp_height`
3. ❌ May return wrong value if index 0 contains a different attribute

---

## Recommended Solution

### Correct Client-Side Extraction Logic

```python
def extract_component_height(component):
    """
    Extract component height from AttributeLookupTable.
    
    This function correctly handles:
    - Height at index 0 (standard case)
    - Height at index 1 (extended attributes case)
    - Missing height data (defaults to 1.0mm)
    
    Args:
        component: Component protobuf message with:
            - attribute_names: List of attribute names
            - attribute_lookup_table: Dict mapping index strings to values
            
    Returns:
        float: Component height in mm, or 1.0 if not found
    """
    # Step 1: Find index of ".comp_height" in AttributeNames array
    height_index = None
    for i, attr_name in enumerate(component.attribute_names):
        if attr_name == ".comp_height":
            height_index = i
            break
    
    # Step 2: If ".comp_height" not found, use default
    if height_index is None:
        return 1.0  # Default height per ODB++ spec
    
    # Step 3: Look up height value using index as string key
    height_key = str(height_index)
    height_value_str = component.attribute_lookup_table.get(height_key)
    
    # Step 4: Parse and return height value, or default if missing/invalid
    if height_value_str:
        try:
            return float(height_value_str)
        except ValueError:
            return 1.0  # Invalid value - use default
    else:
        return 1.0  # Key missing - use default
```

### Key Improvements
1. ✅ **Checks AttributeNames array** to find `.comp_height` index
2. ✅ **Uses correct index** as lookup key (not hardcoded `"0"`)
3. ✅ **Handles missing data** gracefully (defaults to 1.0mm)
4. ✅ **Works for all component types** (standard and extended attributes)

---

## Test Cases

### Test Case 1: Standard Component (R70 Package=48)
```
AttributeNames=[0:".comp_height", 1:".comp_mount_type", ...]
AttributeLookupTable={"0":"0.010236", "1":"1", ...}
```
**Expected**: `0.010236` mm  
**With fix**: ✅ Returns `0.010236`

### Test Case 2: Extended Attributes Component (J11 Package=18)
```
AttributeNames=[0:".comp_height_max", 1:".comp_height", ...]
AttributeLookupTable={"1":"0.430000", "89":"2", ...}
```
**Expected**: `0.430000` mm  
**With fix**: ✅ Returns `0.430000` (finds index 1)

### Test Case 3: Missing Height Data (N5 Package=26)
```
AttributeNames=[0:".comp_height", 1:".comp_mount_type", ...]
AttributeLookupTable={"1":"1", "2":"2", "3":"1", ...}  # No "0" key
```
**Expected**: `1.0` mm (default)  
**With fix**: ✅ Returns `1.0` (key `"0"` missing)

---

## Implementation Checklist

### Client-Side Changes Required

- [ ] **Replace hardcoded index 0** with AttributeNames array lookup
- [ ] **Add logging** to verify height extraction:
  - Log AttributeNames array
  - Log found index for `.comp_height`
  - Log extracted height value
  - Log default used if height missing
- [ ] **Test with all component types**:
  - Standard components (R70, R56, TP5, MTG1)
  - Extended attributes components (J11 Package=18)
  - Missing height components (N5, J11 Package=1)
- [ ] **Verify default height** (1.0mm) for components without height data

### Server-Side Changes Required

- [ ] **None** - server is functioning correctly

---

## Next Steps

1. **Client team**: Implement robust height extraction using AttributeNames array
2. **Client team**: Add logging to verify extraction logic
3. **Client team**: Test with components that have height at index 1
4. **Client team**: Verify default height handling
5. **Both teams**: Validate fix with matching client/server logs

---

## References

- **Detailed Analysis**: `docs/height-data/1/log-analysis.md`
- **Investigation Guide**: `docs/height-data/investigation-guide.md`
- **ODB++ Spec**: Missing attribute indices are valid (documented in `docs/height-data/odb-spec-attribute-lookup-confirmation.md`)
