#pragma once

#include <string>
#include <map>
#include <memory>
#include <filesystem>
#include "../../odbdesign_export.h"
#include "../../enums.h"
#include "../../IProtoBuffable.h"
#include "PropertyRecord.h"


namespace Odb::Lib::FileModel::Design
{
	class ODBDESIGN_EXPORT ComponentsFile
	{
	public:
		ComponentsFile();
		~ComponentsFile();

		bool Parse(std::filesystem::path directory);

		std::string GetUnits() const;
		BoardSide GetSide() const;
		std::filesystem::path GetPath();
		std::filesystem::path GetDirectory();		

		struct ComponentRecord
		{
			~ComponentRecord();

			// data members
			unsigned int pkgRef;	// reference number of PKG in eda/data file
			float locationX;
			float locationY;
			float rotation;
			bool mirror;
			std::string compName;	// refDes
			std::string partName;
			std::string attributes;
			unsigned int id;
			// TODO: deal with index of records
			unsigned int index;

			// constants
			constexpr inline static const char* RECORD_TOKEN = "CMP";

			// typedefs
			typedef std::map<std::string, std::shared_ptr<ComponentRecord>> StringMap;
			typedef std::vector<std::shared_ptr<ComponentRecord>> Vector;			

			struct ToeprintRecord
			{
				// TODO: use pinNumber
				unsigned int pinNumber;		// what does this refer to? own pin # or packages pin #?
				float locationX;
				float locationY;
				float rotation;
				bool mirror;
				unsigned int netNumber;		// net number of NET in eda/data file
				unsigned int subnetNumber;	// subnet number of NET in eda/data file
				std::string name;			// pin name

				// constants
				constexpr inline static const char* RECORD_TOKEN = "TOP";

				// typedefs
				typedef std::map<std::string, std::shared_ptr<ToeprintRecord>> StringMap;
				typedef std::vector<std::shared_ptr<ToeprintRecord>> Vector;
			};

			PropertyRecord::Vector m_propertyRecords;
			ToeprintRecord::Vector m_toeprintRecords;
		};	// ComponentRecord

		const ComponentRecord::Vector& GetComponentRecords() const;
		const ComponentRecord::StringMap& GetComponentRecordsByName() const;
		const std::vector<std::string>& GetAttributeNames() const;
		const std::vector<std::string>& GetAttributeTextValues() const;

		constexpr inline static const char* TOP_COMPONENTS_LAYER_NAME = "comp_+_top";
		constexpr inline static const char* BOTTOM_COMPONENTS_LAYER_NAME = "comp_+_bot";

	private:
		std::string m_units;
		unsigned int m_id;
		BoardSide m_side;
		std::string m_name;
		std::filesystem::path m_path;
		std::filesystem::path m_directory;

		std::vector<std::string> m_attributeNames;
		std::vector<std::string> m_attributeTextValues;

		ComponentRecord::Vector m_componentRecords;
		// TODO: add records to maps by name while adding to their vectors
		ComponentRecord::StringMap m_componentRecordsByName;

		const bool m_allowToepintNetNumbersOfNegative1 = true;

		constexpr inline static const char* COMPONENTS_FILENAMES[] =
		{ 
			"components", 
			"components2", 
			"components3"
		};

		constexpr inline static const char* UNITS_TOKEN = "UNITS";
		constexpr inline static const char* ID_TOKEN = "ID";
		constexpr inline static const char* ATTRIBUTE_NAME_TOKEN = "@";
		constexpr inline static const char* ATTRIBUTE_VALUE_TOKEN = "&";
		constexpr inline static const char* COMMENT_TOKEN = "#";

		// TODO: deal with BOM DATA section lines later
		constexpr inline static const char* BOM_DESCR_RECORD_TOKEN_CPN = "CPN";
		constexpr inline static const char* BOM_DESCR_RECORD_TOKEN_PKG = "PKG";
		constexpr inline static const char* BOM_DESCR_RECORD_TOKEN_IPN = "IPN";
		constexpr inline static const char* BOM_DESCR_RECORD_TOKEN_DSC = "DSC";
		constexpr inline static const char* BOM_DESCR_RECORD_TOKEN_VPL_VND = "VPL_VND";
		constexpr inline static const char* BOM_DESCR_RECORD_TOKEN_VPL_MPN = "VPL_MPN";
		constexpr inline static const char* BOM_DESCR_RECORD_TOKEN_VND = "VND";
		constexpr inline static const char* BOM_DESCR_RECORD_TOKEN_MPN = "MPN";		
	};
}