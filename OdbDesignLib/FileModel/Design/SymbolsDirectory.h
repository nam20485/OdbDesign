#pragma once

#include "../../odbdesign_export.h"
#include "FeaturesFile.h"
#include "AttrListFile.h"
#include <vector>
#include <map>
#include <string>
#include <memory>
#include <filesystem>

namespace Odb::Lib::FileModel::Design
{
	class ODBDESIGN_EXPORT SymbolsDirectory
	{
	public:
		SymbolsDirectory(const std::filesystem::path& path);

		typedef std::vector<std::shared_ptr<SymbolsDirectory>> Vector;
		typedef std::map<std::string, std::shared_ptr<SymbolsDirectory>> StringMap;

		bool Parse();

		std::string GetName() const;
		std::filesystem::path GetPath() const;

		const FeaturesFile& GetFeaturesFile() const;
		const AttrListFile& GetAttrListFile() const;

	private:
		std::string m_name;		
		std::filesystem::path m_path;

		FeaturesFile m_featuresFile;
		AttrListFile m_attrListFile;

		bool ParseFeaturesFile(const std::filesystem::path& directory);
		bool ParseAttrListFile(const std::filesystem::path& directory);

	};
}
