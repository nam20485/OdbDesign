#pragma once

#include "ContourPolygon.h"
#include <filesystem>
#include <vector>
#include <string>


namespace Odb::Lib::FileModel::Design
{
	class FeaturesFile
	{
	public:
		FeaturesFile();
		~FeaturesFile();

		struct FeatureRecord
		{
			~FeatureRecord();

			enum class Type
			{


			};

			// TODO: ALL the feature type fields

			ContourPolygon::Vector m_contourPolygons;

			const ContourPolygon::Vector& GetContourPolygons() const;

			typedef std::vector<std::shared_ptr<FeatureRecord>> Vector;			
		};		

		bool Parse(std::filesystem::path directory);

		std::string GetUnits() const;
		std::filesystem::path GetPath();
		std::filesystem::path GetDirectory();
		int GetNumFeatures() const;
		unsigned int GetId() const;

		const FeatureRecord::Vector& GetFeatureRecords() const;
		
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

