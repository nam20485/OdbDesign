#include "ToolsFile.h"
#include <fstream>
#include <sstream>
#include <charconv>
#include <stdexcept>
#include <cerrno>
#include <cmath>
#include <cstdlib>
#include "../parse_error.h"
#include "../invalid_odb_error.h"
#include "Logger.h"
#include "str_utils.h"
#include "../../Constants.h"
#include <ArchiveExtractor.h>
#include "../../Text/Utf8Sanitizer.h"

namespace
{
	// shortest round-trippable decimal form (avoids both precision loss and
	// fixed/scientific surprises in Save output)
	std::string format_double(double value)
	{
		// non-finite values have no ODB++ representation; emitting inf/nan
		// would produce a file our own parser (isfinite check) rejects
		if (!std::isfinite(value))
		{
			throw std::domain_error("non-finite numeric value cannot be serialized to the tools file grammar");
		}
		char buffer[64];
		const auto result = std::to_chars(buffer, buffer + sizeof(buffer), value);
		if (result.ec != std::errc())
		{
			return "0";
		}
		return std::string(buffer, result.ptr);
	}

	// numeric fields parse through validated helpers: a malformed or
	// hostile tools file surfaces as parse_error with file/line context.
	// from_chars (not stoi/stoul/stod) so that validation is total rather
	// than exception-only: trailing garbage, sign abuse on unsigned fields,
	// and out-of-target-range values are all rejected — sto* would accept
	// "5abc", wrap "-5", or truncate >UINT_MAX on LP64
	using Odb::Lib::FileModel::parse_error;

	unsigned int parseUnsigned(const std::filesystem::path& path, const std::string& line,
		int lineNumber, const std::string& token, const std::string& value)
	{
		unsigned int result = 0;
		const auto* last = value.data() + value.size();
		const auto [ptr, ec] = std::from_chars(value.data(), last, result);
		if (ec != std::errc() || ptr != last)
		{
			throw_parse_error(path, line, token, lineNumber);
		}
		return result;
	}

	double parseDouble(const std::filesystem::path& path, const std::string& line,
		int lineNumber, const std::string& token, const std::string& value)
	{
		// std::from_chars for floating point is unavailable on libc++
		// (Xcode), so the double path uses strtod with equivalent
		// strictness: full consumption (end pointer), range (errno), and a
		// finite result — "1e999", "12abc", and "inf" all reject
		if (value.empty())
		{
			throw_parse_error(path, line, token, lineNumber);
		}
		errno = 0;
		char* end = nullptr;
		const double result = std::strtod(value.c_str(), &end);
		if (end != value.c_str() + value.size() || errno == ERANGE || !std::isfinite(result))
		{
			throw_parse_error(path, line, token, lineNumber);
		}
		return result;
	}
}

namespace Odb::Lib::FileModel::Design
{
	ToolsFile::ToolsFile()
		: m_path("")
		, m_directory("")
		, m_units("")
	{
	}

	bool ToolsFile::ToolRecord::subTypeIsLegal() const
	{
		switch (type)
		{
		case Type::Plated:
			return subType == SubType::Standard || subType == SubType::PressFit;
		case Type::NonPlated:
			return subType == SubType::Standard;
		case Type::Via:
			return subType == SubType::Standard || subType == SubType::Photo || subType == SubType::Laser;
		default:
			return false;
		}
	}

	const char* ToolsFile::ToolRecord::typeName(Type type)
	{
		switch (type)
		{
		case Type::Plated: return TYPE_PLATED;
		case Type::NonPlated: return TYPE_NON_PLATED;
		case Type::Via: return TYPE_VIA;
		default: return "UNKNOWN";
		}
	}

	const char* ToolsFile::ToolRecord::subTypeName(SubType subType)
	{
		switch (subType)
		{
		case SubType::Standard: return SUB_TYPE_STANDARD;
		case SubType::PressFit: return SUB_TYPE_PRESS_FIT;
		case SubType::Photo: return SUB_TYPE_PHOTO;
		case SubType::Laser: return SUB_TYPE_LASER;
		default: return "UNKNOWN";
		}
	}

	std::string ToolsFile::GetUnits() const
	{
		return m_units;
	}

	double ToolsFile::GetThickness() const
	{
		return m_thickness;
	}

	const std::vector<std::string>& ToolsFile::GetUserParams() const
	{
		return m_userParams;
	}

	const ToolsFile::ToolRecord::Vector& ToolsFile::GetTools() const
	{
		return m_tools;
	}

	std::filesystem::path ToolsFile::GetPath() const
	{
		return m_path;
	}

	bool ToolsFile::Parse(std::filesystem::path directory)
	{
		std::ifstream toolsFile;
		int lineNumber = 0;
		std::string line;

		try
		{
			m_directory = directory;

			loginfo("checking for extraction...");

			auto toolsFilePath = Utils::ArchiveExtractor::getUncompressedFilePath(m_directory, TOOLS_FILENAME);
			if (std::filesystem::exists(toolsFilePath) && std::filesystem::is_regular_file(toolsFilePath))
			{
				loginfo("found tools file: [" + toolsFilePath.string() + "]");
			}

			m_path = toolsFilePath;

			if (!std::filesystem::exists(m_path))
			{
				// not every layer carries a drill tools table (only drill/rout
				// layers do); absence is not an error
				logwarn("tools file does not exist: [" + m_directory.string() + "]");
				return true;
			}
			else if (!std::filesystem::is_regular_file(m_path))
			{
				auto message = "tools is not a file: [" + m_path.string() + "]";
				throw invalid_odb_error(message.c_str());
			}

			toolsFile.open(m_path.string(), std::ios::in);
			if (!toolsFile.is_open())
			{
				auto message = "unable to open tools file: [" + m_path.string() + "]";
				throw invalid_odb_error(message.c_str());
			}

			auto parsingRecord = false;
			std::shared_ptr<ToolRecord> pCurrentRecord;

			while (std::getline(toolsFile, line))
			{
				lineNumber++;

				Utils::str_trim(line);
				if (line.empty()) continue;
				if (line.find(Constants::COMMENT_TOKEN) == 0) continue;

				std::stringstream lineStream(line);

				if (!parsingRecord)
				{
					// a TOOLS array record opens with "TOOLS {" — no '=' on the
					// line, so check the array token before key/value parsing.
					// Require the brace (optionally spaced) so a TOOLS-prefixed
					// parameter line is never mistaken for a record open.
					if (line.rfind(TOOLS_ARRAY_TOKEN, 0) == 0)
					{
						// char_traits::length over strlen: constexpr and provably bounded
						// for this class constant (Flawfinder CWE-126 flags strlen on
						// non-literal pointers as a potential over-read)
						auto afterToken = line.substr(std::char_traits<char>::length(TOOLS_ARRAY_TOKEN));
						Utils::str_trim(afterToken);
						if (afterToken != Constants::ARRAY_RECORD_OPEN_TOKEN)
						{
							throw_parse_error(m_path, line, TOOLS_ARRAY_TOKEN, lineNumber);
						}
						pCurrentRecord = std::make_shared<ToolRecord>();
						parsingRecord = true;
						continue;
					}

					// global parameter
					std::string key;
					if (!std::getline(lineStream, key, '='))
					{
						throw_parse_error(m_path, line, "", lineNumber);
					}

					Utils::str_trim(key);

					if (key == Constants::UNITS_TOKEN)
					{
						std::string value;
						std::getline(lineStream, value);
						Utils::str_trim(value);
						m_units = value;
					}
					else if (key == THICKNESS_KEY)
					{
						// obsolete per spec; tolerated and preserved for round-tripping
						std::string value;
						std::getline(lineStream, value);
						Utils::str_trim(value);
						m_thickness = value.empty() ? 0.0 : parseDouble(m_path, line, lineNumber, key, value);
					}
					else if (key == USER_PARAMS_KEY)
					{
						std::string value;
						std::getline(lineStream, value);
						Utils::str_trim(value);
						if (!value.empty())
						{
							m_userParams.push_back(value);
						}
					}
					else
					{
						throw_parse_error(m_path, line, key, lineNumber);
					}
				}
				else
				{
					// inside a TOOLS record
					if (line == Constants::ARRAY_RECORD_CLOSE_TOKEN)
					{
						if (pCurrentRecord->type == ToolRecord::Type::Unknown)
						{
							throw_parse_error(m_path, line, TYPE_KEY, lineNumber);
						}
						else if (!pCurrentRecord->subTypeIsLegal())
						{
							throw_parse_error(m_path, line, TYPE2_KEY, lineNumber);
						}

						m_tools.push_back(pCurrentRecord);
						pCurrentRecord = nullptr;
						parsingRecord = false;
						continue;
					}

					std::string key;
					if (!std::getline(lineStream, key, '='))
					{
						throw_parse_error(m_path, line, "", lineNumber);
					}

					Utils::str_trim(key);

					if (line.find('=') == std::string::npos)
					{
						// no '=' on the line at all — not a key/value field
						// (trailing '=' with empty value, e.g. BIT=, is legal)
						throw_parse_error(m_path, line, key, lineNumber);
					}
					std::string value;
					std::getline(lineStream, value);
					Utils::str_trim(value);

					if (key == NUM_KEY)
					{
						pCurrentRecord->num = parseUnsigned(m_path, line, lineNumber, key, value);
					}
					else if (key == TYPE_KEY)
					{
						if (value == ToolRecord::TYPE_PLATED) pCurrentRecord->type = ToolRecord::Type::Plated;
						else if (value == ToolRecord::TYPE_NON_PLATED) pCurrentRecord->type = ToolRecord::Type::NonPlated;
						else if (value == ToolRecord::TYPE_VIA) pCurrentRecord->type = ToolRecord::Type::Via;
						else throw_parse_error(m_path, line, value, lineNumber);
					}
					else if (key == TYPE2_KEY)
					{
						if (value == ToolRecord::SUB_TYPE_STANDARD) pCurrentRecord->subType = ToolRecord::SubType::Standard;
						else if (value == ToolRecord::SUB_TYPE_PRESS_FIT) pCurrentRecord->subType = ToolRecord::SubType::PressFit;
						else if (value == ToolRecord::SUB_TYPE_PHOTO) pCurrentRecord->subType = ToolRecord::SubType::Photo;
						else if (value == ToolRecord::SUB_TYPE_LASER) pCurrentRecord->subType = ToolRecord::SubType::Laser;
						else throw_parse_error(m_path, line, value, lineNumber);
					}
					else if (key == MIN_TOL_KEY)
					{
						pCurrentRecord->minTol = value.empty() ? 0.0 : parseDouble(m_path, line, lineNumber, key, value);
					}
					else if (key == MAX_TOL_KEY)
					{
						pCurrentRecord->maxTol = value.empty() ? 0.0 : parseDouble(m_path, line, lineNumber, key, value);
					}
					else if (key == BIT_KEY)
					{
						pCurrentRecord->bit = value;
					}
					else if (key == FINISH_SIZE_KEY)
					{
						// spec: -1 when not set
						pCurrentRecord->finishSize = value.empty() ? -1.0 : parseDouble(m_path, line, lineNumber, key, value);
					}
					else if (key == DRILL_SIZE_KEY)
					{
						pCurrentRecord->drillSize = value.empty() ? -1.0 : parseDouble(m_path, line, lineNumber, key, value);
					}
					else
					{
						throw_parse_error(m_path, line, key, lineNumber);
					}
				}
			}

			if (parsingRecord)
			{
				throw_parse_error(m_path, "EOF", Constants::ARRAY_RECORD_CLOSE_TOKEN, lineNumber);
			}

			toolsFile.close();
		}
		catch (parse_error& pe)
		{
			auto m = pe.toString("Parse Error:");
			logerror(m);
			toolsFile.close();
			throw;
		}
		catch (std::exception& e)
		{
			parse_info pi(m_path, line, lineNumber);
			const auto m = pi.toString();
			logexception_msg(e, m);
			toolsFile.close();
			// bare rethrow preserves the original exception type; rethrowing
			// the caught variable would slice invalid_odb_error/parse_error to
			// std::exception
			throw;
		}

		return true;
	}

	bool ToolsFile::Save(std::ostream& os)
	{
		os << Constants::UNITS_TOKEN << "=" << m_units << std::endl;
		os << THICKNESS_KEY << "=" << format_double(m_thickness) << std::endl;
		for (const auto& userParam : m_userParams)
		{
			os << USER_PARAMS_KEY << "=" << userParam << std::endl;
		}

		for (const auto& pTool : m_tools)
		{
			if (pTool->type == ToolRecord::Type::Unknown)
			{
				// TYPE=UNKNOWN is not part of the file grammar; a record with
				// no type cannot round-trip and must fail loudly, not corrupt
				throw_parse_error(m_path, "", TYPE_KEY, 0);
			}
			os << TOOLS_ARRAY_TOKEN << " " << Constants::ARRAY_RECORD_OPEN_TOKEN << std::endl;
			os << '\t' << NUM_KEY << "=" << pTool->num << std::endl;
			os << '\t' << TYPE_KEY << "=" << ToolRecord::typeName(pTool->type) << std::endl;
			os << '\t' << TYPE2_KEY << "=" << ToolRecord::subTypeName(pTool->subType) << std::endl;
			os << '\t' << MIN_TOL_KEY << "=" << format_double(pTool->minTol) << std::endl;
			os << '\t' << MAX_TOL_KEY << "=" << format_double(pTool->maxTol) << std::endl;
			os << '\t' << BIT_KEY << "=" << pTool->bit << std::endl;
			os << '\t' << FINISH_SIZE_KEY << "=" << format_double(pTool->finishSize) << std::endl;
			os << '\t' << DRILL_SIZE_KEY << "=" << format_double(pTool->drillSize) << std::endl;
			os << Constants::ARRAY_RECORD_CLOSE_TOKEN << std::endl;
		}

		return true;
	}

	std::unique_ptr<Odb::Lib::Protobuf::ToolsFile> ToolsFile::to_protobuf() const
	{
		auto message = std::make_unique<Odb::Lib::Protobuf::ToolsFile>();

		message->set_directory(Odb::Lib::Text::ToUtf8(m_directory.string()));
		message->set_path(Odb::Lib::Text::ToUtf8(m_path.string()));
		message->set_units(Odb::Lib::Text::ToUtf8(m_units));
		message->set_thickness(m_thickness);
		for (const auto& userParam : m_userParams)
		{
			message->add_userparams(Odb::Lib::Text::ToUtf8(userParam));
		}
		for (const auto& pTool : m_tools)
		{
			message->add_tools()->CopyFrom(*pTool->to_protobuf());
		}

		return message;
	}

	void ToolsFile::from_protobuf(const Odb::Lib::Protobuf::ToolsFile& message)
	{
		m_directory = message.directory();
		m_path = message.path();
		m_units = message.units();
		m_thickness = message.thickness();
		for (const auto& userParam : message.userparams())
		{
			m_userParams.push_back(userParam);
		}
		for (const auto& tool : message.tools())
		{
			auto pTool = std::make_shared<ToolRecord>();
			pTool->from_protobuf(tool);
			m_tools.push_back(pTool);
		}
	}

	std::unique_ptr<Odb::Lib::Protobuf::ToolsFile::Tool> ToolsFile::ToolRecord::to_protobuf() const
	{
		auto message = std::make_unique<Odb::Lib::Protobuf::ToolsFile::Tool>();

		message->set_num(num);
		message->set_type(static_cast<Odb::Lib::Protobuf::ToolsFile::ToolType>(type));
		message->set_subtype(static_cast<Odb::Lib::Protobuf::ToolsFile::ToolSubType>(subType));
		message->set_mintol(minTol);
		message->set_maxtol(maxTol);
		message->set_bit(Odb::Lib::Text::ToUtf8(bit));
		message->set_finishsize(finishSize);
		message->set_drillsize(drillSize);

		return message;
	}

	void ToolsFile::ToolRecord::from_protobuf(const Odb::Lib::Protobuf::ToolsFile::Tool& message)
	{
		num = message.num();
		type = static_cast<Type>(message.type());
		// absent sub-type deserializes to the parse-time default (STANDARD),
		// not Unknown — matches a record parsed from a file without TYPE2
		subType = message.has_subtype()
			? static_cast<SubType>(message.subtype())
			: SubType::Standard;
		minTol = message.mintol();
		maxTol = message.maxtol();
		bit = message.bit();
		// absent sizes keep the spec's -1 "unset" sentinel rather than 0.0
		finishSize = message.has_finishsize() ? message.finishsize() : -1.0;
		drillSize = message.has_drillsize() ? message.drillsize() : -1.0;
	}
}
