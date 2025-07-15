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

		bool boolArg(const std::string& name, bool defaultValue) const;
		std::string stringArg(const std::string& name, const std::string& defaultValue) const;
		int intArg(const std::string& name, int defaultValue) const;
		double doubleArg(const std::string& name, double defaultValue) const;

		std::filesystem::path executable() const;
		std::filesystem::path executableDirectory() const;
		std::string executableName() const;

		bool isWindows() const;
		bool isLinux() const;

		void printUsage() const;

	protected:
		std::vector<std::string> m_vecArguments;
		std::map<std::string, std::string> m_mapArguments;

		std::string getArgValue(const std::string& name) const;

		void parse();		

		virtual std::string getUsageString() const = 0;		

		const char* EXECUTABLE_ARG_NAME = "executable";
		const char* EXE_EXTENSION = ".exe";
				
	};	
}
