#include "CommandLineArgs.h"


namespace Utils
{
	CommandLineArgs::CommandLineArgs(int argc, char* argv[])
	{
		// save arguments in std::string vector
		for (int i = 0; i < argc; i++)
		{
			m_vecArguments.push_back(argv[i]);
		}
	}

	CommandLineArgs::CommandLineArgs(const std::vector<std::string>& vecArgv)
		: m_vecArguments(vecArgv)
	{
	}
}