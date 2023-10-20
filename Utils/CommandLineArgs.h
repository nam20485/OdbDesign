#pragma once

#include <vector>
#include <string>
#include "utils_export.h"
#include <stdexcept>
#include <map>
#include <filesystem>


namespace Utils
{
	class UTILS_EXPORT CommandLineArgs
	{
	public:
		CommandLineArgs(int argc, char* argv[]);
		CommandLineArgs(const std::vector<std::string>& vecArgv);

		bool boolArg(const std::string& name, bool default) const;
		std::string stringArg(const std::string& name, const std::string& default) const;
		int intArg(const std::string& name, int default) const;
		double doubleArg(const std::string& name, double default) const;

		std::string executable() const;
		std::filesystem::path executableDirectory() const;
		std::filesystem::path executableName() const;

	protected:
		std::vector<std::string> m_vecArguments;
		std::map<std::string, std::string> m_mapArguments;

		std::string getArgValue(const std::string& name) const;

		void parse();

		const char* EXECUTABLE_ARG_NAME = "executable";
				
	};	
}
