// OdbDesignApp.cpp : Defines the entry point for the application.
//
#include "OdbDesignApp.h"
#include "OdbDesign.h"

//using namespace std;

int main()
{
    OdbDesign rigidFlexOdbDesign(R"(C:\Users\nmill\OneDrive\Documents\ODB++\Samples\designodb_rigidflex)");
    auto success = rigidFlexOdbDesign.ParseDesign();
    if (!success) return 1;

    OdbDesign sampleOdbDesign(R"(C:\Users\nmill\OneDrive\Documents\ODB++\Samples\sample_design)");
    success = sampleOdbDesign.ParseDesign();
    if (!success) return 1;

    std::cout << "success" << std::endl;

    return 0;
}
