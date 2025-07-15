#include "CommandLineArgs.h"
#include "CommandLineArgs.h"
#include "CommandLineArgs.h"
#include "CommandLineArgs.h"
#include "CommandLineArgs.h"
#include "CommandLineArgs.h"
#include "CommandLineArgs.h"
#include "CommandLineArgs.h"
#include "CommandLineArgs.h"
#include "CommandLineArgs.h"
#include "CommandLineArgs.h"
#include <exception>
#include <stdexcept>
#include "str_utils.h"
#include <sstream>
#include <iostream>
#include <cstring>


namespace Utils
{
	CommandLineArgs::CommandLineArgs(int argc, char* argv[])
	{
		// save arguments in std::string vector		
		std::copy(argv, argv + argc, std::back_inserter(m_vecArguments));
		parse();
	}

	CommandLineArgs::CommandLineArgs(const std::vector<std::string>& vecArgv)
		: m_vecArguments(vecArgv)
	{
		parse();
	}

	bool CommandLineArgs::boolArg(const std::string& name, bool defaultValue) const
	{
		auto strB = getArgValue(name);
		if (strB.length() > 0)
		{
			bool b;
			std::istringstream(strB) >> std::boolalpha >> b;
			return b;
		}

		return defaultValue;
	}

	std::string CommandLineArgs::stringArg(const std::string& name, const std::string& defaultValue) const
	{
		auto str = getArgValue(name);
		if (str.length() > 0)
		{
			return str;
		}
		return defaultValue;
	}

	int CommandLineArgs::intArg(const std::string& name, int defaultValue) const
	{
		auto strI = getArgValue(name);
		if (strI.length() > 0)
		{
			return std::stoi(strI);
		}
		return defaultValue;
	}

	double CommandLineArgs::doubleArg(const std::string& name, double defaultValue) const
	{
		auto strD = getArgValue(name);
		if (strD.length() > 0)
		{
			return std::stod(strD);
		}
		return defaultValue;
	}

	std::filesystem::path CommandLineArgs::executable() const
	{
		return std::filesystem::path(getArgValue(EXECUTABLE_ARG_NAME));
	}

	std::filesystem::path CommandLineArgs::executableDirectory() const
	{
		return executable().parent_path();
	}

	std::string CommandLineArgs::executableName() const
	{
		return executable().filename().string();
	}

	bool CommandLineArgs::isWindows() const
	{
		auto pos = executableName().find(EXE_EXTENSION);
		return (pos != std::string::npos && 
				pos == executableName().length()-strlen(EXE_EXTENSION));
	}

	bool CommandLineArgs::isLinux() const
	{
		return !isWindows();
	}

	std::string CommandLineArgs::getArgValue(const std::string& name) const
	{
		auto findIt = m_mapArguments.find(name);
		if (findIt == m_mapArguments.end()) return "";
		return findIt->second;		
	}

	void Utils::CommandLineArgs::parse()
	{
		for (std::size_t i = 0; i < m_vecArguments.size(); i++)
		{
			auto& arg = m_vecArguments[i];
			if (arg.length() > 0)
			{
				if (i == 0)
				{
					m_mapArguments["executable"] = arg;
				}
				else if (arg[0] == '-' || arg[0] == '/')
				{
					Utils::str_ltrim(arg, '-');
					Utils::str_ltrim(arg, '/');

					std::string nextArg;
					if (i + 1 < m_vecArguments.size())
					{
						nextArg = m_vecArguments[i + 1];
					}

					if (nextArg.length() > 0 && 
						nextArg[0] != '-' && 
						nextArg[0] != '/')
					{
						m_mapArguments[arg] = nextArg;
						i++;
					}
					else
					{
						m_mapArguments[arg] = "true";
					}

					//if (i + 1 < m_vecArguments.size() &&
					//	m_vecArguments[i + 1].length() > 0 &&
					//	(m_vecArguments[i + 1][0] != '-' || m_vecArguments[i + 1][0] != '/'))
					//{
					//	
					//	m_mapArguments[arg] = m_vecArguments[i+++1];						
					//}
					//else
					//{						
					//	m_mapArguments[arg] = "true";
					//}						
				}				
			}
		}
	}
	void Utils::CommandLineArgs::printUsage() const
	{
		std::cout << getUsageString();
	}
}