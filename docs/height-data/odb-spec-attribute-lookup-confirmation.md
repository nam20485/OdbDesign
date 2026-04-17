# ODB++ Specification: Attribute Lookup Table - Missing Indices Confirmation

**Date**: January 2025  
**Source**: ODB++ Design Format Specification Release 8.1 Update 4 (August 2024)  
**Pages**: 27, 150-151, 235-236  
**Status**: ✅ **SPEC CONFIRMED** - Missing indices are valid per specification

---

## Specification Excerpts

### Page 27: Attributes in General

> "All system attributes are optional."

### Page 150-151: Component Attribute Lookup Table Example

**Attribute Names Table** (defines indices 0-4):
```
@0 .no_pop
@1 .comp_ignore
@2 .desc1
@3 .comp_mount_type
@4 .comp_height
```

**Example CMP Record**:
```
CMP 0 1.55 8.9 270.0 N J10 2300046 ;0,2=0,3=1
```

**Analysis of Assignment String `;0,2=0,3=1`**:
- ✅ Index 0 present: BOOLEAN (`.no_pop = TRUE`)
- ❌ Index 1 **missing**: `.comp_ignore` not assigned
- ✅ Index 2 present: TEXT (`2=0` → `.desc1 = "Internally created connector"`)
- ✅ Index 3 present: OPTION (`3=1` → `.comp_mount_type = SMT`)
- ❌ Index 4 **missing**: `.comp_height` not assigned

### Page 235-236: CMP Record Format

> "attributes A comma separated list of attribute assignments. The attribute is indicated by its index in the attribute name lookup table."

**Format Rules**:
- BOOLEAN: `n` (just the index)
- OPTION: `n=m` (index = value)
- INTEGER: `n=i` (index = integer value)
- FLOAT: `n=f` (index = float value)
- TEXT: `n=s` (index = text string lookup table index)

---

## Key Findings

### 1. Missing Indices Are Valid ✅

**Specification Confirms**:
- Indices only appear in the assignment string if they have values
- Missing indices (like index 1 and 4 in the example) are **valid and expected**
- The spec example explicitly shows indices 1 and 4 missing from `;0,2=0,3=1`

### 2. Indices Don't Need to Be Sequential ✅

**Specification Confirms**:
- Assignment string `;0,2=0,3=1` is valid even though indices 1 and 4 are missing
- Indices can appear in any order
- Only indices with values are included

### 3. Server Behavior Matches Specification ✅

**Current Implementation**:
- Server correctly omits missing indices from `AttributeLookupTable`
- Example: Component N5 has `;1=1,2=2,3=1` (no `;0=`) → Server correctly omits key `"0"`
- This matches the spec exactly: missing indices mean "no value for this attribute"

### 4. Client Must Handle Missing Indices ✅

**Expected Behavior**:
- Client should check if key exists before lookup
- Client should default to reasonable value (1.0mm) when key missing
- Current client fallback logic (PropertyRecords → 1.0mm default) is correct

---

## Real Issue Identified

**The Problem**: Components WITH height data are defaulting incorrectly.

**Evidence**:
- **designodb_rigidflex**: 609/610 components have height data (`;0=...` in ODB++), but ALL defaulting to 1.0mm
- **sample_design**: 791/813 components have height data, but ALL defaulting to 1.0mm
- **Total**: ~1,400 components with height data aren't using their actual heights

**Root Cause Hypothesis**:
1. Server-side parsing: AttributeLookupTable not populated correctly
2. Server-side serialization: AttributeLookupTable not serialized to protobuf
3. Client-side deserialization: AttributeLookupTable not read correctly
4. Client-side lookup: Index "0" lookup failing even when data exists

**Investigation**: Use `ComponentHeightTracer` logging to trace data flow:
- `[CompHeightTrace][PARSE_RESULT]` - Verify parsing
- `[CompHeightTrace][SERIALIZE]` - Verify serialization
- `[CompHeightTrace][GRPC_RESPONSE]` - Verify gRPC transmission

---

## Conclusion

**Missing Indices**: ✅ **Valid per spec** - No code changes needed for handling missing indices

**Server Behavior**: ✅ **Correct** - Omitting missing indices matches specification

**Client Behavior**: ✅ **Correct** - Fallback logic handles missing indices properly

**Real Issue**: 🔴 **Components WITH height data defaulting incorrectly** - Need to investigate data flow from ODB++ parsing → protobuf serialization → client deserialization → lookup

---

## References

- ODB++ Design Format Specification Release 8.1 Update 4 (August 2024)
- Pages 27, 150-151, 235-236
- File: `docs/ODB++ Attributes.txt` (excerpts from spec PDF)
