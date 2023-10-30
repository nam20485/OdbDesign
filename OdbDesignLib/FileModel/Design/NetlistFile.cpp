#include "NetlistFile.h"
#include <fstream>
#include <sstream>
#include <Logger.h>

namespace Odb::Lib::FileModel::Design
{
	NetlistFile::NetlistFile(std::filesystem::path path)
		: m_path(path), m_optimized(false), m_staggered(Staggered::Unknown)
	{
	}

	NetlistFile::~NetlistFile()
	{
	}

	std::filesystem::path NetlistFile::GetPath() const
	{
		return m_path;
	}

	std::string NetlistFile::GetName() const
	{
		return m_name;
	}

	std::string NetlistFile::GetUnits() const
	{
		return m_units;
	}

	bool NetlistFile::GetOptimized() const
	{
		return m_optimized;
	}

	NetlistFile::Staggered NetlistFile::GetStaggered() const
	{
		return m_staggered;
	}

	const std::vector<std::string>& NetlistFile::GetNetNames() const
	{
		return m_netNames;
	}

	bool NetlistFile::Parse()
	{
		m_name = std::filesystem::path(m_path).filename().string();

		loginfo("Parsing netlist: " + m_name + "...");

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
					if (!(lineStream >> strNetNumber)) return false;
					//strNetNumber.erase(0, 1); // remove leading '$'
					//unsigned int netNumber = std::stoul(strNetNumber);
					// don't use net number to specify index of net in vector,
					// order of parsing/occurence in file dictates net number
					// since we use vecotr.push_back() to add net names to vector

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

		loginfo("Parsing netlist: " + m_name + " complete");

		return true;
	}
} // namespace Odb::Lib::FileModel::Design