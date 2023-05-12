#pragma once

#include <filesystem>
#include <string>
#include <vector>
#include <map>
#include "export.h"


namespace OdbDesign::Lib
{	
	class DECLSPEC EdaData
	{
	public:
		EdaData();
		//EdaData(std::filesystem::path path);
		~EdaData();

		std::filesystem::path GetPath() const;
		std::string GetUnits() const;

		bool Parse(std::filesystem::path path);

		struct DECLSPEC PropertyRecord
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

		struct DECLSPEC NetRecord
		{
			bool operator < (const NetRecord c) const
			{
				return name < c.name;
			}
			bool operator == (const NetRecord c) const
			{
				return name == c.name;
			}

			struct DECLSPEC SubnetRecord
			{
				bool operator < (const SubnetRecord c) const
				{
					return type < c.type;
				}
				bool operator == (const SubnetRecord c) const
				{
					return type == c.type;
				}
				enum class Type
				{
					Via,
					Trace,
					Plane,
					Toeprint
				};

				struct DECLSPEC FeatureIdRecord
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
				};

				typedef std::vector<std::shared_ptr<SubnetRecord>> Vector;

				virtual ~SubnetRecord();

				Type type;
				FeatureIdRecord::Vector m_featureIdRecords;
			};

			struct DECLSPEC ToeprintSubnetRecord : public SubnetRecord
			{
				enum class Side
				{
					Top,
					Bottom
				};

				Side side;
				unsigned int componentNumber;
				unsigned toeprintNumber;
			};

			struct DECLSPEC PlaneSubnetRecord : public SubnetRecord
			{
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

				FillType fillType;
				CutoutType cutoutType;
				float fillSize;
			};

			typedef std::vector<std::shared_ptr<NetRecord>> Vector;
			typedef std::map<std::string, std::shared_ptr<NetRecord>> StringMap;

			~NetRecord();

			std::string name;
			std::string attributesIdString;

			SubnetRecord::Vector m_subnetRecords;
			PropertyRecord::Vector m_propertyRecords;
		};

		struct DECLSPEC PackageRecord
		{
			struct DECLSPEC PinRecord
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
					RecommenedSmtPad,
					ThroughHole,
					RecommenedThroughHole,
					Pressfit,
					NonBoard,
					Hole,
					Undefined	// default
				};

				typedef std::vector<std::shared_ptr<PinRecord>> Vector;
				typedef std::map<std::string, std::shared_ptr<PinRecord>> StringMap;

				std::string name;
				Type type;
				float centerX;
				float centerY;
				float finishedHoleSize;	// unused, set to 0
				ElectricalType electricalType;
				MountType mountType;
				unsigned int Id;
			};

			typedef std::vector<std::shared_ptr<PackageRecord>> Vector;
			typedef std::map<std::string, std::shared_ptr<PackageRecord>> StringMap;

			std::string name;
			float pitch;
			float xmin, ymin;
			float xmax, ymax;
			std::string attributesIdString;

			PinRecord::Vector m_pinRecords;
			PinRecord::StringMap m_pinRecordsByName;

			PropertyRecord::Vector m_propertyRecords;
		};

		const std::vector<std::string>& GetLayerNames() const;
		const std::vector<std::string>& GetAttributeNames() const;
		const std::vector<std::string>& GetAttributeTextValues() const;

		const NetRecord::Vector& GetNetRecords() const;
		const NetRecord::StringMap& GetNetRecordsByName() const;
		const PackageRecord::Vector& GetPackageRecords() const;
		const PackageRecord::StringMap& GetPackageRecordsByName() const;

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
		inline static const std::string SUBNET_RECORD_TOKEN = "SNT";
		inline static const std::string FEATURE_ID_RECORD_TOKEN = "FID";
		inline static const std::string PACKAGE_RECORD_TOKEN = "PKG";
		inline static const std::string PIN_RECORD_TOKEN = "PIN";
		inline static const std::string FEATURE_GROUP_RECORD_TOKEN = "FGR";
		// TODO: Outline records:
		// RC, CR, SQ, CT, OB, OS, OC, OE, CE — Outline Records

	};

	//EXPIMP_TEMPLATE template class DECLSPEC std::vector<std::shared_ptr<EdaData::NetRecord>>;
	//EXPIMP_TEMPLATE template class DECLSPEC std::map<std::string, std::shared_ptr<EdaData::NetRecord>>;
}