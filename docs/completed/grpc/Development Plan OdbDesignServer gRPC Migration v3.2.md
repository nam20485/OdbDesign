# **Development Plan: OdbDesignServer gRPC Migration**

Version: 3.2  
Date: 2025-10-29  
Project Lead: \[Project Lead Name\]

## **1\. Project Summary**

This document outlines the development plan to augment the existing C++ OdbDesignServer with a high-performance gRPC API. The current server uses the Crow C++ framework to serve ODB++ data as JSON. This data is generated from an intermediate Protobuf representation (Odb::Lib::Protobuf), which is itself serialized from the core C++ models. This C++ Model \-\> Protobuf \-\> JSON process, especially the MessageToJsonString() call, is a major performance bottleneck. It results in multi-megabyte JSON payloads that are slow to generate, transmit, and parse, leading to a poor user experience in the web client.

**Primary Goal:** To implement a parallel gRPC service *within* the existing OdbDesignServer application. This new service will expose the core ODB++ data directly as Protobuf messages over HTTP/2, bypassing the costly and redundant JSON conversion entirely. This provides the high-throughput, low-latency data stream required by the new .NET desktop client.

## **2\. Technical Strategy**

1. **Augment, Don't Replace:** The new gRPC service will run on a separate port (e.g., 50051\) within the same server process as the existing Crow REST API (on port 8888). This is achieved by launching the gRPC server on its own thread, which guarantees 100% backward compatibility for existing REST-based clients and allows for a phased rollout of the new desktop application.  
2. **Shared Protobuf Contract:** The existing .proto message definitions (e.g., featuresfile.proto, layerdirectory.proto, symbolname.proto) will be reused. A **new** odb\_api\_v1.proto file will be created to define the service contract and its RPC (Remote Procedure Call) methods. This new file will import the existing message definitions, ensuring a single source of truth for data structures.  
3. **Direct C++ to Protobuf Streaming:** The new service implementation will leverage the existing DesignCache to fetch the native C++ data models (e.g., FeaturesFile). It will then call their existing to\_protobuf() methods (defined by the IProtoBuffable interface) and write the resulting Odb::Lib::Protobuf messages *directly* to the gRPC stream.  
   This architecture eliminates the two worst bottlenecks in the system:  
   * **Server-Side:** No more MessageToJsonString(). We avoid the massive CPU cost and memory allocation of serializing binary Protobuf data into a giant JSON string.  
   * **Client-Side:** No more JSON parsing. The .NET client will receive binary data and deserialize it directly into C\# objects, which is orders of magnitude faster than parsing text.

## **3\. Development Phases & Features**

### **Phase 0: Project Setup & Dependencies**

* **Goal:** Integrate gRPC and its dependencies into the C++ project and configure the build system (CMake/vcpkg) to compile the new gRPC service and its generated code.  
* **Tasks:**  
  * \[ \] **Establish Shared Protobuf Contract Repository:**  
    * Initialize the OdbDesignLib/protoc directory as a Git submodule pointing to a new, dedicated repository (e.g., odb-protobuf-definitions).  
    * This repository will become the single source of truth for all .proto files.  
    * The new .NET client repository will also consume this repository as a submodule.  
  * \[ \] **Update CI Pipeline:** Modify all GitHub Actions workflows (.github/workflows) to correctly initialize Git submodules using git submodule update \--init \--recursive before building.  
  * \[ \] **Update vcpkg.json:** Add grpc to the dependency list. vcpkg will automatically handle building gRPC, its dependencies (like Protobuf, c-ares, re2, abseil), and making them available to CMake. Ensure vcpkg is run in manifest mode.  
    {  
      "dependencies": \[  
        "crow",  
        "protobuf",  
        "grpc"  
      \]  
    }

  * \[ \] **Update Root CMakeLists.txt:** Add find\_package(gRPC CONFIG REQUIRED) *after* the existing find\_package(Protobuf). This ensures CMake can locate the gRPC libraries and compiler plugin provided by vcpkg.  
  * \[ \] **Update OdbDesignServer/CMakeLists.txt:** Link the main OdbDesignServer executable target against the necessary gRPC and threading libraries.  
    target\_link\_libraries(OdbDesignServer PRIVATE  
        ... \# (existing libraries)  
        grpc::grpc++  
        grpc::grpc++\_reflection  
        protobuf::libprotobuf  
        absl::base  
        absl::strings  
        Threads::Threads  \# For std::thread  
    )

  * \[ \] **Update OdbDesignLib/CMakeLists.txt (Proto Compilation):** Modify the existing protoc custom command to also generate the gRPC service stub code.  
    * Find the gRPC C++ plugin: find\_program(GRPC\_CPP\_PLUGIN\_EXECUTABLE grpc\_cpp\_plugin)  
    * Check that it's found: if(NOT GRPC\_CPP\_PLUGIN\_EXECUTABLE) ... endif()  
    * Add the \--grpc\_out directive and the plugin path to the protoc command for the new odb\_api\_v1.proto file:  
      add\_custom\_command(  
          ...  
          COMMAND ${Protobuf\_PROTOC\_EXECUTABLE}  
          ARGS  
              \--proto\_path=${CMAKE\_CURRENT\_SOURCE\_DIR}/protoc  
              \--cpp\_out=${CMAKE\_CURRENT\_BINARY\_DIR}/ProtoBuf  
              \--grpc\_out=${CMAKE\_CURRENT\_BINARY\_DIR}/ProtoBuf  \# \<-- NEW  
              \--plugin=protoc-gen-grpc=${GRPC\_CPP\_PLUGIN\_EXECUTABLE}  \# \<-- NEW  
              ${proto\_file}  
          ...  
      )

  * \[ \] **Benchmark Current REST API:** Using a large test file (\>500k features), script a client to measure and record: 1\) Time-to-Last-Byte (TTLB) for the /features JSON endpoint. 2\) Peak server CPU and memory usage during the request. 3\) Total JSON payload size in megabytes.  
  * \[ \] **Sanity Check: "Hello World" gRPC Service:**  
    * Create a temporary hello.proto with a service Greeter { rpc SayHello(HelloRequest) returns (HelloReply); }.  
    * Implement GreeterServiceImpl in main.cpp.  
    * Start this temporary service *before* integrating the full OdbDesignGrpcServiceImpl.  
    * Use a command-line tool like grpcurl to connect to localhost:50051 and call SayHello.  
    * **This task validates the entire build toolchain (vcpkg, CMake, protoc, linking) is working correctly before mixing in the complex ODB logic.**

### **Phase 1: API Definition (The .proto Contract)**

* **Goal:** Define the formal gRPC API contract that the client and server will share, reusing existing messages.  
* **Epic: Create odb\_api\_v1.proto**  
  * **User Story 1:** As a Developer, I want to define the core gRPC service contract in a new .proto file to manage all RPC methods.  
    * **Tasks:**  
      * \[ \] Create new file: OdbDesignLib/protoc/odb\_api\_v1.proto.  
      * \[ \] Add syntax \= "proto3"; and package Odb.Lib.Protobuf;.  
      * \[ \] Add import "google/protobuf/empty.proto"; (for parameter-less requests).  
      * \[ \] Add import "grpc/health/v1/health.proto"; (for K8s health checks).  
      * \[ \] Add import statements for all required data models:  
        import "layerdirectory.proto";  
        import "featuresfile.proto";  
        import "symbolname.proto";  
        import "design.proto";

      * \[ \] Define the main service: service OdbDesignApi { ... }.  
  * **User Story 2:** As a Developer, I want to define unary RPCs for design and step navigation.  
    * **Tasks:**  
      * \[ \] Define rpc GetAvailableDesigns(google.protobuf.Empty) returns (AvailableDesignsResponse);  
      * \[ \] Define message AvailableDesignsResponse { repeated string design\_names \= 1; }  
      * \[ \] Define rpc GetStepList(DesignRequest) returns (StepListResponse);  
      * \[ \] Define message DesignRequest { string design\_name \= 1; }  
      * \[ \] Define message StepListResponse { repeated string step\_names \= 1; }  
  * **User Story 3:** As a Developer, I want to define a unary RPC for fetching the layer list for a given step.  
    * **Tasks:**  
      * \[ \] Define message StepLayerRequest { string design\_name \= 1; string step\_name \= 2; }.  
      * \[ \] Add the RPC: rpc GetLayerList(StepLayerRequest) returns (LayerDirectory);.  
  * **User Story 4:** As a Developer, I want to define a **server-streaming** RPC for efficiently fetching all features for a given layer.  
    * **Tasks:**  
      * \[ \] Define message LayerFeatureRequest { string design\_name \= 1; string step\_name \= 2; string layer\_name \= 3; }.  
      * \[ \] Add the RPC: rpc GetLayerFeaturesStream(LayerFeatureRequest) returns (stream FeatureRecord);.  
      * *(Note: This is the most critical RPC. By streaming FeatureRecord one-by-one, the client can begin rendering the first features in milliseconds, creating a perceived load time that is near-instantaneous. This avoids the high memory overhead of buffering the entire layer's feature set on either the client or server and is vastly superior to a unary RPC returning a single FeaturesFile message.)*  
  * **User Story 5:** As a Developer, I want to define a unary RPC for fetching the symbol definitions for a given step.  
    * **Tasks:**  
      * \[ \] Add the RPC: rpc GetSymbols(StepLayerRequest) returns (SymbolDirectory);.

### **Phase 2: Service Implementation (C++)**

* **Goal:** Implement the C++ logic for the new gRPC service, linking it to the existing DesignCache and authentication.  
* **Epic: Implement OdbDesignGrpcServiceImpl**  
  * **User Story 1:** As a Developer, I want to create a C++ service class that implements the generated gRPC service interface.  
    * **Tasks:**  
      * \[ \] Create new files: OdbDesignServer/Services/OdbDesignGrpcServiceImpl.h and .cpp.  
      * \[ \] In the header, include the generated odb\_api\_v1.grpc.pb.h file.  
      * \[ \] The class OdbDesignGrpcServiceImpl must inherit from Odb::Lib::Protobuf::OdbDesignApi::Service.  
      * \[ \] The class must accept a std::shared\_ptr\<DesignCache\> in its constructor and store it, just as the Crow controllers do.  
  * **User Story 2:** As a Developer, I want to implement the GetAvailableDesigns and GetStepList unary RPC methods.  
    * **Tasks:**  
      * \[ \] Override the virtual functions: grpc::Status GetAvailableDesigns(...) override; and grpc::Status GetStepList(...) override;.  
      * \[ \] In GetAvailableDesigns, call m\_pDesignCache-\>GetDesignNames() (or equivalent) which returns std::vector\<std::string\>.  
      * \[ \] Iterate the vector and use response-\>add\_design\_names(designName) to populate the reply.  
      * \[ \] For GetStepList, get the Odb::ProductModel::Design object from the cache.  
      * \[ \] Call pDesign-\>GetStepNames() (or equivalent), iterate the resulting std::vector\<std::string\>, and use response-\>add\_step\_names(stepName) to populate the reply.  
      * \[ \] Add nullptr checks and try...catch blocks as defined in US 5\.  
  * **User Story 3:** As a Developer, I want to implement the GetLayerList and GetSymbols unary RPC methods.  
    * **Tasks:**  
      * \[ \] Override the virtual functions: grpc::Status GetLayerList(...) override; and grpc::Status GetSymbols(...) override;.  
      * \[ \] Inside each, retrieve the design\_name and step\_name from the request object.  
      * \[ \] Call m\_pDesignCache-\>GetDesign(...) and navigate to the C++ LayerDirectory or SymbolsDirectory object.  
      * \[ \] **Key Step:** Call the existing to\_protobuf(response) method (e.g., pLayerDirectory-\>to\_protobuf(response);). This populates the response message directly using the existing, tested logic.  
      * \[ \] Ensure all string conversions (e.g., from request-\>design\_name()) are safely handled (e.g., std::string copy) to avoid lifetime issues.  
      * \[ \] Implement robust nullptr checks and return appropriate gRPC status codes (e.g., grpc::StatusCode::NOT\_FOUND if pDesign \== nullptr).  
      * \[ \] Return grpc::Status::OK on success.  
  * **User Story 4:** As a Developer, I want to implement the high-performance GetLayerFeaturesStream server-streaming RPC.  
    * **Tasks:**  
      * \[ \] Override the virtual function: grpc::Status GetLayerFeaturesStream(grpc::ServerContext\* context, const LayerFeatureRequest\* request, grpc::ServerWriter\<FeatureRecord\>\* writer) override;.  
      * \[ \] Retrieve design, step, and layer names from the request.  
      * \[ \] Get the C++ FeaturesFile object from the DesignCache. Handle NOT\_FOUND errors.  
      * \[ \] Get the feature list: const auto& features \= pFeaturesFile-\>GetFeatureRecords();.  
      * \[ \] **Key Step:** Iterate this C++ vector and stream results:  
        Odb::Lib::Protobuf::FeatureRecord protoFeature; // Declare once  
        for (const auto\* featureRecord : features) {  
            // Check for client cancellation  
            if (context-\>IsCancelled()) {  
                return grpc::Status(grpc::StatusCode::CANCELLED, "Client disconnected");  
            }

            protoFeature.Clear(); // Reuse the object to avoid allocations  
            featureRecord-\>to\_protobuf(\&protoFeature);

            if (\!writer-\>Write(protoFeature)) {  
                // Write failed, client likely disconnected  
                return grpc::Status(grpc::StatusCode::CANCELLED, "Client disconnected");  
            }  
        }

      * \[ \] Return grpc::Status::OK after the loop completes successfully.  
  * **User Story 5:** As a Developer, I want all gRPC service methods to be robust against C++ exceptions and invalid input.  
    * **Tasks:**  
      * \[ \] Wrap the entire body of *all* overridden service methods in a try...catch block.  
      * \[ \] **Inside the try block, implement granular error checking** to return specific gRPC codes (e.g., grpc::StatusCode::NOT\_FOUND for missing designs, grpc::StatusCode::INVALID\_ARGUMENT for empty request fields).  
        try {  
            if (request-\>design\_name().empty()) {  
                return grpc::Status(grpc::StatusCode::INVALID\_ARGUMENT, "Design name is required");  
            }  
            // ...  
            auto pDesign \= m\_pDesignCache-\>GetDesign(request-\>design\_name());  
            if (pDesign \== nullptr) {  
                return grpc::Status(grpc::StatusCode::NOT\_FOUND, "Design not found");  
            }  
            // ... (all other implementation logic) ...  
            return grpc::Status::OK;  
        } catch (const std::exception& e) {  
            CROW\_LOG\_ERROR \<\< "gRPC service error: " \<\< e.what();  
            return grpc::Status(grpc::StatusCode::INTERNAL, e.what());  
        } catch (...) {  
            CROW\_LOG\_ERROR \<\< "gRPC service unknown error";  
            return grpc::Status(grpc::StatusCode::UNKNOWN, "An unknown error occurred");  
        }

      * This is critical for server stability and client-side error handling.  
  * **User Story 6: Implement gRPC Authentication**  
    * **Tasks:**  
      * \[ \] Create new files OdbDesignServer/Services/AuthInterceptor.h and .cpp.  
      * \[ \] Implement a factory: class AuthInterceptorFactory : public grpc::ServerInterceptorFactoryInterface { ... }.  
      * \[ \] Implement the interceptor: class AuthInterceptor : public grpc::ServerInterceptor { ... }.  
      * \[ \] Inside the Intercept method, extract auth metadata: auto auth\_md \= context-\>client\_metadata().find("authorization");.  
      * \[ \] Check if auth\_md is valid, parse the Basic ... token.  
      * \[ \] **Refactor:** Create new shared utility files OdbDesignLib/Utils/AuthHelper.h and OdbDesignLib/Utils/AuthHelper.cpp. Move the core logic for ParseBasicAuthHeader and ValidateCredentials into this new utility.  
      * \[ \] The Crow middleware (BasicRequestAuthentication) will be updated to call these new shared functions.  
      * \[ \] The gRPC AuthInterceptor will also call these new shared functions. This ensures 100% logic re-use.  
      * \[ \] If auth is valid, return kContinue; to proceed with the call.  
      * \[ \] If invalid or missing, immediately return grpc::Status(grpc::StatusCode::UNAUTHENTICATED, "Invalid credentials");.  
  * **User Story 7: Implement gRPC Health Checking Service**  
    * **Tasks:**  
      * \[ \] Add import "grpc/health/v1/health.proto"; to odb\_api\_v1.proto. (Done in Phase 1).  
      * \[ \] Create new files OdbDesignServer/Services/HealthCheckServiceImpl.h and .cpp.  
      * \[ \] The class HealthCheckServiceImpl must inherit from grpc::health::v1::Health::Service.  
      * \[ \] It should take the std::shared\_ptr\<DesignCache\> in its constructor.  
      * \[ \] Implement the Check method. It should return SERVING if the DesignCache is initialized and NOT\_SERVING otherwise.  
      * \[ \] The Watch method can return UNIMPLEMENTED for now.

### **Phase 3: Server Integration & Launch**

* **Goal:** Start the gRPC server in a separate thread to run concurrently with the Crow REST server, using secure credentials.  
* **Epic: Update OdbDesignServer/main.cpp**  
  * **User Story 1:** As a Developer, I want to instantiate and run the new gRPC server in a background thread.  
    * **Tasks:**  
      * \[ \] Include \<thread\>, \<atomic\>, \<csignal\>, and the new OdbDesignGrpcServiceImpl.h, AuthInterceptor.h, and HealthCheckServiceImpl.h in main.cpp.  
      * \[ \] Create a new helper function: void RunGrpcServer(std::shared\_ptr\<Odb::App::DesignCache\> pDesignCache, const Odb::App::OdbDesignArgs& args, std::atomic\<bool\>& shutdown\_flag).  
      * \[ \] Inside this function, instantiate the services: OdbDesignGrpcServiceImpl odb\_service(pDesignCache); and HealthCheckServiceImpl health\_service(pDesignCache);.  
      * \[ \] Implement a ReadFile helper function to load cert/key files.  
      * \[ \] Add logic to build **secure** credentials:  
        std::shared\_ptr\<grpc::ServerCredentials\> pCredentials;  
        if (args.serverKeyPath.empty() || args.serverCertPath.empty()) {  
            CROW\_LOG\_WARNING \<\< "Starting gRPC with Insecure credentials (dev only)";  
            pCredentials \= grpc::InsecureServerCredentials();  
        } else {  
            CROW\_LOG\_INFO \<\< "Starting gRPC with Secure credentials";  
            grpc::SslServerCredentialsOptions::PemKeyCertPair pkcp \= {  
                ReadFile(args.serverKeyPath), ReadFile(args.serverCertPath)  
            };  
            grpc::SslServerCredentialsOptions ssl\_opts;  
            ssl\_opts.pem\_key\_cert\_pairs.push\_back(pkcp);  
            pCredentials \= grpc::SslServerCredentials(ssl\_opts);  
        }

      * \[ \] **Add gRPC port to OdbDesignArgs:** Add a new std::string grpcListenAddress \= "0.0.0.0:50051"; to OdbDesignArgs.h and parse it from command-line args (e.g., \--grpc-port) in OdbDesignArgs.cpp.  
      * \[ \] Use grpc::ServerBuilder to configure and register the service and interceptor.  
        std::string server\_address \= args.grpcListenAddress;  
        grpc::ServerBuilder builder;  
        builder.AddListeningPort(server\_address, pCredentials);  
        builder.RegisterService(\&odb\_service);  
        builder.RegisterService(\&health\_service); // Register health service

        // Register auth interceptor factory  
        std::vector\<std::unique\_ptr\<grpc::experimental::ServerInterceptorFactoryInterface\>\> creators;  
        creators.push\_back(std::make\_unique\<AuthInterceptorFactory\>());  
        builder.experimental().SetInterceptorCreators(std::move(creators));

        std::unique\_ptr\<grpc::Server\> server(builder.BuildAndStart());  
        CROW\_LOG\_INFO \<\< "gRPC server listening on " \<\< server\_address;

        // Wait for shutdown signal  
        while (\!shutdown\_flag) { std::this\_thread::sleep\_for(std::chrono::seconds(1)); }  
        CROW\_LOG\_WARNING \<\< "gRPC server shutting down...";  
        server-\>Shutdown();

      * \[ \] In main(), set up a std::atomic\<bool\> shutdown\_flag(false).  
      * \[ \] Set up a signal handler for SIGINT and SIGTERM.  
      * \[ \] In the signal handler, set shutdown\_flag \= true; and call app.stop() to unblock the main thread.  
      * \[ \] Launch the gRPC thread: std::thread grpc\_thread(RunGrpcServer, app.getDesignCache(), args, std::ref(shutdown\_flag));  
      * \[ \] Run the Crow app (blocking): app.Run();.  
      * \[ \] After app.Run() returns, call grpc\_thread.join(); to wait for the gRPC server to shut down cleanly.

### **Phase 4: Testing & Validation**

* **Goal:** Verify the gRPC service functions correctly, meets performance targets, and integrates with the new client.  
* **Tasks:**  
  * \[ \] **Unit Testing (OdbDesignTests):**  
    * \[ \] Create OdbDesignGrpcServiceImpl\_tests.cpp.  
    * \[ \] Mock the DesignCache using gMock to return sample C++ models (FeaturesFile, LayerDirectory).  
    * \[ \] Test GetLayerList: Call the method with a mock context and verify the response Protobuf message is populated correctly.  
    * \[ \] Test GetSymbols: Call and verify the response is correct.  
    * \[ \] Test Error Handling: Test all services with a mock cache that returns nullptr or throws an exception. Verify they return the correct gRPC status codes (NOT\_FOUND, INTERNAL).  
    * \[ \] Test AuthInterceptor: Create tests for the interceptor logic with mock ServerContext objects, passing valid, invalid, and missing auth metadata.  
  * \[ \] **Integration Testing (OdbDesignTests):**  
    * \[ \] Add a new CTest target (grpc\_integration\_tests) that builds a small C++ gRPC client executable.  
    * \[ \] This test will run *after* the OdbDesignServer is started. It will connect to localhost:50051.  
    * \[ \] Use grpcurl for early, language-agnostic integration testing from the command line:  
      \# Example grpcurl (plaintext)  
      grpcurl \-plaintext \-d '{"design\_name": "my\_design", "step\_name": "my\_step"}' \\  
              \-H "Authorization: Basic dXNlcjpwYXNz" \\  
              localhost:50051 Odb.Lib.Protobuf.OdbDesignApi/GetLayerList

    * \[ \] **Test 1 (Auth):** Connect with (a) no credentials (fail), (b) bad credentials (fail), and (c) good credentials (pass).  
    * \[ \] **Test 2 (Unary):** Call GetLayerList and verify the layer names received match the test data.  
    * \[ \] **Test 3 (Streaming):** Call GetLayerFeaturesStream and count the number of FeatureRecord messages received, verifying it matches the test data.  
    * \[ \] **Test 4 (Client Cancel):** Start streaming GetLayerFeaturesStream from a test client, then immediately disconnect the client. Verify the server-side log shows "Client disconnected" and the server process remains stable.  
  * \[ \] **Performance Testing:**  
    * \[ \] Use a large design file (\> 500k features) in the DesignCache.  
    * \[ \] Write a client (C++ or .NET) to measure:  
      * REST: Time-to-Last-Byte for .../features (JSON) (using data from Phase 0 benchmark).  
      * gRPC: Time-to-Last-Byte for GetLayerFeaturesStream (Protobuf).  
      * gRPC: Time-to-First-Feature (TTFF) for the stream.  
    * \[ \] **Measure Payload Size:** Record the total bytes transferred for the REST JSON response vs. the sum of all gRPC messages for the same layer.  
    * \[ \] Measure server-side CPU and peak Memory usage during both tests.  
    * **Success Criteria:**  
      * **TTLB (Time to Last Byte):** gRPC stream completion must be \< 25% of the total time for the REST JSON download (e.g., 5s REST vs \< 1.25s gRPC).  
      * **TTFF (Time to First Feature):** The client must receive the *first* FeatureRecord message in \< 100ms.  
      * **Payload Size:** Total bytes transferred via gRPC must be \< 10% of the equivalent JSON payload size.  
      * **Server Resources:** Server-side memory usage per request must be measurably lower (no large JSON string allocation).

### **Phase 5: Deployment & CI/CD**

* **Goal:** Update all deployment and CI configurations to correctly build, test, and expose the new gRPC service.  
* **Tasks:**  
  * \[ \] **Update Dockerfile:** Add EXPOSE 50051 alongside the existing EXPOSE 8888\.  
  * \[ \] **Update deploy/kube/OdbDesignServer/service.yaml:** Add the new port definition to the Kubernetes service.  
    ports:  
      \- name: http  
        port: 8888  
        targetPort: 8888  
        protocol: TCP  
      \- name: grpc  
        port: 50051  
        targetPort: 50051  
        protocol: TCP

  * \[ \] **Update Ingress Controller Config:** This is critical. The Kubernetes Ingress (e.g., NGINX, Traefik) must be configured to handle HTTP/2 and gRPC. For NGINX-Ingress, this requires the nginx.ingress.kubernetes.io/backend-protocol: "GRPC" annotation on the Ingress resource definition. The path-based routing must be updated to direct gRPC traffic to the grpc port.  
  * \[ \] **Update Readiness/Liveness Probes:** The existing HTTP probe for Crow is still valid. Add a *new* probe for the gRPC service.  
    * \[ \] Implement the standard grpc.health.v1.Health service in the gRPC server. (As defined in Phase 2, US7).  
    * \[ \] Configure the Kubernetes deployment (deployment.yaml) to use a grpc\_health\_probe on port 50051 for its readiness and liveness checks.  
  * \[ \] **Update github/workflows:** Ensure the new grpc\_integration\_tests CTest target is built and executed as part of the main CI pipeline.

## **4\. Risks and Mitigation**

| Risk | Likelihood | Impact | Mitigation Strategy |
| :---- | :---- | :---- | :---- |
| **Build Complexity** | High | Medium | Integrating gRPC via vcpkg and CMake can be fragile, with a complex dependency graph (Abseil, Protobuf, c-ares). **Mitigation:** Strictly enforce using the official vcpkg package for grpc. Ensure all developers use the *identical* vcpkg.json manifest. Heavily utilize the devcontainer to provide a 100% consistent build environment. Ensure protoc compiler version and libprotobuf runtime version are identical. |
| **Authentication Logic** | Medium | High | Sharing stateful authentication logic between Crow (HTTP 1.1) and gRPC (HTTP/2) can be difficult. **Mitigation:** **Refactor** the core validation logic (bool IsValid(user, pass)) from BasicRequestAuthentication into a shared, stateless utility function (OdbDesignLib/Utils/AuthHelper.h). Both the Crow middleware and the new gRPC AuthInterceptor will call this common function, ensuring a single source of truth for auth logic. |
| **Per-message allocation/serialization overhead in stream** | Low | High | The to\_protobuf() call inside the GetLayerFeaturesStream loop creates an intermediate FeatureRecord message. For millions of features, this can result in millions of small heap allocations and serializations, creating CPU and memory pressure. |
| **Deployment/Network Configuration** | Medium | Medium | Kubernetes services, ingresses, and service meshes (if used) must be correctly configured to route HTTP/2 (gRPC) traffic, which is different from standard HTTP/1.1 (REST) traffic. **Mitigation:** Add the nginx.ingress.kubernetes.io/backend-protocol: "GRPC" annotation (or equivalent) to the Ingress resource. Use grpcurl with the \-cacert flag against the staging cluster's public endpoint to validate the full TLS and HTTP/2 network path *before* merging to production. |
| **Concurrent Server Shutdown** | Medium | Medium | A simple std::thread::detach() for the gRPC server is a "fire and forget" solution that can lead to improper shutdown, resource leaks, or dropped connections. **Mitigation:** Implement a graceful shutdown mechanism. Use a shared std::atomic\<bool\> shutdown\_flag. The main() thread will catch SIGINT/SIGTERM, set the flag, call app.stop(), call server-\>Shutdown(), and then grpc\_thread.join(). This ensures both servers drain connections and exit cleanly. |
| **API Contract Drift** | High | High | The .NET client and this C++ server will be developed in separate repos. If the .proto files are copied, they will inevitably fall out of sync, breaking the build. |

