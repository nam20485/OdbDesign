#pragma once

#include "LayerDirectory.h"
#include <vector>
#include <string>
#include "../../enums.h"


namespace Odb::Lib::FileModel::Design
{
	class ODBDESIGN_EXPORT ComponentLayerDirectory : public LayerDirectory
	{
	public:
		ComponentLayerDirectory(std::filesystem::path path, BoardSide side);
		~ComponentLayerDirectory();

		bool Parse() override;

		std::string GetUnits() const;
		BoardSide GetSide() const;

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
			inline static const std::string RECORD_TOKEN = "CMP";

			// typedefs
			typedef std::map<std::string, std::shared_ptr<ComponentRecord>> StringMap;
			typedef std::vector<std::shared_ptr<ComponentRecord>> Vector;

			struct PropertyRecord
			{
				// data members
				std::string name;
				std::string value;
				std::vector<float> floatValues;

				// constants
				inline static const std::string RECORD_TOKEN = "PRP";

				// typedefs
				typedef std::map<std::string, std::shared_ptr<PropertyRecord>> StringMap;
				typedef std::vector<std::shared_ptr<PropertyRecord>> Vector;
			};

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
				inline static const std::string RECORD_TOKEN = "TOP";

				// typedefs
				typedef std::map<std::string, std::shared_ptr<ToeprintRecord>> StringMap;
				typedef std::vector<std::shared_ptr<ToeprintRecord>> Vector;
			};

			PropertyRecord::Vector m_propertyRecords;
			ToeprintRecord::Vector m_toeprintRecords;
		};

		const ComponentRecord::Vector& GetComponentRecords() const;
		const ComponentRecord::StringMap& GetComponentRecordsByName() const;
		const std::vector<std::string>& GetAttributeNames() const;
		const std::vector<std::string>& GetAttributeTextValues() const;

	private:
		std::string m_units;
		unsigned int m_id;
		BoardSide m_side;

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

		inline static const char* UNITS_TOKEN = "UNITS";
		inline static const char* ID_TOKEN = "ID";
		inline static const char* ATTRIBUTE_NAME_TOKEN = "@";
		inline static const char* ATTRIBUTE_VALUE_TOKEN = "&";
		inline static const char* COMMENT_TOKEN = "#";

		// TODO: deal with BOM DATA section lines later
		inline static const char* BOM_DESCR_RECORD_TOKEN_CPN = "CPN";
		inline static const char* BOM_DESCR_RECORD_TOKEN_PKG = "PKG";
		inline static const char* BOM_DESCR_RECORD_TOKEN_IPN = "IPN";
		inline static const char* BOM_DESCR_RECORD_TOKEN_DSC = "DSC";
		inline static const char* BOM_DESCR_RECORD_TOKEN_VPL_VND = "VPL_VND";
		inline static const char* BOM_DESCR_RECORD_TOKEN_VPL_MPN = "VPL_MPN";
		inline static const char* BOM_DESCR_RECORD_TOKEN_VND = "VND";
		inline static const char* BOM_DESCR_RECORD_TOKEN_MPN = "MPN";
	};
}