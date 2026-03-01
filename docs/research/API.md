# OdbDesignServer REST API Documentation

## Table of Contents

1. [Overview](#overview)
2. [Architecture](#architecture)
3. [Authentication](#authentication)
4. [Base Configuration](#base-configuration)
5. [Core Endpoints](#core-endpoints)
6. [Data Models](#data-models)
7. [Feature Types Reference](#feature-types-reference)
8. [Error Handling](#error-handling)
9. [TypeScript Integration Guide](#typescript-integration-guide)
10. [Performance Considerations](#performance-considerations)

---

## Overview

The OdbDesignServer provides a RESTful API for accessing ODB++ (Open Database) PCB design data. The server exposes board designs, steps, layers, symbols, and geometric features through JSON endpoints backed by Protocol Buffer serialization.

**Key Features:**

- Full ODB++ design hierarchy navigation
- Layer-by-layer feature extraction
- Symbol library access
- Component and netlist queries
- Attribute metadata retrieval
- Health monitoring endpoints

**Technology Stack:**

- **Framework**: Crow (C++ HTTP framework)
- **Serialization**: Protocol Buffers → JSON via `google::protobuf::util::MessageToJsonString()`
- **Authentication**: HTTP Basic Auth
- **Data Format**: JSON (derived from protobuf schemas)

---

## Architecture

### Request Flow

```
Client Request
    ↓
Crow HTTP Server (Port 8888)
    ↓
FileModelController::steps_layers_features_route_handler()
    ↓
FeaturesFile (C++ Model)
    ↓
IProtoBuffable::to_json()
    ↓
to_protobuf() → Protocol Buffer Message
    ↓
google::protobuf::util::MessageToJsonString()
    ↓
JsonCrowReturnable<T> (Response Wrapper)
    ↓
JSON Response to Client
```

### Key Components

1. **Controllers** (`FileModelController.h/cpp`): HTTP endpoint handlers
2. **Data Models** (`FeaturesFile.h`): C++ domain objects
3. **Protocol Buffers** (`featuresfile.proto`, `common.proto`, `enums.proto`): Schema definitions
4. **Serialization** (`IProtoBuffable.h`): JSON conversion interface

---

## Authentication

All endpoints (except health checks) require HTTP Basic Authentication.

**Headers:**

```http
Authorization: Basic <base64(username:password)>
```

**Example (curl):**

```bash
curl -u username:password http://localhost:8888/filemodels/design1
```

**TypeScript (fetch):**

```typescript
const headers = new Headers({
  Authorization: 'Basic ' + btoa('username:password'),
  'Content-Type': 'application/json',
});

fetch('http://localhost:8888/filemodels/design1', { headers });
```

---

## Base Configuration

### Environment Variables

```bash
# .env or .env.local
VITE_API_BASE_URL=http://localhost:8888
VITE_API_USERNAME=your_username
VITE_API_PASSWORD=your_password
```

### Server URLs

- **Development**: `http://localhost:8888`
- **Production AWS**: `http://default-ingress-1165108808.us-west-2.elb.amazonaws.com`
- **Local Network**: `http://precision5820:8081`

---

## Core Endpoints

### File Models

#### List All Designs

```
GET /filemodels
```

**Response:**

```json
["design1", "design2", "design3"]
```

**Description:** Returns array of available design names.

---

#### Get Design Details

```
GET /filemodels/{name}
```

**Parameters:**

- `name` (path): Design identifier

**Response:** Complete design metadata including steps, layers, symbols, and attributes.

---

### Steps & Layers

#### List Steps

```
GET /filemodels/{name}/steps
```

**Response:** Array of step names (e.g., `["pcb", "panel"]`)

---

#### List Layers in Step

```
GET /filemodels/{name}/steps/{step}/layers
```

**Parameters:**

- `name` (path): Design name
- `step` (path): Step name

**Response:** Array of layer metadata objects

---

#### **Get Layer Features** ⭐ **PRIMARY ENDPOINT**

```
GET /filemodels/{name}/steps/{step}/layers/{layer}/features
```

**Parameters:**

- `name` (path): Design name (e.g., `"design1"`)
- `step` (path): Step name (e.g., `"pcb"`)
- `layer` (path): Layer name (e.g., `"top_copper"`)

**Response:** `FeaturesFile` object (see [Data Models](#data-models))

**Example Request:**

```bash
curl -u user:pass http://localhost:8888/filemodels/myboard/steps/pcb/layers/top_copper/features
```

---

### Symbols

#### List Symbols

```
GET /filemodels/{name}/symbols
```

**Response:** Array of symbol directory names

---

#### Get Symbol Features

```
GET /filemodels/{name}/symbols/{symbol}/features
```

**Parameters:**

- `name` (path): Design name
- `symbol` (path): Symbol name (e.g., `"pad_round_d60"`)

**Response:** `FeaturesFile` object (same structure as layer features)

---

### Design Queries

#### Get Design Components

```
GET /designs/{name}/components
```

#### Get Design Nets

```
GET /designs/{name}/nets
```

#### Get Design Parts

```
GET /designs/{name}/parts
```

#### Get Design Packages

```
GET /designs/{name}/packages
```

---

### Health Checks

#### Liveness Probe

```
GET /healthz/live
```

#### Readiness Probe

```
GET /healthz/ready
```

#### Startup Probe

```
GET /healthz/started
```

**Response (all):** `200 OK`

---

## Data Models

### FeaturesFile (Top-Level Response)

**Description:** Container for all geometric features on a layer or within a symbol.

**TypeScript Interface:**

```typescript
interface FeaturesFile {
  units: string; // "mm" or "inch"
  id: number; // Unique file ID
  path: string; // Original file path
  directory: string; // Parent directory
  numFeatures: number; // Total feature count
  featureRecords: FeatureRecord[]; // Array of geometric shapes
  symbolNamesByName: { [name: string]: SymbolName }; // Symbol lookup
  symbolNames: SymbolName[]; // Symbol definitions
}
```

**JSON Example:**

```json
{
  "units": "mm",
  "id": 1,
  "path": "/steps/pcb/layers/top_copper/features",
  "directory": "layers/top_copper",
  "numFeatures": 1523,
  "featureRecords": [
    /* ... */
  ],
  "symbolNamesByName": {
    "r100": {
      /* SymbolName object */
    }
  },
  "symbolNames": [
    /* ... */
  ]
}
```

---

### FeatureRecord (Geometric Shape)

**Description:** Individual PCB feature (pad, line, arc, surface, text).

**Type Enum:**

- `0` = Arc
- `1` = Pad
- `2` = Surface
- `4` = Text
- `5` = Line

**Common Fields:**

```typescript
interface FeatureRecordBase {
  type: number; // 0-5 (see enum above)
  sym_num?: number; // Symbol index (if applicable)
  polarity?: Polarity; // "POSITIVE" | "NEGATIVE"
  dcode?: number; // D-code reference
  id?: number; // Feature unique ID
  orient_def?: OrientDef; // Orientation metadata
  orient_def_rotation?: number; // Rotation in degrees
  attributeLookupTable?: { [key: string]: string }; // Custom attributes
}
```

**Polarity Enum:**

```typescript
enum Polarity {
  POSITIVE = 'POSITIVE', // Additive material
  NEGATIVE = 'NEGATIVE', // Subtractive (cutout)
}
```

---

### SymbolName

**Description:** Symbol definition with name and bounding box.

**TypeScript Interface:**

```typescript
interface SymbolName {
  name: string; // Symbol identifier (e.g., "r100", "pad_round_d60")
  index: number; // Symbol index for reference
  xSize?: number; // Bounding box width
  ySize?: number; // Bounding box height
  xMin?: number; // Bounding box min X
  yMin?: number; // Bounding box min Y
  xMax?: number; // Bounding box max X
  yMax?: number; // Bounding box max Y
}
```

---

## Feature Types Reference

### 1. Line (type = 5)

**Description:** Straight line segment with width.

**Fields:**

```typescript
interface LineFeature extends FeatureRecordBase {
  type: 5;
  xs: number; // Start X coordinate
  ys: number; // Start Y coordinate
  xe: number; // End X coordinate
  ye: number; // End Y coordinate
  sym_num?: number; // Line width symbol reference
}
```

**JSON Example:**

```json
{
  "type": 5,
  "xs": 10.5,
  "ys": 20.3,
  "xe": 45.8,
  "ye": 20.3,
  "sym_num": 12,
  "polarity": "POSITIVE"
}
```

**Rendering:** Draw line from (xs, ys) to (xe, ye) with width from symbol definition.

---

### 2. Pad (type = 1)

**Description:** Footprint pad or via with symbol shape.

**Fields:**

```typescript
interface PadFeature extends FeatureRecordBase {
  type: 1;
  x: number; // Center X coordinate
  y: number; // Center Y coordinate
  apt_def_symbol_num?: number; // Symbol index for pad shape
  apt_def_resize_factor?: number; // Scale multiplier (default: 1.0)
}
```

**JSON Example:**

```json
{
  "type": 1,
  "x": 15.24,
  "y": 10.16,
  "apt_def_symbol_num": 3,
  "apt_def_resize_factor": 1.0,
  "polarity": "POSITIVE",
  "dcode": 22
}
```

**Rendering:**

1. Lookup symbol from `apt_def_symbol_num` in `symbolNames` array
2. Fetch symbol features via `/symbols/{symbol_name}/features`
3. Scale by `apt_def_resize_factor`
4. Translate to (x, y) position

---

### 3. Arc (type = 0)

**Description:** Circular arc segment.

**Fields:**

```typescript
interface ArcFeature extends FeatureRecordBase {
  type: 0;
  x: number; // Start X coordinate
  y: number; // Start Y coordinate
  xc: number; // Center X coordinate (relative or absolute)
  yc: number; // Center Y coordinate (relative or absolute)
  cw?: boolean; // Clockwise direction (default: false)
  sym_num?: number; // Arc width symbol reference
}
```

**JSON Example:**

```json
{
  "type": 0,
  "x": 10.0,
  "y": 15.0,
  "xc": 12.0,
  "yc": 15.0,
  "cw": true,
  "sym_num": 8,
  "polarity": "POSITIVE"
}
```

**Rendering:** Arc from (x, y) around center (xc, yc) with direction `cw`.

---

### 4. Text (type = 4)

**Description:** Text annotation.

**Fields:**

```typescript
interface TextFeature extends FeatureRecordBase {
  type: 4;
  x: number; // Insertion X coordinate
  y: number; // Insertion Y coordinate
  font?: string; // Font name
  xsize?: number; // Character width
  ysize?: number; // Character height
  width_factor?: number; // Width scaling
  text?: string; // Text content
}
```

**JSON Example:**

```json
{
  "type": 4,
  "x": 25.4,
  "y": 30.0,
  "font": "standard",
  "xsize": 1.5,
  "ysize": 2.0,
  "width_factor": 1.0,
  "text": "U1",
  "polarity": "POSITIVE"
}
```

---

### 5. Surface (type = 2)

**Description:** Filled polygon region (copper pour, pad shape, etc.).

**Fields:**

```typescript
interface SurfaceFeature extends FeatureRecordBase {
  type: 2;
  contourPolygons: ContourPolygon[]; // Array of polygons (islands & holes)
}
```

**ContourPolygon Structure:**

```typescript
interface ContourPolygon {
  type: ContourPolygonType; // "ISLAND" | "HOLE"
  xStart: number; // Starting X coordinate
  yStart: number; // Starting Y coordinate
  polygonParts: PolygonPart[]; // Segments making up the polygon
}

enum ContourPolygonType {
  ISLAND = 'ISLAND', // Solid filled region
  HOLE = 'HOLE', // Cutout within island
}

interface PolygonPart {
  segments?: Segment[]; // Line segments
  arcs?: ArcPart[]; // Arc segments
}

interface Segment {
  xe: number; // End X coordinate
  ye: number; // End Y coordinate
}

interface ArcPart {
  xe: number; // End X coordinate
  ye: number; // End Y coordinate
  xc: number; // Center X (relative to start)
  yc: number; // Center Y (relative to start)
  cw: boolean; // Clockwise direction
}
```

**JSON Example:**

```json
{
  "type": 2,
  "polarity": "POSITIVE",
  "contourPolygons": [
    {
      "type": "ISLAND",
      "xStart": 10.0,
      "yStart": 10.0,
      "polygonParts": [
        {
          "segments": [
            { "xe": 20.0, "ye": 10.0 },
            { "xe": 20.0, "ye": 20.0 },
            { "xe": 10.0, "ye": 20.0 }
          ]
        }
      ]
    },
    {
      "type": "HOLE",
      "xStart": 12.0,
      "yStart": 12.0,
      "polygonParts": [
        {
          "segments": [
            { "xe": 18.0, "ye": 12.0 },
            { "xe": 18.0, "ye": 18.0 },
            { "xe": 12.0, "ye": 18.0 }
          ]
        }
      ]
    }
  ]
}
```

**Rendering Algorithm:**

1. Start at `(xStart, yStart)`
2. Iterate through `polygonParts`:
   - For each `segment`: draw line to `(xe, ye)`, update current position
   - For each `arc`: draw arc from current position to `(xe, ye)` around `(xc, yc)`
3. Close polygon automatically (last point connects to start)
4. Apply winding rules:
   - `ISLAND`: Fill region
   - `HOLE`: Subtract from island

---

## Error Handling

### HTTP Status Codes

| Code  | Description           | Common Causes                                 |
| ----- | --------------------- | --------------------------------------------- |
| `200` | Success               | Request completed successfully                |
| `401` | Unauthorized          | Missing or invalid Basic Auth credentials     |
| `404` | Not Found             | Design, step, layer, or symbol does not exist |
| `500` | Internal Server Error | Server-side processing failure                |

### Error Response Format

```json
{
  "error": "Design 'invalid_name' not found",
  "status": 404
}
```

### TypeScript Error Handling

```typescript
async function fetchLayerFeatures(
  design: string,
  step: string,
  layer: string
): Promise<FeaturesFile> {
  const url = `${API_BASE_URL}/filemodels/${design}/steps/${step}/layers/${layer}/features`;

  const response = await fetch(url, {
    headers: {
      Authorization: 'Basic ' + btoa(`${username}:${password}`),
    },
  });

  if (!response.ok) {
    if (response.status === 401) {
      throw new Error('Authentication failed. Check credentials.');
    }
    if (response.status === 404) {
      throw new Error(`Layer '${layer}' not found in step '${step}'`);
    }
    throw new Error(`HTTP ${response.status}: ${response.statusText}`);
  }

  return response.json();
}
```

---

## TypeScript Integration Guide

### 1. Install Dependencies

```bash
npm install axios  # or use fetch API
```

### 2. Create API Client (`src/api/odbDesignClient.ts`)

```typescript
import { FeaturesFile } from '../models/odb';

const API_BASE_URL = import.meta.env.VITE_API_BASE_URL || 'http://localhost:8888';
const AUTH =
  'Basic ' + btoa(`${import.meta.env.VITE_API_USERNAME}:${import.meta.env.VITE_API_PASSWORD}`);

export async function getLayerFeatures(
  design: string,
  step: string,
  layer: string
): Promise<FeaturesFile> {
  const response = await fetch(
    `${API_BASE_URL}/filemodels/${design}/steps/${step}/layers/${layer}/features`,
    { headers: { Authorization: AUTH } }
  );

  if (!response.ok) throw new Error(`HTTP ${response.status}`);
  return response.json();
}

export async function getSymbolFeatures(design: string, symbol: string): Promise<FeaturesFile> {
  const response = await fetch(`${API_BASE_URL}/filemodels/${design}/symbols/${symbol}/features`, {
    headers: { Authorization: AUTH },
  });

  if (!response.ok) throw new Error(`HTTP ${response.status}`);
  return response.json();
}

export async function listDesigns(): Promise<string[]> {
  const response = await fetch(`${API_BASE_URL}/filemodels`, { headers: { Authorization: AUTH } });

  if (!response.ok) throw new Error(`HTTP ${response.status}`);
  return response.json();
}
```

### 3. Use in React Components

```typescript
import { useEffect, useState } from 'react';
import { getLayerFeatures } from '../api/odbDesignClient';
import { FeaturesFile } from '../models/odb';

export function BoardViewer() {
  const [features, setFeatures] = useState<FeaturesFile | null>(null);
  const [error, setError] = useState<string | null>(null);

  useEffect(() => {
    getLayerFeatures('myboard', 'pcb', 'top_copper')
      .then(setFeatures)
      .catch(err => setError(err.message));
  }, []);

  if (error) return <div>Error: {error}</div>;
  if (!features) return <div>Loading...</div>;

  return (
    <div>
      <h2>Layer Features ({features.numFeatures} total)</h2>
      {features.featureRecords.map((feature, idx) => (
        <div key={idx}>Feature type: {feature.type}</div>
      ))}
    </div>
  );
}
```

### 4. Rendering Features with Three.js

```typescript
import * as THREE from 'three';
import { FeatureRecord, LineFeature } from '../models/odb';

function renderLine(feature: LineFeature, scene: THREE.Scene) {
  const geometry = new THREE.BufferGeometry().setFromPoints([
    new THREE.Vector3(feature.xs, feature.ys, 0),
    new THREE.Vector3(feature.xe, feature.ye, 0),
  ]);

  const material = new THREE.LineBasicMaterial({ color: 0xff0000 });
  const line = new THREE.Line(geometry, material);
  scene.add(line);
}
```

---

## Performance Considerations

### Large Feature Files

- Layer files can contain **10,000+ features**
- Surface polygons may have **1,000+ segments**
- JSON responses can exceed **10 MB**

### Optimization Strategies

1. **Lazy Loading**: Fetch layers on-demand (user selects visible layers)
2. **Pagination**: Request feature subsets (if server supports `?offset=&limit=`)
3. **Web Workers**: Parse JSON in background thread
4. **Geometry Caching**: Store parsed Three.js geometries in IndexedDB
5. **Level of Detail (LOD)**: Simplify polygons at low zoom levels
6. **Viewport Culling**: Only render features within camera frustum

### Example: Chunked Processing

```typescript
async function loadFeaturesInChunks(
  design: string,
  step: string,
  layer: string,
  onChunk: (features: FeatureRecord[]) => void
) {
  const data = await getLayerFeatures(design, step, layer);
  const CHUNK_SIZE = 1000;

  for (let i = 0; i < data.featureRecords.length; i += CHUNK_SIZE) {
    const chunk = data.featureRecords.slice(i, i + CHUNK_SIZE);
    onChunk(chunk);
    await new Promise((resolve) => setTimeout(resolve, 0)); // Yield to UI
  }
}
```

---

## Appendix: Protocol Buffer Schema References

### Source Files (OdbDesign Repository)

- **`featuresfile.proto`**: FeaturesFile, FeatureRecord definitions
- **`common.proto`**: ContourPolygon, PolygonPart structures
- **`enums.proto`**: Polarity, UnitType, ContourPolygonType enums
- **`symbolname.proto`**: SymbolName structure

### Serialization Path

```
C++ Model (FeaturesFile)
  → to_protobuf() [IProtoBuffable]
  → Protocol Buffer Message
  → google::protobuf::util::MessageToJsonString()
  → JSON String
  → JsonCrowReturnable<T>
  → HTTP Response
```

---

## Support & Contributing

- **OdbDesign Repository**: https://github.com/nam20485/OdbDesign
- **Server Issues**: https://github.com/nam20485/OdbDesign/issues
- **Client Issues**: https://github.com/nam20485/board-shape-view-client/issues

---

**Last Updated:** January 2025  
**API Version:** 0.9  
**Protocol:** ODB++ 8.1+
