# CMakeList.txt : CMake project for OdbDesignLib, include source and define
# project specific logic here.
#

add_library(OdbDesign SHARED   "export.h" "ComponentLayer.cpp" "ComponentLayer.h" "EdaData.cpp" "EdaData.h" "Layer.cpp" "Layer.h" "Netlist.cpp" "Netlist.h" "FileModel.cpp" "FileModel.h" "Step.cpp" "Step.h" "string_trim.h" "macros.h" "Net.h" "Net.cpp" "Component.h" "Component.cpp" "Pin.h" "Pin.cpp" "PinConnection.h" "PinConnection.cpp" "Package.h" "Package.cpp" "Part.h" "Part.cpp" "Via.h" "Via.cpp" "Design.h" "Design.cpp")

# state that anybody linking to us needs to include the current source dir
# to find MathFunctions.h, while we don't.
target_include_directories(OdbDesign
                           INTERFACE
                            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
                            $<INSTALL_INTERFACE:include>
                           )
