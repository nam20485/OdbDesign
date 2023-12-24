#include "NetlistFile.h"
#include "NetlistFile.h"
#include "NetlistFile.h"
#include "NetlistFile.h"
#include "NetlistFile.h"
#include "NetlistFile.h"
#include "NetlistFile.h"
#include "NetlistFile.h"
#include <fstream>
#include <sstream>
#include <Logger.h>
#include "../parse_error.h"
#include "../invalid_odb_error.h"
#include <climits>

namespace Odb::Lib::FileModel::Design
{
	NetlistFile::NetlistFile(std::filesystem::path path)
		: m_path(path)
		, m_optimized(false)
		, m_staggered(Staggered::Unknown)
	{
	}

	NetlistFile::~NetlistFile()
	{
		m_netRecords.clear();
		m_netRecordsByName.clear();
		m_netPointRecords.clear();
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

	const NetlistFile::NetRecord::Vector& NetlistFile::GetNetRecords() const
	{
		return m_netRecords;
	}

	const NetlistFile::NetRecord::StringMap& NetlistFile::GetNetRecordsByName() const
	{
		return m_netRecordsByName;
	}

	const NetlistFile::NetPointRecord::Vector& NetlistFile::GetNetPointRecords() const
	{
		return m_netPointRecords;
	}

	bool NetlistFile::Parse()
	{
		std::ifstream netlistFile;
		int lineNumber = 0;
		std::string line;

		try
		{
			m_name = std::filesystem::path(m_path).filename().string();

			loginfo("Parsing netlist: " + m_name + "...");

			auto netlistFilePath = m_path / "netlist";

			if (!std::filesystem::exists(netlistFilePath))
			{
				auto message = "netlist file does not exist: [" + m_path.string() + "]";
				throw invalid_odb_error(message.c_str());
			}

			else if (!std::filesystem::is_regular_file(netlistFilePath))
			{
				auto message = "netlist file is not a file: [" + m_path.string() + "]";
				throw invalid_odb_error(message.c_str());
			}
			
			netlistFile.open(netlistFilePath.string(), std::ios::in);
			if (!netlistFile.is_open())
			{
				auto message = "unable to open netlist file: [" + m_path.string() + "]";
				throw invalid_odb_error(message.c_str());
			}

			while (std::getline(netlistFile, line))
			{
				lineNumber++;
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
						if (!std::getline(lineStream, token, '='))
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}
						else if (!std::getline(lineStream, token, '='))
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}
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
						if (token != "optimize")
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}

						char optimized;						
						if (!(lineStream >> optimized))
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}

						if (optimized == 'Y' || std::tolower(optimized) == 'y')
						{
							m_optimized = true;
						}
						else if (optimized == 'N' || std::tolower(optimized) == 'n')
						{
							m_optimized = false;
						}
						else
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}

						// staggered (optional)
						if (lineStream >> token && token == "staggered")
						{
							char staggered;
							if (!(lineStream >> staggered))
							{
								throw_parse_error(m_path, line, token, lineNumber);
							}
							
							if (staggered == 'Y' || std::tolower(staggered) == 'y')
							{
								m_staggered = Staggered::Yes;
							}
							else if (staggered == 'N' || std::tolower(staggered) == 'n')
							{
								m_staggered = Staggered::No;
							}
							else
							{
								throw_parse_error(m_path, line, token, lineNumber);
							}
						}
						else
						{
							m_staggered = Staggered::Unknown;
						}
					}
					else if (line.find(NetRecord::FIELD_TOKEN) == 0)
					{
						// net record line						

						// net serial number
						std::string token;
						if (!(lineStream >> token))
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}

						auto strSerialNumber = token;
						strSerialNumber.erase(0, 1); // remove leading '$'
						auto ulSerialNumber = std::stoul(strSerialNumber);
						if (ulSerialNumber > UINT_MAX)
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}			

						auto unSerialNumber = static_cast<unsigned int>(ulSerialNumber);

						// net name
						if (!(lineStream >> token))
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}

						auto& netName = token;

						auto pNetRecord = std::make_shared<NetRecord>();
						pNetRecord->serialNumber = unSerialNumber;
						pNetRecord->netName = netName;
						m_netRecords.push_back(pNetRecord);
						//m_netRecordsByName[pNetRecord->netName] = pNetRecord;						
					}
					else // NetPointRecord (starts with an unsigned int, netNumber)
					{
						std::string token;
						if (!(lineStream >> token))
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}

						auto ulNetNumber = std::stoul(token);
						if (ulNetNumber >= UINT_MAX)
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}

						auto unNetNumber = static_cast<unsigned int>(ulNetNumber);

						auto pNetPointRecord = std::make_shared<NetPointRecord>();
						pNetPointRecord->netNumber = unNetNumber;

						if (!(lineStream >> pNetPointRecord->radius))
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}

						if (!(lineStream >> pNetPointRecord->x))
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}

						if (!(lineStream >> pNetPointRecord->y))
						{
							throw_parse_error(m_path, line, token, lineNumber);
						}

						// TODO: parse the rest of the NetPointRecord fields

						m_netPointRecords.push_back(pNetPointRecord);																											
					}
				}
			}

			loginfo("Parsing netlist: " + m_name + " complete");

			netlistFile.close();
		}
		catch (parse_error& pe)
		{
			auto m = pe.toString("Parse Error:");
			logerror(m);
			// cleanup file
			netlistFile.close();
			throw pe;

		}
		catch (std::exception& e)
		{
			parse_info pi(m_path, line, lineNumber);
			const auto m = pi.toString();
			logexception_msg(e, m);
			netlistFile.close();
			throw e;
		}

		return true;
	}

	std::unique_ptr<Odb::Lib::Protobuf::NetlistFile> NetlistFile::to_protobuf() const
	{
		std::unique_ptr<Odb::Lib::Protobuf::NetlistFile> pNetlistFileMessage(new Odb::Lib::Protobuf::NetlistFile);
		pNetlistFileMessage->set_units(m_units);
		pNetlistFileMessage->set_path(m_path.string());	
		pNetlistFileMessage->set_name(m_name);
		pNetlistFileMessage->set_optimized(m_optimized);
		pNetlistFileMessage->set_staggered(static_cast<Odb::Lib::Protobuf::NetlistFile::Staggered>(m_staggered));

		for (const auto& pNetRecord : m_netRecords)
		{
			auto pNetRecordMessage = pNetRecord->to_protobuf();
			pNetlistFileMessage->add_netrecordss()->CopyFrom(*pNetRecordMessage);
		}

		for (const auto& kvNetRecord : m_netRecordsByName)
		{
			(*pNetlistFileMessage->mutable_netrecordsbyname())[kvNetRecord.first] = *kvNetRecord.second->to_protobuf();
		}

		for (const auto& pNetPointRecord : m_netPointRecords)
		{
			auto pNetPointRecordMessage = pNetPointRecord->to_protobuf();
			pNetlistFileMessage->add_netpointrecords()->CopyFrom(*pNetPointRecordMessage);
		}

		return pNetlistFileMessage;		
	}

	void NetlistFile::from_protobuf(const Odb::Lib::Protobuf::NetlistFile& message)
	{
		m_name = message.name();
		m_path = message.path();
		m_units = message.units();
		m_optimized = message.optimized();
		m_staggered = static_cast<Staggered>(message.staggered());

		for (const auto& netRecordMessage : message.netrecordss())
		{
			auto pNetRecord = std::make_shared<NetRecord>();
			pNetRecord->from_protobuf(netRecordMessage);
			m_netRecords.push_back(pNetRecord);
		}

		for (const auto& kvNetRecord : message.netrecordsbyname())
		{
			auto pNetRecord = std::make_shared<NetRecord>();
			pNetRecord->from_protobuf(kvNetRecord.second);			
			m_netRecordsByName[pNetRecord->netName] = pNetRecord;
		}

		for (const auto& netPointRecordMessage : message.netpointrecords())
		{
			auto pNetPointRecord = std::make_shared<NetPointRecord>();
			pNetPointRecord->from_protobuf(netPointRecordMessage);
			m_netPointRecords.push_back(pNetPointRecord);
		}
	}

	std::unique_ptr<Odb::Lib::Protobuf::NetlistFile::NetRecord> NetlistFile::NetRecord::to_protobuf() const
	{
		std::unique_ptr<Odb::Lib::Protobuf::NetlistFile::NetRecord> pNetRecordMessage(new Odb::Lib::Protobuf::NetlistFile::NetRecord);
		pNetRecordMessage->set_serialnumber(serialNumber);
		pNetRecordMessage->set_netname(netName);
		return pNetRecordMessage;		
	}

	void NetlistFile::NetRecord::from_protobuf(const Odb::Lib::Protobuf::NetlistFile::NetRecord& message)
	{
		serialNumber = message.serialnumber();
		netName = message.netname();
	}

	std::unique_ptr<Odb::Lib::Protobuf::NetlistFile::NetPointRecord> NetlistFile::NetPointRecord::to_protobuf() const
	{
		std::unique_ptr<Odb::Lib::Protobuf::NetlistFile::NetPointRecord> pNetPointRecordMessage(new Odb::Lib::Protobuf::NetlistFile::NetPointRecord);
		pNetPointRecordMessage->set_netnumber(netNumber);
		pNetPointRecordMessage->set_radius(radius);
		pNetPointRecordMessage->set_x(x);
		pNetPointRecordMessage->set_y(y);
		pNetPointRecordMessage->set_epoint(std::to_string(epoint));
		pNetPointRecordMessage->set_width(width);
		pNetPointRecordMessage->set_exp(std::to_string(exp));
		pNetPointRecordMessage->set_fiducialpoint(fiducialPoint);
		pNetPointRecordMessage->set_height(height);
		pNetPointRecordMessage->set_commentpoint(commentPoint);
		pNetPointRecordMessage->set_netnumber(netNumber);
		pNetPointRecordMessage->set_side(static_cast<Odb::Lib::Protobuf::NetlistFile::NetPointRecord::AccessSide>(side));
		pNetPointRecordMessage->set_staggeredradius(staggeredRadius);
		pNetPointRecordMessage->set_staggeredx(staggeredX);
		pNetPointRecordMessage->set_staggeredy(staggeredY);
		pNetPointRecordMessage->set_testexecutionside(std::to_string(testExecutionSide));
		pNetPointRecordMessage->set_testpoint(testPoint);
		pNetPointRecordMessage->set_viapoint(viaPoint);
		pNetPointRecordMessage->set_fiducialpoint(fiducialPoint);
		return pNetPointRecordMessage;
	}

	void NetlistFile::NetPointRecord::from_protobuf(const Odb::Lib::Protobuf::NetlistFile::NetPointRecord& message)
	{
		netNumber = message.netnumber();
		radius = message.radius();
		x = message.x();
		y = message.y();		
		if (message.epoint().empty())
		{
			epoint = message.epoint()[0];
		}
		width = message.width();
		if (!message.exp().empty())
		{
			exp = message.exp()[0];
		}
		fiducialPoint = message.fiducialpoint();
		height = message.height();
		commentPoint = message.commentpoint();
		netNumber = message.netnumber();
		side = static_cast<AccessSide>(message.side());
		staggeredRadius = message.staggeredradius();
		staggeredX = message.staggeredx();
		staggeredY = message.staggeredy();
		if (!message.testexecutionside().empty())
		{
			testExecutionSide = message.testexecutionside()[0];
		}
		testPoint = message.testpoint();
		viaPoint = message.viapoint();
		fiducialPoint = message.fiducialpoint();
	}

	
} // namespace Odb::Lib::FileModel::Design