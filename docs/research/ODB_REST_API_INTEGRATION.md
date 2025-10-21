# ODB++ REST API Integration Guide

## Overview

This document provides comprehensive guidance for integrating with the ODB++ Design Server REST API to retrieve and render PCB shape data in the board-shape-view-client application.

The OdbDesign REST API exposes ODB++ layer feature data through JSON endpoints, transforming the internal ProtoBuf representation into web-friendly formats suitable for client-side rendering.

> **⚠️ CRITICAL: Field Naming Convention**
>
> All JSON field names returned by the REST API use **`snake_case`** (e.g., `sym_num`, `xs`, `ye`, `apt_def_resize_factor`), NOT `camelCase`.
>
> The server preserves field names exactly as defined in the ProtoBuf schema without transformation.
>
> Additionally, the `type` field is **numeric** (0-5), not a string enum:
>
> - `0` = ARC
> - `1` = PAD
> - `2` = SURFACE
> - `4` = TEXT
> - `5` = LINE

## Field Naming Conventions

### ProtoBuf to JSON Mapping

The ODB++ Design Server uses ProtoBuf internally and exposes data via JSON REST endpoints. **Important:** The server preserves field names exactly as defined in the ProtoBuf schema.

- **ProtoBuf (Internal)**: `snake_case` (e.g., `sym_num`, `xs`, `ye`, `apt_def_resize_factor`)
- **JSON (REST API)**: `snake_case` (e.g., `sym_num`, `xs`, `ye`, `apt_def_resize_factor`)

The server uses default ProtoBuf JSON serialization (`google::protobuf::util::MessageToJsonString`) which preserves the original field names without transformation. There is **NO** camelCase conversion.

> **NOTE: MIXED Naming Convention for Surface Features**
>
> Surface features use a **mixed** naming convention:
>
> - **Top-level feature fields**: `snake_case` (e.g., `type`, `polarity`, `sym_num`)
> - **Surface polygon nested fields**: `camelCase` (e.g., `contourPolygons`, `xStart`, `yStart`, `polygonParts`, `endX`, `endY`, `xCenter`, `yCenter`, `isClockwise`)
>
> This is due to the ProtoBuf schema definition for polygon structures using camelCase field names.

## Complete Field Name Mapping

### Common Feature Fields

| ProtoBuf Field        | JSON Field            | Type        | Description                                                 |
| --------------------- | --------------------- | ----------- | ----------------------------------------------------------- |
| `type`                | `type`                | number/enum | Feature type code (0=ARC, 1=PAD, 2=SURFACE, 4=TEXT, 5=LINE) |
| `sym_num`             | `sym_num`             | number      | Symbol index reference (for width/shape lookup)             |
| `polarity`            | `polarity`            | string      | "POSITIVE" (additive) or "NEGATIVE" (subtractive)           |
| `dcode`               | `dcode`               | number      | D-code reference (optional)                                 |
| `id`                  | `id`                  | number      | Unique feature identifier (optional)                        |
| `orient_def`          | `orient_def`          | object      | Orientation metadata (mirror, angle)                        |
| `orient_def_rotation` | `orient_def_rotation` | number      | Rotation angle in degrees                                   |

### Line-Specific Fields

| ProtoBuf Field            | JSON Field | Type   | Description                 |
| ------------------------- | ---------- | ------ | --------------------------- |
| `line_record.x_start`     | `xs`       | number | Start X coordinate (inches) |
| `line_record.y_start`     | `ys`       | number | Start Y coordinate (inches) |
| `line_record.x_end`       | `xe`       | number | End X coordinate (inches)   |
| `line_record.y_end`       | `ye`       | number | End Y coordinate (inches)   |
| `line_record.symbol_name` | `sym_num`  | number | Symbol index for line width |

### Pad-Specific Fields

| ProtoBuf Field             | JSON Field              | Type   | Description                      |
| -------------------------- | ----------------------- | ------ | -------------------------------- |
| `pad_record.x`             | `x`                     | number | Pad center X coordinate (inches) |
| `pad_record.y`             | `y`                     | number | Pad center Y coordinate (inches) |
| `pad_record.resize_factor` | `apt_def_resize_factor` | number | Scale factor (default 1.0)       |
| `pad_record.symbol_num`    | `apt_def_symbol_num`    | number | Symbol index for pad shape       |

### Arc-Specific Fields

| ProtoBuf Field           | JSON Field | Type    | Description                      |
| ------------------------ | ---------- | ------- | -------------------------------- |
| `arc_record.x_start`     | `x`        | number  | Start X coordinate (inches)      |
| `arc_record.y_start`     | `y`        | number  | Start Y coordinate (inches)      |
| `arc_record.x_center`    | `xc`       | number  | Arc center X coordinate (inches) |
| `arc_record.y_center`    | `yc`       | number  | Arc center Y coordinate (inches) |
| `arc_record.symbol_name` | `sym_num`  | number  | Symbol index for arc width       |
| `arc_record.clockwise`   | `cw`       | boolean | Arc direction flag               |

### Text-Specific Fields

| ProtoBuf Field             | JSON Field     | Type   | Description                       |
| -------------------------- | -------------- | ------ | --------------------------------- |
| `text_record.x`            | `x`            | number | Text origin X coordinate (inches) |
| `text_record.y`            | `y`            | number | Text origin Y coordinate (inches) |
| `text_record.text`         | `text`         | string | Text content                      |
| `text_record.font_name`    | `font`         | string | Font reference                    |
| `text_record.x_size`       | `xsize`        | number | Character width (inches)          |
| `text_record.y_size`       | `ysize`        | number | Character height (inches)         |
| `text_record.width_factor` | `width_factor` | number | Width scale factor                |

### Surface-Specific Fields

| ProtoBuf Field                      | JSON Field                       | Type   | Description                       |
| ----------------------------------- | -------------------------------- | ------ | --------------------------------- |
| `surface_record.contour_polygons`   | `contourPolygons`                | array  | Array of polygon contours         |
| `surface_record.contour_polygons[]` | `contourPolygons[].type`         | string | "ISLAND" or "HOLE"                |
| `surface_record.contour_polygons[]` | `contourPolygons[].xStart`       | number | Starting X coordinate (inches)    |
| `surface_record.contour_polygons[]` | `contourPolygons[].yStart`       | number | Starting Y coordinate (inches)    |
| `surface_record.contour_polygons[]` | `contourPolygons[].polygonParts` | array  | Array of polygon parts (segments) |

## JSON Response Examples

### Line Feature

```json
{
  "type": 5,
  "polarity": "POSITIVE",
  "sym_num": 0,
  "xs": 1.25,
  "ys": 0.75,
  "xe": 2.5,
  "ye": 0.75
}
```

### Pad Feature

```json
{
  "type": 1,
  "polarity": "POSITIVE",
  "x": 1.5,
  "y": 1.0,
  "apt_def_symbol_num": 2,
  "apt_def_resize_factor": 1.0
}
```

### Arc Feature

```json
{
  "type": 0,
  "polarity": "POSITIVE",
  "sym_num": 1,
  "x": 1.0,
  "y": 1.0,
  "xc": 1.5,
  "yc": 1.5,
  "cw": true
}
```

### Text Feature

```json
{
  "type": 4,
  "polarity": "POSITIVE",
  "x": 0.5,
  "y": 0.25,
  "text": "U1",
  "font": "standard",
  "xsize": 0.05,
  "ysize": 0.08,
  "width_factor": 1.0
}
```

### Surface Feature

```json
{
  "type": 2,
  "polarity": "POSITIVE",
  "contourPolygons": [
    {
      "type": "ISLAND",
      "xStart": 0.0,
      "yStart": 0.0,
      "polygonParts": [
        {
          "segments": [
            { "endX": 1.0, "endY": 0.0 },
            { "endX": 1.0, "endY": 1.0 },
            { "endX": 0.0, "endY": 1.0 }
          ]
        }
      ]
    }
  ]
}
```

## TypeScript Type Definitions

### Core Feature Types

```typescript
// Feature type enumeration (numeric codes from ODB++ spec)
export enum FeatureType {
  ARC = 0,
  PAD = 1,
  SURFACE = 2,
  TEXT = 4,
  LINE = 5,
}

// Polarity enumeration
export enum Polarity {
  POSITIVE = 'POSITIVE',
  NEGATIVE = 'NEGATIVE',
}

// Base feature interface
export interface FeatureRecordBase {
  type: FeatureType;
  sym_num?: number;
  polarity?: Polarity;
  dcode?: number;
  id?: number;
  orient_def?: OrientDef;
  orient_def_rotation?: number;
  attributeLookupTable?: { [key: string]: string };
}

// Orientation definition
export interface OrientDef {
  mirror?: boolean;
  angle?: number;
}

// Line feature
export interface LineFeature extends FeatureRecordBase {
  type: FeatureType.LINE;
  xs: number;
  ys: number;
  xe: number;
  ye: number;
  sym_num?: number;
}

// Pad feature
export interface PadFeature extends FeatureRecordBase {
  type: FeatureType.PAD;
  x: number;
  y: number;
  apt_def_symbol_num?: number;
  apt_def_resize_factor?: number;
}

// Arc feature
export interface ArcFeature extends FeatureRecordBase {
  type: FeatureType.ARC;
  x: number;
  y: number;
  xc: number;
  yc: number;
  cw?: boolean;
  sym_num?: number;
}

// Text feature
export interface TextFeature extends FeatureRecordBase {
  type: FeatureType.TEXT;
  x: number;
  y: number;
  font?: string;
  xsize?: number;
  ysize?: number;
  width_factor?: number;
  text?: string;
}

// Surface feature
export interface SurfaceFeature extends FeatureRecordBase {
  type: FeatureType.SURFACE;
  contourPolygons: ContourPolygon[];
}

// Contour polygon type
export enum ContourPolygonType {
  ISLAND = 'ISLAND',
  HOLE = 'HOLE',
}

// Contour polygon structure
export interface ContourPolygon {
  type: ContourPolygonType;
  xStart: number;
  yStart: number;
  polygonParts: PolygonPart[];
}

// Polygon part
export interface PolygonPart {
  segments?: Segment[];
  arcs?: ArcPart[];
}

// Segment (line endpoint)
export interface Segment {
  endX: number;
  endY: number;
}

// Arc part
export interface ArcPart {
  endX: number;
  endY: number;
  xCenter: number;
  yCenter: number;
  isClockwise: boolean;
}

// Union type for all features
export type FeatureRecord = LineFeature | PadFeature | ArcFeature | TextFeature | SurfaceFeature;
```

### Symbol and Standard Definitions

```typescript
// Symbol name definition
export interface SymbolName {
  name: string;
  index: number;
  xSize?: number;
  ySize?: number;
  xMin?: number;
  yMin?: number;
  xMax?: number;
  yMax?: number;
}

// Attribute definition
export interface Attribute {
  name: string;
  value: string;
}
```

## Symbol and Attribute Resolution

### Symbol Resolution Pattern

Symbols must be resolved from the ODB++ design data before rendering PAD features:

```typescript
async function resolveSymbol(symbolName: string, layerName: string): Promise<Symbol> {
  // 1. Check layer-specific symbols first
  const layerSymbol = await fetch(
    `/api/design/{id}/steps/{step}/layers/${layerName}/symbols/${symbolName}`
  ).then((r) => r.json());

  if (layerSymbol) return layerSymbol;

  // 2. Fall back to standard symbols
  const standardSymbol = await fetch(
    `/api/design/{id}/steps/{step}/symbols/standard/${symbolName}`
  ).then((r) => r.json());

  return standardSymbol;
}

// Usage in rendering
async function renderPad(feature: PadFeature) {
  // Note: symbol name is in apt_def_symbol_num field
  const symbol = await resolveSymbol(feature.apt_def_symbol_num, currentLayer);

  // Apply transformations - note snake_case field names
  const { x, y, apt_def_resize_factor } = feature;
  const rotation = feature.orient_def_rotation || 0;

  // Render based on symbol type
  switch (symbol.type) {
    case 'CIRCLE':
      renderCircle(x, y, (symbol.width * apt_def_resize_factor) / 2);
      break;
    case 'RECTANGLE':
      renderRectangle(
        x,
        y,
        symbol.width * apt_def_resize_factor,
        symbol.height * apt_def_resize_factor,
        rotation
      );
      break;
    // ... other symbol types
  }
}
```

### Standard (Line Width) Resolution

Line widths are referenced by standard names and must be resolved:

```typescript
async function resolveLineWidth(symNum: string): Promise<number> {
  const standard = await fetch(`/api/design/{id}/steps/{step}/standards/${symNum}`).then((r) =>
    r.json()
  );

  return standard.width; // Width in inches
}

// Usage in line rendering
async function renderLine(feature: LineFeature) {
  const width = await resolveLineWidth(feature.sym_num);
  const { xs, ys, xe, ye } = feature;

  drawLine(xs, ys, xe, ye, width);
}
```

### Attribute Resolution

Attributes provide metadata about features (net names, component references, etc.):

```typescript
async function resolveAttributes(attributesId: string): Promise<Attribute[]> {
  if (!attributesId) return [];

  const attributes = await fetch(`/api/design/{id}/steps/{step}/attributes/${attributesId}`).then(
    (r) => r.json()
  );

  return attributes;
}

// Usage for feature inspection
async function getFeatureInfo(feature: Feature): Promise<FeatureInfo> {
  const attributes = await resolveAttributes(feature.attributes);

  return {
    type: feature.type,
    polarity: feature.polarity,
    attributes: attributes.reduce(
      (acc, attr) => {
        acc[attr.name] = attr.value;
        return acc;
      },
      {} as Record<string, string>
    ),
  };
}
```

## API Request/Response Examples

### Get Layer Features

**Request:**

```http
GET /api/design/{designId}/steps/{stepName}/layers/{layerName}/features
Accept: application/json
```

**Response:**

```json
{
  "layerName": "top",
  "featureCount": 1523,
  "features": [
    {
      "type": 5,
      "polarity": "POSITIVE",
      "xs": 1.25,
      "ys": 0.75,
      "xe": 2.5,
      "ye": 0.75,
      "sym_num": "r10"
    },
    {
      "type": 1,
      "polarity": "POSITIVE",
      "x": 1.5,
      "y": 1.0,
      "apt_def_symbol_num": "circle_50",
      "apt_def_resize_factor": 1.0,
      "orient_def_rotation": 0.0
    }
  ]
}
```

### Get Symbol Definition

**Request:**

```http
GET /api/design/{designId}/steps/{stepName}/symbols/standard/circle_50
Accept: application/json
```

**Response:**

```json
{
  "name": "circle_50",
  "type": "CIRCLE",
  "width": 0.05,
  "height": 0.05
}
```

### Get Standard Definition

**Request:**

```http
GET /api/design/{designId}/steps/{stepName}/standards/r10
Accept: application/json
```

**Response:**

```json
{
  "name": "r10",
  "width": 0.01
}
```

### Get Layer Metadata

**Request:**

```http
GET /api/design/{designId}/steps/{stepName}/layers/{layerName}
Accept: application/json
```

**Response:**

```json
{
  "name": "top",
  "type": "SIGNAL",
  "polarity": "POSITIVE",
  "context": "BOARD",
  "featureCount": 1523,
  "bounds": {
    "minX": 0.0,
    "minY": 0.0,
    "maxX": 5.0,
    "maxY": 3.5
  }
}
```

## Rendering Pipeline Guidance

### Complete Rendering Flow

```typescript
// 1. Fetch layer data
async function loadLayer(designId: string, stepName: string, layerName: string) {
  const features = await fetch(
    `/api/design/${designId}/steps/${stepName}/layers/${layerName}/features`
  ).then((r) => r.json());

  return features;
}

// 2. Build symbol/standard cache
async function buildReferenceCache(designId: string, stepName: string) {
  const [symbols, standards] = await Promise.all([
    fetch(`/api/design/${designId}/steps/${stepName}/symbols`).then((r) => r.json()),
    fetch(`/api/design/${designId}/steps/${stepName}/standards`).then((r) => r.json()),
  ]);

  return { symbols, standards };
}

// 3. Render features with caching
async function renderFeatures(
  features: Feature[],
  cache: { symbols: Map<string, Symbol>; standards: Map<string, Standard> }
) {
  for (const feature of features) {
    // Note: type is numeric (0-5)
    switch (feature.type) {
      case 5: // LINE
        await renderLine(feature, cache.standards);
        break;
      case 1: // PAD
        await renderPad(feature, cache.symbols);
        break;
      case 0: // ARC
        await renderArc(feature, cache.standards);
        break;
      case 4: // TEXT
        await renderText(feature);
        break;
      case 2: // SURFACE
        await renderSurface(feature);
        break;
    }
  }
}

// 4. Handle coordinate transformation
function transformCoordinates(x: number, y: number, viewport: Viewport): [number, number] {
  // Convert from inches to screen pixels
  const screenX = (x - viewport.minX) * viewport.scale + viewport.offsetX;
  const screenY = (viewport.maxY - y) * viewport.scale + viewport.offsetY;

  return [screenX, screenY];
}
```

### Polarity Handling

```typescript
function applyPolarity(feature: Feature, context: RenderContext) {
  // Note: polarity is an enum string ("POSITIVE" or "NEGATIVE")
  if (feature.polarity === 'POSITIVE') {
    // Additive - draw with layer color
    context.fillStyle = context.layerColor;
    context.globalCompositeOperation = 'source-over';
  } else {
    // Subtractive - remove material
    context.globalCompositeOperation = 'destination-out';
  }
}
```

### Performance Optimization

```typescript
// Use spatial indexing for large datasets
class SpatialIndex {
  private quadTree: QuadTree<Feature>;

  constructor(features: Feature[], bounds: Bounds) {
    this.quadTree = new QuadTree(bounds);
    features.forEach((f) => this.quadTree.insert(f, this.getBounds(f)));
  }

  query(viewport: Viewport): Feature[] {
    return this.quadTree.query(viewport);
  }

  private getBounds(feature: Feature): Bounds {
    // Calculate feature bounding box based on type (numeric 0-5)
    switch (feature.type) {
      case 5: // LINE
        return this.getLineBounds(feature);
      case 1: // PAD
        return this.getPadBounds(feature);
      // ... other types
    }
  }
}

// Render only visible features
async function renderViewport(viewport: Viewport, spatialIndex: SpatialIndex) {
  const visibleFeatures = spatialIndex.query(viewport);
  await renderFeatures(visibleFeatures, referenceCache);
}
```

## Common Integration Patterns

### Pattern 1: Layer Stack Rendering

```typescript
async function renderLayerStack(designId: string, stepName: string, layers: string[]) {
  const cache = await buildReferenceCache(designId, stepName);

  for (const layerName of layers) {
    const features = await loadLayer(designId, stepName, layerName);
    const layerColor = getLayerColor(layerName);

    await renderFeatures(features, cache, { color: layerColor });
  }
}
```

### Pattern 2: Interactive Feature Selection

```typescript
function findFeatureAtPoint(
  x: number,
  y: number,
  features: Feature[],
  tolerance: number = 0.005 // 5 mils
): Feature | null {
  for (const feature of features) {
    if (isPointInFeature(x, y, feature, tolerance)) {
      return feature;
    }
  }
  return null;
}

async function handleClick(event: MouseEvent) {
  const [x, y] = screenToWorld(event.clientX, event.clientY);
  const feature = findFeatureAtPoint(x, y, currentFeatures);

  if (feature) {
    const info = await getFeatureInfo(feature);
    displayFeatureInfo(info);
  }
}
```

### Pattern 3: Progressive Loading

```typescript
async function loadLayerProgressive(
  designId: string,
  stepName: string,
  layerName: string,
  onProgress: (features: Feature[], total: number) => void
) {
  const chunkSize = 1000;
  let offset = 0;
  let total = 0;

  do {
    const response = await fetch(
      `/api/design/${designId}/steps/${stepName}/layers/${layerName}/features?` +
        `offset=${offset}&limit=${chunkSize}`
    ).then((r) => r.json());

    total = response.total;
    onProgress(response.features, total);

    offset += chunkSize;
  } while (offset < total);
}
```

### Pattern 4: Multi-Layer Feature Search

```typescript
async function findNetFeatures(
  designId: string,
  stepName: string,
  netName: string
): Promise<Map<string, Feature[]>> {
  const layers = await getSignalLayers(designId, stepName);
  const results = new Map<string, Feature[]>();

  for (const layer of layers) {
    const features = await loadLayer(designId, stepName, layer.name);

    const netFeatures = await Promise.all(
      features
        .filter((f) => f.attributes) // Note: attributes field, not attributesId
        .map(async (f) => {
          const attrs = await resolveAttributes(f.attributes);
          const netAttr = attrs.find((a) => a.name === '.net_name');
          return netAttr?.value === netName ? f : null;
        })
    );

    results.set(
      layer.name,
      netFeatures.filter((f) => f !== null)
    );
  }

  return results;
}
```

## Coordinate System and Units

### Unit Conversion

All coordinates in the API are expressed in **inches**. Convert to your rendering units:

```typescript
const INCHES_TO_MM = 25.4;
const INCHES_TO_MILS = 1000;

function inchesToMillimeters(inches: number): number {
  return inches * INCHES_TO_MM;
}

function inchesToMils(inches: number): number {
  return inches * INCHES_TO_MILS;
}

function inchesToPixels(inches: number, dpi: number): number {
  return inches * dpi;
}
```

### Coordinate Origin

The ODB++ coordinate system:

- **Origin (0, 0)**: Bottom-left corner
- **X-axis**: Increases to the right
- **Y-axis**: Increases upward

Canvas/screen coordinates typically have origin at top-left with Y increasing downward. Apply transformation:

```typescript
function odbToCanvas(
  odbX: number,
  odbY: number,
  bounds: Bounds,
  canvasHeight: number,
  scale: number
): [number, number] {
  const canvasX = (odbX - bounds.minX) * scale;
  const canvasY = canvasHeight - (odbY - bounds.minY) * scale;
  return [canvasX, canvasY];
}
```

## Error Handling

### API Error Response Format

```typescript
interface ApiError {
  error: string;
  message: string;
  statusCode: number;
  details?: Record<string, any>;
}

async function fetchWithErrorHandling<T>(url: string): Promise<T> {
  const response = await fetch(url);

  if (!response.ok) {
    const error: ApiError = await response.json();
    throw new Error(`API Error ${error.statusCode}: ${error.message}`);
  }

  return response.json();
}
```

### Common Error Scenarios

| Status Code | Scenario                    | Client Action                  |
| ----------- | --------------------------- | ------------------------------ |
| 404         | Design/step/layer not found | Verify resource exists         |
| 400         | Invalid parameters          | Check query string format      |
| 500         | Server processing error     | Retry with exponential backoff |
| 503         | Service unavailable         | Display maintenance message    |

## Performance Considerations

### Caching Strategy

```typescript
class OdbApiCache {
  private symbolCache = new Map<string, Symbol>();
  private standardCache = new Map<string, Standard>();
  private featureCache = new Map<string, Feature[]>();

  async getSymbol(key: string, fetcher: () => Promise<Symbol>): Promise<Symbol> {
    if (!this.symbolCache.has(key)) {
      this.symbolCache.set(key, await fetcher());
    }
    return this.symbolCache.get(key)!;
  }

  // Similar methods for standards and features

  clear() {
    this.symbolCache.clear();
    this.standardCache.clear();
    this.featureCache.clear();
  }
}
```

### Batch API Requests

```typescript
async function batchFetchSymbols(symbolNames: string[]): Promise<Map<string, Symbol>> {
  const unique = [...new Set(symbolNames)];

  const symbols = await Promise.all(
    unique.map((name) =>
      fetch(`/api/design/{id}/steps/{step}/symbols/${name}`).then((r) => r.json())
    )
  );

  return new Map(symbols.map((s, i) => [unique[i], s]));
}
```

## References

### Related Documentation

- [ODB++ Shape Definitions](./ODB_SHAPE_DEFINITIONS.md) - Detailed shape type specifications
- [ODB++ Specification Research](./ODB++_Shape_Representation_Research.md) - Research findings
- [API Documentation](./API.md) - Complete REST API reference
- [Architecture Overview](../plan_docs/architecture.md) - System architecture

### Source Code References

#### Server-Side (OdbDesign)

- **ProtoBuf Definitions**: `OdbDesign/OdbDesignLib/ProtoBuf/feature.proto`
- **JSON Serialization**: `OdbDesign/OdbDesignServer/Controllers/*Controller.cpp`
- **Feature Models**: `OdbDesign/OdbDesignLib/FileModel/Design/Feature*.h`

#### Client-Side (board-shape-view-client)

- **Type Definitions**: `src/models/odb.ts`
- **API Client**: `src/api/odbDesignClient.ts`
- **Shape Renderers**: `src/rendering/shapes/*.tsx`
- **Data Hooks**: `src/hooks/useBoardData.ts`

### External Resources

- [ODB++ Format Specification](http://www.odb-sa.com/) - Official ODB++ documentation
- [Protocol Buffers](https://developers.google.com/protocol-buffers) - ProtoBuf documentation
- [REST API Best Practices](https://restfulapi.net/) - REST design patterns

## Changelog

| Version | Date       | Changes                                                               |
| ------- | ---------- | --------------------------------------------------------------------- |
| 1.2     | 2025-01-21 | Corrected Surface polygon field names to camelCase (endX, endY, etc.) |
| 1.1     | 2025-01-21 | Corrected all field names to use actual snake_case API format         |
| 1.0     | 2025-01-21 | Initial documentation created from research task                      |

## Contributing

When updating this documentation:

1. Maintain consistency with actual API implementation in OdbDesign server
2. Update TypeScript types in `src/models/odb.ts` when adding examples
3. Test all code examples against running OdbDesign server instance
4. Cross-reference with `ODB_SHAPE_DEFINITIONS.md` for shape-specific details
5. Document any new field mappings discovered during integration

## License

This documentation is part of the board-shape-view-client project and follows the same license terms.
