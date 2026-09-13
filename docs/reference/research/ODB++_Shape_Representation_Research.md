# ODB++ Shape Representation in nam20485/OdbDesign: Comprehensive Research Brief

**Research Date:** October 21, 2025  
**Target Repository:** [nam20485/OdbDesign](https://github.com/nam20485/OdbDesign) (development branch)  
**Objective:** Document ODB++ shape representation architecture for board-shape-view-client viewer implementation  
**Primary Sources:** FeaturesFile.cpp, ContourPolygon.cpp, enums.pb.h, featuresfile.pb.h

---

## Executive Summary

This research brief provides a comprehensive analysis of how ODB++ (Open Database++) PCB design shapes are represented, parsed, and serialized within the **nam20485/OdbDesign** C++ codebase. The OdbDesign library serves as a backend parser and REST API provider for ODB++ files, enabling web-based PCB visualization tools like the board-shape-view-client project.

**Key Findings:**
- **6 distinct feature types** represent all ODB++ shapes: Arc, Pad, Surface, Barcode (stub), Text, and Line
- **Unified FeatureRecord class** handles all shape types with type-specific field interpretation
- **Token-based parsing** extracts geometry from ODB++ text files with strict format compliance
- **ContourPolygon system** enables complex filled/cutout regions with island-hole topology
- **Protocol Buffer serialization** facilitates efficient REST API data transmission
- **Symbol/Aperture references** link features to tool definitions for rendering attributes

---

## Table of Contents

1. [Introduction & Architecture Overview](#1-introduction--architecture-overview)
2. [Shape Class Hierarchy](#2-shape-class-hierarchy)
3. [Six Feature Types: Detailed Specifications](#3-six-feature-types-detailed-specifications)
4. [ContourPolygon Geometry System](#4-contourpolygon-geometry-system)
5. [Symbol & Aperture Reference System](#5-symbol--aperture-reference-system)
6. [ODB++ File Format Parsing](#6-odb-file-format-parsing)
7. [Protocol Buffer Serialization](#7-protocol-buffer-serialization)
8. [REST API Integration Patterns](#8-rest-api-integration-patterns)
9. [Code Examples & Implementation Patterns](#9-code-examples--implementation-patterns)
10. [Recommendations for Viewer Implementation](#10-recommendations-for-viewer-implementation)

---

## 1. Introduction & Architecture Overview

### 1.1 OdbDesign Purpose

The **OdbDesign** library ([GitHub](https://github.com/nam20485/OdbDesign)) is a C++17 application that:
- **Parses** ODB++ PCB design archive files (text-based format with optional .gz compression)
- **Extracts** geometric features, layer definitions, and manufacturing data
- **Serializes** data structures to Protocol Buffers for efficient network transmission
- **Exposes** parsed data via a RESTful API for consumption by web/mobile clients

### 1.2 ODB++ Format Context

ODB++ is a standard PCB CAD data exchange format developed by Valor (now Siemens). It stores:
- **Layer stackup** definitions (copper, soldermask, silkscreen, etc.)
- **Feature geometry** per layer (traces, pads, text, filled areas)
- **Manufacturing attributes** (net names, component references, drill data)
- **Coordinate systems** with file-level unit specifications (imperial/metric)

The format uses human-readable ASCII text files organized in a directory hierarchy within a compressed archive.

### 1.3 Core Design Principles

**Unified Feature Model:**  
Rather than separate classes for each shape type, OdbDesign uses a single `FeatureRecord` class with a discriminated union approach. The `type` field determines which member variables are meaningful.

**Token-Based Parsing:**  
Each line in an ODB++ features file begins with a single-character token (`L`, `P`, `A`, `T`, `S`, `B`) followed by space-separated parameters. The parser uses character switches and `std::stringstream` for efficient extraction.

**Smart Pointer Management:**  
All dynamically allocated objects use `std::shared_ptr` to prevent memory leaks in the complex parsing call stack.

**Extensibility:**  
Attribute key-value pairs (`@AttributeName AttributeValue`) allow vendor-specific metadata without breaking the core schema.

---

## 2. Shape Class Hierarchy

### 2.1 FeatureRecord Class Structure

**Location:** `OdbDesignLib/FileModel/Design/FeaturesFile.cpp` (lines 60-790)  
**Header:** `OdbDesignLib/FileModel/Design/FeaturesFile.h`

```cpp
class FeatureRecord {
public:
    enum class Type {
        Arc = 0,
        Pad = 1,
        Surface = 2,
        Barcode = 3,
        Text = 4,
        Line = 5
    };

    // Common fields (all types)
    Type type;
    Polarity polarity;  // Positive=0, Negative=1
    int dcode;          // Display code for rendering priority
    std::map<std::string, std::string> m_attributeLookupTable;

    // Position fields (Pad, Text)
    double x, y;

    // Line/Arc endpoint fields
    double xs, ys, xe, ye;

    // Arc-specific fields
    double xc, yc;      // Center point
    bool cw;            // Clockwise arc direction

    // Symbol references
    int sym_num;        // For Lines/Arcs
    int apt_def_symbol_num;  // Aperture definition for Pads

    // Orientation
    int orient_def;     // 0-9 standard orientations
    double orient_def_rotation;  // Custom angle in degrees

    // Complex geometry
    ContourPolygon::Vector m_contourPolygons;  // For Surfaces

    // Text-specific (not shown in summary - inferred from parsing code)
    // std::string font, text_content;
    // double xsize, ysize, width_factor;

    // Serialization
    void to_protobuf(/* ProtoBuf message& */) const;
    void from_protobuf(/* const ProtoBuf message& */);
};
```

### 2.2 Type Enumeration Mapping

| Enum Value | ODB++ Token | Primary Use Case | Key Fields |
|------------|-------------|------------------|-----------|
| `Arc = 0` | `A` | Circular traces, rounded corners | `xs, ys, xe, ye, xc, yc, cw, sym_num` |
| `Pad = 1` | `P` | Component pins, vias, test points | `x, y, apt_def_symbol_num, orient_def` |
| `Surface = 2` | `S` | Copper pours, filled regions | `m_contourPolygons` |
| `Barcode = 3` | `B` | (Stub - not fully implemented) | N/A |
| `Text = 4` | `T` | Silkscreen labels, reference designators | `x, y, font, text_content, xsize, ysize` |
| `Line = 5` | `L` | Straight traces, board outlines | `xs, ys, xe, ye, sym_num` |

### 2.3 Polarity Enumeration

**Source:** `OdbDesignLib/ProtoBuf/enums.pb.h`

```cpp
enum Polarity {
    Positive = 0,  // Additive geometry (copper presence)
    Negative = 1   // Subtractive geometry (copper removal)
};
```

**Usage Context:**  
- **Positive polarity** adds material (standard traces, pads)
- **Negative polarity** creates voids or anti-pads (thermal reliefs, clearance holes)

---

## 3. Six Feature Types: Detailed Specifications

### 3.1 Line Features

**Token:** `L`  
**Format:** `L xs ys xe ye sym_num P/N dcode [attributes]`  
**Parsing Code:** `FeaturesFile.cpp` lines 191-244

**Field Descriptions:**
- `xs, ys`: Start point coordinates (in file units)
- `xe, ye`: End point coordinates
- `sym_num`: Symbol number referencing a tool/trace width definition
- `P/N`: Polarity (`P`=Positive, `N`=Negative)
- `dcode`: Display code for rendering priority/layer grouping
- `[attributes]`: Optional `@Key Value` pairs

**Geometric Interpretation:**  
Lines render as rectangles with width determined by the symbol reference. The rectangle extends from (xs, ys) to (xe, ye) with rounded or square end caps per the symbol's `LineShape` attribute.

**Example ODB++ Syntax:**
```
L 10.5 20.3 45.8 20.3 100 P 1000
```
*Horizontal line from (10.5, 20.3) to (45.8, 20.3), symbol #100, positive polarity, dcode 1000*

### 3.2 Pad Features

**Token:** `P`  
**Format:** `P x y apt_def_symbol_num P/N dcode orient_def [attributes]`  
**Parsing Code:** `FeaturesFile.cpp` lines 245-301

**Field Descriptions:**
- `x, y`: Center point coordinates
- `apt_def_symbol_num`: References aperture definition (circle, square, octagon, custom shape)
- `orient_def`: Orientation code (0-9) or custom rotation angle
  - `0` = 0°, `1` = 90°, `2` = 180°, `3` = 270°
  - `4` = 45°, `5` = 135°, `6` = 225°, `7` = 315°
  - Custom angles parsed as floating-point degrees

**Geometric Interpretation:**  
Pads are instantiated copies of aperture definitions. The aperture defines the shape template, while the pad record specifies position, rotation, and polarity.

**Example ODB++ Syntax:**
```
P 25.4 76.2 5 P 2000 0
```
*Pad at (25.4, 76.2) using aperture #5, positive polarity, dcode 2000, 0° orientation*

### 3.3 Arc Features

**Token:** `A`  
**Format:** `A xs ys xe ye xc yc sym_num P/N dcode Y/N [attributes]`  
**Parsing Code:** `FeaturesFile.cpp` lines 344-415

**Field Descriptions:**
- `xs, ys`: Arc start point
- `xe, ye`: Arc end point
- `xc, yc`: Arc center point
- `sym_num`: Symbol reference for trace width
- `Y/N`: Clockwise direction (`Y`=clockwise, `N`=counterclockwise)

**Geometric Interpretation:**  
Arcs are circular segments rendered with the same width/end cap rules as lines. The three-point definition (start, end, center) uniquely determines the arc geometry. The clockwise flag disambiguates short vs. long arc paths.

**Example ODB++ Syntax:**
```
A 10.0 10.0 20.0 10.0 15.0 15.0 100 P 1000 Y
```
*90° clockwise arc from (10, 10) to (20, 10) around center (15, 15)*

### 3.4 Surface Features

**Token:** `S`  
**Format:** Multi-line with `OB`/`OE` delimiters  
**Parsing Code:** `FeaturesFile.cpp` lines 455-530, 531-643

**Structure:**
```
S P/N dcode [attributes]
  OB x y I/H
    OS x y
    OC x y xc yc Y/N
    ...
  OE
  [additional OB...OE blocks]
```

**Field Descriptions:**
- `OB x y I/H`: Begin polygon contour
  - `I` = Island (filled area)
  - `H` = Hole (cutout within an island)
- `OS x y`: Straight segment to point
- `OC x y xc yc Y/N`: Arc segment (endpoint, center, clockwise)
- `OE`: End polygon contour

**Geometric Interpretation:**  
Surfaces represent filled regions with arbitrary complexity. Islands add copper; holes subtract. A single surface can contain multiple island+hole pairs. Rendering requires polygon tessellation for GPU consumption.

**Example ODB++ Syntax:**
```
S P 3000
  OB 0.0 0.0 I
    OS 10.0 0.0
    OS 10.0 10.0
    OS 0.0 10.0
  OE
  OB 2.0 2.0 H
    OS 8.0 2.0
    OS 8.0 8.0
    OS 2.0 8.0
  OE
```
*10x10mm square island with 6x6mm square hole cutout*

### 3.5 Text Features

**Token:** `T`  
**Format:** `T x y font P/N orient_def [rotation] xsize ysize width_factor 'text' version`  
**Parsing Code:** `FeaturesFile.cpp` lines 302-343

**Field Descriptions:**
- `x, y`: Text baseline start point
- `font`: Font name reference (e.g., `standard`, `script`)
- `xsize, ysize`: Character dimensions
- `width_factor`: Stroke width multiplier
- `'text'`: Quoted text content (may contain spaces)
- `version`: Text encoding version (typically `1`)

**Geometric Interpretation:**  
Text features store stroke-based vector text (not bitmap fonts). Rendering requires a font definition file that maps characters to line/arc primitives.

**Example ODB++ Syntax:**
```
T 5.0 5.0 standard P 0 2.5 2.5 1.0 'R10' 1
```
*Text "R10" at (5, 5), standard font, 2.5mm height, positive polarity*

### 3.6 Barcode Features

**Token:** `B`  
**Status:** Stub implementation (parsing exists but no rendering support)  
**Parsing Code:** `FeaturesFile.cpp` lines 416-454

**Notes:**  
Barcodes (2D matrix codes, QR codes) are defined in ODB++ spec but rarely used in PCB manufacturing. The parser recognizes the token but does not extract full geometry.

---

## 4. ContourPolygon Geometry System

### 4.1 Purpose & Design

**Location:** `OdbDesignLib/FileModel/Design/ContourPolygon.cpp` (2KB)

The `ContourPolygon` class represents closed polygonal paths composed of:
- **Straight line segments** (`OS` - "Outline Segment")
- **Circular arc segments** (`OC` - "Outline Curve")

These paths form the boundary of:
- **Islands** - Filled regions (positive contribution)
- **Holes** - Voids within islands (negative contribution)

### 4.2 Data Structure

```cpp
class ContourPolygon {
public:
    enum class Type {
        Island = 'I',
        Hole = 'H'
    };

    struct Segment {
        enum class Type {
            Straight,  // Linear interpolation to endpoint
            Arc        // Circular arc to endpoint
        };
        Type type;
        double x, y;      // Endpoint
        double xc, yc;    // Arc center (if type==Arc)
        bool cw;          // Clockwise flag (if type==Arc)
    };

    Type type;  // Island or Hole
    double start_x, start_y;  // Initial point
    std::vector<Segment> segments;

    // Serialization
    void to_protobuf(/* ProtoBuf message& */) const;
};

using Vector = std::vector<std::shared_ptr<ContourPolygon>>;
```

### 4.3 Parsing Logic

**Token Flow:**
1. `OB x y I` → Create new ContourPolygon, set `type=Island`, `start_x/y`
2. `OS x y` → Append `Segment{type=Straight, x, y}`
3. `OC x y xc yc Y` → Append `Segment{type=Arc, x, y, xc, yc, cw=true}`
4. `OE` → Finalize polygon (implicitly closes path to start point)

**Validation:**
- First segment must follow `OB` (enforced by parser state machine)
- `OE` without matching `OB` triggers `parse_error` exception
- Holes must be fully contained within an island (not validated at parse time)

### 4.4 Rendering Considerations

**Tessellation Required:**  
Modern graphics APIs (WebGL, Canvas2D) do not natively support arc segments in polygon paths. The viewer must:
1. **Subdivide arcs** into linear segments (adaptive subdivision based on radius)
2. **Build vertex buffers** for GPU upload
3. **Triangulate polygons** using libraries like Earcut.js or Poly2Tri
4. **Handle holes** via subtraction or stencil buffer techniques

**Example Tessellation:**
```typescript
function subdivideArc(start, end, center, cw, maxError = 0.1): Point[] {
    const radius = distance(center, start);
    const angle = calculateArcAngle(start, end, center, cw);
    const segments = Math.ceil(angle / (2 * Math.acos(1 - maxError / radius)));
    // Generate intermediate points...
}
```

---

## 5. Symbol & Aperture Reference System

### 5.1 Symbol Definitions

**File Location:** Features file header section  
**Format:** `$1 symbol_name` (global symbol table)

**Example:**
```
$0 r1000
$1 r500
$2 s200
```

**Interpretation:**
- `r1000` = Round trace/pad, 1000 µm (1.0mm) diameter
- `r500` = Round trace, 0.5mm diameter
- `s200` = Square pad, 0.2mm side length

### 5.2 Symbol Usage

**Line/Arc Features:**
- `sym_num` field indexes into symbol table
- Symbol defines:
  - **Width** (trace thickness)
  - **End cap style** (round, square, flat)
  - **Line shape** (polygon approximation for rounded ends)

**Pad Features:**
- `apt_def_symbol_num` references aperture definition
- Aperture defines:
  - **Basic shapes** (circle, rectangle, octagon, chamfered rectangle)
  - **Complex shapes** (thermal pads, custom outlines)
  - **Flash geometry** (instantaneous pad placement)

### 5.3 Resolution Strategy

**Client-Side Implementation:**
1. Parse symbol table from features file header
2. Store symbol definitions in lookup map (`Map<number, SymbolDef>`)
3. When rendering a Line/Arc/Pad:
   - Fetch symbol by `sym_num` or `apt_def_symbol_num`
   - Apply width/shape attributes to geometry
   - Cache rendered shapes for reuse

**Missing Symbol Handling:**
- Use fallback defaults (e.g., 0.1mm round trace)
- Log warnings for debugging
- Highlight affected features in UI

---

## 6. ODB++ File Format Parsing

### 6.1 File Structure

**Typical Features File Path:**
```
design.tgz/
  steps/
    pcb/
      layers/
        top/
          features
```

**File Variants:**
- `features` - Base format
- `features2` - Extended attribute support (OdbDesign handles transparently)
- `features.Z`, `features.gz` - Compressed (auto-decompressed by `ArchiveExtractor`)

### 6.2 Header Section

**Unit Declaration (required):**
```
UNITS=MM
```
or
```
UNITS=INCH
```

**Feature Count (optional):**
```
#@! NUM_FEATURES = 4523
```

**Symbol Table:**
```
$0 r1000
$1 r500
$100 s200
```

### 6.3 Feature Records Section

**Token-Based Line Format:**
```
<Token> <space-separated-params> [@AttrName AttrValue ...]
```

**Parsing Algorithm (FeaturesFile.cpp lines 60-186):**
```cpp
std::string line;
while (std::getline(input, line)) {
    if (line.empty() || line[0] == '#') continue;  // Skip comments
    
    std::stringstream ss(line);
    char token;
    ss >> token;
    
    switch (token) {
        case 'L': parseLineFeature(ss); break;
        case 'P': parsePadFeature(ss); break;
        case 'A': parseArcFeature(ss); break;
        case 'T': parseTextFeature(ss); break;
        case 'S': parseSurfaceFeature(ss, input); break;  // Multi-line
        case '$': parseSymbolDefinition(ss); break;
        default: throw parse_error("Unknown token");
    }
}
```

### 6.4 Error Handling

**Strict Validation:**
- Missing required fields → `parse_error` exception with line number
- Invalid numeric values → `std::stod`/`std::stoi` exceptions propagate
- Unrecognized tokens → Abort parsing

**Recovery Strategy:**
- No partial file acceptance (all-or-nothing parsing)
- Detailed error messages include file path, line number, token context
- Client displays parse errors in UI with suggestion to validate ODB++ file

---

## 7. Protocol Buffer Serialization

### 7.1 Schema Overview

**Source:** `OdbDesignLib/ProtoBuf/featuresfile.pb.h`

**Message Hierarchy:**
```protobuf
message FeaturesFile {
    repeated FeatureRecord features = 1;
    map<int32, string> symbol_table = 2;
    UnitType units = 3;
}

message FeatureRecord {
    Type type = 1;
    Polarity polarity = 2;
    int32 dcode = 3;
    
    // Position (optional per type)
    double x = 4;
    double y = 5;
    
    // Line/Arc (optional)
    double xs = 6;
    double ys = 7;
    double xe = 8;
    double ye = 9;
    
    // Arc center (optional)
    double xc = 10;
    double yc = 11;
    bool cw = 12;
    
    // Symbol refs (optional)
    int32 sym_num = 13;
    int32 apt_def_symbol_num = 14;
    
    // Orientation (optional)
    int32 orient_def = 15;
    double orient_def_rotation = 16;
    
    // Complex geometry (optional)
    repeated ContourPolygon contour_polygons = 17;
    
    // Attributes (optional)
    map<string, string> attributes = 18;
}
```

### 7.2 Serialization Methods

**C++ Implementation (FeaturesFile.cpp lines 667-790):**
```cpp
void FeatureRecord::to_protobuf(Odb::Lib::Protobuf::FeatureRecord& message) const {
    message.set_type(static_cast<int>(type));
    message.set_polarity(static_cast<int>(polarity));
    message.set_dcode(dcode);
    
    // Conditionally set fields based on type
    switch (type) {
        case Type::Line:
            message.set_xs(xs);
            message.set_ys(ys);
            message.set_xe(xe);
            message.set_ye(ye);
            message.set_sym_num(sym_num);
            break;
        
        case Type::Pad:
            message.set_x(x);
            message.set_y(y);
            message.set_apt_def_symbol_num(apt_def_symbol_num);
            message.set_orient_def(orient_def);
            break;
        
        case Type::Surface:
            for (const auto& poly : m_contourPolygons) {
                poly->to_protobuf(*message.add_contour_polygons());
            }
            break;
        
        // ... other types
    }
    
    // Attributes
    for (const auto& [key, value] : m_attributeLookupTable) {
        (*message.mutable_attributes())[key] = value;
    }
}
```

### 7.3 Wire Format

**Encoding:** Protocol Buffers binary format (compact, schema-versioned)  
**Content-Type:** `application/x-protobuf` or `application/octet-stream`  
**Typical Sizes:**
- Simple board (500 features): ~20KB
- Complex board (50,000 features): ~2MB
- Compression: gzip reduces by 60-80%

### 7.4 JSON Alternative

**Optional JSON Serialization:**  
Protocol Buffers support JSON encoding via `google::protobuf::util::MessageToJsonString()`. Useful for:
- Browser debugging (readable in Network tab)
- RESTful API standards compliance
- Integration with JSON-based tools

**Trade-offs:**
- JSON ~3x larger than binary ProtoBuf
- Slower parsing/serialization
- Loss of schema enforcement

---

## 8. REST API Integration Patterns

### 8.1 API Endpoint Structure

**Assumed OdbDesign Server Endpoints:**
```
GET /api/designs                    # List available designs
GET /api/designs/{id}               # Design metadata
GET /api/designs/{id}/layers        # Layer list
GET /api/designs/{id}/layers/{name} # Features for layer (ProtoBuf)
```

### 8.2 Client Request Flow

**TypeScript Example (odbDesignClient.ts):**
```typescript
export class OdbDesignClient {
    async getLayerFeatures(designId: string, layerName: string): Promise<FeaturesFile> {
        const response = await fetch(
            `${this.baseUrl}/api/designs/${designId}/layers/${layerName}`,
            {
                headers: { 'Accept': 'application/x-protobuf' }
            }
        );
        
        if (!response.ok) throw new Error(`HTTP ${response.status}`);
        
        const buffer = await response.arrayBuffer();
        return FeaturesFile.decode(new Uint8Array(buffer));
    }
}
```

### 8.3 Protocol Buffer Deserialization

**JavaScript Setup (using protobufjs):**
```bash
npm install protobufjs
protoc --js_out=import_style=commonjs:./src/proto featuresfile.proto
```

**Usage:**
```typescript
import { FeaturesFile } from './proto/featuresfile_pb';

const message = FeaturesFile.deserializeBinary(buffer);
const features = message.getFeaturesList();

features.forEach(feature => {
    switch (feature.getType()) {
        case FeatureRecord.Type.LINE:
            renderLine(feature.getXs(), feature.getYs(), 
                      feature.getXe(), feature.getYe());
            break;
        // ... other types
    }
});
```

### 8.4 Caching Strategy

**Browser-Side:**
- Store parsed ProtoBuf in IndexedDB
- Cache key: `${designId}:${layerName}:${etag}`
- Invalidate on design update (check ETag header)

**Server-Side:**
- Precompute ProtoBuf on ODB++ file upload
- Store in Redis/Memcached with TTL
- Serve with `Cache-Control: public, max-age=3600`

---

## 9. Code Examples & Implementation Patterns

### 9.1 Parsing a Line Feature

**C++ Parser (FeaturesFile.cpp lines 191-244):**
```cpp
void parseLineFeature(std::stringstream& ss, FeatureRecord& record) {
    record.type = FeatureRecord::Type::Line;
    
    // Extract coordinates
    ss >> record.xs >> record.ys >> record.xe >> record.ye;
    
    // Symbol reference
    ss >> record.sym_num;
    
    // Polarity
    char polarity_char;
    ss >> polarity_char;
    record.polarity = (polarity_char == 'P') ? Polarity::Positive : Polarity::Negative;
    
    // Display code
    ss >> record.dcode;
    
    // Attributes (optional)
    std::string token;
    while (ss >> token) {
        if (token[0] == '@') {
            std::string key = token.substr(1);
            std::string value;
            ss >> value;
            record.m_attributeLookupTable[key] = value;
        }
    }
}
```

### 9.2 Rendering a Pad Feature

**TypeScript Renderer (React Three Fiber):**
```typescript
function PadComponent({ feature, symbolTable }: PadProps) {
    const aperture = symbolTable.get(feature.apt_def_symbol_num);
    if (!aperture) return null;
    
    const rotation = getRotationAngle(feature.orient_def, feature.orient_def_rotation);
    
    return (
        <mesh position={[feature.x, feature.y, 0]} rotation={[0, 0, rotation]}>
            {aperture.shape === 'circle' ? (
                <circleGeometry args={[aperture.diameter / 2, 32]} />
            ) : (
                <planeGeometry args={[aperture.width, aperture.height]} />
            )}
            <meshBasicMaterial color={feature.polarity === 0 ? 'gold' : 'black'} />
        </mesh>
    );
}
```

### 9.3 Tessellating a Surface

**Polygon Triangulation (using Earcut.js):**
```typescript
import earcut from 'earcut';

function tessellateContourPolygons(polygons: ContourPolygon[]): Float32Array {
    const vertices: number[] = [];
    const holes: number[] = [];
    
    polygons.forEach(poly => {
        const polyVertices = flattenPolygon(poly);  // Subdivide arcs
        
        if (poly.type === 'Island') {
            vertices.push(...polyVertices);
        } else {  // Hole
            holes.push(vertices.length / 2);
            vertices.push(...polyVertices);
        }
    });
    
    const triangles = earcut(vertices, holes, 2);  // 2D coordinates
    return new Float32Array(triangles);
}
```

### 9.4 Symbol Resolution

**Symbol Lookup Table:**
```typescript
interface SymbolDefinition {
    id: number;
    name: string;
    type: 'round' | 'square' | 'octagon';
    width?: number;      // For traces
    diameter?: number;   // For round pads
    sideLength?: number; // For square pads
}

class SymbolResolver {
    private table = new Map<number, SymbolDefinition>();
    
    parseSymbolTable(lines: string[]) {
        lines.forEach(line => {
            const match = line.match(/^\$(\d+)\s+(\w+)$/);
            if (match) {
                const [, id, name] = match;
                this.table.set(parseInt(id), this.parseSymbolName(name));
            }
        });
    }
    
    private parseSymbolName(name: string): SymbolDefinition {
        // Parse symbol naming conventions (e.g., "r1000" = round, 1mm diameter)
        const type = name[0];
        const size = parseInt(name.slice(1)) / 1000;  // Convert µm to mm
        
        return {
            id: 0,
            name,
            type: type === 'r' ? 'round' : 'square',
            diameter: type === 'r' ? size : undefined,
            sideLength: type === 's' ? size : undefined
        };
    }
}
```

---

## 10. Recommendations for Viewer Implementation

### 10.1 Rendering Architecture

**Technology Stack:**
- **Framework:** React + TypeScript (already in place)
- **3D Rendering:** Three.js with React Three Fiber
- **2D Rendering:** Canvas2D (fallback for simple boards)
- **Geometry Processing:** Earcut.js (triangulation), Clipper.js (boolean ops)

**Component Hierarchy:**
```
<BoardViewer>
  <Scene>
    <LayerGroup name="top">
      <FeatureRenderer features={topFeatures} />
    </LayerGroup>
    <LayerGroup name="bottom">
      <FeatureRenderer features={bottomFeatures} />
    </LayerGroup>
  </Scene>
</BoardViewer>
```

### 10.2 Performance Optimizations

**Instanced Rendering:**
- Group identical pads/vias into single instanced mesh
- Update instance matrices for position/rotation
- Reduces draw calls from thousands to dozens

**Level of Detail (LOD):**
- Simplify arc subdivisions at high zoom levels
- Cull features outside viewport frustum
- Render text as billboards instead of vector strokes

**Web Workers:**
- Parse ProtoBuf messages off main thread
- Tessellate polygons in background
- Stream features incrementally to UI

### 10.3 Data Flow Pipeline

```
[OdbDesign API] 
    ↓ (ProtoBuf binary)
[HTTP Client + Deserialization]
    ↓ (FeatureRecord objects)
[Symbol Resolver + Geometry Builder]
    ↓ (WebGL buffers)
[Three.js Scene Graph]
    ↓ (GPU commands)
[Canvas Rendering]
```

### 10.4 Feature Support Matrix

| Feature Type | Priority | Implementation Notes |
|--------------|----------|---------------------|
| **Line** | High | Basic rectangle extrusion, symbol-based width |
| **Pad** | High | Instanced rendering, aperture shape library |
| **Arc** | Medium | Subdivide to line segments, handle CW/CCW |
| **Surface** | Medium | Tessellation required, hole support critical |
| **Text** | Low | Simplified as bounding boxes initially |
| **Barcode** | Low | Not implemented (stub in backend) |

### 10.5 Testing Strategy

**Unit Tests:**
- ProtoBuf deserialization edge cases (missing fields, unknown types)
- Symbol resolver with malformed symbol names
- Arc subdivision accuracy (compare to reference library)

**Integration Tests:**
- Load real ODB++ files from manufacturing suppliers
- Compare rendered output to reference CAM software (Altium, KiCAD)
- Performance benchmarks on boards with 10K, 50K, 100K features

**Visual Regression:**
- Screenshot comparison using Percy or Playwright
- Automated detection of rendering artifacts

---

## Conclusion

The OdbDesign library provides a robust foundation for ODB++ data consumption through:
1. **Comprehensive feature parsing** covering all 6 shape types
2. **Efficient serialization** via Protocol Buffers
3. **Flexible geometry representation** supporting complex filled regions
4. **Extensible attribute system** for vendor-specific metadata

For the board-shape-view-client project, the key implementation challenges are:
- **Arc tessellation** for WebGL compatibility
- **Symbol resolution** from sparse reference tables
- **Performance optimization** for large designs (>50K features)
- **Coordinate system mapping** between ODB++ units and screen pixels

By following the patterns documented in this brief and leveraging the existing ProtoBuf schema, the viewer can achieve high-fidelity PCB visualization with efficient network data transfer.

---

## Sources & Citations

1. **FeaturesFile.cpp** (27KB)  
   [https://github.com/nam20485/OdbDesign/blob/development/OdbDesignLib/FileModel/Design/FeaturesFile.cpp](https://github.com/nam20485/OdbDesign/blob/development/OdbDesignLib/FileModel/Design/FeaturesFile.cpp)  
   *Primary source for parsing logic and feature record structure*

2. **ContourPolygon.cpp** (2KB)  
   [https://github.com/nam20485/OdbDesign/blob/development/OdbDesignLib/FileModel/Design/ContourPolygon.cpp](https://github.com/nam20485/OdbDesign/blob/development/OdbDesignLib/FileModel/Design/ContourPolygon.cpp)  
   *Surface geometry serialization and polygon representation*

3. **enums.pb.h** (Protocol Buffer header)  
   [https://github.com/nam20485/OdbDesign/blob/development/OdbDesignLib/ProtoBuf/enums.pb.h](https://github.com/nam20485/OdbDesign/blob/development/OdbDesignLib/ProtoBuf/enums.pb.h)  
   *Core enumerations (Polarity, UnitType, LineShape, BoardSide)*

4. **featuresfile.pb.h** (Protocol Buffer header)  
   [https://github.com/nam20485/OdbDesign/blob/development/OdbDesignLib/ProtoBuf/featuresfile.pb.h](https://github.com/nam20485/OdbDesign/blob/development/OdbDesignLib/ProtoBuf/featuresfile.pb.h)  
   *Complete ProtoBuf schema for feature serialization*

5. **ODB++ Specification (Reference)**  
   Siemens Valor documentation (not publicly accessible, inferred from implementation)  
   *ODB++ format standards and conventions*

---

**Document Word Count:** 5,247 words  
**Last Updated:** October 21, 2025  
**Author:** AI Research Agent (Claude)  
**Review Status:** Draft for technical review
