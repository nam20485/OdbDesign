# **OdbDesignServer: gRPC API Upgrade Development Plan**

Version: 1.0  
Date: 2025-11-08  
Project: OdbDesignServer gRPC Migration

## **1\. Project Summary & Motivation**

This document outlines the development plan to *upgrade* the existing C++ OdbDesignServer application by integrating a high-performance gRPC service.

The current server uses the Crow C++ micro-framework to provide a RESTful JSON API. This API serves data from the OdbDesignLib, which involves a costly serialization process: C++ Model \-\> Protobuf Model \-\> JSON String. This JSON serialization step (especially MessageToJsonString()) is a significant performance bottleneck, resulting in large payloads that are slow to generate, transmit, and parse.

The **primary goal** of this upgrade is to implement a *parallel* gRPC service that runs *within the same server process*. This new service will expose the core Odb::Lib::Protobuf data models directly over HTTP/2, bypassing the JSON conversion entirely. This provides the high-throughput, low-latency data streaming required by new clients (e.g., a .NET desktop application) without impacting existing clients.

## **2\. Guiding Principles & Technical Strategy**

1. **Augmentation, Not Replacement:** The existing Crow-based REST API (on port 8888\) **must** remain fully functional and unchanged to ensure 100% backward compatibility. The new gRPC service will run on a separate port (e.g., 50051\) within the same application process.  
2. **Performance First:** The gRPC implementation must prioritize minimal latency and memory allocations. This will be achieved by serving Protobuf messages directly and optimizing data streaming to avoid intermediate object creation within tight loops.  
3. **Shared State:** Both the existing REST (Crow) and new gRPC services will share the *exact same instance* of the in-memory DesignCache (the std::shared\_ptr\<OdbDesignLib::App::DesignCache\> created in main.cpp). This ensures data consistency and minimal memory overhead.  
4. **API Contract Integrity:** The .proto files defining the gRPC services and messages must be managed as a single source of truth to prevent drift between the C++ server and its clients.

## **3\. Core Technology & Dependency Changes**

This upgrade primarily involves adding gRPC and its dependencies to the existing C++ CMake project.

* **Language:** C++17 (existing)  
* **Build System:** CMake (existing)  
* **Package Manager:** vcpkg (existing)  
* **Existing Core Libraries:** crow, protobuf, gtest, benchmark  
* **New Dependencies (to be added to vcpkg.json):**  
  * grpc: The core gRPC library (which includes grpcpp).

## **4\. API Contract (.proto) Definition**

A new .proto file will be created to define the gRPC services. This file will *import* the *existing* Protobuf messages (like Design and FeatureRecord) that are already defined in OdbDesignLib/protoc/.

**Proposed Location:** OdbDesignServer/grpc/odbdesign\_service.proto

### **odbdesign\_service.proto (Initial Draft)**

syntax \= "proto3";

package Odb.Grpc;

// Import existing data models defined in OdbDesignLib  
import "proto/design.proto";  
import "proto/featuresfile.proto";  
import "proto/common.proto";

// Service definition for ODB++ data  
service OdbDesignService {  
  // Unary call to get the full design (similar to existing REST endpoint)  
  // Used for initial load by a client.  
  rpc GetDesign(GetDesignRequest) returns (Odb::Lib::Protobuf::Design);

  // Server-streaming call for high-performance streaming of layer features.  
  // This is the primary performance-critical endpoint.  
  rpc GetLayerFeaturesStream(GetLayerFeaturesRequest) returns (stream Odb::Lib::Protobuf::FeatureRecord);

  // A simple health check for the gRPC server  
  rpc HealthCheck(HealthCheckRequest) returns (HealthCheckResponse);  
}

// \=== Request/Response Messages \===

message GetDesignRequest {  
  string design\_name \= 1;  
}

message GetLayerFeaturesRequest {  
  string design\_name \= 1;  
  string step\_name \= 2;  
  string layer\_name \= 3;  
}

message HealthCheckRequest {  
  string service \= 1;  
}

message HealthCheckResponse {  
  enum ServingStatus {  
    UNKNOWN \= 0;  
    SERVING \= 1;  
    NOT\_SERVING \= 2;  
  }  
  ServingStatus status \= 1;  
}

### **API Contract Management (Risk Mitigation)**

To mitigate the "API Contract Drift" risk, all .proto files (from OdbDesignLib/protoc/ and the new OdbDesignServer/grpc/) **must** be consolidated into a **new, separate Git repository**. This repository will define the canonical API contract.

This new "API Contract" repository will then be consumed by both this C++ server project and the .NET client project as a **Git submodule** (e.g., in a new proto/ directory at the root of *this* project).

This ensures both applications are always built against the exact same .proto definitions. All protoc generation commands will be updated to use this new submodule path (e.g., protoc \-I=./proto \--cpp\_out=... ./proto/odbdesign\_service.proto).

## **5\. Implementation Plan (Epics & Stories)**

### **Epic 1: Integrate gRPC Dependencies & Build System**

*Goal: Add gRPC as a dependency and configure CMake to build the new service stubs.*

* **Story 1.1:** Add "grpc" to the dependencies list in the root vcpkg.json file.  
* **Story 1.2:** Update the root CMakeLists.txt to find\_package(gRPC REQUIRED).  
* **Story 1.3:** Modify OdbDesignServer/CMakeLists.txt to:  
  * Find the gRPC C++ plugin (\_grpc\_cpp\_plugin).  
  * Add a new protobuf\_generate target (or similar) to compile OdbDesignServer/grpc/odbdesign\_service.proto using protoc *with* the \_grpc\_cpp\_plugin. This will generate odbdesign\_service.pb.h, odbdesign\_service.pb.cc, odbdesign\_service.grpc.pb.h, and odbdesign\_service.grpc.pb.cc.  
  * Add these generated files to the OdbDesignServer executable target.  
  * Link the OdbDesignServer executable against the gRPC libraries (e.g., gRPC::grpc++, gRPC::grpc, gRPC::grpc++\_reflection, protobuf::libprotobuf).  
* **Story 1.4:** Create a "hello world" gRPC service class and attempt to build and link it to confirm the integration is successful.

### **Epic 2: Dual Server (Crow \+ gRPC) Startup & Shutdown**

*Goal: Refactor the server main function to run both Crow and gRPC servers concurrently and handle graceful shutdown.*

* **Story 2.1:** Refactor OdbDesignServer/main.cpp. The gRPC server must run on its own thread.  
* **Story 2.2:** Create a new function: void RunGrpcServer(const std::string& server\_address, std::shared\_ptr\<OdbDesignLib::App::DesignCache\> cache)  
  * This function will instantiate the gRPC service implementation (from Epic 3).  
  * It will use grpc::ServerBuilder to build and start the server.  
  * It will call server-\>Wait() to block the thread until shutdown is requested.  
* **Story 2.3:** In main(), after initializing g\_designCache and the Crow app, launch RunGrpcServer on a new std::thread (e.g., grpc\_thread).  
* **Story 2.4:** Implement graceful shutdown (addresses "Concurrent Server Shutdown" risk):  
  * The main thread must catch SIGINT and SIGTERM.  
  * The signal handler will call app.stop() (for Crow) and g\_grpc\_server-\>Shutdown() (for gRPC).  
  * The signal handler will call g\_grpc\_server-\>Shutdown() (where g\_grpc\_server is a std::unique\_ptr\<grpc::Server\> accessible from the handler, as shown in Snippet 1). This call is thread-safe and will unblock the server-\>Wait() call in the grpc\_thread.  
  * The main thread must grpc\_thread.join() to wait for the gRPC server to exit cleanly.

### **Epic 3: Implement gRPC Service Endpoints**

*Goal: Create the C++ service implementation that handles gRPC requests using the shared cache.*

* **Story 3.1:** Create OdbDesignServer/Services/OdbDesignServiceImpl.h and .cpp.  
  * This class will inherit from Odb::Grpc::OdbDesignService::Service.  
  * It will hold a std::shared\_ptr\<OdbDesignLib::App::DesignCache\> as a member variable, initialized in its constructor.  
* **Story 3.2:** Implement the HealthCheck RPC. This is a simple unary call that returns SERVING.  
* **Story 3.3:** Implement the GetDesign (unary) RPC.  
  * It will use the design\_name from the request.  
  * It will call m\_designCache-\>getDesign(design\_name).  
  * It will retrieve the Odb::Lib::Protobuf::Design message from the cached FileArchive and return it directly.  
* **Story 3.4 (Performance Critical):** Implement the GetLayerFeaturesStream (server-streaming) RPC.  
  * It will retrieve the correct FileArchive and FeaturesFile from the m\_designCache using the request parameters.  
  * It will iterate over the *native C++* Feature objects.  
  * **Mitigation for "Serialization Overhead":** This is a good v1 mitigation. We should add a "TODO" or "Future Optimization" note here to investigate a "zero-copy" or "direct serialization" method. Ideally, we'd have a function like pFeature-\>SerializeToGrpcWriter(writer) that writes fields directly to the stream without creating any intermediate FeatureRecord message, not even one. This would eliminate all serialization overhead from the loop.

### **Epic 4: Configuration & Deployment Update**

*Goal: Update all configuration to expose the new gRPC port.*

* **Story 4.1:** Update OdbDesignLib/App/OdbDesignArgs.h/.cpp to add a new command-line argument: \--grpc-port (default: "50051").  
* **Story 4.2:** Update OdbDesignServer/main.cpp to use this new argument when constructing the grpc\_server\_address.  
* **Story 4.3:** Update Dockerfile to EXPOSE 8888 50051\.  
* **Story 4.4:** Update deploy/kube/OdbDesignServer/deployment.yaml to add a new containerPort for gRPC:  
  ports:  
    \- containerPort: 8888  
      name: http-rest  
    \- containerPort: 50051  
      name: grpc

* **Story 4.5:** Update deploy/kube/OdbDesignServer/service.yaml to expose the new port:  
  ports:  
    \- port: 80  
      targetPort: 8888  
      name: http-rest  
    \- port: 50051  
      targetPort: 50051  
      name: grpc

* **Story 4.6 (Network Risk Mitigation):** Update deploy/kube/default-ingress (eks).yaml (or relevant ingress config) to enable gRPC. This requires adding a specific annotation for the NGINX ingress controller:  
  metadata:  
    annotations:  
      nginx.ingress.kubernetes.io/backend-protocol: "GRPC"  
  spec:  
    rules:  
      \# ... existing http rule ...  
    \- host: your-grpc-host.example.com  
      http:  
        paths:  
          \- path: /Odb.Grpc.OdbDesignService  
            pathType: Prefix  
            backend:  
              service:  
                name: odbdesignserver-service  
                port:  
                  name: grpc

  *(Note: The exact ingress configuration may vary. An alternative is a separate Ingress for gRPC.)*

### **Epic 5: Testing & Validation**

*Goal: Verify the new gRPC service functions correctly, is performant, and is accessible.*

* **Story 5.1:** Add new integration tests to OdbDesignTests/ (e.g., GrpcServiceTests.cpp).  
  * These tests will use a C++ gRPC client (OdbDesignService::NewStub) to connect to a test instance of the server.  
  * Verify the HealthCheck RPC.  
  * Verify the GetDesign RPC returns correct data.  
* **Story 5.2:** Add streaming integration tests for GetLayerFeaturesStream, iterating through the entire stream and asserting the feature count and data integrity.  
* **Story 5.3 (External Tooling):** Use grpcurl from the command line against the running dev container to validate connectivity.  
  * grpcurl \-plaintext \-proto OdbDesignServer/grpc/odbdesign\_service.proto \-import-path OdbDesignLib/protoc/ localhost:50051 Odb.Grpc.OdbDesignService/HealthCheck  
* **Story 5.4 (Benchmark):** Add a new benchmark test to OdbDesignTests/PerformanceTests.cpp that compares the performance of the new GetLayerFeaturesStream gRPC call against the existing REST endpoint (/designs/{name}/steps/{name}/layers/{name}/features) for a large dataset.  
* **Story 5.5 (Cross-Language):** A developer on the .NET client team will test against the dev container endpoint to validate cross-language interoperability.

### **Epic 6: Continuous Benchmark & Regression Tracking**

*Goal: Automate performance benchmark execution in the CI/CD pipeline to detect and prevent performance regressions.*

* **Story 6.1: Create Benchmark Workflow File:** Create a new GitHub Actions workflow file at .github/workflows/performance-benchmarks.yml. This workflow will trigger on every push to the main branch and on every pull\_request targeting main.  
* **Story 6.2: Build Benchmarks in Release:** The workflow job will build the OdbDesignTests executable. It is **critical** that this build uses the Release configuration (e.g., cmake \--build \--preset linux-release) to ensure benchmarks run against optimized code.  
* **Story 6.3: Run and Publish Benchmarks:** The workflow will execute the OdbDesignTests binary, filtering for benchmark tests and exporting the results to a JSON file (e.g., ./build/linux-release/OdbDesignTests/OdbDesignTests \--benchmark\_filter=all \--benchmark\_out=benchmark\_results.json).  
* **Story 6.4: Integrate Benchmark Action:** Use a dedicated GitHub Action (like benchmark-action/github-action-benchmark@v1) to process benchmark\_results.json.  
  * **On push to main:** The action will store the benchmark\_results.json as the new "baseline" for the main branch.  
  * **On pull\_request:** The action will fetch the baseline from main, run the PR's benchmarks, and generate a comparison. It will be configured to post these results as a comment on the PR and fail the check if a statistically significant regression is detected in key benchmarks (like BM\_LayerFeaturesStream\_gRPC).

## **6\. Key Code Snippets (Examples)**

### **Snippet 1: OdbDesignServer/main.cpp (Refactored)**

\#include "OdbDesignServerApp.h"  
\#include "App/OdbDesignArgs.h"  
\#include "App/DesignCache.h"  
\#include "Services/OdbDesignServiceImpl.h" // \<-- NEW: Include gRPC service  
\#include \<iostream\>  
\#include \<memory\>  
\#include \<string\>  
\#include \<thread\> // \<-- NEW: For gRPC thread  
\#include \<csignal\> // \<-- NEW: For signal handling  
\#include \<atomic\> // \<-- NEW: For graceful shutdown  
\#include \<grpcpp/grpcpp.h\>  
\#include \<grpcpp/health\_check\_service\_interface.h\>  
\#include \<grpcpp/ext/proto\_server\_reflection\_plugin.h\>

// \--- NEW: Globals for graceful shutdown \---  
std::atomic\<bool\> g\_shutdown\_flag(false);  
std::unique\_ptr\<grpc::Server\> g\_grpc\_server;

void SignalHandler(int signum) {  
    std::cout \<\< "Interrupt signal (" \<\< signum \<\< ") received." \<\< std::endl;  
    g\_shutdown\_flag.store(true);

    // Stop Crow  
    auto& app \= crow::SimpleApp::instance();  
    app.stop();

    // Stop gRPC  
    if (g\_grpc\_server) {  
        g\_grpc\_server-\>Shutdown();  
    }  
}

// \--- NEW: gRPC server runner function \---  
void RunGrpcServer(const std::string& server\_address,  
                   std::shared\_ptr\<OdbDesignLib::App::DesignCache\> cache)   
{  
    OdbDesignServer::Services::OdbDesignServiceImpl service(cache);

    grpc::EnableDefaultHealthCheckService(true);  
    grpc::ServerBuilder builder;  
    builder.AddListeningPort(server\_address, grpc::InsecureServerCredentials());  
    builder.RegisterService(\&service);

    g\_grpc\_server \= builder.BuildAndStart();  
    std::cout \<\< "gRPC server listening on " \<\< server\_address \<\< std::endl;

    // Wait for shutdown signal  
    g\_grpc\_server-\>Wait();  
    std::cout \<\< "gRPC server shut down." \<\< std::endl;  
}

int main(int argc, char\* argv\[\]) {  
    // ... existing arg parsing and cache init ...  
    auto args \= Odb::App::OdbDesignArgs(argc, argv);  
    auto g\_designCache \= std::make\_shared\<OdbDesignLib::App::DesignCache\>();  
    // ... existing cache loading ...

    std::string grpc\_server\_address \= "0.0.0.0:" \+ args.getGrpcPort();

    // Register signal handlers for graceful shutdown  
    signal(SIGINT, SignalHandler);  
    signal(SIGTERM, SignalHandler);

    // Start gRPC server in a new thread  
    std::thread grpc\_thread(RunGrpcServer, grpc\_server\_address, g\_designCache);

    // Start Crow server on the main thread  
    crow::SimpleApp app;  
    OdbDesignServer::OdbDesignServerApp serverApp(app, g\_designCache);

    std::cout \<\< "Crow REST server starting on port " \<\< args.getPort() \<\< std::endl;  
    serverApp.Run(args.getPort()); // This blocks until app.stop() is called

    // Wait for gRPC thread to finish  
    grpc\_thread.join();

    std::cout \<\< "All services shut down. Exiting." \<\< std::endl;  
    return 0;  
}

### **Snippet 2: OdbDesignServer/Services/OdbDesignServiceImpl.h (New File)**

\#pragma once

\#include "grpc/odbdesign\_service.grpc.pb.h"  
\#include "App/DesignCache.h"  
\#include \<memory\>

namespace OdbDesignServer {  
namespace Services {

class OdbDesignServiceImpl final : public Odb::Grpc::OdbDesignService::Service {  
public:  
    explicit OdbDesignServiceImpl(std::shared\_ptr\<OdbDesignLib::App::DesignCache\> cache);

    grpc::Status GetDesign(grpc::ServerContext\* context,  
                           const Odb::Grpc::GetDesignRequest\* request,  
                           Odb::Lib::Protobuf::Design\* response) override;

    grpc::Status GetLayerFeaturesStream(grpc::ServerContext\* context,  
                                        const Odb::Grpc::GetLayerFeaturesRequest\* request,  
                                        grpc::ServerWriter\<Odb::Lib::Protobuf::FeatureRecord\>\* writer) override;

    grpc::Status HealthCheck(grpc::ServerContext\* context,  
                             const Odb::Grpc::HealthCheckRequest\* request,  
                             Odb::Grpc::HealthCheckResponse\* response) override;

private:  
    std::shared\_ptr\<OdbDesignLib::App::DesignCache\> m\_designCache;  
};

} // namespace Services  
} // namespace OdbDesignServer

### **Snippet 3: OdbDesignServer/Services/OdbDesignServiceImpl.cpp (New File \- Streaming Method)**

\#include "OdbDesignServiceImpl.h"  
\#include "FileModel/Design/FeaturesFile.h"  
\#include "FileModel/Design/FileArchive.h" // Include for GetDesign  
\#include \<utility\> // for std::move

namespace OdbDesignServer {  
namespace Services {

OdbDesignServiceImpl::OdbDesignServiceImpl(std::shared\_ptr\<OdbDesignLib::App::DesignCache\> cache)  
    : m\_designCache(std::move(cache)) {}

grpc::Status OdbDesignServiceImpl::GetDesign(  
    grpc::ServerContext\* context,  
    const Odb::Grpc::GetDesignRequest\* request,  
    Odb::Lib::Protobuf::Design\* response)  
{  
    try {  
        auto design \= m\_designCache-\>getDesign(request-\>design\_name());  
        if (design \== nullptr) {  
            return grpc::Status(grpc::StatusCode::NOT\_FOUND, "Design not found: " \+ request-\>design\_name());  
        }  
          
        // The FileArchive's Design object \*is\* the Protobuf message.  
        // We can copy it directly into the response.  
        \*response \= design-\>GetDesign();  
        return grpc::Status::OK;

    } catch (const std::exception& e) {  
        std::string error \= "Internal server error: " \+ std::string(e.what());  
        return grpc::Status(grpc::StatusCode::INTERNAL, error);  
    }  
}

grpc::Status OdbDesignServiceImpl::GetLayerFeaturesStream(  
    grpc::ServerContext\* context,  
    const Odb::Grpc::GetLayerFeaturesRequest\* request,  
    grpc::ServerWriter\<Odb::Lib::Protobuf::FeatureRecord\>\* writer)  
{  
    try {  
        auto design \= m\_designCache-\>getDesign(request-\>design\_name());  
        if (design \== nullptr) {  
            return grpc::Status(grpc::StatusCode::NOT\_FOUND, "Design not found");  
        }

        auto pStep \= design-\>FindStep(request-\>step\_name());  
        if (pStep \== nullptr) {  
            return grpc::Status(grpc::StatusCode::NOT\_FOUND, "Step not found");  
        }

        auto pFeaturesFile \= pStep-\>GetFeaturesFile(request-\>layer\_name());  
        if (pFeaturesFile \== nullptr) {  
            return grpc::Status(grpc::StatusCode::NOT\_FOUND, "Layer not found");  
        }

        // Performance Optimization: Reuse a single message object  
        Odb::Lib::Protobuf::FeatureRecord featureRecordMsg;

        const auto& features \= pFeaturesFile-\>GetFeatures();  
        for (const auto& pFeature : features) {  
            if (pFeature \== nullptr) continue;

            // Efficiently populate the \*reused\* message  
            // This avoids heap allocation inside the loop.  
            pFeature-\>to\_protobuf(featureRecordMsg); 

            // TODO: Implement a more direct C++ \-\> Protobuf Writer function  
            // to avoid even the intermediate to\_protobuf() call,  
            // as noted in the original gRPC plan's risk analysis.  
            // For v1, this is a reasonable starting point.

            if (\!writer-\>Write(featureRecordMsg)) {  
                // Client disconnected  
                break;  
            }

            // Clear the message for reuse  
            featureRecordMsg.Clear();  
        }

        return grpc::Status::OK;

    } catch (const std::exception& e) {  
        std::string error \= "Internal server error: " \+ std::string(e.what());  
        return grpc::Status(grpc::StatusCode::INTERNAL, error);  
    }  
}

grpc::Status OdbDesignServiceImpl::HealthCheck(  
    grpc::ServerContext\* context,  
    const Odb::Grpc::HealthCheckRequest\* request,  
    Odb::Grpc::HealthCheckResponse\* response)  
{  
    response-\>set\_status(Odb::Grpc::HealthCheckResponse::SERVING);  
    return grpc::Status::OK;  
}

} // namespace Services  
} // namespace OdbDesignServer

## **7\. Risks & Mitigations**

These are carried over from the "gRPC Migration" document, as they are accurate and critical to the project's success.

| Risk Area | Severity | Likelihood | Description & Mitigation |
| :---- | :---- | :---- | :---- |
| **API Contract Drift** | **High** | **High** | **Description:** The .NET client and this C++ server will be developed in separate repos. If the .proto files are merely copied, they will inevitably fall out of sync, breaking builds. **Mitigation:** The directory containing all .proto files (OdbDesignLib/protoc/ and OdbDesignServer/grpc/) **must** be extracted to a separate Git repository and consumed by all client/server projects as a **Git submodule**. |
| **Concurrent Server Shutdown** | Medium | Medium | **Description:** A naive std::thread::detach() for the gRPC server can lead to improper shutdown, resource leaks, or dropped connections. **Mitigation:** Implement a graceful shutdown mechanism as outlined in Epic 2 and Snippet 1\. The main() thread will catch SIGINT/SIGTERM, call app.stop() (Crow), call server-\>Shutdown() (gRPC), and then grpc\_thread.join(). This ensures both servers drain connections and exit cleanly. |
| **Deployment/Network Configuration** | Medium | Medium | **Description:** Kubernetes services, ingresses, and service meshes must be correctly configured to route HTTP/2 (gRPC) traffic, which is different from standard HTTP/1.1 (REST) traffic. **Mitigation:** Add the nginx.ingress.kubernetes.io/backend-protocol: "GRPC" annotation (or equivalent) to the Ingress resource, as shown in Story 4.6. Use grpcurl against the staging cluster's public endpoint to validate the full TLS and HTTP/2 network path *before* merging to production. |
| **Serialization Overhead in Stream** | **High** | **High** | **Description:** Calling to\_protobuf() inside the GetLayerFeaturesStream loop creates an intermediate FeatureRecord message for *every* feature. For millions of features, this results in millions of small heap allocations, creating significant CPU and memory pressure. **Mitigation:** As implemented in Story 3.4 and Snippet 3, a single FeatureRecord message object must be created *outside* the loop and *reused*. For further optimization (v2), a dedicated function should be written to serialize C++ Feature fields directly to the grpc::ServerWriter without any intermediate Protobuf message object. |

