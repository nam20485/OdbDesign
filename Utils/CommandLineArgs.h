#pragma once

#include <vector>
#include <string>
#include "utils_export.h"


namespace Utils
{
	class UTILS_EXPORT CommandLineArgs
	{
	public:
		CommandLineArgs(int argc, char* argv[]);
		CommandLineArgs(const std::vector<std::string>& vecArgv);

	protected:
		std::vector<std::string> m_vecArguments;

	};
}
