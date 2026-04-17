# OdbDesign Architecture Documentation

## Document Purpose
This document describes the current OdbDesignServer architecture and the planned evolution to support dual REST + gRPC APIs for high-performance ODB++ data access.

**Last Updated**: 2025-11-09  
**Status**: Current Architecture + gRPC Migration Design

---

## Table of Contents
1. [Current Architecture](#current-architecture)
2. [REST API Structure](#rest-api-structure)
3. [Data Model & Serialization](#data-model--serialization)
4. [gRPC Migration Architecture](#grpc-migration-architecture)
5. [API Comparison](#api-comparison)
6. [Integration Points](#integration-points)
7. [Deployment Architecture](#deployment-architecture)
8. [Security Architecture](#security-architecture)

---

## Current Architecture

### High-Level System Overview

```
┌─────────────────────────────────────────────────────────┐
│                    Client Applications                   │
│         (Web, Python, JavaScript, Command-line)          │
└────────────────────┬────────────────────────────────────┘
                     │ HTTP/1.1 (JSON)
                     ▼
┌─────────────────────────────────────────────────────────┐
│              OdbDesignServer (Port 8888)                 │
│  ┌───────────────────────────────────────────────────┐  │
│  │         Crow Web Framework (REST API)             │  │
│  │  ┌─────────────────────────────────────────────┐  │  │
│  │  │ Controllers:                                │  │  │
│  │  │  • FileUploadController    (POST /upload)   │  │  │
│  │  │  • DesignsController       (GET /designs)   │  │  │
│  │  │  • FileModelController     (GET /filemodel) │  │  │
│  │  │  • HealthCheckController   (GET /health)    │  │  │
│  │  └─────────────────────────────────────────────┘  │  │
│  │                        │                           │  │
│  │                        ▼                           │  │
│  │  ┌─────────────────────────────────────────────┐  │  │
│  │  │   Middleware: BasicAuth, CORS, Logging     │  │  │
│  │  └─────────────────────────────────────────────┘  │  │
│  └─────────────────┬───────────────────────────────────┘
│                    │                                     │
│                    ▼                                     │
│  ┌─────────────────────────────────────────────────┐   │
│  │         OdbServerApp (Application Layer)        │   │
│  │  • Design Cache Management                      │   │
│  │  • Request Routing & Dispatch                   │   │
│  │  • Business Logic Orchestration                 │   │
│  └─────────────────┬───────────────────────────────┘   │
│                    │                                     │
└────────────────────┼─────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────┐
│              OdbDesignLib (Core Library)                 │
│  ┌─────────────────────────────────────────────────┐   │
│  │              ProductModel Layer                  │   │
│  │  • Design       • Net         • Component        │   │
│  │  • Package      • Part        • Layer           │   │
│  │  • Step         • Symbol      • Feature         │   │
│  └─────────────────┬───────────────────────────────┘   │
│                    │                                     │
│  ┌─────────────────▼───────────────────────────────┐   │
│  │              FileModel Layer                     │   │
│  │  • FileArchive  • EdaDataFile  • NetlistFile    │   │
│  │  • FeaturesFile • ComponentsFile • AttributeList│   │
│  └─────────────────┬───────────────────────────────┘   │
│                    │                                     │
│  ┌─────────────────▼───────────────────────────────┐   │
│  │          Protobuf Serialization Layer            │   │
│  │  • 26 .proto definitions (design.proto, etc.)    │   │
│  │  • IProtoBuffable<T> interface                   │   │
│  │  • C++ ↔ Protobuf conversion methods            │   │
│  └─────────────────┬───────────────────────────────┘   │
│                    │                                     │
└────────────────────┼─────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────┐
│                 Utils Layer (Shared)                     │
│  • FileReader        • ArchiveExtractor                  │
│  • Logger            • StopWatch (performance timing)    │
│  • CommandLineArgs   • StringUtils                       │
└─────────────────────────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────┐
│              External Dependencies                       │
│  • libarchive (ODB++ .tgz extraction)                    │
│  • protobuf (serialization runtime)                      │
│  • openssl (TLS/auth)                                    │
└─────────────────────────────────────────────────────────┘
```

### Key Components

#### 1. OdbDesignServer (Application Entry Point)
- **Location**: `OdbDesignServer/`
- **Responsibilities**:
  - Initialize Crow web server
  - Register route controllers
  - Manage server lifecycle
  - Handle graceful shutdown
- **Key Files**:
  - `main.cpp`: Entry point, server initialization
  - `OdbDesignServerApp.cpp`: Application orchestration

#### 2. Controllers (API Layer)
- **Pattern**: MVC-style route handlers
- **Base Class**: `RouteController` (abstract)
- **Implementations**:
  - **FileUploadController**: 
    - `POST /upload` - Multipart form data + octet-stream
    - Saves ODB++ archives to temp directory
    - Returns design name for subsequent queries
  - **DesignsController**:
    - `GET /designs` - List all loaded designs
    - `GET /designs/{name}` - Get design details
    - `GET /designs/{name}/components` - Components list
    - `GET /designs/{name}/components/{refDes}` - Single component
    - `GET /designs/{name}/nets` - Nets list
    - `GET /designs/{name}/packages` - Packages list
    - `GET /designs/{name}/parts` - Parts list
  - **FileModelController**:
    - `GET /filemodels` - List file models
    - `GET /filemodels/{name}` - Get file model
    - `POST /filemodels/{name}` - Load ODB++ from disk
    - `GET /filemodels/{name}/steps` - Steps list
    - `GET /filemodels/{name}/steps/{step}` - Step details
    - `GET /filemodels/{name}/steps/{step}/layers` - Layers list
    - `GET /filemodels/{name}/steps/{step}/layers/{layer}` - Layer details
    - `GET /filemodels/{name}/steps/{step}/layers/{layer}/features` - **PERFORMANCE CRITICAL**
    - `GET /filemodels/{name}/symbols` - Symbols list
    - `GET /filemodels/{name}/symbols/{symbol}/features` - Symbol features
  - **HealthCheckController**:
    - `GET /health` - Liveness probe

#### 3. Design Cache (State Management)
- **Pattern**: Thread-safe singleton
- **Responsibilities**:
  - In-memory storage of loaded ODB++ designs
  - Design lifecycle management (load, retrieve, unload)
  - Prevents redundant file I/O
- **Data Structure**: `std::map<std::string, std::shared_ptr<Design>>`
- **Thread Safety**: Mutex-protected

#### 4. OdbDesignLib (Core Business Logic)
- **Location**: `OdbDesignLib/`
- **Layer Architecture**:
  1. **ProductModel**: High-level design entities (Design, Net, Component, etc.)
  2. **FileModel**: ODB++ file parsers (maps to ODB++ spec structure)
  3. **Protobuf**: Serialization layer (26 proto files)
- **Key Abstraction**: `IProtoBuffable<T>` - Enables C++ objects to serialize to Protobuf

---

## REST API Structure

### Endpoint Taxonomy

```
/upload                         → FileUploadController
/health                         → HealthCheckController

/designs                        → DesignsController
/designs/{name}
/designs/{name}/components
/designs/{name}/components/{refDes}
/designs/{name}/nets
/designs/{name}/nets/{name}
/designs/{name}/packages
/designs/{name}/packages/{name}
/designs/{name}/parts
/designs/{name}/parts/{name}

/filemodels                     → FileModelController
/filemodels/{name}
/filemodels/{name}/steps
/filemodels/{name}/steps/{step}
/filemodels/{name}/steps/{step}/edadata
/filemodels/{name}/steps/{step}/attrlist
/filemodels/{name}/steps/{step}/netlists
/filemodels/{name}/steps/{step}/layers
/filemodels/{name}/steps/{step}/layers/{layer}
/filemodels/{name}/steps/{step}/layers/{layer}/features   ← Bottleneck
/filemodels/{name}/steps/{step}/layers/{layer}/components
/filemodels/{name}/steps/{step}/layers/{layer}/attrlist
/filemodels/{name}/symbols
/filemodels/{name}/symbols/{symbol}
/filemodels/{name}/symbols/{symbol}/features
```

### Request/Response Flow

```
Client Request
    │
    ▼
Crow HTTP Parser
    │
    ▼
BasicAuth Middleware  (validate credentials)
    │
    ▼
Route Matcher  (find controller method)
    │
    ▼
Controller Handler  (e.g., steps_layers_features_route_handler)
    │
    ▼
OdbServerApp.getDesignCache()  (retrieve cached design)
    │
    ▼
C++ ProductModel Object  (e.g., Layer::getFeatures())
    │
    ▼
IProtoBuffable<T>::to_protobuf()  (C++ → Protobuf)
    │
    ▼
google::protobuf::util::MessageToJsonString()  ← BOTTLENECK
    │
    ▼
crow::response(json_string)
    │
    ▼
Client receives JSON
```

### Performance Bottleneck Analysis

**Problem**: The `MessageToJsonString()` call is CPU-intensive for large messages.

**Example - Layer Features Endpoint**:
- **Input**: 100,000 feature records (pads, lines, arcs)
- **Protobuf Size**: ~5MB binary
- **JSON Size**: ~15MB text (3x expansion)
- **Serialization Time**: 300-500ms
- **Memory Copies**: 3 (C++ → Protobuf → JSON → HTTP buffer)

---

## Data Model & Serialization

### Protobuf Schema Organization

```
OdbDesignLib/protoc/
├── design.proto              → Top-level design
├── filearchive.proto         → File model root
├── component.proto           → Component entities
├── net.proto                 → Net entities
├── package.proto             → Package entities
├── part.proto                → Part entities
├── stepdirectory.proto       → Step hierarchy
├── layerdirectory.proto      → Layer hierarchy
├── featuresfile.proto        → Feature records (LARGE)
├── symbolname.proto          → Symbol definitions
├── symbolsdirectory.proto    → Symbol hierarchy
├── netlistfile.proto         → Netlist data
├── edadatafile.proto         → EDA metadata
├── attrlistfile.proto        → Attributes
├── common.proto              → Shared types (Point, Polygon, etc.)
└── enums.proto               → Shared enums (Polarity, etc.)
```

### Key Data Structures

#### Design (Top-Level)
```protobuf
message Design {
    optional string productModel = 1;
    optional string name = 2;
    optional FileArchive fileModel = 3;
    repeated Net nets = 4;
    repeated Component components = 8;
    repeated Package packages = 6;
    repeated Part parts = 10;
}
```

#### FeaturesFile (Performance-Critical)
```protobuf
message FeaturesFile {
    message FeatureRecord {
        enum Type { Arc, Pad, Surface, Barcode, Text, Line }
        optional Type type = 2;
        optional double xs = 3;  // Start X
        optional double ys = 4;  // Start Y
        optional double xe = 5;  // End X
        optional double ye = 6;  // End Y
        // ... 25+ fields per feature
        repeated ContourPolygon contourPolygons = 1;
        map<string, string> attributeLookupTable = 29;
    }
    
    repeated FeatureRecord featureRecords = 8;  // Can be 100K+ records
    repeated SymbolName symbolNames = 10;
}
```

**Why This Is Problematic for JSON**:
- Each feature has 20-30 fields (many optional)
- Complex nested structures (polygons, attributes)
- Large arrays (100K+ features typical for PCB layers)
- JSON text format adds 200-300% size overhead

---

## gRPC Migration Architecture

### Augmentation Strategy (Not Replacement)

```
┌──────────────────────────────────────────────────────────────┐
│              OdbDesignServer (Dual Protocol)                  │
│                                                                │
│  ┌──────────────────────┐    ┌──────────────────────────┐   │
│  │   Crow REST API      │    │    gRPC Server           │   │
│  │   Port: 8888         │    │    Port: 50051           │   │
│  │   Protocol: HTTP/1.1 │    │    Protocol: HTTP/2      │   │
│  │   Format: JSON       │    │    Format: Protobuf      │   │
│  └──────────┬───────────┘    └───────────┬──────────────┘   │
│             │                             │                   │
│             └─────────────┬───────────────┘                   │
│                           │                                   │
│                           ▼                                   │
│              ┌────────────────────────┐                       │
│              │   Shared DesignCache   │                       │
│              │   (Thread-Safe)        │                       │
│              └────────────────────────┘                       │
│                           │                                   │
└───────────────────────────┼───────────────────────────────────┘
                            │
                            ▼
                   OdbDesignLib (unchanged)
```

### gRPC Service Definitions (New)

**Location**: `OdbDesignLib/protoc/odb_api_v1.proto` (to be created)

```protobuf
syntax = "proto3";

package odb.api.v1;

import "design.proto";
import "featuresfile.proto";
import "layerdirectory.proto";
// ... other imports

service OdbDesignService {
  // Design-level operations
  rpc ListDesigns(ListDesignsRequest) returns (ListDesignsResponse);
  rpc GetDesign(GetDesignRequest) returns (Design);  // Unary
  rpc LoadDesignFromPath(LoadDesignRequest) returns (LoadDesignResponse);
  
  // Component operations
  rpc ListComponents(ListComponentsRequest) returns (ListComponentsResponse);
  rpc GetComponent(GetComponentRequest) returns (Component);
  
  // Net operations
  rpc ListNets(ListNetsRequest) returns (ListNetsResponse);
  rpc GetNet(GetNetRequest) returns (Net);
  
  // Layer operations (performance-critical)
  rpc ListLayers(ListLayersRequest) returns (ListLayersResponse);
  rpc GetLayer(GetLayerRequest) returns (Layer);
  rpc GetLayerFeaturesStream(GetLayerFeaturesRequest) 
      returns (stream FeatureChunk);  // Server-streaming ← KEY BENEFIT
  
  // Symbol operations
  rpc ListSymbols(ListSymbolsRequest) returns (ListSymbolsResponse);
  rpc GetSymbolFeaturesStream(GetSymbolFeaturesRequest) 
      returns (stream FeatureChunk);  // Server-streaming
  
  // Health check
  rpc HealthCheck(HealthCheckRequest) returns (HealthCheckResponse);
}

// Streaming chunk for large datasets
message FeatureChunk {
  repeated FeaturesFile.FeatureRecord features = 1;
  uint32 chunk_index = 2;
  uint32 total_chunks = 3;
  bool is_final = 4;
}
```

### Performance Comparison: REST vs gRPC

#### Scenario: Get Layer Features (100K features)

**REST API (Current)**:
```
Client → HTTP GET /filemodels/design1/steps/pcb/layers/top/features
         ↓
Server:  C++ Layer object → Protobuf (5MB) → JSON (15MB) → HTTP response
         ↓ (300-500ms serialization)
Client ← JSON 15MB (single response)
```

**gRPC API (New - Streaming)**:
```
Client → gRPC GetLayerFeaturesStream(design1, pcb, top)
         ↓
Server:  C++ Layer object → Protobuf chunks (20 x 250KB) → HTTP/2 stream
         ↓ (50-100ms total, streamed incrementally)
Client ← Protobuf 5MB (20 streamed chunks, progressive rendering)
```

**Benefits**:
- ✅ **5x faster**: 50-100ms vs 300-500ms
- ✅ **60% smaller**: 5MB vs 15MB
- ✅ **Streaming**: Client renders data as it arrives
- ✅ **Memory efficient**: Server sends chunks, doesn't buffer entire response

---

## API Comparison

### REST vs gRPC Feature Parity Matrix

| Feature                    | REST API (v0.9) | gRPC API (v1.0) | Notes                          |
|----------------------------|-----------------|-----------------|--------------------------------|
| List Designs               | ✅ GET /designs | ✅ ListDesigns  | Identical data                 |
| Get Design Details         | ✅ GET /designs/{name} | ✅ GetDesign | Includes fileModel option      |
| Upload Design              | ✅ POST /upload | ⏳ Phase 2      | gRPC client-streaming later    |
| List Components            | ✅ GET /designs/{name}/components | ✅ ListComponents | |
| Get Component              | ✅ GET /designs/{name}/components/{id} | ✅ GetComponent | |
| List Nets                  | ✅ GET /designs/{name}/nets | ✅ ListNets | |
| Get Net                    | ✅ GET /designs/{name}/nets/{name} | ✅ GetNet | |
| List Layers                | ✅ GET /filemodels/{name}/steps/{step}/layers | ✅ ListLayers | |
| Get Layer Details          | ✅ GET .../layers/{layer} | ✅ GetLayer | |
| Get Layer Features         | ✅ GET .../layers/{layer}/features | ✅ GetLayerFeaturesStream | **gRPC uses streaming** |
| List Symbols               | ✅ GET /filemodels/{name}/symbols | ✅ ListSymbols | |
| Get Symbol Features        | ✅ GET .../symbols/{symbol}/features | ✅ GetSymbolFeaturesStream | **gRPC uses streaming** |
| Health Check               | ✅ GET /health | ✅ HealthCheck  | |
| Authentication             | ✅ BasicAuth    | ✅ gRPC Interceptors | Shared auth logic |

### Migration Path for Clients

**Phase 1 (Discovery)**: Both APIs coexist, clients choose:
- **Use REST if**: Simple queries, existing integrations, low performance needs
- **Use gRPC if**: High-performance queries, large datasets, modern clients

**Phase 2+ (Optional)**: New features may be gRPC-first (e.g., bidirectional streaming)

---

## Integration Points

### Shared Components (No Duplication)

```
┌─────────────────────────────────────────────────┐
│           Shared Infrastructure                  │
├─────────────────────────────────────────────────┤
│  • DesignCache (thread-safe singleton)          │
│  • Authentication (BasicAuth + gRPC interceptor)│
│  • Logging (Logger utility)                     │
│  • OdbDesignLib (all business logic)            │
│  • Protobuf models (reused directly in gRPC)    │
└─────────────────────────────────────────────────┘
```

### Authentication Refactoring

**Current (REST-only)**:
```cpp
// OdbDesignLib/App/BasicAuth.h
namespace CrowBasicAuth {
    bool validate(const crow::request& req);
}
```

**Refactored (Shared)**:
```cpp
// OdbDesignLib/App/Auth/AuthValidator.h (new)
namespace Odb::Lib::App::Auth {
    class AuthValidator {
    public:
        bool validateCredentials(const std::string& username, 
                                 const std::string& password);
    };
}

// OdbDesignServer/Middleware/CrowAuthMiddleware.h
class CrowAuthMiddleware {
    AuthValidator validator;
    // Wraps Crow-specific request validation
};

// OdbDesignServer/Interceptors/GrpcAuthInterceptor.h
class GrpcAuthInterceptor : public grpc::ServerInterceptor {
    AuthValidator validator;
    // Extracts metadata from gRPC context
};
```

### Thread Safety Considerations

**DesignCache**: Already thread-safe (mutex-protected)
- REST and gRPC can access concurrently
- Read-heavy workload (no locking contention expected)

**Protobuf Serialization**: Thread-safe (each request gets its own message instance)

---

## Deployment Architecture

### Container Configuration

**Dockerfile Changes**:
```dockerfile
# Existing
EXPOSE 8888

# New (dual-port)
EXPOSE 8888 50051

# gRPC health check
HEALTHCHECK CMD grpc_health_probe -addr=:50051 || exit 1
```

### Kubernetes Service

**Current**:
```yaml
apiVersion: v1
kind: Service
metadata:
  name: odbdesign-server
spec:
  ports:
  - name: http
    port: 8888
    targetPort: 8888
```

**Post-Migration**:
```yaml
apiVersion: v1
kind: Service
metadata:
  name: odbdesign-server
spec:
  ports:
  - name: http
    port: 8888
    targetPort: 8888
    protocol: TCP
  - name: grpc
    port: 50051
    targetPort: 50051
    protocol: TCP
```

### Ingress Configuration

**gRPC Ingress (Nginx)**:
```yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: odbdesign-grpc-ingress
  annotations:
    nginx.ingress.kubernetes.io/backend-protocol: "GRPC"
    nginx.ingress.kubernetes.io/grpc-backend: "true"
spec:
  rules:
  - host: grpc.odbdesign.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: odbdesign-server
            port:
              number: 50051
```

---

## Security Architecture

### TLS/SSL Configuration

**REST (Current)**: Optional TLS termination at load balancer

**gRPC (Required)**: TLS strongly recommended (gRPC over plaintext for dev only)

**Server Configuration**:
```cpp
grpc::SslServerCredentialsOptions ssl_opts;
ssl_opts.pem_root_certs = LoadFile("ca.pem");
ssl_opts.pem_key_cert_pairs.push_back({
    LoadFile("server-key.pem"),
    LoadFile("server-cert.pem")
});
auto creds = grpc::SslServerCredentials(ssl_opts);
builder.AddListeningPort("0.0.0.0:50051", creds);
```

### Authentication Flow

**REST Flow**:
```
Client → HTTP Header: Authorization: Basic <base64>
      → CrowAuthMiddleware validates
      → Proceeds to controller
```

**gRPC Flow**:
```
Client → gRPC Metadata: authorization: Bearer <token>
      → GrpcAuthInterceptor validates
      → Proceeds to service method
```

**Shared Validation**: Both use `AuthValidator::validateCredentials()`

---

## Performance Expectations

### Benchmark Targets (Post-Migration)

| Metric                     | REST (Current) | gRPC (Target) | Improvement |
|----------------------------|----------------|---------------|-------------|
| Get Design (small)         | 50ms           | 20ms          | 2.5x        |
| Get Layer Features (100K)  | 500ms          | 100ms         | 5x          |
| Payload Size (features)    | 15MB           | 5MB           | 3x smaller  |
| Concurrent Requests (RPS)  | 100            | 500+          | 5x          |
| Memory per Request         | 30MB           | 10MB          | 3x less     |

### Load Testing Strategy
- **Tool**: ghz (gRPC benchmarking) + Apache Bench (REST baseline)
- **Scenarios**: 
  1. Simple queries (design list) - 1000 RPS
  2. Medium complexity (component list) - 500 RPS
  3. Large datasets (layer features) - 100 RPS concurrent
- **Success Criteria**: gRPC ≥3x faster than REST for large datasets

---

## Migration Phases

### Phase 1: Discovery & Foundation (Weeks 1-2)
- Create proto service definitions
- Add grpc dependency to vcpkg.json
- Build gRPC "hello world" endpoint

### Phase 2: Core RPC Implementation (Weeks 3-5)
- Implement 15+ unary RPCs (ListDesigns, GetDesign, etc.)
- Implement 2 server-streaming RPCs (features)
- Integrate with existing DesignCache

### Phase 3: Authentication & Testing (Weeks 6-7)
- Refactor auth into shared component
- Create gRPC auth interceptor
- Write integration tests (REST vs gRPC parity)

### Phase 4: Deployment & Observability (Week 8)
- Update Dockerfile, K8s configs
- Add metrics (gRPC request counts, latency)
- Documentation & client examples

### Phase 5: Rollout & Validation (Week 9-10)
- Canary deployment (internal users)
- Performance benchmarking
- Collect feedback & iterate

---

## Open Questions & Risks

### Technical Risks
1. **gRPC Learning Curve**: Team unfamiliar with gRPC patterns → Mitigate with training & examples
2. **Deployment Complexity**: Dual-port configuration in K8s → Test in dev environment first
3. **Client Adoption**: Will clients migrate to gRPC? → Provide SDKs in Python/JS

### Architectural Decisions Needed
1. **Streaming Chunk Size**: 5K features/chunk? 10K? → Performance test to determine
2. **Authentication Strategy**: Bearer tokens vs mTLS? → Start with tokens, add mTLS later
3. **Error Handling**: gRPC status codes mapping → Define in implementation phase

---

## Appendix: File Hierarchy

```
OdbDesignServer/
├── main.cpp                  → Entry point (initialize both Crow + gRPC)
├── OdbDesignServerApp.cpp    → Application orchestration
├── Controllers/              → REST controllers (unchanged)
│   ├── DesignsController.cpp
│   ├── FileModelController.cpp
│   └── ...
├── Services/                 → NEW: gRPC service implementations
│   ├── OdbDesignServiceImpl.cpp
│   └── OdbDesignServiceImpl.h
├── Interceptors/             → NEW: gRPC auth interceptor
│   └── GrpcAuthInterceptor.h
└── CMakeLists.txt            → Add gRPC link targets

OdbDesignLib/
├── protoc/
│   ├── odb_api_v1.proto      → NEW: gRPC service definitions
│   ├── design.proto          → Existing (reused)
│   └── ...
└── App/
    └── Auth/                 → NEW: Shared auth logic
        └── AuthValidator.h
```

---

## References

- **Current REST API Spec**: `swagger/odbdesign-server-0.9-swagger.yaml`
- **gRPC C++ Tutorial**: https://grpc.io/docs/languages/cpp/basics/
- **Streaming Best Practices**: https://grpc.io/docs/guides/performance/
- **ODB++ Spec**: `docs/odb_spec_user.pdf`

---

## Revision History

| Date       | Version | Changes                                | Author |
|------------|---------|----------------------------------------|--------|
| 2025-11-09 | 1.0     | Initial architecture + gRPC design doc | PM     |
