#include "Netlist.h"
#include <fstream>
#include <sstream>


Netlist::Netlist(std::filesystem::path path)
	: m_path(path)
{
}

Netlist::~Netlist()
{
}

std::filesystem::path Netlist::GetPath() const
{
    return m_path;
}

std::string Netlist::GetName() const
{
    return m_name;
}

std::string Netlist::GetUnits() const
{
	return m_units;
}

bool Netlist::GetOptimized() const
{
	return m_optimized;
}

Netlist::Staggered Netlist::GetStaggered() const
{
	return m_staggered;
}

const std::vector<std::string>& Netlist::GetNetNames() const
{
	return m_netNames;
}

bool Netlist::Parse()
{
    m_name = std::filesystem::path(m_path).filename().string();

	auto netlistFilePath = m_path / "netlist";

	if (!std::filesystem::exists(netlistFilePath)) return false;
	else if (!std::filesystem::is_regular_file(netlistFilePath)) return false;

	std::ifstream netlistFile;
	netlistFile.open(netlistFilePath.string(), std::ios::in);
	if (!netlistFile.is_open()) return false;

	std::string line;
	while (std::getline(netlistFile, line))
	{
		if (!line.empty())
		{
			std::stringstream lineStream(line);					

			if (line.find(COMMENT_TOKEN) == 0)
			{
				// comment line
			}
			else if (line.find(UNITS_TOKEN) == 0)
			{
				// units line
				std::string token;
				if (!std::getline(lineStream, token, '=')) return false;
				else if (!std::getline(lineStream, token, '=')) return false;
				m_units = token;
			}
			else if (line.find(HEADER_TOKEN) == 0)
			{
				// header line
				std::string token;
				
				// H (throw away
				lineStream >> token;
				
				// optimize(d)
				lineStream >> token;
				if (token != "optimize") return false;				
				char optimized;
				lineStream >> optimized;
				m_optimized = (std::tolower(optimized) == 'y');

				// staggered (optional)
				if (lineStream >> token &&
					token == "staggered")
				{
					char staggered;
					lineStream >> staggered;				
					if (std::tolower(staggered) == 'y')
					{
						m_staggered = Staggered::Yes;
					}
					else if (std::tolower(staggered) == 'n')
					{
						m_staggered = Staggered::No;
					}
					else
					{
						m_staggered = Staggered::Unknown;
					}
				}
				else
				{
					m_staggered = Staggered::Unknown;
				}
			}	
			else if (line.find(NET_TOKEN) == 0)
			{
				// net record line
				
				// net number
				std::string strNetNumber;
				if (! (lineStream >> strNetNumber)) return false;
				strNetNumber.erase(0, 1); // remove leading '$'
				unsigned int netNumber = std::stoul(strNetNumber);

				// net name
				std::string netName;
				if (!(lineStream >> netName)) return false;
				m_netNames.push_back(netName);
			}
			else // if (starts with a (signed) int, including -1)
			{
				// netlist points record
				// TODO: parse netlist points
			}
		}
	}

    return true;
}
