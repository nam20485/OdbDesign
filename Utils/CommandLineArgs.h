#pragma once

#include <vector>
#include <string>
#include "export.h"


namespace Utils
{
	class DECLSPEC CommandLineArgs
	{
	public:
		CommandLineArgs(int argc, char* argv[]);
		CommandLineArgs(const std::vector<std::string>& vecArgv);

	protected:
		std::vector<std::string> m_vecArguments;

	};
}
