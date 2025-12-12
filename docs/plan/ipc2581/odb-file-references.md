# ODB++ File Parsing References - Sections Needing Updates

This document lists all sections in `implementation-plan.md` that reference parsing ODB++ files directly or accessing FeaturesFile protobuf for symbols/units. These sections need to be updated to use gRPC/REST API endpoints instead.

## Task 1.1: Implement Symbol Parsing (Gap 2.4)

### Section: Objectives (Line 39)
**Current Text**: "Parse ODB++ symbol strings (e.g., "r100", "rect50x100", "s50")"
**Issue**: Implies parsing from files
**Update Needed**: Change to "Parse symbol strings received from gRPC symbol streaming endpoint"

### Section: Step 1.1.2: Implement Unit Conversion (Line 93-96)
**Current Text**: 
```
- Handle unit type from `FeaturesFile.units` field
```
**Issue**: Accesses FeaturesFile.units directly
**Update Needed**: Units should come from symbol stream or separate API call

### Section: Step 1.1.3: Add Symbol Name Resolution (Line 98-101)
**Current Text**:
```
- Resolve symbol name from `FeaturesFile.symbolNamesByName` or `symbolNames` list
```
**Issue**: Accesses FeaturesFile.symbolNamesByName/symbolNames directly
**Update Needed**: Symbols will come from gRPC symbol streaming endpoint, not FeaturesFile

### Section: Step 1.1.4: Integrate with Pad Conversion (Line 103-126)
**Current Code Example**:
```csharp
private IShapeDefinition ConvertPad(FeatureRecord feature, Color4? layerColor, float yLevel, FeaturesFile? featuresFile)
{
    // ...
    if (feature.HasSymNum && featuresFile != null)
    {
        var symbolName = ResolveSymbolName(feature.SymNum, featuresFile);
        if (symbolName != null)
        {
            var unitType = GetUnitType(featuresFile.Units);
            if (SymbolParser.TryParse(symbolName.Name, unitType, out var shape))
            {
                return CreateShapeFromSymbol(shape, position, yLevel, layerColor);
            }
        }
    }
    // ...
}
```
**Issue**: 
- Passes `FeaturesFile` parameter
- Accesses `featuresFile.symbolNamesByName`
- Accesses `featuresFile.Units`
**Update Needed**: 
- Remove `FeaturesFile` parameter
- Use gRPC symbol streaming endpoint instead
- Get units from symbol stream or separate API

### Section: Step 1.1.5: Update Interface (Line 128-130)
**Current Text**:
```
- Modify `IFeatureConverter` if needed to pass `FeaturesFile` to converter
- Update `MainViewModel` to pass `FeaturesFile` to converter
```
**Issue**: Plans to pass FeaturesFile to converter
**Update Needed**: Instead, add symbol streaming client/service to fetch symbols

---

## Additional References

### Section: Task 2.1.3: Update FeatureToShapeConverter (Line 424)
**Current Text**: "Update method signature to accept `MatrixFileInfo`"
**Status**: ✅ **OK** - MatrixFile already comes from REST API (`GetMatrixFileAsync`)

### Section: Task 2.3.2: TransformationExtractor (Line 543)
**Current Text**: "May need ODB++ specification for exact mapping"
**Status**: ✅ **OK** - This is just a comment about understanding orient_def codes, not file parsing

---

## Summary of Changes Needed

### Task 1.1: Symbol Parsing - Major Changes Required

1. **Remove FeaturesFile dependency**: 
   - Remove all references to `FeaturesFile.symbolNamesByName`
   - Remove all references to `FeaturesFile.symbolNames`
   - Remove all references to `FeaturesFile.units`

2. **Add gRPC Symbol Streaming**:
   - Create `IOdbDesignClient.GetSymbolsStreamAsync()` method (similar to `GetLayerFeaturesStreamAsync`)
   - Add symbol lookup/caching mechanism
   - Symbols will be streamed from server, not parsed from files

3. **Add REST API for Symbol Path**:
   - May need REST endpoint to get symbol directory path
   - Similar to how step and layer are selected

4. **Update Symbol Resolution**:
   - Instead of resolving from FeaturesFile, fetch from symbol stream
   - Use `sym_num` from FeatureRecord to lookup symbol from stream
   - Cache symbols for performance

5. **Update Unit Handling**:
   - Units should come from symbol stream or separate API call
   - Not from FeaturesFile.units field

---

## Questions for Clarification

1. **Symbol Streaming Endpoint**:
   - What will be the gRPC method name? (e.g., `GetSymbolsStream`?)
   - What parameters will it take? (designName, stepName, symbolPath?)
   - What will the stream return? (SymbolRecord with name, unitType, shape data?)

2. **REST API for Symbol Path**:
   - What REST endpoint will provide the symbol directory path?
   - How will it be called? (similar to GetLayersAsync?)
   - What will it return?

3. **Symbol Lookup**:
   - How will `sym_num` from FeatureRecord map to symbols in the stream?
   - Will symbols be indexed by number, or do we need to search by name?
   - Should symbols be cached after first fetch?

4. **Units**:
   - Will units come from the symbol stream itself?
   - Or from a separate API call?
   - Or from the design/step metadata?

---

## Files That Will Need Updates

1. **New Files to Create**:
   - `src/OdbDesign3DClient.Core/Services/Interfaces/IOdbDesignClient.cs` - Add symbol streaming method
   - `src/OdbDesign3DClient.Core/Services/Implementations/OdbDesignGrpcClient.cs` - Implement symbol streaming
   - `src/OdbDesign3DClient.Core/Services/Implementations/SymbolCache.cs` - Cache symbols for lookup

2. **Files to Modify**:
   - `src/OdbDesign3DClient.Core/Services/Implementations/FeatureToShapeConverter.cs` - Remove FeaturesFile dependency
   - `src/OdbDesign3DClient.Core/ViewModels/MainViewModel.cs` - Add symbol fetching logic
   - `src/OdbDesign3DClient.Core/Protos/grpc/service.proto` - Add symbol streaming RPC (if not already defined)

3. **Files That Are OK** (no ODB++ parsing):
   - `ContourPolygonParser.cs` - Only parses protobuf ContourPolygon structures (from FeatureRecord), not files ✅
   - `TransformationExtractor.cs` - Only extracts from FeatureRecord protobuf ✅
   - `LayerZCalculator.cs` - Uses MatrixFileInfo from REST API ✅

---

**Next Steps**: Once symbol streaming endpoint details are provided, update `implementation-plan.md` accordingly.


