#pragma once

#include <filesystem>
#include <string>
#include <vector>
#include <map>
#include "../../odbdesign_export.h"
#include "../../enums.h"
#include "../../ProtoBuf/edadatafile.pb.h"
#include "google/protobuf/message.h"
#include "../../IProtoBuffable.h"


namespace Odb::Lib::FileModel::Design
{	
	class ODBDESIGN_EXPORT EdaDataFile : public IProtoBuffable<Odb::Lib::Protobuf::EdaDataFile>
	{
	public:
		EdaDataFile();		
		~EdaDataFile();

		const std::filesystem::path& GetPath() const;
		const std::filesystem::path& GetDirectory() const;
		const std::string& GetUnits() const;
		const std::string& GetSource() const;

		bool Parse(std::filesystem::path path);		

		struct ODBDESIGN_EXPORT PropertyRecord : public IProtoBuffable<Odb::Lib::Protobuf::EdaDataFile::PropertyRecord>
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

			// Inherited via IProtoBuffable
			std::unique_ptr<Odb::Lib::Protobuf::EdaDataFile::PropertyRecord> to_protobuf() const override;
			void from_protobuf(const Odb::Lib::Protobuf::EdaDataFile::PropertyRecord& message) override;
		};

		struct ODBDESIGN_EXPORT NetRecord : public IProtoBuffable<Odb::Lib::Protobuf::EdaDataFile::NetRecord>
		{
			struct ODBDESIGN_EXPORT SubnetRecord : public IProtoBuffable<Odb::Lib::Protobuf::EdaDataFile::NetRecord::SubnetRecord>
			{
				// common subnet enums
				enum class Type
				{
					Via,
					Trace,
					Plane,
					Toeprint
				};

				// Plane subnet type enums
				enum class FillType
				{
					Solid,
					Outline
				};

				enum class CutoutType
				{
					Circle,
					Rectangle,
					Octagon,
					Exact
				};				

				struct ODBDESIGN_EXPORT FeatureIdRecord : public IProtoBuffable<Odb::Lib::Protobuf::EdaDataFile::NetRecord::SubnetRecord::FeatureIdRecord>
				{
					enum class Type
					{
						Copper,
						Laminate,
						Hole
					};

					typedef std::vector<std::shared_ptr<FeatureIdRecord>> Vector;

					Type type;
					unsigned int layerNumber;
					unsigned int featureNumber;

					// Inherited via IProtoBuffable
					std::unique_ptr<Odb::Lib::Protobuf::EdaDataFile::NetRecord::SubnetRecord::FeatureIdRecord> to_protobuf() const override;
					void from_protobuf(const Odb::Lib::Protobuf::EdaDataFile::NetRecord::SubnetRecord::FeatureIdRecord& message) override;
				};

				typedef std::vector<std::shared_ptr<SubnetRecord>> Vector;

				virtual ~SubnetRecord();

				// common subnet fields
				Type type;
				FeatureIdRecord::Vector m_featureIdRecords;

				// Toeprint subnet type fields
				BoardSide side;
				unsigned int componentNumber;	// component index in the layer components/placements file
				unsigned toeprintNumber;		// toeprint index of component reference in the layer components/placements file

				// Plane subnet type fields
				FillType fillType;
				CutoutType cutoutType;
				float fillSize;

				inline static const std::string RECORD_TOKEN = "SNT";
				inline static const std::string RECORD_TYPE_TRACE_TOKEN = "TRC";
				inline static const std::string RECORD_TYPE_VIA_TOKEN = "VIA";
				inline static const std::string RECORD_TYPE_TOEPRINT_TOKEN = "TOP";
				inline static const std::string RECORD_TYPE_PLANE_TOKEN = "PLN";				

				// Inherited via IProtoBuffable
				std::unique_ptr<Odb::Lib::Protobuf::EdaDataFile::NetRecord::SubnetRecord> to_protobuf() const override;
				void from_protobuf(const Odb::Lib::Protobuf::EdaDataFile::NetRecord::SubnetRecord& message) override;

			}; // SubnetRecord	

			typedef std::vector<std::shared_ptr<NetRecord>> Vector;
			typedef std::map<std::string, std::shared_ptr<NetRecord>> StringMap;

			~NetRecord();

			std::string name;
			std::string attributesIdString;
			// TODO: store index of records
			unsigned long index;

			SubnetRecord::Vector m_subnetRecords;
			PropertyRecord::Vector m_propertyRecords;

			// Inherited via IProtoBuffable
			std::unique_ptr<Odb::Lib::Protobuf::EdaDataFile::NetRecord> to_protobuf() const override;
			void from_protobuf(const Odb::Lib::Protobuf::EdaDataFile::NetRecord& message) override;

		}; // NetRecord

		struct ODBDESIGN_EXPORT PackageRecord : public IProtoBuffable<Odb::Lib::Protobuf::EdaDataFile::PackageRecord>
		{
			struct ODBDESIGN_EXPORT PinRecord : public IProtoBuffable<Odb::Lib::Protobuf::EdaDataFile::PackageRecord::PinRecord>
			{
				enum class Type
				{
					ThroughHole,
					Blind,
					Surface
				};

				enum class ElectricalType
				{
					Electrical,
					NonElectrical,
					Undefined
				};

				enum class MountType
				{
					Smt,
					RecommendedSmtPad,
					MT_ThroughHole,
					RecommendedThroughHole,
					Pressfit,
					NonBoard,
					Hole,
					MT_Undefined	// default
				};

				typedef std::vector<std::shared_ptr<PinRecord>> Vector;
				typedef std::map<std::string, std::shared_ptr<PinRecord>> StringMap;

				std::string name;
				Type type;
				float xCenter;
				float yCenter;
				float finishedHoleSize;	// unused, set to 0
				ElectricalType electricalType;
				MountType mountType;
				unsigned int id;
				unsigned int index;

				// Inherited via IProtoBuffable
				std::unique_ptr<Odb::Lib::Protobuf::EdaDataFile::PackageRecord::PinRecord> to_protobuf() const override;
				void from_protobuf(const Odb::Lib::Protobuf::EdaDataFile::PackageRecord::PinRecord& message) override;

			}; // PinRecord

			typedef std::vector<std::shared_ptr<PackageRecord>> Vector;
			typedef std::map<std::string, std::shared_ptr<PackageRecord>> StringMap;

			std::string name;
			float pitch;
			float xMin, yMin;
			float xMax, yMax;
			std::string attributesIdString;

			PinRecord::Vector m_pinRecords;
			PinRecord::StringMap m_pinRecordsByName;

			PropertyRecord::Vector m_propertyRecords;

			// Inherited via IProtoBuffable
			std::unique_ptr<Odb::Lib::Protobuf::EdaDataFile::PackageRecord> to_protobuf() const override;
			void from_protobuf(const Odb::Lib::Protobuf::EdaDataFile::PackageRecord& message) override;

		}; // PackageRecord

		const std::vector<std::string>& GetLayerNames() const;
		const std::vector<std::string>& GetAttributeNames() const;
		const std::vector<std::string>& GetAttributeTextValues() const;

		const NetRecord::Vector& GetNetRecords() const;
		const NetRecord::StringMap& GetNetRecordsByName() const;
		const PackageRecord::Vector& GetPackageRecords() const;
		const PackageRecord::StringMap& GetPackageRecordsByName() const;

		// Inherited via IProtoBuffable
		std::unique_ptr<Odb::Lib::Protobuf::EdaDataFile> to_protobuf() const override;
		void from_protobuf(const Odb::Lib::Protobuf::EdaDataFile& message) override;		

	private:
		std::filesystem::path m_directory;
		std::filesystem::path m_path;
		std::string m_units;

		std::string m_source;
		std::vector<std::string> m_layerNames;

		std::vector<std::string> m_attributeNames;
		std::vector<std::string> m_attributeTextValues;

		NetRecord::Vector m_netRecords;
		NetRecord::StringMap m_netRecordsByName;

		PackageRecord::Vector m_packageRecords;
		PackageRecord::StringMap m_packageRecordsByName;

		static std::filesystem::path getUncompressedFilePath(const std::filesystem::path& directory, const std::string& filename);

		inline static const char* EDADATA_FILENAME = "data";
		
		inline static const char* COMMENT_TOKEN = "#";
		inline static const char* UNITS_TOKEN = "UNITS";
		inline static const char* HEADER_RECORD_TOKEN = "HDR";
		inline static const char* LAYER_NAMES_RECORD_TOKEN = "LYR";
		inline static const char* PROPERTY_RECORD_TOKEN = "PRP";
		inline static const char* ATTRIBUTE_NAME_TOKEN = "@";
		inline static const char* ATTRIBUTE_VALUE_TOKEN = "&";
		inline static const char* NET_RECORD_TOKEN = "NET";
		inline static const char* FEATURE_ID_RECORD_TOKEN = "FID";
		inline static const char* PACKAGE_RECORD_TOKEN = "PKG";
		inline static const char* PIN_RECORD_TOKEN = "PIN";
		inline static const char* FEATURE_GROUP_RECORD_TOKEN = "FGR";
		// TODO: Outline records:
		// RC, CR, SQ, CT, OB, OS, OC, OE, CE � Outline Records
	
	}; // EdaDataFile

	//EXPIMP_TEMPLATE template class ODBDESIGN_EXPORT std::vector<std::shared_ptr<EdaData::NetRecord>>;
	//EXPIMP_TEMPLATE template class ODBDESIGN_EXPORT std::map<std::string, std::shared_ptr<EdaData::NetRecord>>;
}