# OdbDesign Technology Stack

## Document Purpose
This document catalogs the current technology stack of the OdbDesign project and planned additions for the gRPC API migration initiative.

**Last Updated**: 2025-11-09  
**Status**: Current + gRPC Migration Planning

---

## Core Technologies

### Language & Build System
- **Language**: C++17
- **Build System**: CMake 3.16+
- **Package Manager**: vcpkg (manifest mode)
- **Compiler Support**: 
  - GCC 9+ (Linux)
  - MSVC 2019+ (Windows)
  - Clang 10+ (macOS)

### Current Production Dependencies

#### REST API Framework
- **crow** (v1.2.0+): Lightweight C++ web framework
  - HTTP/1.1 REST API server
  - JSON request/response handling
  - Middleware support for authentication
  - Port: 8888 (default)

#### Serialization
- **protobuf** (v5.29.2): Protocol Buffers
  - Data model serialization
  - Current usage: C++ models → Protobuf → JSON (REST API)
  - Proto definitions in `OdbDesignLib/protoc/`
  - 26+ proto files defining ODB++ data structures

#### Compression & Archive
- **libarchive** (v3.7.7): Multi-format archive support
  - ODB++ TGZ file extraction
  - Supports .tar.gz, .zip formats
- **zlib** (v1.3.1): Compression library

#### Security
- **openssl** (v3.x): Cryptographic library
  - TLS/SSL support
  - Basic authentication utilities
  - Certificate handling

#### Testing
- **GoogleTest** (v1.15.2): C++ testing framework
  - Unit tests
  - Integration tests
  - Performance benchmarks

---

## Planned Additions: gRPC Migration

### New Dependencies

#### gRPC Stack
- **grpc** (v1.62+): High-performance RPC framework
  - Will be added to `vcpkg.json`
  - Includes:
    - gRPC C++ core library
    - HTTP/2 transport
    - protoc-gen-grpc-cpp (code generator)
    - grpc++ (C++ runtime)

#### Build Integration
- CMake `find_package(gRPC CONFIG REQUIRED)`
- Proto compilation will use: `grpc_cpp_plugin`
- New CMake targets: `gRPC::grpc++` and `gRPC::grpc++_reflection`

### Architecture Changes

#### Dual-Server Architecture
```
Current:
  OdbDesignServer (Crow) → Port 8888 (REST/JSON)

Post-Migration:
  OdbDesignServer {
    Crow REST API   → Port 8888 (HTTP/1.1 + JSON)
    gRPC Server     → Port 50051 (HTTP/2 + Protobuf)
  }
  Both share: DesignCache (thread-safe singleton)
```

#### Data Flow Comparison
```
REST Path (Current):
  C++ Model → Protobuf → MessageToJsonString() → JSON → HTTP/1.1
  ❌ Bottleneck: Double serialization + large payloads

gRPC Path (New):
  C++ Model → Protobuf → HTTP/2 streaming
  ✅ Benefit: Direct binary transfer, ~60% size reduction, streaming support
```

---

## Development Tools

### Version Control
- **Git**: Distributed version control
- **GitHub**: Repository hosting, CI/CD, project management

### CI/CD Pipeline
- **GitHub Actions**: Automated workflows
  - Linux/Windows/macOS builds
  - Unit test execution
  - Code coverage (future)
  - Docker image builds

### Containerization
- **Docker**: Container runtime
  - Multi-stage builds
  - Alpine Linux base images
- **Kubernetes (K3s)**: Orchestration
  - Helm charts in `deploy/helm/`
  - Service manifests in `deploy/kube/`

### Code Quality
- **CodeQL**: Static analysis security scanning
- **clang-format**: Code formatting
- **cppcheck** (optional): Static analysis

---

## Deployment Stack

### Container Registry
- **GitHub Container Registry (ghcr.io)**
  - Image: `ghcr.io/nam20485/odbdesign-server:latest`

### Kubernetes Resources
- **Service Types**:
  - Current: ClusterIP (port 8888)
  - Future: ClusterIP with dual-port (8888 REST + 50051 gRPC)
- **Ingress**:
  - REST: Standard HTTP ingress
  - gRPC: Will require HTTP/2 annotations

### Environment
- **K3s**: Lightweight Kubernetes (dev/test)
- **Production**: Cloud-agnostic (AWS/Azure/GCP compatible)

---

## Data Formats

### ODB++ Specification
- **Input Format**: ODB++ v8.0+ (IPC-2581)
  - Compressed archives (.tgz, .tar.gz)
  - Directory structure with ASCII data files
- **Specification**: `docs/odb_spec_user.pdf`

### API Data Formats
- **REST API**: JSON (application/json)
- **gRPC API**: Protocol Buffers (application/grpc)

### Proto Packages
```protobuf
// Existing namespaces (26 proto files)
package Odb.Lib.Protobuf;
package Odb.Lib.Protobuf.ProductModel;

// New gRPC service definitions (to be created)
package odb.api.v1;
```

---

## Performance Characteristics

### REST API (Current)
- **Latency**: 100-500ms for large designs
- **Payload Size**: 5-50MB JSON (uncompressed)
- **Bottleneck**: JSON serialization via `google::protobuf::util::MessageToJsonString()`
- **Concurrency**: Crow thread pool (default: 16 threads)

### gRPC API (Target)
- **Latency Goal**: 20-100ms (5x improvement)
- **Payload Size**: 2-20MB Protobuf (~60% reduction)
- **Streaming**: Server-streaming for large datasets (e.g., layer features)
- **Concurrency**: gRPC async server (configurable thread pool)

---

## Security Model

### Authentication (Current - REST)
- HTTP Basic Auth
- Implemented in `OdbDesignLib/App/BasicAuth.h`
- Middleware: `Odb::Lib::App::BasicAuth::CrowBasicAuth`

### Authentication (Future - gRPC)
- Refactored shared authentication utility
- gRPC interceptors for auth token validation
- Mutual TLS (mTLS) support for production

### Authorization
- Design-level access control (planned)
- Role-based access control (RBAC) - future enhancement

---

## Documentation & Schema

### API Documentation
- **REST**: Swagger/OpenAPI 3.0 (`swagger/odbdesign-server-0.9-swagger.yaml`)
- **gRPC**: Will use protobuf comments + grpcurl/Postman

### Code Documentation
- **Doxygen**: C++ API documentation (`OdbDesignLib/doxygen.conf`)
- **Markdown**: Planning docs in `plan_docs/`

---

## Key Technical Decisions

### Why gRPC?
1. **Performance**: Direct Protobuf serialization eliminates JSON conversion overhead
2. **Streaming**: Server-streaming RPCs for large datasets (e.g., 100K+ layer features)
3. **Type Safety**: Strongly-typed contracts via proto definitions
4. **Ecosystem**: Broad client library support (Python, JavaScript, Go, Java, C#)
5. **HTTP/2**: Multiplexing, header compression, bidirectional streaming

### Why Dual-API (Not Replace)?
1. **Backward Compatibility**: Existing REST clients unaffected (0 breaking changes)
2. **Progressive Migration**: Clients adopt gRPC at their own pace
3. **Flexibility**: REST for simple queries, gRPC for high-performance use cases
4. **Risk Mitigation**: Gradual rollout with fallback to proven REST API

### Why Shared DesignCache?
1. **Memory Efficiency**: Single in-memory representation of designs
2. **Consistency**: Both APIs serve identical data
3. **Simplified Logic**: No data synchronization needed between APIs

---

## Migration Dependency Updates

### vcpkg.json Changes
```json
{
  "dependencies": [
    "crow",
    "protobuf",
    "libarchive",
    "zlib",
    "openssl",
    "grpc"  // ← NEW
  ]
}
```

### CMakeLists.txt Changes
```cmake
# Existing
find_package(Crow CONFIG REQUIRED)
find_package(Protobuf CONFIG REQUIRED)

# New additions
find_package(gRPC CONFIG REQUIRED)
target_link_libraries(OdbDesignServer 
  PRIVATE 
    gRPC::grpc++
    gRPC::grpc++_reflection
)
```

---

## Testing Strategy

### Current Test Coverage
- **GoogleTest suites**: 12 test files
- **Test types**: Unit, integration, performance, protobuf serialization
- **Test data**: `test_data/` directory with sample ODB++ files

### gRPC Testing Additions
1. **Unit Tests**: Proto message serialization
2. **Integration Tests**: gRPC service endpoint validation
3. **Performance Tests**: Latency and throughput benchmarks (REST vs gRPC)
4. **Load Tests**: Concurrent client simulation (grpc_bench)
5. **Compatibility Tests**: REST and gRPC serving identical data

---

## Future Enhancements

### Observability (Post-Migration)
- **Metrics**: Prometheus exporters for gRPC/REST metrics
- **Tracing**: OpenTelemetry instrumentation
- **Logging**: Structured logging (spdlog integration)

### Advanced gRPC Features (Phase 2+)
- **Client-streaming**: Batch design uploads
- **Bidirectional streaming**: Interactive design queries
- **gRPC-Web**: Browser-based clients via Envoy proxy
- **Load Balancing**: gRPC load balancing policies

---

## External References

- **Crow Documentation**: https://crowcpp.org/
- **gRPC C++ Quick Start**: https://grpc.io/docs/languages/cpp/quickstart/
- **Protobuf C++ Tutorial**: https://protobuf.dev/getting-started/cpptutorial/
- **vcpkg Package Registry**: https://vcpkg.io/en/packages.html
- **ODB++ Specification**: `docs/odb_spec_user.pdf`

---

## Revision History

| Date       | Version | Changes                                      | Author |
|------------|---------|----------------------------------------------|--------|
| 2025-11-09 | 1.0     | Initial tech stack documentation + gRPC plan | PM     |
