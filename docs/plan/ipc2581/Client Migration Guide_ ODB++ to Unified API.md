# **Client Migration Guide: ODB++ to Unified API**

**Target Audience:** Client Development Team

**Subject:** Migrating from Local File Parsing to Unified Server API

## **Overview**

We are rolling out **IPC-2581 support** on the backend. To support this, the Server API now abstracts the underlying file format (ODB++ vs. IPC-2581).

**Critical Change:** The client application should **no longer** parse ODB++ directory structures, features files, or raw Protobuf FileModel structures locally. All design logic has been moved to the server to ensure format agnosticism.

Below is the guide to resolving the implementation gaps identified in odb-file-references.md.

## **1\. Resolving Implementation Gaps**

### **Gap 2.4: Symbol & Geometry Parsing**

* **Previous Concern:** "Parse ODB++ symbol strings (e.g., 'r100', 'rect50x100') manually."  
* **New Workflow:**  
  * **Do NOT** parse symbol strings locally.  
  * **Action:** Consume the gRPC stream GetPackageGeometry(designName, packageName).  
  * **Response:** The server returns a standardized Geometry object (Polygon/Contour).  
    * *ODB++ Source:* Server translates r100 \-\> Rectangle (Width=100) automatically.  
    * *IPC-2581 Source:* Server translates \<Outline\> XML tags \-\> Rectangle automatically.  
  * **Benefit:** Your 3D viewer code works for both formats without changing a single line of logic.

### **Step 1.1.2: Units of Measurement**

* **Previous Concern:** "Handle unit type from FeaturesFile.units field."  
* **New Workflow:**  
  * **Do NOT** check file headers for "Imperial" vs "Metric".  
  * **Behavior:** The API normalizes all spatial data.  
    * **Integers:** Always Nanometers.  
    * **Floats:** Always Millimeters.  
  * **Action:** Assume standardized units coming from the gRPC/REST stream.

### **Step 1.1.3: Symbol Name Resolution**

* **Previous Concern:** "Resolve symbol name from FeaturesFile.symbolNamesByName."  
* **New Workflow:**  
  * The Component model in the API now contains direct references to Package definitions.  
  * **Iterate:** design.components \-\> component.package \-\> package.geometry.  
  * The intermediate "Symbol Name" lookup (which is specific to ODB++) is handled internally by the server's linker.

### **Step 1.1.4: Integrate with Pad Conversion**

* **Previous Concern:** "Access FeaturesFile directly to find pad locations."  
* **New Workflow:**  
  * Use the GetNets() API.  
  * It returns fully resolved PinConnections. Each connection includes the **Physical Coordinate** $(X, Y)$ relative to the board origin.  
  * You do not need to calculate transforms or parse features files to find where a pin is located.

## **2\. Implementation Checklist (Client Side)**

Based on odb-file-references.md, here are the specific actions for your codebase:

| File | Action |  
| src/OdbDesign3DClient.Core/Services/Implementations/FeatureToShapeConverter.cs | Delete/Rewrite. Remove dependencies on FeaturesFile. Use standardized API geometry objects instead. |  
| src/OdbDesign3DClient.Core/Services/Implementations/OdbDesignGrpcClient.cs | Update to implement the symbol streaming method GetPackageGeometry. |  
| src/OdbDesign3DClient.Core/Services/Interfaces/IOdbDesignClient.cs | Add the symbol streaming interface method. |  
| src/OdbDesign3DClient.Core/Services/Implementations/SymbolCache.cs | Create this to cache symbols by name (as requested in Step 1.1.3) to avoid re-fetching standard geometries. |  
| src/OdbDesign3DClient.Core/ViewModels/MainViewModel.cs | Update to trigger symbol fetching logic. Remove logic that checks for .tgz vs .xml. |

## **3\. Feature Usage Guide**

### **Unified File Upload**

You can now upload **either** .tgz (ODB++) or .xml (IPC-2581) files to the same endpoint. The server identifies the format automatically.

POST /files/upload  
Content-Type: multipart/form-data  
Body: \[file\]

### **Fetching Design Data**

Regardless of the source file, the data model is identical.

**1\. Get the BOM (Components)**

GET /designs/{designName}/components

*Returns:* List of RefDes, PartName, PackageName, Side (Top/Bottom), and Center X/Y.

**2\. Get the Netlist**

GET /designs/{designName}/nets

*Returns:* List of Nets, containing Pins (RefDes \+ PinName) and their physical locations.