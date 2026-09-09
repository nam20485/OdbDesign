# ODB++ Shape Definitions and Feature Architecture

**Document Version:** 1.0  
**Based on:** ODB++Design Format Specification Release 8.1 Update 4 (August 2024)  
**Author:** Research Agent  
**Date:** 2025-10-21  
**Purpose:** Comprehensive reference for implementing ODB++ shape rendering in board-shape-view-client

---

## Table of Contents

1. [Overview and Architecture](#overview-and-architecture)
2. [Feature File Format](#feature-file-format)
3. [Feature Record Types](#feature-record-types)
4. [Standard Symbol Definitions](#standard-symbol-definitions)
5. [Coordinate Systems and Transformations](#coordinate-systems-and-transformations)
6. [Data Structures and Parsing](#data-structures-and-parsing)
7. [Implementation Considerations](#implementation-considerations)
8. [Rendering Guidelines](#rendering-guidelines)
9. [Appendix: Complete Symbol Reference](#appendix-complete-symbol-reference)

---

## Overview and Architecture

### What is ODB++?

ODB++ (Open Database++) is an intelligent, open data format for exchanging PCB design information throughout the entire electronics manufacturing process. At its core, ODB++ represents PCB designs as hierarchical collections of **layers**, each containing **features** defined by geometric **symbols**.

### Key Architectural Concepts

**Features** are the fundamental building blocks of PCB layers. Each feature represents a geometric entity with:
- **Type**: Line, Pad, Arc, Text, Barcode, or Surface
- **Geometry**: Defined by coordinates and a symbol reference
- **Attributes**: Associated metadata (nets, components, user-defined properties)
- **Polarity**: Positive (additive) or Negative (subtractive) rendering

**Symbols** define reusable shape templates. ODB++ provides 40+ standard symbols (circles, rectangles, donuts, thermal reliefs, etc.) plus support for user-defined custom symbols. Each symbol has:
- **Name**: Unique identifier (e.g., "r100x50", "donut_r150x100")
- **Parameters**: Dimensions embedded in the name (in 1/10 microns)
- **Primitive shapes**: Constructed from circles, rectangles, and polygons

**Layers** organize features into functional groups (copper, soldermask, silkscreen, etc.). Each layer contains:
- A `features` file with geometry definitions
- Optional attribute files for metadata
- Layer-specific properties (type, context, side)

### Data Flow in board-shape-view-client

1. **Parse ODB++ archive** → Extract layer structure and feature files
2. **Read feature file** → Parse UNITS, ID, and F records
3. **Resolve symbols** → Look up standard symbols or parse custom definitions
4. **Transform coordinates** → Apply orientation/mirroring transformations
5. **Render features** → Convert to canvas/WebGL primitives with polarity
6. **Display board** → Layer composition with proper Z-order and colors

---

## Feature File Format

### File Structure

Each layer's `features` file follows this structure:

```
UNITS=<resolution_mode>
#
# Optional comments
#
ID=<layer_id>,<attribute_list>
```

F <x> <y> <symbol> <polarity> <dcode> <orient> [<attrib>]
F <x> <y> <symbol> <polarity> <dcode> <orient> [<attrib>]
...
$<symbol_name>
<symbol_definition_primitives>

@<attribute_table_name>
<attribute_records>

&<user_attribute_table_name>
<user_attribute_records>
```

### UNITS Directive

**Syntax:** `UNITS=<mode>`

Defines the coordinate resolution for all numeric values in the file:

- `UNITS=I` (Imperial): 1 unit = 1/10000 inch (2.54 microns)
- `UNITS=M` (Metric): 1 unit = 1/1000 mm (1 micron) **[MOST COMMON]**

**Critical:** All coordinate calculations must account for this unit system. Modern ODB++ files overwhelmingly use metric (M).

### ID Directive

**Syntax:** `ID=<layer_name>,<attr_list>`

Specifies the layer name and optional comma-separated attributes. Example:

```
ID=L1,TYP=SIGNAL,CONTEXT=BOARD,SIDE=TOP
```

Common attributes:
- `TYP`: Layer type (SIGNAL, POWER_GROUND, SOLDERMASK, SILKSCREEN, etc.)
- `CONTEXT`: BOARD, MISC (board outline vs. auxiliary data)
- `SIDE`: TOP, BOTTOM, INTERNAL

### Feature Records (F)

**Syntax:** `F <x> <y> <symbol> <polarity> <dcode> <orient> [<attrib>]`

Each F record defines one feature instance:

- **x, y** (required): Location coordinates (in UNITS resolution)
- **symbol** (required): Symbol name reference (e.g., "r100x50")
- **polarity** (required): P (positive/additive) or N (negative/subtractive)
- **dcode** (optional): Drawing code for legacy compatibility (usually 0)
- **orient** (optional): Orientation mode (0-9, see transformation section)
- **attrib** (optional): Attribute string index (references @ or & tables)

**Examples:**

```
F 12500 34000 r200x100 P 0 0         # Rectangular pad at (12.5, 34.0) mm
F 0 0 donut_r150x100 P 0 0           # Donut at origin
F -5000 10000 c50 N 0 0              # Circular negative feature
```

### Symbol Lookup ($)

Custom symbols can be defined inline using $ directives:

```
$custom_symbol_name
C 0 0 500         # Circle primitive: center_x, center_y, diameter
```

### Attribute Tables (@ and &)

Attribute tables store metadata referenced by feature records:

```
@NET
```
0,NET=GND
1,NET=VCC
2,NET=SIGNAL_A

&USER_ATTRS
0,COMPONENT=U1,PIN=5
```

Features reference these by index in their `[attrib]` field.

---

## Feature Record Types

ODB++ defines six core feature types, each with specific geometric properties and use cases.

### 1. Line (L Record)

**Purpose:** Represents linear trace segments with specified width.

**Syntax:** `L <x_start> <y_start> <x_end> <y_end> <width> <polarity> [<attrib>]`

**Parameters:**
- `x_start, y_start`: Starting point coordinates
- `x_end, y_end`: Ending point coordinates
- `width`: Line width (in UNITS resolution)
- `polarity`: P or N
- `attrib`: Optional attribute index

**Use Cases:**
- PCB traces and routing
- Silkscreen lines
- Board outline segments

**Example:**
```
L 10000 20000 30000 20000 250 P     # Horizontal 0.25mm trace
L 0 0 5000 5000 150 N                # 45° negative line
```

**Rendering Notes:**
- Lines have rounded end caps (semicircular)
- Width defines the stroke thickness
- Negative polarity subtracts from existing geometry

### 2. Pad (P Record) - MOST COMMON

**Purpose:** Places symbol instances at specific locations with transformations.

**Syntax:** `P <x> <y> <symbol> <polarity> <dcode> <orient> [<attrib>]`

**Parameters:**
- `x, y`: Placement coordinates
- `symbol`: Symbol name (references standard or custom symbol)
- `polarity`: P (copper pour) or N (keepout/cutout)
- `dcode`: Legacy drawing code (usually 0)
- `orient`: Transformation mode (see transformation section)
- `attrib`: Optional attribute index

**Use Cases:**
- Component pads and vias
- Thermal relief connections
- Soldermask openings
- Any repeated geometric shape

**Example:**
```
P 15000 25000 r300x200 P 0 0         # Rectangular pad
P 0 0 donut_r200x150 P 0 2           # 90° rotated donut
P 10000 10000 thermal30 P 0 0 @0     # Thermal relief with net attr
```

**Symbol Resolution Priority:**
1. Check for inline $ definition in current file
2. Look up in standard symbol library
3. Check for user-defined symbols in symbols/ directory

### 3. Arc (A Record)

**Purpose:** Represents curved trace segments.

**Syntax:** `A <x_start> <y_start> <x_end> <y_end> <x_center> <y_center> <width> <polarity> [<attrib>]`

**Parameters:**
- `x_start, y_start`: Arc starting point
- `x_end, y_end`: Arc ending point
- `x_center, y_center`: Arc center point (defines radius and sweep)
- `width`: Arc stroke width
- `polarity`: P or N
- `attrib`: Optional attribute index

**Geometry Rules:**
- Arc is drawn counter-clockwise from start to end
- Center point defines the radius: `r = distance(center, start)`
- Sweep angle determined by start/end positions relative to center

**Example:**
```
A 10000 0 0 10000 0 0 200 P          # 90° quarter circle, radius=10mm
A 5000 5000 15000 5000 10000 0 150 N # Curved cutout
```

**Rendering Notes:**
- May require tessellation into line segments for WebGL
- Smaller width arcs need more segments for smooth curves
- End caps are rounded (like line features)

### 4. Text (T Record)

**Purpose:** Places text strings for labels, reference designators, and documentation.

**Syntax:** `T <x> <y> <font> <polarity> <orient> <x_size> <y_size> <width_factor> <text> [<version>] [<attrib>]`

**Parameters:**
- `x, y`: Text insertion point (baseline left)
- `font`: Font name (e.g., "standard", "courier")
- `polarity`: P (rendered) or N (masked text)
- `orient`: Rotation (see transformation modes)
- `x_size`: Character width (in UNITS)
- `y_size`: Character height (in UNITS)
- `width_factor`: Stroke width multiplier (1.0 = normal)
- `text`: String content (spaces preserved)
- `version`: Optional font version
- `attrib`: Optional attribute index

**Example:**
```
T 10000 20000 standard P 0 800 800 1.0 "R1"
T 5000 5000 courier P 2 1000 1200 1.2 "VCC" 1
```

**Rendering Considerations:**
- Text is typically converted to stroke paths or texture-mapped glyphs
- Font definitions may reference external font files
- Mirror/rotation affects text orientation

### 5. Barcode (B Record)

**Purpose:** Encodes machine-readable data (mostly for panelization/manufacturing).

**Syntax:** `B <x> <y> <barcode_type> <polarity> <font> <orient> <barcode_string> [<attrib>]`

**Parameters:**
- `x, y`: Barcode origin
- `barcode_type`: Format (e.g., "code39", "qr")
- `polarity`: P or N
- `font`: Font for human-readable text
- `orient`: Rotation
- `barcode_string`: Encoded data
- `attrib`: Optional attribute index

**Use Cases:**
- Panel serialization
- Manufacturing tracking
- Component identification

**Implementation Priority:** Low (rare in typical PCB designs)

### 6. Surface (S Record)

**Purpose:** Defines complex filled regions and copper pours.

**Syntax:** `S <polarity> [<attrib>]`  
Followed by contour definition records defining the boundary polygon.

**Parameters:**
- `polarity`: P (copper fill) or N (cutout/void)
- `attrib`: Optional attribute index
- **Contour data**: Subsequent lines define polygon vertices and islands (holes)

**Contour Format:**
```
S P
OB x1 y1              # Begin outer contour at (x1, y1)
OS x2 y2              # Segment to next vertex
OS x3 y3
OE                    # End outer contour (auto-closes to OB point)
IB x4 y4              # Begin island (hole) contour
IS x5 y5
IE                    # End island
```

**Use Cases:**
- Copper pours and ground planes
- Complex pad shapes
- Board outlines and cutouts

**Rendering Notes:**
- Requires polygon tessellation for WebGL rendering
- Islands create holes in the filled region
- Negative surfaces cut out from positive geometry

---

## Standard Symbol Definitions

ODB++ defines 40+ standard symbols for common PCB features. Symbols are parametric with dimensions encoded in their names.

### Naming Convention

Format: `<shape><width>x<height>` or `<shape>_<modifiers>`

Dimensions are in **1/10 microns** (multiply by 0.1 to get microns).

**Examples:**
- `r200x100` → Rectangle, 20.0 µm × 10.0 µm (0.02 mm × 0.01 mm)
- `c150` → Circle, diameter = 15.0 µm (0.015 mm)
- `s200x150` → Square, 20 µm side (height param ignored)

### Basic Shapes

#### Circle (c)
**Syntax:** `c<diameter>`  
**Example:** `c1000` → 100 µm (0.1 mm) diameter circle

**Primitives:**
```
C 0 0 <diameter>      # Single circle primitive at origin
```

**Use Cases:** Vias, round pads, holes

#### Rectangle (r)
**Syntax:** `r<width>x<height>`  
**Example:** `r3000x1500` → 300 µm × 150 µm rectangle

**Primitives:**
```
R 0 0 <width> <height>     # Rectangle centered at origin
```

**Use Cases:** SMD pads, rectangular lands

#### Square (s)
**Syntax:** `s<size>` (height parameter present but ignored)  
**Example:** `s2000x0` → 200 µm × 200 µm square

**Primitives:**
```
R 0 0 <size> <size>
```

#### Rounded Rectangle (rnd_r)
**Syntax:** `rnd_r<width>x<height>x<corner_radius>`  
**Example:** `rnd_r4000x2000x500` → 400 µm × 200 µm with 50 µm corner radius

**Primitives:** Combination of rectangles and arc segments

**Use Cases:** Modern SMD pads with rounded corners

#### Oval (oval)
**Syntax:** `oval<width>x<height>`  
**Example:** `oval5000x3000` → Elongated oval pad

**Primitives:** Rectangle with semicircular end caps

#### Oblong (oblong)
**Syntax:** `oblong<width>x<height>`  
**Example:** `oblong6000x2000` → Stadium shape (pill shape)

**Primitives:** Similar to oval but with specific construction rules

### Donut Shapes (Annular Rings)

Donuts represent circular features with holes (like via pads with drilled centers).

#### Round Donut (donut_r)
**Syntax:** `donut_r<outer_diameter>x<inner_diameter>`  
**Example:** `donut_r2000x1000` → 200 µm outer / 100 µm inner

**Primitives:**
```
C 0 0 <outer_diameter>     # Outer circle
H 0 0 <inner_diameter>     # Hole (negative circle)
```

**Use Cases:** Via pads, plated through-holes

#### Square Donut (donut_s)
**Syntax:** `donut_s<outer_size>x<inner_diameter>`  
**Example:** `donut_s3000x1500` → 300 µm square with 150 µm hole

**Primitives:**
```
R 0 0 <outer_size> <outer_size>
H 0 0 <inner_diameter>
```

#### Rectangular Donut (donut_rc)
**Syntax:** `donut_rc<width>x<height>x<hole_diameter>`  
**Example:** `donut_rc4000x2000x1200` → 400×200 µm rect with 120 µm hole

### Thermal Relief Shapes

Thermal reliefs provide electrical connection while minimizing heat transfer during soldering.

#### Round Thermal (thermal)
**Syntax:** `thermal<outer_diameter>x<gap_width>x<spoke_width>x<num_spokes>x<angle>`  
**Example:** `thermal3000x400x600x4x45` → 300 µm pad, 4 spokes at 45°

**Primitives:** Outer annulus with radial gaps and connecting spokes

**Spoke Patterns:**
- `num_spokes=4`: Cross pattern (0°, 90°, 180°, 270°)
- `num_spokes=3`: Y-pattern (0°, 120°, 240°)
- `angle`: Rotation offset for first spoke

**Use Cases:** Power/ground plane connections, thermal management

#### Square Thermal (thermal_s)
**Syntax:** `thermal_s<outer_size>x<gap_width>x<spoke_width>x<num_spokes>x<angle>`

**Example:** `thermal_s4000x500x800x4x0` → Square thermal with cross pattern

### Specialized Shapes

#### Diamond (di)
**Syntax:** `di<width>x<height>`  
**Example:** `di2000x2000` → 200 µm diamond (rotated square)

#### Octagon (oct)
**Syntax:** `oct<width>x<height>x<corner_size>`  
**Example:** `oct5000x5000x1000` → Octagonal pad with beveled corners

#### Ellipse (el)
**Syntax:** `el<width>x<height>`  
**Example:** `el6000x4000` → True ellipse (not oval)

#### Triangle (tri)
**Syntax:** `tri<base>x<height>`  
**Example:** `tri3000x2000` → Isosceles triangle

#### Half-circle (half_circle)
**Syntax:** `half_circle<diameter>x<orientation>`  
**Example:** `half_circle2000x0` → Semicircle, flat edge on specified side

#### Chamfered Rectangle (ch_r)
**Syntax:** `ch_r<width>x<height>x<chamfer_size>`  
**Example:** `ch_r4000x3000x500` → Rectangle with 45° corner cuts

### Stencil-Specific Shapes

Used for solder paste stencils and aperture reduction:

#### Reduced Rectangle (rect_reduced)
**Syntax:** `rect_reduced<width>x<height>x<reduction>`  
**Example:** `rect_reduced3000x2000x200` → 300×200 µm rect minus 20 µm on all sides

#### Home Plate (home)
**Syntax:** `home<width>x<height>x<point_size>`

**Example:** `home5000x3000x1000` → Pentagon shape (like home plate in baseball)

**Use Cases:** Directional pads, alignment marks

---

## Coordinate Systems and Transformations

### Coordinate Resolution

ODB++ uses fixed-point integer coordinates with resolution defined by UNITS:

- **Metric (M):** 1 unit = 1 µm = 0.001 mm  
  Example: `10000` = 10,000 µm = 10.0 mm
  
- **Imperial (I):** 1 unit = 0.0001 inch = 2.54 µm  
  Example: `10000` = 1.0 inch = 25.4 mm

**Valid Range:** -1,000,000,000 to +1,000,000,000 units  
→ Metric: ±1000 meters  
→ Imperial: ±100,000 inches

### Origin and Orientation

- **Origin (0,0):** Bottom-left corner of the PCB design space
- **X-axis:** Increases to the right
- **Y-axis:** Increases upward
- **Rotation:** Counter-clockwise (mathematical convention)

### Orientation Modes (orient parameter)

The `orient` field in P records defines transformation applied to the symbol:

#### Legacy Modes (0-7): Fixed 90° Rotations + Mirror

| Mode | Rotation | Mirror X | Mirror Y | Description |
|------|----------|----------|----------|-------------|
| 0    | 0°       | No       | No       | No transform |
| 1    | 90°      | No       | No       | CCW quarter turn |
| 2    | 180°     | No       | No       | Half turn |
| 3    | 270°     | No       | No       | CW quarter turn |
| 4    | 0°       | Yes      | No       | Flip horizontal |
| 5    | 90°      | Yes      | No       | Flip then rotate 90° |
| 6    | 180°     | Yes      | No       | Flip then rotate 180° |
| 7    | 270°     | Yes      | No       | Flip then rotate 270° |

**Transform Order:** Mirror (if enabled) → Rotate → Translate to (x, y)

#### Modern Modes (8-9): Arbitrary Angles

- **Mode 8:** Arbitrary rotation, encode angle in symbol name  
  Example: `symbol@45.5` → Rotate 45.5° counter-clockwise
  
- **Mode 9:** Arbitrary rotation + mirror X, encode in symbol name  
  Example: `symbol@-30.0` → Mirror X, then rotate -30° (CW 30°)

**Angle Encoding:** Append `@<angle>` to symbol name where angle is in degrees, range [-360, 360].

### Transformation Matrix (for implementation)

For rendering engines, convert orientation to a 3×3 affine transformation matrix:

```python
def get_transform_matrix(orient, x, y, angle=0.0):
    # Modes 0-7: Fixed rotations
    rotations = [0, 90, 180, 270, 0, 90, 180, 270]
    mirror_x = [False] * 4 + [True] * 4
    
    if orient < 8:
        theta = radians(rotations[orient])
        mx = -1 if mirror_x[orient] else 1
        
        # Matrix: [mx*cos(θ)  -sin(θ)  x]
        #         [mx*sin(θ)   cos(θ)  y]
        #         [0           0       1]
        return affine_matrix(mx * cos(theta), sin(theta), x, y)
    
    # Modes 8-9: Arbitrary angle
    elif orient in [8, 9]:
        theta = radians(angle)
        mx = -1 if orient == 9 else 1
        return affine_matrix(mx * cos(theta), sin(theta), x, y)
```

### Polarity Handling

**Positive (P):** Additive geometry
- Copper layers: Adds conductive material
- Soldermask: Opens window (exposes copper)
- Silkscreen: Prints ink

**Negative (N):** Subtractive geometry
- Copper layers: Creates clearance/cutout
- Soldermask: Blocks opening (covers copper)
- Silkscreen: Removes ink

**Rendering Strategy:**
1. Render all positive features first
2. Subtract negative features using stencil buffer or boolean operations
3. Apply layer-specific color/opacity

---

## Data Structures and Parsing

### TypeScript Type Definitions

```typescript
// Core Feature Types
export type FeatureType = 'L' | 'P' | 'A' | 'T' | 'B' | 'S';
export type Polarity = 'P' | 'N';
export type UnitMode = 'I' | 'M';

// Base Feature Interface
export interface Feature {
  type: FeatureType;
  polarity: Polarity;
  attributes?: AttributeRef;
}

// Line Feature
export interface LineFeature extends Feature {
  type: 'L';
  start: Point;
  end: Point;
  width: number;
}

// Pad Feature (most common)
export interface PadFeature extends Feature {
  type: 'P';
  position: Point;
  symbol: string;
  dcode: number;
  orient: number;
}

// Arc Feature
export interface ArcFeature extends Feature {
  type: 'A';
  start: Point;
  end: Point;
  center: Point;
  width: number;
}

// Text Feature
export interface TextFeature extends Feature {
  type: 'T';
  position: Point;
  font: string;
  orient: number;
  xSize: number;
  ySize: number;
  widthFactor: number;
  text: string;
  version?: number;
}

// Surface Feature (complex polygons)
export interface SurfaceFeature extends Feature {
  type: 'S';
  contours: Contour[];
}

export interface Contour {
  type: 'outer' | 'island';
  points: Point[];
}

// Symbol Definition
export interface Symbol {
  name: string;
  primitives: Primitive[];
  params: SymbolParams;
}

export interface Primitive {
  type: 'C' | 'R' | 'P' | 'H';  // Circle, Rectangle, Polygon, Hole
  geometry: number[];
}

// Layer Context
export interface Layer {
  name: string;
  type: string;
  context: 'BOARD' | 'MISC';
  side?: 'TOP' | 'BOTTOM' | 'INTERNAL';
  features: Feature[];
  symbols: Map<string, Symbol>;
  attributes: Map<string, Attribute[]>;
}

// Feature File
export interface FeatureFile {
  units: UnitMode;
  layer: Layer;
}
```

### Python Parsing Algorithm

```python
from dataclasses import dataclass
from enum import Enum

class FeatureType(Enum):
    LINE = 'L'
    PAD = 'P'
    ARC = 'A'
    TEXT = 'T'
    BARCODE = 'B'
    SURFACE = 'S'

@dataclass
class FeatureFile:
    units: str
    layer_id: str
    features: list
    symbols: dict
    attributes: dict

def parse_feature_file(filepath: str) -> FeatureFile:
    with open(filepath, 'r') as f:
        lines = f.readlines()
    
    result = FeatureFile(units='M', layer_id='', features=[], 
                         symbols={}, attributes={})
    
    i = 0
    while i < len(lines):
        line = lines[i].strip()
        
        # Skip comments and empty lines
        if not line or line.startswith('#'):
            i += 1
            continue
        
        # Parse directives
        if line.startswith('UNITS='):
            result.units = line.split('=')[1]
        
        elif line.startswith('ID='):
            result.layer_id = line.split('=')[1]
        
        # Parse features
        elif line.startswith('F '):
            result.features.append(parse_feature(line))
        
        elif line.startswith('L '):
            result.features.append(parse_line(line))
        
        elif line.startswith('P '):
            result.features.append(parse_pad(line))
        
        elif line.startswith('A '):
            result.features.append(parse_arc(line))
        
        elif line.startswith('T '):
            result.features.append(parse_text(line))
        
        elif line.startswith('S '):
            result.features.append(parse_surface(lines, i))
        
        # Parse symbol definitions
        elif line.startswith('$'):
            symbol_name = line[1:]
            symbol_def, i = parse_symbol_definition(lines, i + 1)
            result.symbols[symbol_name] = symbol_def
            continue
        
        # Parse attribute tables
        elif line.startswith('@') or line.startswith('&'):
            table_name = line[1:]
            attrs, i = parse_attribute_table(lines, i + 1)
            result.attributes[table_name] = attrs
            continue
        
        i += 1
    
    return result

def parse_pad(line: str) -> dict:
    parts = line.split()
    return {
        'type': 'P',
        'x': int(parts[1]),
        'y': int(parts[2]),
        'symbol': parts[3],
        'polarity': parts[4],
        'dcode': int(parts[5]),
        'orient': int(parts[6]),
        'attrib': parts[7] if len(parts) > 7 else None
    }

def parse_line(line: str) -> dict:
    parts = line.split()
    return {
        'type': 'L',
        'x_start': int(parts[1]),
        'y_start': int(parts[2]),
        'x_end': int(parts[3]),
        'y_end': int(parts[4]),
        'width': int(parts[5]),
        'polarity': parts[6],
        'attrib': parts[7] if len(parts) > 7 else None
    }
```

### Symbol Name Parsing

```typescript
export interface SymbolParams {
  shape: string;        // 'c', 'r', 's', 'donut_r', 'thermal', etc.
  dimensions: number[]; // Parsed dimension values in microns
  angle?: number;       // For modes 8-9
}

export function parseSymbolName(name: string): SymbolParams {
  // Handle arbitrary angle notation
  const [baseName, angleStr] = name.split('@');
  const angle = angleStr ? parseFloat(angleStr) : undefined;
  
  // Extract shape prefix
  const shapeMatch = baseName.match(/^([a-z_]+)/);
  const shape = shapeMatch ? shapeMatch[1] : '';
  
  // Extract dimensions (format: <shape><dim1>x<dim2>x<dim3>...)
  const dimMatch = baseName.match(/(\d+)/g);
  const dimensions = dimMatch 
    ? dimMatch.map(d => parseInt(d) * 0.1)  // Convert to microns
    : [];
  
  return { shape, dimensions, angle };
}

// Example usage:
parseSymbolName('r3000x1500')
// → { shape: 'r', dimensions: [300, 150], angle: undefined }

parseSymbolName('thermal5000x800x1000x4x45')
// → { shape: 'thermal', dimensions: [500, 80, 100, 4, 45] }

parseSymbolName('donut_r2000x1200@37.5')
// → { shape: 'donut_r', dimensions: [200, 120], angle: 37.5 }
```

---

## Implementation Considerations

### Performance Optimization

**Progressive Loading**
- Parse layer metadata first (UNITS, ID)
- Load features on-demand as viewport changes
- Use spatial indexing (quadtree/R-tree) for large layers

**Caching Strategy**
```typescript
class SymbolCache {
  private cache: Map<string, RenderedSymbol> = new Map();
  
  getSymbol(name: string, orient: number): RenderedSymbol {
    const cacheKey = `${name}_${orient}`;
    
    if (!this.cache.has(cacheKey)) {
      const symbol = this.resolveSymbol(name);
      const rendered = this.renderSymbol(symbol, orient);
      this.cache.set(cacheKey, rendered);
    }
    
    return this.cache.get(cacheKey)!;
  }
}
```

**Batch Rendering**
- Group features by symbol type and polarity
- Use instanced rendering for repeated symbols
- Combine lines/arcs into single path when possible

### Memory Management

**Large File Handling**
- Stream parse instead of loading entire file
- Paginate feature records (e.g., 10,000 per chunk)
- Use typed arrays for coordinate storage

**Geometry Simplification**
- Cull features outside viewport + margin
- LOD (level-of-detail) for complex symbols at low zoom
- Tessellation granularity based on screen size

### Error Handling

**Robust Parsing**
```python
def safe_parse_feature(line: str) -> Optional[Feature]:
    try:
        return parse_feature(line)
    except (IndexError, ValueError) as e:
        logger.warning(f"Failed to parse feature: {line[:50]}, error: {e}")
        return None
```

**Fallback Strategies**
- Unknown symbols → render as simple bounding box
- Invalid coordinates → skip feature, log warning
- Missing attribute → use default values

### Testing Strategy

**Unit Tests**
- Symbol name parsing edge cases
- Transformation matrix calculations
- Polarity composition
- Coordinate conversion

**Integration Tests**
- Parse real ODB++ files from test corpus
- Validate feature counts against reference
- Compare rendered output to golden images

**Performance Tests**
- Benchmark parse speed (target: >100k features/sec)
- Memory profiling for large files (>1M features)
- Render framerate at various zoom levels

---

## Rendering Guidelines

### Layer Composition

**Z-Order (bottom to top):**
1. Board outline (substrate)
2. Internal copper layers (layer 2...n-1)
3. Bottom copper (layer n)
4. Bottom soldermask
5. Bottom silkscreen
6. Top copper (layer 1)
7. Top soldermask
8. Top silkscreen

**Blending Modes:**
- Positive features: Additive (normal blending)
- Negative features: Subtractive (use stencil buffer or multiply blend)

### Color Schemes

**Default PCB Colors:**
```typescript
export const LayerColors = {
  COPPER: '#C87533',           // Copper orange
  SOLDERMASK_GREEN: '#1B5E20', // Dark green (translucent)
  SOLDERMASK_RED: '#B71C1C',
  SOLDERMASK_BLUE: '#0D47A1',
  SILKSCREEN: '#FFFFFF',       // White
  SUBSTRATE: '#D4C5A9',        // FR4 tan
  HOLE: '#000000',             // Black (drilled holes)
  PASTE: '#C0C0C0',            // Silver (solder paste)
};
```

**Opacity Recommendations:**
- Copper: 100% opaque
- Soldermask: 60-80% transparent (show underlying copper)
- Silkscreen: 100% opaque
- Negative features: Use stencil cutout (don't render with alpha)

### Anti-Aliasing

**Canvas 2D:**
```typescript
ctx.imageSmoothingEnabled = true;
ctx.imageSmoothingQuality = 'high';
ctx.lineWidth = width;
ctx.lineCap = 'round';  // Critical for line features
```

**WebGL:**
- Use MSAA (multisample anti-aliasing) framebuffer
- Shader-based FXAA for post-processing
- Distance field rendering for text and fine features

### Viewport and Scaling

**Coordinate Mapping:**
```typescript
function worldToScreen(worldX: number, worldY: number, 
                       zoom: number, panX: number, panY: number): Point {
  return {
    x: (worldX - panX) * zoom + canvasWidth / 2,
    y: (worldY - panY) * zoom + canvasHeight / 2
  };
}
```

**Zoom Levels:**
- Board overview: 1x-10x
- Component detail: 10x-100x
- Trace inspection: 100x-1000x

**Culling:**
```typescript
function isFeatureVisible(feature: Feature, viewport: Rect): boolean {
  const bounds = getFeatureBounds(feature);
  return bounds.intersects(viewport);
}
```

### Text Rendering

**Approaches:**

1. **Vector Paths** (best quality)
   - Convert font glyphs to SVG paths
   - Cache rendered paths per character
   - Scales perfectly at all zoom levels

2. **Texture Atlas** (best performance)
   - Prerender common characters to texture
   - Use sprite sheet for rendering
   - Trade memory for speed

3. **Hybrid** (recommended)
   - Texture atlas for UI text and labels
   - Vector paths for rotated/scaled feature text

---

## Appendix: Complete Symbol Reference

### Quick Lookup Table

| Symbol Family | Syntax Example | Parameters | Common Uses |
|--------------|----------------|------------|-------------|
| **Basic Shapes** |
| Circle | `c1000` | diameter | Vias, round pads |
| Rectangle | `r3000x1500` | width × height | SMD pads |
| Square | `s2000` | size | Square pads |
| Oval | `oval5000x3000` | width × height | Elongated pads |
| Oblong | `oblong6000x2000` | width × height | Pill-shaped pads |
| Diamond | `di2000` | size | Fiducial marks |
| Octagon | `oct5000x5000x1000` | width × height × corner | Thermal pads |
| Triangle | `tri3000x2000` | base × height | Directional marks |
| Ellipse | `el6000x4000` | width × height | True ellipse |
| **Rounded Shapes** |
| Rounded Rect | `rnd_r4000x2000x500` | width × height × radius | Modern SMD pads |
| Chamfered Rect | `ch_r4000x3000x500` | width × height × chamfer | BGA pads |
| **Donuts (Annular Rings)** |
| Round Donut | `donut_r2000x1000` | outer_dia × inner_dia | Via pads |
| Square Donut | `donut_s3000x1500` | outer_size × hole_dia | Square via pads |
| Rect Donut | `donut_rc4000x2000x1200` | width × height × hole | Slot pads |
| Oval Donut | `donut_oval5000x3000x1500` | width × height × hole | Elongated via pads |
| **Thermal Reliefs** |
| Round Thermal | `thermal3000x400x600x4x45` | dia × gap × spoke × num × angle | Power plane conn |
| Square Thermal | `thermal_s4000x500x800x4x0` | size × gap × spoke × num × angle | Ground plane conn |
| Rect Thermal | `thermal_r5000x3000x400x600x4x0` | w × h × gap × spoke × num × angle | Power pads |
| **Specialized** |
| Half Circle | `half_circle2000x0` | diameter × orient | Edge pads |
| Home Plate | `home5000x3000x1000` | width × height × point | Alignment marks |
| Reduced Rect | `rect_reduced3000x2000x200` | width × height × reduction | Stencil apertures |

### Dimension Conversion Reference

**ODB++ stores dimensions in 1/10 microns:**

| Symbol Value | Microns (µm) | Millimeters (mm) | Inches |
|--------------|--------------|------------------|---------|
| 100 | 10 | 0.01 | 0.000394 |
| 500 | 50 | 0.05 | 0.00197 |
| 1000 | 100 | 0.10 | 0.00394 |
| 2000 | 200 | 0.20 | 0.00787 |
| 5000 | 500 | 0.50 | 0.0197 |
| 10000 | 1000 | 1.00 | 0.0394 |
| 25400 | 2540 | 2.54 | 0.10 |

**Conversion Formulas:**
```typescript
const TENTHS_MICRON_TO_MM = 0.0001;
const TENTHS_MICRON_TO_INCH = 0.000003937;

function toMillimeters(value: number): number {
  return value * TENTHS_MICRON_TO_MM;
}

function toInches(value: number): number {
  return value * TENTHS_MICRON_TO_INCH;
}
```

### Symbol Primitive Reference

**Primitive Codes (used in $ definitions):**

| Code | Primitive | Parameters | Description |
|------|-----------|------------|-------------|
| C | Circle | x, y, diameter | Filled circle |
| R | Rectangle | x, y, width, height | Filled rectangle |
| P | Polygon | x1, y1, x2, y2, ... | Closed polygon |
| H | Hole | x, y, diameter | Circular cutout (negative) |
| A | Arc | x_start, y_start, x_end, y_end, x_center, y_center | Arc segment |
| L | Line | x_start, y_start, x_end, y_end, width | Stroked line |

**Example Custom Symbol Definition:**
```
$my_custom_pad
C 0 0 2000           # 200 µm circle at origin
R 1000 0 1000 500    # 100×50 µm rectangle at offset
```

---

## References and Resources

### Primary Sources


- **ODB++ Specification v8.1 Update 4** (August 2024)  
  The official specification document for ODB++ format  
  Location: `~/src/github/nam20485/OdbDesign/docs/odb_spec_user.pdf`

- **ODB++ Format by Mentor Graphics / Siemens**  
  Industry-standard PCB data exchange format

### Related Projects

- **OdbDesign Repository**  
  C# implementation of ODB++ parsing and manipulation  
  Location: `~/src/github/nam20485/OdbDesign`  
  [GitHub Repository](https://github.com/nam20485/OdbDesign)

- **shape-sdk**  
  Shape definition and geometry processing toolkit  
  Location: `~/src/github/nam20485/shape-sdk`  
  [GitHub Repository](https://github.com/nam20485/shape-sdk)

- **board-shape-view-client**  
  Web-based PCB viewer leveraging ODB++ and shape-sdk  
  Current project implementation

### Related Documentation

- [`API.md`](./API.md) - API endpoints and integration guide
- [`CONTRIBUTING.md`](./CONTRIBUTING.md) - Development guidelines
- [`DEPLOYMENT.md`](./DEPLOYMENT.md) - Deployment procedures
- [`../README.md`](../README.md) - Project overview

### External Resources

- **IPC Standards** - PCB design and manufacturing standards
- **Gerber X2/X3** - Alternative PCB exchange formats
- **Canvas API** - HTML5 2D rendering context for web visualization
- **Three.js** - 3D rendering library (for future 3D visualization)

---

## Document Revision History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 1.0 | 2025-01-21 | Research Agent | Initial comprehensive documentation of ODB++ shape definitions, feature types, coordinate systems, and implementation guidelines |

---

**Last Updated:** January 21, 2025  
**Document Status:** Complete  
**Target Audience:** Developers implementing ODB++ parsers and PCB visualization tools
