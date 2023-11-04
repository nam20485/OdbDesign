#pragma once

#include <map>
#include <vector>
#include <string>
#include <memory>
#include <filesystem>
#include "../../odbdesign_export.h"


namespace Odb::Lib::FileModel::Design
{
	class ODBDESIGN_EXPORT NetlistFile
	{
	public:
		struct NetRecord
		{
			unsigned int serialNumber;
			std::string netName;

			typedef std::vector<std::shared_ptr<NetRecord>> Vector;
			typedef std::map<std::string, std::shared_ptr<NetRecord>> StringMap;

			inline constexpr static const char* FIELD_TOKEN = "$";
		};

		struct NetPointRecord
		{
			enum AccessSide
			{
				Top,
				Down,
				Both,
				Inner
			};

			unsigned int netNumber;
			float radius;
			float x;
			float y;
			AccessSide side;
			float width;
			float height;
			char epoint;
			char exp;
			bool commentPoint;
			float staggeredX;
			float staggeredY;
			float staggeredRadius;
			float viaPoint;
			float fiducialPoint;
			float testPoint;
			// ...
			char testExecutionSide;

			typedef std::vector<std::shared_ptr<NetPointRecord>> Vector;
		};

		enum class Staggered
		{
			Yes,
			No,
			Unknown
		};

		NetlistFile(std::filesystem::path path);
		~NetlistFile();

		std::filesystem::path GetPath() const;
		std::string GetName() const;
		std::string GetUnits() const;

		bool GetOptimized() const;
		Staggered GetStaggered() const;

		const NetRecord::Vector& GetNetRecords() const;
		const NetRecord::StringMap& GetNetRecordsByName() const;
		const NetPointRecord::Vector& GetNetPointRecords() const;

		bool Parse();

		typedef std::vector<std::shared_ptr<NetlistFile>> Vector;
		typedef std::map<std::string, std::shared_ptr<NetlistFile>> StringMap;

	private:
		std::filesystem::path m_path;
		std::string m_name;
		std::string m_units;
		bool m_optimized;
		Staggered m_staggered;
		
		NetRecord::Vector m_netRecords;
		NetRecord::StringMap m_netRecordsByName;
		NetPointRecord::Vector m_netPointRecords;		

		inline constexpr static const char* UNITS_TOKEN = "UNITS";
		inline constexpr static const char* COMMENT_TOKEN = "#";
		inline constexpr static const char* HEADER_TOKEN = "H";

	};
}