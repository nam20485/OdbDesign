#include "CommandLineArgs.h"
#include "CommandLineArgs.h"
#include "CommandLineArgs.h"
#include "CommandLineArgs.h"
#include <exception>
#include <stdexcept>
#include "str_trim.h"
#include <sstream>


namespace Utils
{
	CommandLineArgs::CommandLineArgs(int argc, char* argv[])
	{
		// save arguments in std::string vector
		for (int i = 0; i < argc; i++)
		{
			m_vecArguments.push_back(argv[i]);
		}
		parse();
	}

	CommandLineArgs::CommandLineArgs(const std::vector<std::string>& vecArgv)
		: m_vecArguments(vecArgv)
	{
		parse();
	}

	bool CommandLineArgs::boolArg(const std::string& name) const
	{
		auto strB = getArgValue(name);
		if (strB.length() == 0) throw std::invalid_argument("Argument not found: " + name);
		bool b;
		std::istringstream(strB) >> std::boolalpha >> b;
		return b;
	}

	std::string CommandLineArgs::stringArg(const std::string& name) const
	{
		return getArgValue(name);
	}

	int CommandLineArgs::intArg(const std::string& name) const
	{
		auto strI = getArgValue(name);
		if (strI.length() == 0) throw std::invalid_argument("Argument not found: " + name);
		return std::stoi(strI);
	}

	double CommandLineArgs::doubleArg(const std::string& name) const
	{
		auto strD = getArgValue(name);
		if (strD.length() == 0) throw std::invalid_argument("Argument not found: " + name);
		return std::stod(strD);
	}

	std::string CommandLineArgs::getArgValue(const std::string& name) const
	{
		auto findIt = m_mapArguments.find(name);
		if (findIt == m_mapArguments.end()) return "";
		return findIt->second;		
	}

	void Utils::CommandLineArgs::parse()
	{
		for (int i = 1; i < m_vecArguments.size(); i++)
		{
			auto& arg = m_vecArguments[i];
			if (arg.length() > 0)
			{
				if (arg[0] == '-' || arg[0] == '/')
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
}