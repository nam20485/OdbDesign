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
