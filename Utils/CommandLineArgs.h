#pragma once

#include <vector>
#include <string>
#include "utils_export.h"
#include <stdexcept>
#include <map>


namespace Utils
{
	class UTILS_EXPORT CommandLineArgs
	{
	public:
		CommandLineArgs(int argc, char* argv[]);
		CommandLineArgs(const std::vector<std::string>& vecArgv);

		bool boolArg(const std::string& name) const;
		std::string stringArg(const std::string& name) const;
		int intArg(const std::string& name) const;
		double doubleArg(const std::string& name) const;

	protected:
		std::vector<std::string> m_vecArguments;
		std::map<std::string, std::string> m_mapArguments;

		std::string getArgValue(const std::string& name) const;

		void parse();
				
	};	
}
