#pragma once

#include <map>
#include <vector>
#include <string>
#include <memory>
#include <filesystem>
#include "../../odbdesign_export.h"
#include "../../IProtoBuffable.h"
#include "netlistfile.pb.h"
#include "../ISaveable.h"
#include "../IStreamSaveable.h"
#include "EnumMap.h"


namespace Odb::Lib::FileModel::Design
{
	class ODBDESIGN_EXPORT NetlistFile : public IProtoBuffable<Odb::Lib::Protobuf::NetlistFile>, public ISaveable, public IStreamSaveable
	{
	public:
		struct ODBDESIGN_EXPORT NetRecord : public IProtoBuffable<Odb::Lib::Protobuf::NetlistFile::NetRecord>
		{
			unsigned int serialNumber = 0;
			std::string netName;

			// Inherited via IProtoBuffable
			std::unique_ptr<Odb::Lib::Protobuf::NetlistFile::NetRecord> to_protobuf() const override;
			void from_protobuf(const Odb::Lib::Protobuf::NetlistFile::NetRecord& message) override;

			typedef std::vector<std::shared_ptr<NetRecord>> Vector;
			typedef std::map<std::string, std::shared_ptr<NetRecord>> StringMap;

			inline constexpr static const char* FIELD_TOKEN = "$";
		};

		struct ODBDESIGN_EXPORT NetPointRecord : public IProtoBuffable<Odb::Lib::Protobuf::NetlistFile::NetPointRecord>, public IStreamSaveable
		{
			enum AccessSide
			{
				Top,
				Down,
				Both,
				Inner
			};

			unsigned int netNumber = 0;
			double radius = 0.0;
			double x = 0.0;
			double y = 0.0;
			AccessSide side = Top;
			double width = 0.0;
			double height = 0.0;
			char epoint = '\0';
			char exp = '\0';
			bool commentPoint = false;
			double staggeredX = 0.0;
			double staggeredY = 0.0;
			double staggeredRadius = 0.0;
			double viaPoint = 0.0;
			double fiducialPoint = 0.0;
			double testPoint = 0.0;
			// ...
			char testExecutionSide = '\0';

			// Inherited via IProtoBuffable
			std::unique_ptr<Odb::Lib::Protobuf::NetlistFile::NetPointRecord> to_protobuf() const override;
			void from_protobuf(const Odb::Lib::Protobuf::NetlistFile::NetPointRecord& message) override;

			typedef std::vector<std::shared_ptr<NetPointRecord>> Vector;

			inline static const Utils::EnumMap<AccessSide> accessSideMap{
				{
					"Top",
					"Down",
					"Both",
					"Inner"
				}
			};

			// Inherited via IStreamSaveable
			bool Save(std::ostream& os) override;

		};	// NetPointRecord

		enum class Staggered
		{
			Yes,
			No,
			Unknown
		};

		inline static const Utils::EnumMap<Staggered> staggeredMap{
			{
				"Y",
				"N",
				"Unknown"
			}
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
		// Inherited via ISaveable
		bool Save(const std::filesystem::path& directory) override;

		// Inherited via IProtoBuffable
		std::unique_ptr<Odb::Lib::Protobuf::NetlistFile> to_protobuf() const override;
		void from_protobuf(const Odb::Lib::Protobuf::NetlistFile& message) override;

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

		inline constexpr static const char HEADER_TOKEN[] = "H";
		inline constexpr static const char STAGGERED_KEY[] = "staggered";

		// Inherited via IStreamSaveable
		bool Save(std::ostream& os) override;

	};	// NetlistFile
}
