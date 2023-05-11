# CMakeList.txt : CMake project for OdbDesignLib, include source and define
# project specific logic here.
#

add_library(OdbDesignLib SHARED "OdbDesignLib.cpp" "OdbDesignLib.h" "export.h" "ComponentLayer.cpp" "ComponentLayer.h" "EdaData.cpp" "EdaData.h" "Layer.cpp" "Layer.h" "Netlist.cpp" "Netlist.h" "OdbDesign.cpp" "OdbDesign.h" "Step.cpp" "Step.h" "string_trim.h")

# state that anybody linking to us needs to include the current source dir
# to find MathFunctions.h, while we don't.
target_include_directories(OdbDesignLib
                           INTERFACE
                            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
                            $<INSTALL_INTERFACE:include>
                           )

# define the symbol stating we are using the declspec(dllexport) when
# building on windows
target_compile_definitions(OdbDesignLib PRIVATE "EXPORTING_ODBDESIGNLIB")
