// OdbDesignServer.cpp : Source file for your target.
//

#include "OdbDesignServer.h"
#include "OdbDesignServerApp.h"


int main(int argc, char* argv[])
{
	return (int) Odb::App::Server::OdbDesignServerApp(argc, argv).Run();				
}