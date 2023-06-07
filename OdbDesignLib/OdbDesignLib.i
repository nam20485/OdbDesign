// OdbDesignLib.i
%module PyOdbDesignLib

%header %{
#include "Component.h"
#include "Design.h"
#include "EdaDataFile.h"
#include "enums.h"
#include "FileArchive.h"
#include "LayerDirectory.h"
#include "ComponentLayerDirectory.h"
#include "Net.h"
#include "NetlistFile.h"
#include "Package.h"
#include "Part.h"
#include "Pin.h"
#include "PinConnection.h"
#include "StepDirectory.h"
#include "string_trim.h"
#include "Via.h"

/* some objects' namespaces aren't included correctly */
using PinConnection = Odb::Lib::ProductModel::PinConnection;
using Component = Odb::Lib::ProductModel::Component;
using Pin = Odb::Lib::ProductModel::Pin;
using Package = Odb::Lib::ProductModel::Package;
using Component = Odb::Lib::ProductModel::Component;
using namespace Odb::Lib;
//using BoardSide = Odb::Lib::BoardSide;
using StepDirectory = Odb::Lib::FileModel::Design::StepDirectory;
%}

// for std::string
%include <std_string.i>

// std::vector
%include <std_vector.i>

// to handle declspec(dllexport) on Windows
%include <windows.i>
%include "export.h"

// code definitions
%include "Net.h"
%include "Component.h"
%include "Design.h"
%include "EdaDataFile.h"
%include "enums.h"
%include "FileArchive.h"
%include "LayerDirectory.h"
%include "ComponentLayerDirectory.h"
%include "Net.h"
%include "NetlistFile.h"
%include "Package.h"
%include "Part.h"
%include "Pin.h"
%include "PinConnection.h"
%include "StepDirectory.h"
%include "string_trim.h"
%include "Via.h"
