### **Purpose and Core Capabilities**

**OdbDesign** is a free, open-source, cross-platform C++ library specifically designed to parse ODB++ Design archives. Its core capability lies in extracting complex data from these archives and using it to build comprehensive net list product models. It serves as a bridge, allowing applications to programmatically access and utilize ODB++ printed circuit board (PCB) design data.

### **Key Features**

* **High Performance:** Written in C++ and compiled into native code, the parser avoids the overhead of interpreters or virtual machines. It is also multi-threaded to fully leverage modern multi-core CPUs for fast processing.  
* **Cross-Platform Flexibility:** The core library runs natively on Windows, Linux, and macOS. Additionally, its Dockerized version ensures it can run on virtually any platform or Kubernetes cluster.  
* **Robust Security:** The project maintains a high OpenSSF Security Scorecard rating (7.8). It is built using the latest C++ standards and compilers, and all code, dependencies, and Docker images undergo rigorous automated security scanning.  
* **Domain Expertise:** The library is maintained by a developer with over a decade of specialized experience in the PCB manufacturing hardware industry, ensuring the parser is highly efficient and aligned with industry standards.

### **Primary Utility for Engineers**

For engineers and developers working with PCB manufacturing, ODB++ archives contain critical, dense design data. OdbDesign simplifies the extraction of this data. Instead of writing custom parsers from scratch to decode the proprietary-style ODB++ format, engineers can use this library to instantly access parsed data and net list models. This accelerates the development of Electronic Design Automation (EDA), Design for Manufacturing (DFM), and PCB viewing tools.

>*The project operates under the strict [AGPL V3.0 license](https://www.gnu.org/licenses/agpl-3.0.en.html), which is a copy-left license. Use in closed-source commercial applications **requires** contacting the author to obtain a separate commercial license, while open-source projects can use the library freely under the AGPL V3.0 terms.*

### **How the REST API Integration Works**

The library isn't just for C++ developers; its REST API makes the data universally accessible.

* **Language Agnostic:** Because the parsed data is exposed via standard REST and gRPC APIs, developers can query the ODB++ data using any programming language (Python, JavaScript, Rust, etc.).  
* **Containerized Deployment:** The REST API server is packaged inside a ready-to-deploy Docker image, making it trivial to spin up locally using Docker Compose or deploy onto scalable Kubernetes (k8s) clusters.  
* **Decoupled Architecture:** The architecture allows you to run the heavy, multi-threaded C++ parser on a high-performance server or workstation.  
* **Lightweight Client Access:** Because the heavy lifting is done server-side, low-power devices—such as mobile phones, web browsers, or Raspberry Pis—can easily request and consume the parsed ODB++ data over the network without needing substantial local processing power.
