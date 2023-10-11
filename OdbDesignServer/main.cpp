// main.cpp : Source file for your target.
//

#include "OdbDesignServer.h"
#include "OdbDesignServerApp.h"

using namespace Odb::App::Server;

int main(int argc, char* argv[])
{
	OdbDesignServerApp serverApp(argc, argv);
	return (int) serverApp.Run();			
}