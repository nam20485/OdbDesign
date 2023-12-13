#pragma once

#include "ContourPolygon.h"
#include <filesystem>
#include <vector>
#include <string>
#include "../../enums.h"
#include "../../IProtoBuffable.h"
#include "../../ProtoBuf/featuresfile.pb.h"


namespace Odb::Lib::FileModel::Design
{
	class FeaturesFile : public IProtoBuffable<Odb::Lib::Protobuf::FeaturesFile>
	{
	public:
		FeaturesFile();
		~FeaturesFile();

		struct FeatureRecord : public IProtoBuffable<Odb::Lib::Protobuf::FeaturesFile::FeatureRecord>
		{
			~FeatureRecord();

			enum class Type
			{
				Arc,
				Pad,
				Surface,
				Barcode,
				Text,
				Line
			};			

			Type type;			
				
			// Line
			float xs, ys;
			float xe, ye;

			// Pad / Text
			float x, y;
			int apt_def;
			int apt_def_symbol_num;
			float apt_def_resize_factor;

			// Arc
			float xc, yc;
			bool cw;

			// Text
			std::string font;
			float xsize, ysize;
			float width_factor;
			std::string text;
			int version;

			//TODO: Barcode			

			// common
			int sym_num;
			Polarity polarity;
			int dcode;
			int atr;
			std::string value;
			unsigned int id;

			int orient_def;
			float orient_def_rotation;

			ContourPolygon::Vector m_contourPolygons;

			const ContourPolygon::Vector& GetContourPolygons() const;

			typedef std::vector<std::shared_ptr<FeatureRecord>> Vector;		

			// Inherited via IProtoBuffable
			std::unique_ptr<Odb::Lib::Protobuf::FeaturesFile::FeatureRecord> to_protobuf() const override;
			void from_protobuf(const Odb::Lib::Protobuf::FeaturesFile::FeatureRecord& message) override;
		};		

		bool Parse(std::filesystem::path directory);

		std::string GetUnits() const;
		std::filesystem::path GetPath();
		std::filesystem::path GetDirectory();
		int GetNumFeatures() const;
		unsigned int GetId() const;

		const FeatureRecord::Vector& GetFeatureRecords() const;

		// Inherited via IProtoBuffable
		std::unique_ptr<Odb::Lib::Protobuf::FeaturesFile> to_protobuf() const override;
		void from_protobuf(const Odb::Lib::Protobuf::FeaturesFile& message) override;
		
	private:
		std::string m_units;
		std::filesystem::path m_path;
		std::filesystem::path m_directory;
		int m_numFeatures;					// from field in file
		unsigned int m_id;

		FeatureRecord::Vector m_featureRecords;

		std::vector<std::string> m_attributeNames;
		std::vector<std::string> m_attributeTextValues;

		constexpr inline static const char* FEATURES_FILENAMES[] =
		{
			"features",
			"features2",
			"features3"
		};

		constexpr inline static const char* UNITS_TOKEN = "UNITS";
		constexpr inline static const char* ID_TOKEN = "ID";
		constexpr inline static const char* ATTRIBUTE_NAME_TOKEN = "@";
		constexpr inline static const char* ATTRIBUTE_VALUE_TOKEN = "&";
		constexpr inline static const char* SYMBOL_NAME_TOKEN = "&";
		constexpr inline static const char* COMMENT_TOKEN = "#";
		constexpr inline static const char* NUM_FEATURES_TOKEN = "F";		

	};
}

