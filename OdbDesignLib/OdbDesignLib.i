// OdbDesignLib.i
%module PyOdbDesignLib

%{
#include "Component.h"
#include "Design.h"
#include "EdaDataFile.h"
#include "enums.h"
#include "FileModel.h"
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
%}

// to handle declspec(dllexport) in windows
%include <windows.i>
%include "export.h"

%include "Net.h"
%include "Component.h"
%include "Design.h"
%include "EdaDataFile.h"
%include "enums.h"
%include "FileModel.h"
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
