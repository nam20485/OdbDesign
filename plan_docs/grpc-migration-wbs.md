# OdbDesignServer gRPC Migration - Work Breakdown Structure

**Version:** 1.0  
**Date:** 2025-11-09  
**Project:** OdbDesignServer gRPC Migration  
**Total Estimated Duration:** 13-18 weeks

---

## Overview

This Work Breakdown Structure (WBS) provides a detailed breakdown of all work required to successfully migrate OdbDesignServer to a dual-mode architecture supporting both REST (Crow) and gRPC APIs. The migration follows an **augmentation strategy** - the new gRPC service will run in parallel with the existing REST API, ensuring 100% backward compatibility.

**Key Principles:**
- Augmentation, not replacement
- Shared DesignCache between both services
- Performance-first implementation
- API contract integrity via Git submodules
- Continuous performance regression tracking

---

## Phase 0: Discovery & Design (2-3 weeks)

### Epic 0.1: Technical Research & Baseline Establishment
**Duration:** 1 week  
**Team:** Backend Developer, Performance Engineer

#### Story 0.1.1: Performance Baseline Measurement
**Effort:** 2 days  
**Acceptance Criteria:**
- Benchmark current REST API performance for large dataset (>500k features)
- Measure Time-to-Last-Byte (TTLB) for `/designs/{name}/steps/{name}/layers/{name}/features`
- Record peak server CPU and memory usage during request
- Document total JSON payload size in megabytes
- Generate benchmark report with graphs and metrics

**Deliverables:**
- Baseline performance report (JSON)
- Performance graphs and charts
- Documented metrics for comparison

#### Story 0.1.2: gRPC Technology Spike
**Effort:** 3 days  
**Acceptance Criteria:**
- Research gRPC C++ best practices and patterns
- Investigate vcpkg gRPC package integration
- Study server-streaming optimization techniques
- Document authentication patterns for gRPC
- Create proof-of-concept "Hello World" gRPC service

**Deliverables:**
- Technical research document
- Hello World gRPC prototype
- Integration findings report

### Epic 0.2: API Contract Design
**Duration:** 1-2 weeks  
**Team:** Backend Developer, Product Manager, Client Team Lead

#### Story 0.2.1: Design gRPC Service Contract
**Effort:** 3 days  
**Acceptance Criteria:**
- Define `odb_api_v1.proto` service interface
- Specify unary RPCs: `GetDesign`, `GetAvailableDesigns`, `GetStepList`, `GetLayerList`, `GetSymbols`
- Specify streaming RPC: `GetLayerFeaturesStream`
- Define request/response message types
- Document API design rationale

**Deliverables:**
- `odb_api_v1.proto` draft specification
- API design document
- Service method documentation

#### Story 0.2.2: Client Team API Review
**Effort:** 2 days  
**Acceptance Criteria:**
- Present API contract to .NET client team
- Gather feedback on endpoint design
- Validate streaming approach meets client needs
- Agree on error handling conventions
- Finalize API contract

**Deliverables:**
- API review meeting notes
- Finalized `odb_api_v1.proto`
- Approved API contract document

### Epic 0.3: Protobuf Repository Setup
**Duration:** 3 days  
**Team:** Backend Developer, DevOps Engineer

#### Story 0.3.1: Create Shared Protobuf Repository
**Effort:** 1 day  
**Acceptance Criteria:**
- Create new Git repository: `odb-protobuf-definitions`
- Migrate all existing `.proto` files from `OdbDesignLib/protoc/`
- Add new `odb_api_v1.proto` service definition
- Create README with usage instructions
- Set up repository structure and documentation

**Deliverables:**
- New `odb-protobuf-definitions` repository
- Migrated proto files
- Repository documentation

#### Story 0.3.2: Configure Git Submodules
**Effort:** 2 days  
**Acceptance Criteria:**
- Add protobuf repo as submodule to OdbDesignServer at `proto/`
- Update all CMake protoc generation commands to use new submodule path
- Update CI/CD workflows to initialize submodules (`git submodule update --init --recursive`)
- Verify build succeeds with submodule structure
- Document submodule update procedures

**Deliverables:**
- Updated `.gitmodules` configuration
- Modified CMakeLists.txt files
- Updated CI/CD workflow files
- Submodule usage documentation

---

## Phase 1: Core Implementation (4-6 weeks)

### Epic 1.1: Build System Integration
**Duration:** 1 week  
**Team:** Backend Developer

#### Story 1.1.1: Add gRPC Dependencies
**Effort:** 1 day  
**Acceptance Criteria:**
- Add `grpc` to `vcpkg.json` dependencies
- Update root `CMakeLists.txt` with `find_package(gRPC CONFIG REQUIRED)`
- Verify vcpkg successfully installs gRPC and dependencies
- Document dependency versions

**Deliverables:**
- Updated `vcpkg.json`
- Modified root `CMakeLists.txt`
- Dependency verification report

#### Story 1.1.2: Configure Protobuf Code Generation
**Effort:** 2 days  
**Acceptance Criteria:**
- Locate gRPC C++ plugin (`grpc_cpp_plugin`)
- Update `OdbDesignLib/CMakeLists.txt` to generate gRPC stubs
- Add `--grpc_out` and `--plugin` directives to protoc command
- Verify generation of `.grpc.pb.h` and `.grpc.pb.cc` files
- Add generated files to build targets

**Deliverables:**
- Modified `OdbDesignLib/CMakeLists.txt`
- Generated gRPC stub files
- Build verification

#### Story 1.1.3: Link gRPC Libraries
**Effort:** 1 day  
**Acceptance Criteria:**
- Link `OdbDesignServer` against `gRPC::grpc++`, `gRPC::grpc++_reflection`
- Link against `protobuf::libprotobuf`
- Add `Threads::Threads` for threading support
- Verify successful compilation and linking
- No linker errors

**Deliverables:**
- Updated `OdbDesignServer/CMakeLists.txt`
- Successful build logs

#### Story 1.1.4: Hello World gRPC Validation
**Effort:** 1 day  
**Acceptance Criteria:**
- Create temporary `hello.proto` with `SayHello` RPC
- Implement `GreeterServiceImpl` in `main.cpp`
- Start gRPC server on port 50051
- Test with `grpcurl` CLI tool
- Validate entire build toolchain works

**Deliverables:**
- Working Hello World gRPC service
- `grpcurl` test results
- Build toolchain validation report

### Epic 1.2: Service Implementation
**Duration:** 2-3 weeks  
**Team:** Backend Developer, Senior Engineer

#### Story 1.2.1: Create Service Implementation Skeleton
**Effort:** 2 days  
**Acceptance Criteria:**
- Create `OdbDesignServer/Services/OdbDesignServiceImpl.h/.cpp`
- Inherit from generated `Odb::Grpc::OdbDesignService::Service`
- Add `std::shared_ptr<DesignCache>` member variable
- Implement constructor accepting shared cache
- Define all virtual method signatures

**Deliverables:**
- `OdbDesignServiceImpl.h`
- `OdbDesignServiceImpl.cpp` skeleton
- Class structure documentation

#### Story 1.2.2: Implement HealthCheck RPC
**Effort:** 1 day  
**Acceptance Criteria:**
- Override `HealthCheck` method
- Return `SERVING` status
- Add basic error handling
- Write unit test for HealthCheck

**Deliverables:**
- Implemented `HealthCheck` method
- Unit test: `HealthCheckTest.cpp`

#### Story 1.2.3: Implement GetDesign Unary RPC
**Effort:** 2 days  
**Acceptance Criteria:**
- Override `GetDesign` method
- Extract `design_name` from request
- Call `m_designCache->getDesign(design_name)`
- Return `Odb::Lib::Protobuf::Design` message directly
- Handle `nullptr` with `NOT_FOUND` status
- Add try-catch for exception handling
- Write unit tests

**Deliverables:**
- Implemented `GetDesign` method
- Unit tests with mock DesignCache
- Error handling validation

#### Story 1.2.4: Implement GetAvailableDesigns & GetStepList RPCs
**Effort:** 2 days  
**Acceptance Criteria:**
- Implement `GetAvailableDesigns` unary RPC
- Implement `GetStepList` unary RPC
- Query DesignCache for data
- Populate response messages
- Add comprehensive error handling
- Write unit tests for both methods

**Deliverables:**
- Implemented unary RPC methods
- Unit tests with mock data
- Integration test scenarios

#### Story 1.2.5: Implement GetLayerList & GetSymbols RPCs
**Effort:** 3 days  
**Acceptance Criteria:**
- Implement `GetLayerList` unary RPC
- Implement `GetSymbols` unary RPC
- Navigate DesignCache to retrieve C++ models
- Call existing `to_protobuf()` methods
- Return `LayerDirectory` and `SymbolDirectory` messages
- Handle navigation errors with appropriate status codes
- Write comprehensive unit tests

**Deliverables:**
- Implemented `GetLayerList` and `GetSymbols`
- Unit tests with mock DesignCache
- Error path validation

#### Story 1.2.6: Implement GetLayerFeaturesStream (Critical Path)
**Effort:** 5 days  
**Acceptance Criteria:**
- Override `GetLayerFeaturesStream` server-streaming RPC
- Retrieve `FeaturesFile` from DesignCache
- **Critical:** Create single `FeatureRecord` message OUTSIDE loop
- Iterate through features, reusing message object
- Call `pFeature->to_protobuf(featureRecordMsg)` per feature
- Call `writer->Write(featureRecordMsg)` for each
- Call `featureRecordMsg.Clear()` after each write
- Check `context->IsCancelled()` periodically
- Handle client disconnection gracefully
- Add TODO comment for v2 zero-copy optimization
- Write streaming integration tests

**Deliverables:**
- Implemented `GetLayerFeaturesStream` with optimization
- Streaming integration tests
- Performance benchmark test
- Memory profiling report

### Epic 1.3: Authentication Integration
**Duration:** 1 week  
**Team:** Backend Developer, Security Engineer

#### Story 1.3.1: Extract Shared Authentication Logic
**Effort:** 2 days  
**Acceptance Criteria:**
- Create `OdbDesignLib/Utils/AuthHelper.h/.cpp`
- Extract `ParseBasicAuthHeader()` function
- Extract `ValidateCredentials()` function
- Make functions stateless and reusable
- Unit test auth helper functions

**Deliverables:**
- `AuthHelper.h/.cpp` utility
- Refactored shared authentication logic
- Unit tests for auth functions

#### Story 1.3.2: Implement gRPC Authentication Interceptor
**Effort:** 3 days  
**Acceptance Criteria:**
- Create `OdbDesignServer/Services/AuthInterceptor.h/.cpp`
- Implement `AuthInterceptorFactory` class
- Implement `AuthInterceptor` class
- Extract `authorization` metadata from context
- Call shared `AuthHelper::ValidateCredentials()`
- Return `UNAUTHENTICATED` status for invalid/missing auth
- Return `kContinue` for valid auth
- Write unit tests with mock `ServerContext`

**Deliverables:**
- `AuthInterceptor.h/.cpp`
- Authentication interceptor implementation
- Unit tests for auth flow

#### Story 1.3.3: Refactor Crow Middleware to Use Shared Auth
**Effort:** 1 day  
**Acceptance Criteria:**
- Update `BasicRequestAuthentication` middleware
- Replace inline auth logic with calls to `AuthHelper`
- Verify existing REST auth still works
- No breaking changes to REST API
- Integration tests pass

**Deliverables:**
- Refactored Crow middleware
- Passing integration tests
- Auth consistency verification

---

## Phase 2: Dual-Mode Support (2-3 weeks)

### Epic 2.1: Concurrent Server Architecture
**Duration:** 1 week  
**Team:** Backend Developer, DevOps Engineer

#### Story 2.1.1: Implement Graceful Shutdown Mechanism
**Effort:** 2 days  
**Acceptance Criteria:**
- Create `std::atomic<bool> g_shutdown_flag`
- Create `std::unique_ptr<grpc::Server> g_grpc_server` global
- Implement `SignalHandler(int signum)` for SIGINT/SIGTERM
- Call `app.stop()` for Crow
- Call `g_grpc_server->Shutdown()` for gRPC
- Set `g_shutdown_flag = true`
- Register signal handlers in `main()`

**Deliverables:**
- Signal handler implementation
- Graceful shutdown mechanism
- Shutdown integration tests

#### Story 2.1.2: Implement RunGrpcServer Thread Function
**Effort:** 3 days  
**Acceptance Criteria:**
- Create `void RunGrpcServer(const std::string& server_address, std::shared_ptr<DesignCache> cache)`
- Instantiate `OdbDesignServiceImpl` with shared cache
- Use `grpc::ServerBuilder` to configure server
- Add listening port with credentials
- Register service implementations
- Register auth interceptor factory
- Call `builder.BuildAndStart()`
- Block on `server->Wait()` until shutdown
- Log startup and shutdown events

**Deliverables:**
- `RunGrpcServer()` function implementation
- gRPC server thread logic
- Logging and monitoring

#### Story 2.1.3: Refactor main() for Dual-Server Launch
**Effort:** 2 days  
**Acceptance Criteria:**
- Launch gRPC server on `std::thread`
- Pass shared `g_designCache` to both servers
- Start Crow server on main thread (blocking)
- After Crow stops, join gRPC thread
- Verify both servers share same cache instance
- Test concurrent access to DesignCache
- Measure memory usage (no duplication)

**Deliverables:**
- Refactored `main.cpp`
- Dual-server initialization
- Memory usage report

### Epic 2.2: Configuration Management
**Duration:** 1 week  
**Team:** Backend Developer, DevOps Engineer

#### Story 2.2.1: Add gRPC Port Configuration
**Effort:** 1 day  
**Acceptance Criteria:**
- Add `std::string grpcPort = "50051"` to `OdbDesignArgs.h`
- Parse `--grpc-port` command-line argument
- Add getter method `getGrpcPort()`
- Update argument parsing documentation
- Add default value handling

**Deliverables:**
- Updated `OdbDesignArgs.h/.cpp`
- Command-line argument parsing
- Configuration documentation

#### Story 2.2.2: Implement TLS/SSL Support
**Effort:** 3 days  
**Acceptance Criteria:**
- Add `--grpc-cert` and `--grpc-key` arguments to `OdbDesignArgs`
- Implement `ReadFile()` helper for loading cert/key files
- Create `std::shared_ptr<grpc::ServerCredentials>` conditional logic:
  - Use `InsecureServerCredentials()` for development (if paths empty)
  - Use `SslServerCredentials()` for production (if paths provided)
- Log credential type being used
- Test with both insecure and secure modes
- Document TLS configuration

**Deliverables:**
- TLS/SSL configuration implementation
- Certificate loading logic
- Security configuration documentation

#### Story 2.2.3: Docker Configuration Updates
**Effort:** 1 day  
**Acceptance Criteria:**
- Update `Dockerfile` to `EXPOSE 8888 50051`
- Verify both ports accessible in container
- Test container with dual ports
- Update container documentation

**Deliverables:**
- Updated Dockerfile
- Container port verification
- Updated deployment docs

---

## Phase 3: Testing & Validation (3-4 weeks)

### Epic 3.1: Unit Testing
**Duration:** 1 week  
**Team:** Backend Developer, QA Engineer

#### Story 3.1.1: Service Implementation Unit Tests
**Effort:** 3 days  
**Acceptance Criteria:**
- Create `OdbDesignTests/GrpcServiceTests.cpp`
- Mock `DesignCache` using gMock
- Test all unary RPCs with mock data
- Test error paths (nullptr, exceptions)
- Verify correct status codes returned
- Achieve >85% code coverage for service impl

**Deliverables:**
- `GrpcServiceTests.cpp`
- Mock DesignCache implementation
- Unit test suite with >85% coverage

#### Story 3.1.2: Authentication Unit Tests
**Effort:** 2 days  
**Acceptance Criteria:**
- Test `AuthHelper` functions with various inputs
- Test `AuthInterceptor` with mock `ServerContext`
- Test valid credentials (pass)
- Test invalid credentials (fail)
- Test missing credentials (fail)
- Test malformed auth header (fail)

**Deliverables:**
- Authentication unit tests
- Test coverage report
- Security validation

### Epic 3.2: Integration Testing
**Duration:** 1 week  
**Team:** Backend Developer, QA Engineer

#### Story 3.2.1: End-to-End Integration Tests
**Effort:** 3 days  
**Acceptance Criteria:**
- Create integration test executable using C++ gRPC client
- Start test OdbDesignServer instance
- Connect to `localhost:50051`
- Test all RPCs with real test data
- Verify data integrity end-to-end
- Test authentication flow
- Test error scenarios

**Deliverables:**
- Integration test suite
- E2E test scenarios
- Test data fixtures

#### Story 3.2.2: grpcurl Command-Line Validation
**Effort:** 1 day  
**Acceptance Criteria:**
- Test all RPCs using `grpcurl` CLI tool
- Verify connectivity with plaintext
- Test with TLS/SSL credentials
- Test authentication headers
- Document `grpcurl` test commands
- Create test script for CI/CD

**Deliverables:**
- `grpcurl` test script
- CLI validation documentation
- CI integration

#### Story 3.2.3: Client Cancellation & Disconnect Tests
**Effort:** 1 day  
**Acceptance Criteria:**
- Test client disconnection during streaming
- Verify server detects cancellation
- Verify no resource leaks
- Verify server remains stable
- Log shows "Client disconnected" message
- Process monitoring shows no memory leaks

**Deliverables:**
- Cancellation test suite
- Resource leak validation
- Stability verification report

### Epic 3.3: Performance Testing
**Duration:** 1-2 weeks  
**Team:** Backend Developer, Performance Engineer

#### Story 3.3.1: Performance Benchmark Implementation
**Effort:** 3 days  
**Acceptance Criteria:**
- Add benchmark to `OdbDesignTests/PerformanceTests.cpp`
- Benchmark `GetLayerFeaturesStream` gRPC call
- Compare against existing REST `/features` endpoint
- Use large test dataset (>500k features)
- Measure Time-to-Last-Byte (TTLB)
- Measure Time-to-First-Feature (TTFF)
- Measure server CPU and memory usage
- Measure total payload size
- Generate performance comparison report

**Deliverables:**
- Performance benchmark test
- Metrics collection implementation
- Performance comparison report

#### Story 3.3.2: Performance Targets Validation
**Effort:** 2 days  
**Acceptance Criteria:**
- **Target 1:** gRPC TTLB < 25% of REST TTLB ✓
- **Target 2:** gRPC TTFF < 100ms ✓
- **Target 3:** gRPC payload size < 10% of REST JSON size ✓
- **Target 4:** Server memory per-request measurably lower ✓
- Document all performance metrics
- Create graphs and visualizations

**Deliverables:**
- Performance validation report
- Metrics dashboard
- Performance graphs

#### Story 3.3.3: Memory Profiling & Optimization
**Effort:** 2 days  
**Acceptance Criteria:**
- Profile memory usage with Valgrind/AddressSanitizer
- Verify message reuse optimization working
- Verify no memory leaks in streaming loop
- Measure heap allocations per feature
- Document memory optimization results
- Identify any further optimization opportunities

**Deliverables:**
- Memory profiling report
- Optimization validation
- Performance tuning documentation

### Epic 3.4: Cross-Language Validation
**Duration:** 3-5 days  
**Team:** .NET Client Team, Backend Developer

#### Story 3.4.1: .NET Client Integration Testing
**Effort:** 3 days  
**Acceptance Criteria:**
- .NET client team connects to dev environment
- Test all gRPC RPCs from C# client
- Verify Protobuf deserialization works
- Test streaming with large datasets
- Measure client-side performance
- Document any interop issues

**Deliverables:**
- .NET client test results
- Interoperability validation
- Client-side performance report

#### Story 3.4.2: Cross-Platform Testing
**Effort:** 2 days  
**Acceptance Criteria:**
- Test on Linux development environment
- Test on Windows (if applicable)
- Verify consistent behavior across platforms
- Test with different gRPC client versions
- Document platform compatibility

**Deliverables:**
- Platform compatibility matrix
- Cross-platform test results
- Compatibility documentation

---

## Phase 4: CI/CD & Automation (2-3 weeks)

### Epic 4.1: Continuous Integration
**Duration:** 1 week  
**Team:** DevOps Engineer, Backend Developer

#### Story 4.1.1: Create Performance Benchmark Workflow
**Effort:** 2 days  
**Acceptance Criteria:**
- Create `.github/workflows/performance-benchmarks.yml`
- Trigger on `push` to `nam20485` branch
- Trigger on `pull_request` targeting `nam20485`
- Build in **Release** configuration (critical)
- Run benchmark tests
- Export results to JSON

**Deliverables:**
- GitHub Actions workflow file
- Automated benchmark execution
- CI integration

#### Story 4.1.2: Integrate Benchmark Action
**Effort:** 2 days  
**Acceptance Criteria:**
- Use `benchmark-action/github-action-benchmark@v1`
- Store baseline results from `nam20485` branch
- Compare PR results against baseline
- Generate comparison report
- Post results as PR comment
- **Fail check** if significant regression detected
- Configure statistical significance threshold

**Deliverables:**
- Benchmark action configuration
- Regression detection logic
- PR comment integration

#### Story 4.1.3: Update Existing CI Workflows
**Effort:** 1 day  
**Acceptance Criteria:**
- Update all workflows to initialize Git submodules
- Add `git submodule update --init --recursive`
- Verify builds succeed with submodule structure
- Test workflow on feature branch
- Update CI documentation

**Deliverables:**
- Updated CI workflow files
- Submodule CI integration
- CI documentation

### Epic 4.2: Kubernetes Deployment
**Duration:** 1-2 weeks  
**Team:** DevOps Engineer, Backend Developer

#### Story 4.2.1: Update Kubernetes Deployment Manifest
**Effort:** 1 day  
**Acceptance Criteria:**
- Update `deploy/kube/OdbDesignServer/deployment.yaml`
- Add container port for gRPC:
  ```yaml
  ports:
    - containerPort: 8888
      name: http-rest
    - containerPort: 50051
      name: grpc
  ```
- Verify deployment applies successfully
- Test pod creation

**Deliverables:**
- Updated deployment manifest
- Deployment validation

#### Story 4.2.2: Update Kubernetes Service Manifest
**Effort:** 1 day  
**Acceptance Criteria:**
- Update `deploy/kube/OdbDesignServer/service.yaml`
- Expose gRPC port:
  ```yaml
  ports:
    - port: 80
      targetPort: 8888
      name: http-rest
    - port: 50051
      targetPort: 50051
      name: grpc
  ```
- Verify service routing
- Test internal cluster access

**Deliverables:**
- Updated service manifest
- Service routing validation

#### Story 4.2.3: Configure Ingress for gRPC
**Effort:** 2 days  
**Acceptance Criteria:**
- Update `deploy/kube/default-ingress(eks).yaml`
- Add NGINX annotation:
  ```yaml
  metadata:
    annotations:
      nginx.ingress.kubernetes.io/backend-protocol: "GRPC"
  ```
- Configure path routing for gRPC service
- Verify HTTP/2 support enabled
- Test external access with `grpcurl`
- Validate TLS termination works

**Deliverables:**
- Updated ingress configuration
- External access validation
- gRPC routing documentation

#### Story 4.2.4: Health Check Implementation
**Effort:** 2 days  
**Acceptance Criteria:**
- Implement `grpc.health.v1.Health` service
- Create `HealthCheckServiceImpl.h/.cpp`
- Override `Check()` method
- Return `SERVING` if DesignCache initialized
- Return `NOT_SERVING` otherwise
- Configure Kubernetes probes:
  ```yaml
  readinessProbe:
    exec:
      command: ["/bin/grpc_health_probe", "-addr=:50051"]
  livenessProbe:
    exec:
      command: ["/bin/grpc_health_probe", "-addr=:50051"]
  ```
- Test probe functionality

**Deliverables:**
- Health check service implementation
- Kubernetes probe configuration
- Health monitoring validation

---

## Phase 5: Hardening & Rollout (2-3 weeks)

### Epic 5.1: Production Readiness
**Duration:** 1 week  
**Team:** Backend Developer, DevOps Engineer, QA Engineer

#### Story 5.1.1: Load Testing
**Effort:** 3 days  
**Acceptance Criteria:**
- Perform load testing with realistic traffic patterns
- Test concurrent client connections
- Measure throughput under sustained load
- Identify bottlenecks or performance degradation
- Verify no memory leaks under load
- Test graceful shutdown under load
- Document load test results

**Deliverables:**
- Load test suite
- Performance under load report
- Bottleneck analysis

#### Story 5.1.2: Security Hardening
**Effort:** 2 days  
**Acceptance Criteria:**
- Review authentication implementation
- Test with security scanning tools
- Verify TLS/SSL configuration secure
- Check for common vulnerabilities
- Review error messages (no info leakage)
- Conduct security audit
- Document security posture

**Deliverables:**
- Security audit report
- Vulnerability assessment
- Security hardening documentation

#### Story 5.1.3: Logging & Monitoring Enhancement
**Effort:** 2 days  
**Acceptance Criteria:**
- Add structured logging for gRPC service
- Log request start/end with timing
- Log authentication attempts
- Log errors with context
- Add metrics for monitoring:
  - Request count per RPC
  - Request duration histogram
  - Error rate
  - Active stream count
- Integrate with monitoring system
- Create monitoring dashboard

**Deliverables:**
- Enhanced logging implementation
- Metrics collection
- Monitoring dashboard

### Epic 5.2: Documentation & Rollout
**Duration:** 1-2 weeks  
**Team:** Backend Developer, Technical Writer, DevOps Engineer

#### Story 5.2.1: Technical Documentation
**Effort:** 3 days  
**Acceptance Criteria:**
- Document gRPC service architecture
- Document all RPC endpoints with examples
- Create API usage guide for clients
- Document authentication requirements
- Document TLS/SSL configuration
- Create troubleshooting guide
- Document performance characteristics

**Deliverables:**
- Technical architecture document
- API reference documentation
- Client integration guide
- Operations runbook

#### Story 5.2.2: Deployment Runbook
**Effort:** 2 days  
**Acceptance Criteria:**
- Document deployment procedures
- Create rollback plan
- Document configuration options
- Create health check procedures
- Document monitoring and alerting
- Create incident response guide
- Review with operations team

**Deliverables:**
- Deployment runbook
- Rollback procedures
- Operations documentation

#### Story 5.2.3: Staged Rollout Plan
**Effort:** 3 days  
**Acceptance Criteria:**
- Define rollout phases:
  1. Development environment
  2. Staging environment
  3. Production canary (10%)
  4. Production gradual rollout (50%, 100%)
- Define success criteria for each phase
- Define rollback triggers
- Create monitoring plan for rollout
- Schedule rollout windows
- Coordinate with stakeholders

**Deliverables:**
- Rollout plan document
- Phase success criteria
- Monitoring and rollback plan

#### Story 5.2.4: Production Deployment
**Effort:** 1-2 days (per environment)  
**Acceptance Criteria:**
- Deploy to development environment
- Verify functionality and performance
- Deploy to staging environment
- Run full integration test suite
- Deploy canary to production (10% traffic)
- Monitor metrics for 24 hours
- Gradual production rollout (50%, 100%)
- Verify no degradation in REST API performance
- Validate client connections working

**Deliverables:**
- Successful production deployment
- Deployment validation report
- Post-deployment monitoring

---

## Dependencies & Critical Path

### Critical Path (Longest Duration):
1. **Phase 0:** Discovery & Design (2-3 weeks)
2. **Phase 1:** Core Implementation (4-6 weeks) - **LONGEST**
3. **Phase 2:** Dual-Mode Support (2-3 weeks)
4. **Phase 3:** Testing & Validation (3-4 weeks)
5. **Phase 4:** CI/CD & Automation (2-3 weeks)
6. **Phase 5:** Hardening & Rollout (2-3 weeks)

**Total Critical Path:** 13-18 weeks

### Key Dependencies:

- **Phase 1** depends on **Phase 0** completion (API contract, protobuf repo setup)
- **Phase 2** depends on **Phase 1 Epic 1.2** (service implementation must exist to launch)
- **Phase 3** depends on **Phase 2** (need dual-server working to test)
- **Phase 4** can partially overlap with **Phase 3** (CI work can start early)
- **Phase 5** depends on successful **Phase 3** validation

### Parallelization Opportunities:

- **Epic 1.3** (Authentication) can partially overlap with **Epic 1.2** (Service Implementation)
- **Epic 3.1** (Unit Testing) can progress alongside **Epic 1.2** development
- **Epic 4.1** (CI) can start while **Epic 3.3** (Performance Testing) is ongoing
- **Epic 4.2** (K8s Deployment) can start once **Epic 2.2** (Configuration) is complete

---

## Resource Allocation

### Team Composition:
- **Backend Developer (Lead):** 100% allocation, all phases
- **Backend Developer (Support):** 50% allocation, Phase 1-3
- **DevOps Engineer:** 30% allocation, Phase 0, 50% Phase 2, 80% Phase 4-5
- **QA Engineer:** 80% allocation, Phase 3
- **Performance Engineer:** 50% allocation, Phase 0, 80% Phase 3
- **Security Engineer:** Part-time, Phase 1 (Epic 1.3), Phase 5 (Epic 5.1.2)
- **Technical Writer:** Part-time, Phase 5 (Epic 5.2.1)
- **.NET Client Team Lead:** Part-time, Phase 0 (API review)

### Estimated Effort by Phase:
- **Phase 0:** 80-120 person-hours
- **Phase 1:** 240-360 person-hours (Critical Path)
- **Phase 2:** 120-180 person-hours
- **Phase 3:** 200-280 person-hours
- **Phase 4:** 120-180 person-hours
- **Phase 5:** 120-180 person-hours

**Total Estimated Effort:** 880-1,300 person-hours

---

## Success Criteria

### Phase Completion Gates:

**Phase 0 Complete:**
- ✅ Performance baseline established
- ✅ API contract finalized and approved
- ✅ Protobuf repository created and submodule configured
- ✅ Hello World gRPC service working

**Phase 1 Complete:**
- ✅ All gRPC dependencies integrated and building
- ✅ All service RPCs implemented and unit tested
- ✅ Authentication integration complete
- ✅ Code coverage >85%

**Phase 2 Complete:**
- ✅ Dual-server architecture working
- ✅ Graceful shutdown verified
- ✅ Both servers sharing DesignCache
- ✅ TLS/SSL configuration working
- ✅ Docker configuration updated

**Phase 3 Complete:**
- ✅ All unit tests passing (>85% coverage)
- ✅ Integration tests passing
- ✅ Performance targets met (gRPC TTLB < 25% REST TTLB)
- ✅ .NET client integration validated
- ✅ No memory leaks detected

**Phase 4 Complete:**
- ✅ CI/CD pipeline automated
- ✅ Performance benchmarks in CI
- ✅ Kubernetes manifests updated and tested
- ✅ Health checks working
- ✅ External gRPC access verified

**Phase 5 Complete:**
- ✅ Load testing passed
- ✅ Security audit completed
- ✅ Documentation complete
- ✅ Production deployment successful
- ✅ 100% REST API backward compatibility maintained

---

## Change Control

Any changes to this WBS must be reviewed and approved by:
- Project Lead
- Backend Development Lead
- DevOps Lead

Major scope changes require:
- Updated effort estimates
- Timeline impact analysis
- Resource allocation review
- Stakeholder approval

---

**Document Control:**
- **Version:** 1.0
- **Last Updated:** 2025-11-09
- **Next Review:** Start of each phase
- **Owner:** Project Lead
