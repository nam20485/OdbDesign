#include "LayerDirectory.h"
#include "LayerDirectory.h"
#include "LayerDirectory.h"
#include "LayerDirectory.h"
#include "Logger.h"

namespace Odb::Lib::FileModel::Design
{
	LayerDirectory::LayerDirectory(std::filesystem::path path)
		: m_path(path)
	{
	}

	LayerDirectory::~LayerDirectory()
	{
	}

	std::string LayerDirectory::GetName() const
	{
		return m_name;
	}

	std::filesystem::path LayerDirectory::GetPath() const
	{
		return m_path;
	}

	bool LayerDirectory::Parse()
	{		
		if (!std::filesystem::exists(m_path)) return false;
		else if (!std::filesystem::is_directory(m_path)) return false;

		m_name = std::filesystem::path(m_path).filename().string();

		loginfo("Parsing layer directory: " + m_name + "...");
		
		if (!ParseComponentsFile(m_path)) return false;
		if (!ParseFeaturesFile(m_path)) return false;

		loginfo("Parsing layer directory: " + m_name + " complete");

		return true;
	}

	bool Odb::Lib::FileModel::Design::LayerDirectory::ParseComponentsFile(std::filesystem::path directory)
	{
		return m_componentsFile.Parse(directory);
	}

	bool Odb::Lib::FileModel::Design::LayerDirectory::ParseFeaturesFile(std::filesystem::path directory)
	{
		return true;
	}
	const ComponentsFile& Odb::Lib::FileModel::Design::LayerDirectory::GetComponentsFile() const
	{
		return m_componentsFile;
	}
}