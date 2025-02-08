#pragma once

#include <filesystem>
#include <string>
#include <vector>
#include <map>
#include "../../odbdesign_export.h"
#include "../../enums.h"
#include "../../ProtoBuf/edadatafile.pb.h"
#include "../../ProtoBuf/common.pb.h"
#include "../../IProtoBuffable.h"
#include "PropertyRecord.h"
#include "ContourPolygon.h"
#include "AttributeLookupTable.h"
#include "../IStreamSaveable.h"


namespace Odb::Lib::FileModel::Design
{	
	class ODBDESIGN_EXPORT EdaDataFile : public IProtoBuffable<Odb::Lib::Protobuf::EdaDataFile>, public IStreamSaveable
	{
	public:
		EdaDataFile(bool logAllLineParsing = false);
		~EdaDataFile();

		const std::filesystem::path& GetPath() const;
		const std::filesystem::path& GetDirectory() const;
		const std::string& GetUnits() const;
		const std::string& GetSource() const;

		bool Parse(std::filesystem::path path);	
		// Inherited via IStreamSaveable
		bool Save(std::ostream& os) override;
		
		struct ODBDESIGN_EXPORT FeatureIdRecord : public IProtoBuffable<Odb::Lib::Protobuf::EdaDataFile::FeatureIdRecord>
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
			std::unique_ptr<Odb::Lib::Protobuf::EdaDataFile::FeatureIdRecord> to_protobuf() const override;
			void from_protobuf(const Odb::Lib::Protobuf::EdaDataFile::FeatureIdRecord& message) override;
		};

		struct ODBDESIGN_EXPORT NetRecord : public IProtoBuffable<Odb::Lib::Protobuf::EdaDataFile::NetRecord>, public AttributeLookupTable
		{
			struct ODBDESIGN_EXPORT SubnetRecord final : public IProtoBuffable<Odb::Lib::Protobuf::EdaDataFile::NetRecord::SubnetRecord>
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

				typedef std::vector<std::shared_ptr<SubnetRecord>> Vector;

				~SubnetRecord();

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
				double fillSize;
				unsigned int index;

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
			unsigned int index;

			SubnetRecord::Vector m_subnetRecords;
			PropertyRecord::Vector m_propertyRecords;

			// Inherited via IProtoBuffable
			std::unique_ptr<Odb::Lib::Protobuf::EdaDataFile::NetRecord> to_protobuf() const override;
			void from_protobuf(const Odb::Lib::Protobuf::EdaDataFile::NetRecord& message) override;

		}; // NetRecord

		struct ODBDESIGN_EXPORT PackageRecord : public IProtoBuffable<Odb::Lib::Protobuf::EdaDataFile::PackageRecord>, public AttributeLookupTable
		{
			struct ODBDESIGN_EXPORT OutlineRecord : public IProtoBuffable<Odb::Lib::Protobuf::EdaDataFile::PackageRecord::OutlineRecord>
			{				
				enum class Type
				{
					Rectangle,
					Circle,
					Square,
					Contour
				};

				typedef std::vector<std::shared_ptr<OutlineRecord>> Vector;

				~OutlineRecord()
				{
					m_contourPolygons.clear();
				}

				Type type;

				// Rectangle
				double lowerLeftX;
				double lowerLeftY;
				double width;
				double height;

				// Square/Circle
				double xCenter;
				double yCenter;

				// Square
				double halfSide;
				// Circle
				double radius;

				ContourPolygon::Vector m_contourPolygons;

				// Inherited via IProtoBuffable
				std::unique_ptr<Odb::Lib::Protobuf::EdaDataFile::PackageRecord::OutlineRecord> to_protobuf() const override;
				void from_protobuf(const Odb::Lib::Protobuf::EdaDataFile::PackageRecord::OutlineRecord& message) override;

				inline static const char* RECTANGLE_RECORD_TOKEN = "RC";
				inline static const char* CIRCLE_RECORD_TOKEN = "CR";
				inline static const char* SQUARE_RECORD_TOKEN = "SQ";
				inline static const char* CONTOUR_BEGIN_RECORD_TOKEN = "CT";
				inline static const char* CONTOUR_END_RECORD_TOKEN = "CE";

			}; // struct OutlineRecord

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

				~PinRecord()
				{
					m_outlineRecords.clear();
				}

				std::string name;
				Type type;
				double xCenter;
				double yCenter;
				double finishedHoleSize;	// unused, set to 0
				ElectricalType electricalType;
				MountType mountType;
				unsigned int id;
				unsigned int index;

				OutlineRecord::Vector m_outlineRecords;

				// Inherited via IProtoBuffable
				std::unique_ptr<Odb::Lib::Protobuf::EdaDataFile::PackageRecord::PinRecord> to_protobuf() const override;
				void from_protobuf(const Odb::Lib::Protobuf::EdaDataFile::PackageRecord::PinRecord& message) override;

			}; // PinRecord

			typedef std::vector<std::shared_ptr<PackageRecord>> Vector;
			typedef std::map<std::string, std::shared_ptr<PackageRecord>> StringMap;

			~PackageRecord()
			{
				m_outlineRecords.clear();
				m_pinRecords.clear();
				m_pinRecordsByName.clear();
				m_propertyRecords.clear();
			}

			std::string name;
			double pitch;
			double xMin, yMin;
			double xMax, yMax;			
			unsigned int index;

			OutlineRecord::Vector m_outlineRecords;
			PinRecord::Vector m_pinRecords;
			PinRecord::StringMap m_pinRecordsByName;
			PropertyRecord::Vector m_propertyRecords;

			// Inherited via IProtoBuffable
			std::unique_ptr<Odb::Lib::Protobuf::EdaDataFile::PackageRecord> to_protobuf() const override;
			void from_protobuf(const Odb::Lib::Protobuf::EdaDataFile::PackageRecord& message) override;

		}; // PackageRecord

		struct FeatureGroupRecord : public IProtoBuffable<Odb::Lib::Protobuf::EdaDataFile::FeatureGroupRecord>
		{
			~FeatureGroupRecord()
			{
				m_propertyRecords.clear();
				m_featureIdRecords.clear();
			}

			std::string type;	// always "TEXT" per spec
			
			PropertyRecord::Vector m_propertyRecords;
			FeatureIdRecord::Vector m_featureIdRecords;

			typedef std::shared_ptr<FeatureGroupRecord> shared_ptr;
			typedef std::vector<FeatureGroupRecord::shared_ptr> Vector;	

			// Inherited via IProtoBuffable
			std::unique_ptr<Odb::Lib::Protobuf::EdaDataFile::FeatureGroupRecord> to_protobuf() const override;
			void from_protobuf(const Odb::Lib::Protobuf::EdaDataFile::FeatureGroupRecord& message) override;

		}; // FeatureGroupRecord

		const std::vector<std::string>& GetLayerNames() const;
		const std::vector<std::string>& GetAttributeNames() const;
		const std::vector<std::string>& GetAttributeTextValues() const;

		const NetRecord::Vector& GetNetRecords() const;
		const NetRecord::StringMap& GetNetRecordsByName() const;
		const PackageRecord::Vector& GetPackageRecords() const;
		const PackageRecord::StringMap& GetPackageRecordsByName() const;
		const FeatureGroupRecord::Vector& GetFeatureGroupRecords() const;
		const PropertyRecord::Vector& GetPropertyRecords() const;

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

		FeatureGroupRecord::Vector m_featureGroupRecords;

		PropertyRecord::Vector m_propertyRecords;

		bool m_logAllLineParsing;
		
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
	
	}; // EdaDataFile

	//EXPIMP_TEMPLATE template class ODBDESIGN_EXPORT std::vector<std::shared_ptr<EdaData::NetRecord>>;
	//EXPIMP_TEMPLATE template class ODBDESIGN_EXPORT std::map<std::string, std::shared_ptr<EdaData::NetRecord>>;
}