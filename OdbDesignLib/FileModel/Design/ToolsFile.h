#pragma once

#include <string>
#include <map>
#include <memory>
#include <vector>
#include <filesystem>
#include "../../odbdesign_export.h"
#include "../../IProtoBuffable.h"
#include "toolsfile.pb.h"
#include "../IStreamSaveable.h"

namespace Odb::Lib::FileModel::Design
{
	/// ODB++ layer drill-tools table (<layer>/tools, spec "Layer Entities — Drill Tools").
	/// Clean-room implementation authored from the ODB++ specification and
	/// sample design data; see docs/plan/clean-room-reimplementation-plan.md.
	class ODBDESIGN_EXPORT ToolsFile : public IProtoBuffable<Odb::Lib::Protobuf::ToolsFile>, public IStreamSaveable
	{
	public:
		ToolsFile();

		struct ToolRecord : public IProtoBuffable<Odb::Lib::Protobuf::ToolsFile::Tool>
		{
			enum class Type
			{
				Unknown = 0,
				Plated,
				NonPlated,
				Via
			};

			enum class SubType
			{
				Unknown = 0,
				Standard,
				PressFit,
				Photo,
				Laser
			};

			// spec: tool number
			unsigned int num = 0;
			// tracked for close-brace validation (NUM must be present)
			bool hasNum = false;
			// spec: PLATED | NON_PLATED | VIA
			Type type = Type::Unknown;
			// spec: legality depends on type; defaults to STANDARD when absent
			SubType subType = SubType::Standard;
			// spec: allowed tolerance (mils or microns per UNITS)
			double minTol = 0.0;
			double maxTol = 0.0;
			// spec: drill bit string
			std::string bit;
			// spec: required finished drill size; -1 when not set
			double finishSize = -1.0;
			// spec: optional drill tool size
			double drillSize = -1.0;

			// constants
			constexpr inline static const char* TYPE_PLATED = "PLATED";
			constexpr inline static const char* TYPE_NON_PLATED = "NON_PLATED";
			constexpr inline static const char* TYPE_VIA = "VIA";
			constexpr inline static const char* SUB_TYPE_STANDARD = "STANDARD";
			constexpr inline static const char* SUB_TYPE_PRESS_FIT = "PRESS_FIT";
			constexpr inline static const char* SUB_TYPE_PHOTO = "PHOTO";
			constexpr inline static const char* SUB_TYPE_LASER = "LASER";

			// spec: legal sub-type per type (TYPE2 field); TYPE2 defaults to
			// STANDARD when absent
			bool subTypeIsLegal() const;

			// names as they appear in the file grammar (used by Save)
			static const char* typeName(Type type);
			static const char* subTypeName(SubType subType);

			// typedefs
			typedef std::vector<std::shared_ptr<ToolRecord>> Vector;
			typedef std::map<unsigned int, std::shared_ptr<ToolRecord>> NumMap;

			// Inherited via IProtoBuffable
			std::unique_ptr<Odb::Lib::Protobuf::ToolsFile::Tool> to_protobuf() const override;
			void from_protobuf(const Odb::Lib::Protobuf::ToolsFile::Tool& message) override;
		};

		bool Parse(std::filesystem::path directory);
		// Inherited via IStreamSaveable
		bool Save(std::ostream& os) override;

		std::string GetUnits() const;
		double GetThickness() const;
		const std::vector<std::string>& GetUserParams() const;
		const ToolRecord::Vector& GetTools() const;
		std::filesystem::path GetPath() const;

		// Inherited via IProtoBuffable
		std::unique_ptr<Odb::Lib::Protobuf::ToolsFile> to_protobuf() const override;
		void from_protobuf(const Odb::Lib::Protobuf::ToolsFile& message) override;

		constexpr inline static const char* TOOLS_FILENAME = "tools";
		constexpr inline static const char* TOOLS_ARRAY_TOKEN = "TOOLS";
		constexpr inline static const char* THICKNESS_KEY = "THICKNESS";
		constexpr inline static const char* USER_PARAMS_KEY = "USER_PARAMS";
		constexpr inline static const char* NUM_KEY = "NUM";
		constexpr inline static const char* TYPE_KEY = "TYPE";
		constexpr inline static const char* TYPE2_KEY = "TYPE2";
		constexpr inline static const char* MIN_TOL_KEY = "MIN_TOL";
		constexpr inline static const char* MAX_TOL_KEY = "MAX_TOL";
		constexpr inline static const char* BIT_KEY = "BIT";
		constexpr inline static const char* FINISH_SIZE_KEY = "FINISH_SIZE";
		constexpr inline static const char* DRILL_SIZE_KEY = "DRILL_SIZE";

	private:
		std::filesystem::path m_path;
		std::filesystem::path m_directory;
		std::string m_units;
		double m_thickness = 0.0;
		std::vector<std::string> m_userParams;
		ToolRecord::Vector m_tools;
	};
}
