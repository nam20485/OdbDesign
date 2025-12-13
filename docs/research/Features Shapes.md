# Research Report on ODB++ Specification and OdbDesign Tool

## Executive Summary

This report provides a detailed analysis of the ODB++ (Open Database++) specification and the OdbDesign tool, focusing on technical aspects relevant to developing shapes libraries and board viewers. ODB++ is an ASCII-based intelligent CAD-to-CAM data exchange format for PCB design, widely used in electronics manufacturing. OdbDesign is a high-performance C++ library that parses ODB++ files, enabling visualization, manipulation, and integration into applications.

The report covers the format and structure of features and symbols in ODB++ files, OdbDesign capabilities for visualization and manipulation, REST API endpoints, and practical guidance for developing the shapes library and board viewer. It includes examples, code snippets, and citations to support the development efforts.

## ODB++ Specification Overview

### Format and Structure of Features and Symbols

ODB++ files use a hierarchical directory structure with ASCII text records for human readability and machine parsing. The format employs UTF-8/ASCII encoding with newline-terminated records, making it both accessible and efficient for data exchange.

**Key Structural Elements:**
- **Records**: Single-character prefixes (e.g., "L" for lines, "P" for pads)
- **Coordinates**: Integer-based in micrometers or mils (e.g., 1000 2000)
- **Attributes**: Key-value pairs (e.g., ROT=90 for rotation, NET=signal1 for nets)
- **Symbols**: Reusable shapes stored in `.sym` files, referenced by name
- **Polarity**: "+" (additive) or "-" (subtractive) for manufacturing processes

**File Hierarchy Example:**
```
odb_job/
├── steps/
│   ├── step1/
│   │   ├── layers/
│   │   │   ├── comp_+_top/
│   │   │   │   ├── features
│   │   │   │   ├── components
│   │   │   │   └── attrlist
│   │   │   └── misc/
│   │   │       ├── matrix
│   │   │       └── info
│   │   └── symbols/
│   │       ├── ROUND.sym
│   │       └── RECT.sym
└── fonts/
    └── standard
```

### Types of Features Supported

ODB++ supports comprehensive feature types for PCB representation:

| Feature Type | Token | Description | Key Attributes |
|-------------|-------|-------------|----------------|
| **Line** | L | Straight line segments | Start/end coordinates, width |
| **Pad** | P | Connection points | Shape, drill information, size |
| **Arc** | A | Curved segments/circles | Center, radius, sweep angle |
| **Text** | T | Labels and annotations | Font, size, orientation, content |
| **Surface** | S | Filled areas/polygons | Vertices, fill type |
| **Barcode** | B | Encoded manufacturing data | Type, position, content |

**Additional Types:**
- **Via (V)**: Through-hole connections
- **Component (C)**: Placed parts with footprints

### Feature Definition Structure

Features follow a standardized record format:
```
TYPE ID SYMBOL POLARITY D_CODE X Y [ATTRIBUTES]
```

**Example Breakdown:**
- **TYPE**: Single character (L, P, A, T, S, B)
- **ID**: Unique identifier (integer)
- **SYMBOL**: Shape reference (e.g., "ROUND", "RECT", "NONE")
- **POLARITY**: "+" (positive/additive) or "-" (negative/subtractive)
- **D_CODE**: Aperture/drawing code for manufacturing
- **X Y**: Coordinate position (integers)
- **ATTRIBUTES**: Optional parameters (e.g., ROT=90, MIRROR=Y, NET=signal1)

### Examples of ODB++ Feature Records

**Line Feature:**
```
L 123 NONE + 25 0 0 1000 1000
```
- ID: 123, No symbol, Positive polarity, D-code 25, From (0,0) to (1000,1000)

**Pad Feature:**
```
P 456 ROUND - 50 2000 3000 0 0
```
- ID: 456, Round symbol, Negative polarity, D-code 50, At (2000,3000)

**Arc Feature:**
```
A 789 CIRCLE + 15 5000 5000 1000 0 360
```
- ID: 789, Circle symbol, Positive polarity, D-code 15, Center (5000,5000), Radius 1000, Full circle (0-360°)

**Text Feature:**
```
T 1010 ARIAL + 12 6000 6000 0 "Component U1"
```
- ID: 1010, Arial font, Positive polarity, Size 12, At (6000,6000), No rotation, Text "Component U1"

**Surface Feature:**
```
S 1111 POLY - 20 7000 7000 8000 7000 8000 8000 7000 8000
```
- ID: 1111, Polygon symbol, Negative polarity, D-code 20, Vertices at listed coordinates


---

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

---

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

