### **OdbDesignServer: gRPC API Upgrade Implementation Specification**

#### **App Title**

OdbDesignServer: gRPC API Upgrade

#### **Description**

##### **Overview**

This project upgrades the existing C++ OdbDesignServer application by integrating a high-performance, parallel gRPC service. This new service will run within the same server process as the existing Crow RESTful API, sharing the same in-memory DesignCache. The primary goal is to bypass the current JSON serialization bottleneck by exposing the core Odb::Lib::Protobuf data models directly over HTTP/2. This provides the high-throughput, low-latency data streaming required by new clients (e.g., .NET desktop applications) without impacting any existing clients.

##### **Document Links**

* **Development Plan:** \[OdbDesignServer: gRPC API Upgrade Development Plan v1.0\](uploaded:OdbDesignServer: gRPC API Upgrade Development Plan v1.0)  
* **Existing Source (main.cpp):** main.cpp  
* **Existing Source (CMake):** CMakeLists.txt  
* **Existing Source (vcpkg):** vcpkg.json

#### **Requirements**

##### **Features**

* Integrate the grpc dependency via vcpkg.json.  
* Update CMakeLists.txt to find\_package(gRPC REQUIRED) and configure it to build the new gRPC service stubs using the gRPC C++ plugin \[cite: 49, 51-53\].  
* Link the OdbDesignServer executable against the new gRPC and reflection libraries.  
* Refactor OdbDesignServer/main.cpp to run the gRPC server on a new std::thread concurrently with the existing Crow server.  
* Implement a graceful shutdown mechanism using signal handlers (SIGINT, SIGTERM) that calls app.stop() (Crow) and g\_grpc\_server-\>Shutdown() (gRPC) and joins the gRPC thread \[cite: 65-70\].  
* Create a new OdbDesignServer/Services/OdbDesignServiceImpl.h/.cpp class that inherits from the generated gRPC service stub.  
* The OdbDesignServiceImpl class must be initialized with and hold the shared std::shared\_ptr\<OdbDesignLib::App::DesignCache\>.  
* Implement a unary HealthCheck RPC endpoint that returns a SERVING status.  
* Implement a unary GetDesign RPC that fetches the Odb::Lib::Protobuf::Design message from the shared cache and returns it directly \[cite: 77-80\].  
* Implement a performance-critical, server-streaming GetLayerFeaturesStream RPC. This implementation must reuse a single FeatureRecord message object inside the loop to avoid heap allocations \[cite: 153-158\].  
* Add a new \--grpc-port command-line argument (default: "50051") to OdbDesignLib/App/OdbDesignArgs.h/.cpp.  
* Update Dockerfile to EXPOSE 8888 50051\.  
* Update Kubernetes deployment.yaml and service.yaml files to expose the new grpc port (50051).  
* Update the Kubernetes Ingress configuration (e.g., default-ingress (eks).yaml) to enable gRPC routing with the annotation nginx.ingress.kubernetes.io/backend-protocol: "GRPC" \[cite: 95-97\].  
* All .proto files (new and existing) *must* be consolidated into a new, separate Git repository and consumed by this project as a Git submodule to mitigate "API Contract Drift" \[cite: 39-43\].

##### **Test cases**

* **C++ Integration Tests:** Add new tests to OdbDesignTests/ (e.g., GrpcServiceTests.cpp) that use a C++ gRPC client stub to connect to a test server instance and verify the HealthCheck and GetDesign RPCs \[cite: 100-103\].  
* **C++ Streaming Tests:** Add integration tests for GetLayerFeaturesStream, iterating through the entire stream to assert feature count and data integrity.  
* **External Tooling Validation:** Use grpcurl from the command line against the running dev container to validate connectivity and query the HealthCheck endpoint.  
* **Benchmark Test:** Add a new benchmark to OdbDesignTests/PerformanceTests.cpp that compares the performance of the new GetLayerFeaturesStream gRPC call against the existing REST endpoint for a large dataset.  
* **Cross-Language Validation:** The .NET client team must test and validate interoperability against the dev container's gRPC endpoint.  
* **CI Workflow:** A new GitHub Actions workflow (.github/workflows/performance-benchmarks.yml) will be created. This workflow must:  
  * Trigger on push to nam20485 and pull\_request targeting nam20485.  
  * Build the tests in Release configuration.  
  * Run the benchmarks and export results to JSON.  
  * Use an action (like benchmark-action/github-action-benchmark) to compare PR results against the nam20485 baseline.  
  * Post results as a PR comment and **fail the check** if a statistically significant regression is detected.

##### **Logging**

* Log gRPC server startup event, including the listening address, to std::cout.  
* Log gRPC server shutdown event to std::cout.  
* Catch std::exception in gRPC service methods and return a grpc::Status(grpc::StatusCode::INTERNAL, ...) containing the error message.  
* Log receipt of interrupt signals (SIGINT, SIGTERM) to std::cout.

##### **Containerization: Docker**

* The existing Dockerfile will be updated to EXPOSE 8888 50051 to expose both the REST and new gRPC ports.

##### **Containerization: Docker Compose**

* N/A (Deployment is managed via Kubernetes).

##### **Swagger/OpenAPI**

* N/A for the new gRPC service. The API contract is defined by Protocol Buffers (.proto) files.  
* **New API Contract:** OdbDesignServer/grpc/odbdesign\_service.proto  
* **Imported Contracts:** proto/design.proto, proto/featuresfile.proto, proto/common.proto

##### **Documentation**

* A new, separate Git repository will be created to host the canonical .proto API contract files. This repository will be consumed as a Git submodule and serve as the single source of truth for the API.

##### **Acceptance Criteria**

* The existing Crow-based REST API on port 8888 *must* remain 100% functional, unchanged, and backward-compatible.  
* The new gRPC service must run on a separate port (e.g., 50051\) *within the same server process*.  
* Both the REST (Crow) and gRPC services must share the *exact same instance* of the std::shared\_ptr\<OdbDesignLib::App::DesignCache\>.  
* The GetLayerFeaturesStream benchmark must show a significant performance improvement over the equivalent REST endpoint.  
* The new CI benchmark workflow must successfully fail a pull request that introduces a performance regression.  
* The server process must shut down both services cleanly and exit gracefully when receiving a SIGINT or SIGTERM signal \[cite: 65-70\].

#### **Language**

##### **Language**

C++

##### **Language Version**

C++17

#### **Include global.json? sdk: "9.0.0" rollwForward: "latestFeature"**

N/A (Project is C++)

#### **Frameworks, Tools, Packages**

* **Build System:** CMake (existing)  
* **Package Manager:** vcpkg (existing)  
* **Existing Core Libraries:** crow, protobuf, gtest, benchmark  
* **New Dependencies (to add to vcpkg.json):** grpc

#### **Project Structure/Package System**

* **New API Contract:** OdbDesignServer/grpc/odbdesign\_service.proto (This file will define the OdbDesignService gRPC service).  
* **New Service Implementation:** OdbDesignServer/Services/OdbDesignServiceImpl.h / .cpp (This will contain the C++ implementation of the gRPC service).  
* **Refactored Core File:** OdbDesignServer/main.cpp (Will be modified to launch both Crow and gRPC servers in separate threads and handle graceful shutdown).  
* **Modified Build Files:**  
  * vcpkg.json (Add grpc dependency).  
  * CMakeLists.txt (Root file \- to find\_package(gRPC)).  
  * OdbDesignServer/CMakeLists.txt (To add protobuf\_generate target for gRPC stubs and link new libraries) \[cite: 50-55\].  
* **Modified Configuration:**  
  * OdbDesignLib/App/OdbDesignArgs.h / .cpp (Add \--grpc-port argument).  
  * Dockerfile (Expose new port).  
  * deploy/kube/OdbDesignServer/deployment.yaml (Add gRPC containerPort).  
  * deploy/kube/OdbDesignServer/service.yaml (Expose gRPC targetPort).  
  * deploy/kube/default-ingress (eks).yaml (Add gRPC backend protocol annotation).  
* **New Test/CI Files:**  
  * OdbDesignTests/GrpcServiceTests.cpp (Suggested file for new integration tests).  
  * OdbDesignTests/PerformanceTests.cpp (Will be *modified* to add new gRPC benchmark).  
  * .github/workflows/performance-benchmarks.yml (New workflow for automated regression testing).  
* **New Submodule:**  
  * proto/ (New directory at the project root, added via git submodule add ..., containing the new API contract repository).

#### **GitHub**

##### **Repo**

[https://github.com/nam20485/odbdesign](https://www.google.com/search?q=https://github.com/nam20485/odbdesign)

##### **Branch**

All feature development should be done on a feature branch created from the nam20485 branch. All pull requests will target the nam20485 branch, which will trigger the new performance benchmark workflow.

#### **Deliverables**

1. A modified OdbDesignServer application executable that runs both the existing Crow REST service and the new gRPC service in parallel from the same process.  
2. A new, separate Git repository containing the canonical .proto files for the API contract.  
3. The OdbDesignServer repository will be updated to consume this new API contract repository as a Git submodule in a proto/ directory.  
4. A new GitHub Actions workflow file (.github/workflows/performance-benchmarks.yml) that automatically runs, stores, and compares performance benchmarks to prevent regressions.  
5. Updated Kubernetes deployment configurations (deployment.yaml, service.yaml, ingress.yaml) to correctly route both HTTP/1.1 (REST) and HTTP/2 (gRPC) traffic to the running service.