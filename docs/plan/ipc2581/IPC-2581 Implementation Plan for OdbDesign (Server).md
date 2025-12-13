# **IPC-2581 Implementation Plan for OdbDesign**

This document outlines the development plan to add **IPC-2581 (DPMX)** import support to the OdbDesign library.

## **1\. Format Overview: IPC-2581 vs. ODB++**

Understanding the structural differences is key to mapping the data correctly to our internal ProductModel.

| Feature | ODB++ (Current) | IPC-2581 (New) | Implementation Strategy |  
| Physical Storage | Directory hierarchy (steps/, layers/, matrix/) inside a .tgz or folder. | Single, monolithic XML file. | Parallel Import: We will parse the XML in-memory and bypass the FileModel (directory) abstraction entirely. |  
| Hierarchy | Job \-\> Step \-\> Layer | IPC-2581 \-\> Content \-\> Step | Map IPC \<Step\> tags directly to Odb::Lib::ProductModel::Step. |  
| Components | components file (text-based, row-column format). | \<BillOfMaterial\> (Logical) & \<Step\> (Physical locations). | Parse BOM for definitions, then cross-reference placement data to build Component objects. |  
| Netlist | eda/data file. | \<Ecad\>\<Data\>\<Net\> | Parse directly into Net objects. IPC uses explicit pin names (strings) rather than indices. |  
| Geometry | features file (standard primitives r100, macros). | \<Dictionary\> (Profiles) & \<Features\>. | Phase 1: Focus on Logical/Placement data. Phase 2: Map IPC \<Contour\>/\<Polygon\> to our Package geometry. |

## **2\. Architecture Guide**

We will implement a **Parallel Import Strategy**. The ProductModel::Design class will serve as the unified container for both formats.

### **Current Architecture**

graph LR  
    A\[ODB++ .tgz\] \--\> B\[FileArchive (FileModel)\]  
    B \--\> C\[Design::Build(FileArchive)\]  
    C \--\> D\[ProductModel (Ready for API)\]

### **New Architecture**

graph LR  
    A\[ODB++ .tgz\] \--\> B\[FileArchive\]  
    B \--\> D\[Design::Build(...)\]  
      
    E\[IPC-2581 .xml\] \--\> F\[Ipc2581Importer\]  
    F \--\> D\[Design::Build(...)\]  
      
    D \--\> G\[ProductModel (Unified)\]

**Key Architectural Decision:** For IPC-2581 designs, the m\_pFileModel member in Design will remain nullptr. This is intentional. It forces us to decouple the *Physical Product* (Nets, Components) from the *Source File Format* (Files, Directories).

## **3\. Necessary Changes**

### **3.1 Dependencies**

We need a high-performance XML parser. pugixml is the industry standard for C++ due to its speed and low memory footprint (DOM-based).

* **Action:** Add pugixml to vcpkg.json.  
* **Action:** Link pugixml::pugixml in OdbDesignLib/CMakeLists.txt.

### **3.2 ProductModel Updates**

The Design class currently relies heavily on FileArchive. We need to open it up for direct population.

* **Action:** Add friend class Odb::Lib::App::Ipc2581Importer; to OdbDesignLib/ProductModel/Design.h.  
* **Action:** Ensure Package::AddPin(string name) and Package::GetPin(string name) exist to support string-based IPC pin IDs (e.g., "A1", "B2").

### **3.3 New Class: Ipc2581Importer**

Create OdbDesignLib/App/Ipc2581Importer (.h/.cpp). This class is responsible for:

1. Loading the XML file.  
2. Traversing the DOM.  
3. Instantiating Component, Net, and Package objects.  
4. Injecting them into the Design instance.

### **3.4 Controller Logic**

Update OdbDesignServer/Controllers/FileUploadController.cpp to detect .xml extensions.

if (path.extension() \== ".xml") {  
    Ipc2581Importer importer(path);  
    auto design \= importer.Build();  
    // Add to DesignCache...  
}

## **4\. Data Model Integration (Mapping Strategy)**

### **4.1 Packages (Footprints)**

* **IPC Source:** \<Content\>\<Ecad\>\<CadData\>\<Package\>  
* **Target:** Odb::Lib::ProductModel::Package  
* **Mapping:** \* name attribute \-\> m\_name  
  * \<Pin\> children \-\> m\_pins

### **4.2 Components (Instances)**

* **IPC Source:** \<Content\>\<Step\>\<BillOfMaterial\>  
* **Target:** Odb::Lib::ProductModel::Component  
* **Mapping:**  
  * refDes \-\> m\_refDes  
  * partRef \-\> m\_partName  
  * layerRef ("TOP"/"BOTTOM") \-\> m\_side  
  * location (x, y, rotation) \-\> (Needs to be added to Component class if not fully exposed yet, or handled via features file mapping in Phase 2).

### **4.3 Nets (Connectivity)**

* **IPC Source:** \<Content\>\<Step\>\<Ecad\>\<Data\>\<Net\>  
* **Target:** Odb::Lib::ProductModel::Net  
* **Mapping:**  
  * name \-\> m\_name  
  * \<Node\> children \-\> PinConnection (links Component RefDes \+ Pin Name)

## **5\. XML Parsing Options**

We will use pugixml in **DOM mode** for simplicity and speed.

* **Pros:** Easy to traverse (child("Step").child("Ecad")), extremely fast.  
* **Cons:** Loads entire file into RAM.  
* **Mitigation:** IPC-2581 files can be 100MB+. pugixml overhead is approx 2-3x file size. A 100MB file uses \~300MB RAM. This is acceptable for our server environment. If files exceed 2GB, we will switch to xml\_text\_reader (SAX parser), but that is premature optimization for now.

## **6\. Serving over REST and gRPC**

Zero Changes Required for Endpoints.  
Because we are populating the unified ProductModel, the existing controllers (DesignsController.cpp) will automatically serve the correct data.

* GET /designs/{name}/components \-\> Returns the list populated by Ipc2581Importer.  
* GET /designs/{name}/nets \-\> Returns connectivity populated by Ipc2581Importer.

The client remains unaware that the source was XML.

## **7\. Opportunities for Optimization**

1. **String Interning:** IPC-2581 repeats strings often (e.g., layer names, part names). Use a string pool during import to reduce memory usage.  
2. **Lazy Loading Geometry:** If parsing the \<Profile\> (geometry) section is too slow, we can skip it during the initial upload and only parse it when the 3D Viewer specifically requests geometry for a specific package.