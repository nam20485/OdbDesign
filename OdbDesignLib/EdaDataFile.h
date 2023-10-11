#pragma once

#include <filesystem>
#include <string>
#include <vector>
#include <map>
#include "export.h"
#include "BoardSide.h"
#include "proto/edadatafile.pb.h"
#include <google/protobuf/message.h>
#include "IProtoBuffable.h"


namespace Odb::Lib::FileModel::Design
{	
	class DECLSPEC EdaDataFile : public IProtoBuffable<odbdesign::proto::EdaDataFile>
	{
	public:
		EdaDataFile();		
		~EdaDataFile();

		const std::filesystem::path& GetPath() const;
		const std::string& GetUnits() const;
		const std::string& GetSource() const;

		bool Parse(std::filesystem::path path);		

		struct DECLSPEC PropertyRecord : public IProtoBuffable<odbdesign::proto::EdaDataFile::PropertyRecord>
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
			std::unique_ptr<odbdesign::proto::EdaDataFile::PropertyRecord> to_protobuf() const override;
			void from_protobuf(const odbdesign::proto::EdaDataFile::PropertyRecord& message) override;
		};

		struct DECLSPEC NetRecord : public IProtoBuffable<odbdesign::proto::EdaDataFile::NetRecord>
		{
			struct DECLSPEC SubnetRecord : public IProtoBuffable<odbdesign::proto::EdaDataFile::NetRecord::SubnetRecord>
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

				struct DECLSPEC FeatureIdRecord : public IProtoBuffable<odbdesign::proto::EdaDataFile::NetRecord::SubnetRecord::FeatureIdRecord>
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
					std::unique_ptr<odbdesign::proto::EdaDataFile::NetRecord::SubnetRecord::FeatureIdRecord> to_protobuf() const override;
					void from_protobuf(const odbdesign::proto::EdaDataFile::NetRecord::SubnetRecord::FeatureIdRecord& message) override;
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
				std::unique_ptr<odbdesign::proto::EdaDataFile::NetRecord::SubnetRecord> to_protobuf() const override;
				void from_protobuf(const odbdesign::proto::EdaDataFile::NetRecord::SubnetRecord& message) override;

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
			std::unique_ptr<odbdesign::proto::EdaDataFile::NetRecord> to_protobuf() const override;
			void from_protobuf(const odbdesign::proto::EdaDataFile::NetRecord& message) override;

		}; // NetRecord

		struct DECLSPEC PackageRecord : public IProtoBuffable<odbdesign::proto::EdaDataFile::PackageRecord>
		{
			struct DECLSPEC PinRecord : public IProtoBuffable<odbdesign::proto::EdaDataFile::PackageRecord::PinRecord>
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
				size_t index;

				// Inherited via IProtoBuffable
				std::unique_ptr<odbdesign::proto::EdaDataFile::PackageRecord::PinRecord> to_protobuf() const override;
				void from_protobuf(const odbdesign::proto::EdaDataFile::PackageRecord::PinRecord& message) override;

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
			std::unique_ptr<odbdesign::proto::EdaDataFile::PackageRecord> to_protobuf() const override;
			void from_protobuf(const odbdesign::proto::EdaDataFile::PackageRecord& message) override;

		}; // PackageRecord

		const std::vector<std::string>& GetLayerNames() const;
		const std::vector<std::string>& GetAttributeNames() const;
		const std::vector<std::string>& GetAttributeTextValues() const;

		const NetRecord::Vector& GetNetRecords() const;
		const NetRecord::StringMap& GetNetRecordsByName() const;
		const PackageRecord::Vector& GetPackageRecords() const;
		const PackageRecord::StringMap& GetPackageRecordsByName() const;

		// Inherited via IProtoBuffable
		std::unique_ptr<odbdesign::proto::EdaDataFile> to_protobuf() const override;
		void from_protobuf(const odbdesign::proto::EdaDataFile& message) override;		

	private:
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

		inline static const std::string COMMENT_TOKEN = "#";
		inline static const std::string UNITS_TOKEN = "UNITS";
		inline static const std::string HEADER_RECORD_TOKEN = "HDR";
		inline static const std::string LAYER_NAMES_RECORD_TOKEN = "LYR";
		inline static const std::string PROPERTY_RECORD_TOKEN = "PRP";
		inline static const std::string ATTRIBUTE_NAME_TOKEN = "@";
		inline static const std::string ATTRIBUTE_VALUE_TOKEN = "&";
		inline static const std::string NET_RECORD_TOKEN = "NET";
		inline static const std::string FEATURE_ID_RECORD_TOKEN = "FID";
		inline static const std::string PACKAGE_RECORD_TOKEN = "PKG";
		inline static const std::string PIN_RECORD_TOKEN = "PIN";
		inline static const std::string FEATURE_GROUP_RECORD_TOKEN = "FGR";
		// TODO: Outline records:
		// RC, CR, SQ, CT, OB, OS, OC, OE, CE — Outline Records
	
	}; // EdaDataFile

	//EXPIMP_TEMPLATE template class DECLSPEC std::vector<std::shared_ptr<EdaData::NetRecord>>;
	//EXPIMP_TEMPLATE template class DECLSPEC std::map<std::string, std::shared_ptr<EdaData::NetRecord>>;
}