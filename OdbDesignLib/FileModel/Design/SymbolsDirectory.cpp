#include "SymbolsDirectory.h"
#include "Logger.h"
#include <filesystem>

namespace Odb::Lib::FileModel::Design
{
	SymbolsDirectory::SymbolsDirectory(const std::filesystem::path& path)
		: m_name("")
		, m_path(path)
	{
	}

	bool SymbolsDirectory::Parse()
	{

		if (!std::filesystem::exists(m_path)) return false;
		else if (!std::filesystem::is_directory(m_path)) return false;

		m_name = std::filesystem::path(m_path).filename().string();

		loginfo("Parsing symbols directory: " + m_name + "...");

		if (!ParseFeaturesFile(m_path)) return false;
		if (!ParseAttrListFile(m_path)) return false;

		loginfo("Parsing symbols directory: " + m_name + " complete");

		return true;
	}

	std::string SymbolsDirectory::GetName() const { return m_name; }
	std::filesystem::path SymbolsDirectory::GetPath() const { return m_path; }

	const FeaturesFile& SymbolsDirectory::GetFeaturesFile() const { return m_featuresFile; }
	const AttrListFile& SymbolsDirectory::GetAttrListFile() const { return m_attrListFile; }
	
	bool SymbolsDirectory::ParseFeaturesFile(const std::filesystem::path& directory)
	{
		return m_featuresFile.Parse(directory);
	}

	bool SymbolsDirectory::ParseAttrListFile(const std::filesystem::path& directory)
	{
		return m_attrListFile.Parse(directory);
	}
}