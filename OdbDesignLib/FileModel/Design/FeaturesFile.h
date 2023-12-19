#pragma once

#include "ContourPolygon.h"
#include <filesystem>
#include <vector>
#include <string>
#include "../../enums.h"
#include "../../IProtoBuffable.h"
#include "../../ProtoBuf/featuresfile.pb.h"
#include "SymbolName.h"
#include "AttributeLookupTable.h"


namespace Odb::Lib::FileModel::Design
{
	class FeaturesFile : public IProtoBuffable<Odb::Lib::Protobuf::FeaturesFile>
	{
	public:
		FeaturesFile();
		~FeaturesFile();

		struct FeatureRecord : public IProtoBuffable<Odb::Lib::Protobuf::FeaturesFile::FeatureRecord>, public AttributeLookupTable
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
			unsigned int id;

			int orient_def;
			float orient_def_rotation;			

			ContourPolygon::Vector m_contourPolygons;

			const ContourPolygon::Vector& GetContourPolygons() const;

			typedef std::vector<std::shared_ptr<FeatureRecord>> Vector;		

			// Inherited via IProtoBuffable
			std::unique_ptr<Odb::Lib::Protobuf::FeaturesFile::FeatureRecord> to_protobuf() const override;
			void from_protobuf(const Odb::Lib::Protobuf::FeaturesFile::FeatureRecord& message) override;

			constexpr inline static const char* LINE_TOKEN = "L";
			constexpr inline static const char* PAD_TOKEN = "P";
			constexpr inline static const char* TEXT_TOKEN = "T";
			constexpr inline static const char* ARC_TOKEN = "A";
			constexpr inline static const char* BARCODE_TOKEN = "B";
			constexpr inline static const char* SURFACE_START_TOKEN = "S";
			constexpr inline static const char* SURFACE_END_TOKEN = "SE";
		};		

		bool Parse(std::filesystem::path directory, const std::string& alternateFilename = "");

		std::string GetUnits() const;
		std::filesystem::path GetPath();
		std::filesystem::path GetDirectory();
		int GetNumFeatures() const;
		unsigned int GetId() const;

		const SymbolName::StringMap& GetSymbolNamesByName() const;
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
		SymbolName::StringMap m_symbolNamesByName;

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
		constexpr inline static const char* SYMBOL_NAME_TOKEN = "$";
		constexpr inline static const char* COMMENT_TOKEN = "#";
		constexpr inline static const char* NUM_FEATURES_TOKEN = "F";		

	};
}

