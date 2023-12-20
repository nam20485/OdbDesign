#pragma once

#include <string>
#include <map>
#include <memory>
#include <filesystem>
#include <vector>
#include "../../odbdesign_export.h"
#include "../../enums.h"
#include "../../IProtoBuffable.h"
#include "PropertyRecord.h"
#include "../../ProtoBuf/componentsfile.pb.h"
#include "AttributeLookupTable.h"


namespace Odb::Lib::FileModel::Design
{
	class ODBDESIGN_EXPORT ComponentsFile : public IProtoBuffable<Odb::Lib::Protobuf::ComponentsFile>
	{
	public:
		ComponentsFile();
		~ComponentsFile();

		bool Parse(std::filesystem::path directory);

		std::string GetUnits() const;
		BoardSide GetSide() const;
		std::filesystem::path GetPath();
		std::filesystem::path GetDirectory();
		std::string GetLayerName() const;

		struct ComponentRecord : public IProtoBuffable<Odb::Lib::Protobuf::ComponentsFile::ComponentRecord>, public AttributeLookupTable
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
			//std::string attributes;
			unsigned int id;
			// TODO: deal with index of records
			unsigned int index;

			// constants
			constexpr inline static const char* RECORD_TOKEN = "CMP";

			// typedefs
			typedef std::map<std::string, std::shared_ptr<ComponentRecord>> StringMap;
			typedef std::vector<std::shared_ptr<ComponentRecord>> Vector;			

			struct ToeprintRecord : public IProtoBuffable<Odb::Lib::Protobuf::ComponentsFile::ComponentRecord::ToeprintRecord>
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

				// Inherited via IProtoBuffable
				std::unique_ptr<Odb::Lib::Protobuf::ComponentsFile::ComponentRecord::ToeprintRecord> to_protobuf() const override;
				void from_protobuf(const Odb::Lib::Protobuf::ComponentsFile::ComponentRecord::ToeprintRecord& message) override;
			};

			PropertyRecord::Vector m_propertyRecords;
			ToeprintRecord::Vector m_toeprintRecords;

			// Inherited via IProtoBuffable
			std::unique_ptr<Odb::Lib::Protobuf::ComponentsFile::ComponentRecord> to_protobuf() const override;
			void from_protobuf(const Odb::Lib::Protobuf::ComponentsFile::ComponentRecord& message) override;

		};	// ComponentRecord

		struct BomDescriptionRecord : public IProtoBuffable<Odb::Lib::Protobuf::ComponentsFile::BomDescriptionRecord>
		{
			std::string cpn;
			std::string pkg;
			std::string ipn;
			std::vector<std::string> descriptions;
			std::string dsc;
			std::string vpl_vnd;
			std::string vpl_mpn;
			std::string vnd;
			std::string mpn;

			// Inherited via IProtoBuffable
			std::unique_ptr<Odb::Lib::Protobuf::ComponentsFile::BomDescriptionRecord> to_protobuf() const override;
			void from_protobuf(const Odb::Lib::Protobuf::ComponentsFile::BomDescriptionRecord& message) override;

			typedef std::vector<std::shared_ptr<BomDescriptionRecord>> Vector;
			typedef std::map<std::string, std::shared_ptr<BomDescriptionRecord>> StringMap;

			inline static const char* CPN_RECORD_TOKEN = "CPN";
			inline static const char* PKG_RECORD_TOKEN = "PKG";
			inline static const char* IPN_RECORD_TOKEN = "IPN";
			inline static const char* DSC_RECORD_TOKEN = "DSC";
			inline static const char* VPL_VND_RECORD_TOKEN = "VPL_VND";
			inline static const char* VPL_MPN_RECORD_TOKEN = "VPL_MPN";
			inline static const char* VND_RECORD_TOKEN = "VND";
			inline static const char* MPN_RECORD_TOKEN = "MPN";

		};	// BomDescriptionRecord

		const ComponentRecord::Vector& GetComponentRecords() const;
		const ComponentRecord::StringMap& GetComponentRecordsByName() const;
		const std::vector<std::string>& GetAttributeNames() const;
		const std::vector<std::string>& GetAttributeTextValues() const;
		const BomDescriptionRecord::StringMap& GetBomDescriptionRecordsByCpn() const;

		constexpr inline static const char* TOP_COMPONENTS_LAYER_NAME = "comp_+_top";
		constexpr inline static const char* BOTTOM_COMPONENTS_LAYER_NAME = "comp_+_bot";
		
		// Inherited via IProtoBuffable
		std::unique_ptr<Odb::Lib::Protobuf::ComponentsFile> to_protobuf() const override;
		void from_protobuf(const Odb::Lib::Protobuf::ComponentsFile& message) override;

	private:
		std::string m_units;
		unsigned int m_id;
		BoardSide m_side;
		std::string m_layerName;
		std::filesystem::path m_path;
		std::filesystem::path m_directory;

		std::vector<std::string> m_attributeNames;
		std::vector<std::string> m_attributeTextValues;

		ComponentRecord::Vector m_componentRecords;
		// TODO: add records to maps by name while adding to their vectors
		ComponentRecord::StringMap m_componentRecordsByName;

		BomDescriptionRecord::StringMap m_bomDescriptionRecordsByCpn;

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