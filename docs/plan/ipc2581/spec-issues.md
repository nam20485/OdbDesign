# Specification Issues

1. DO NOT build a std::shared_ptr<Odb::Lib::ProductModel::Design> directly (w/o FileArchive)

- Instead, mirror and create the path that a FileArchive would take to build a Design.
- Instead of FileArchive, create a IpcXmlFile class that can parse the IPC-2581 XML file.
- "Overload" Design class so that it can also be build from an IpcXmlFile.

3. DO NOT create friends or Addxxx() public methods in the Design class.
- INSTEAD, mirror the way FileArchive is used to build a Design.

DO NOT pre-load Ipc2581 files. ODB++ does not pre-load files, so these should not be pre-loaded either.

Open Questions:

1. Yes, a FileModel is the file representation of a design class. A FileArchive is one filemodle type and IpcXmlFile is another. So yes /filemodels/<name> should also load ipc2581 xml files. If the stream matches, then it should be loaded, whether its an ODB++ or an IPC-2581 file.

2. Keep the existing behavior, DO NOT pre-load. Keep lazy loading behavior.

3. Not yet. I will obtain some IPC-2581 files later.

Add some test coverage for the new behavior.
