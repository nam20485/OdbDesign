// main.cpp : Source file for your target.
//

#include "OdbDesignServer.h"
#include "OdbDesignServerApp.h"


int main(int argc, char* argv[])
{
	Odb::App::Server::OdbDesignServerApp serverApp(argc, argv);
	return (int) serverApp.Run();			
}