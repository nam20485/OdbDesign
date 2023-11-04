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
		m_name = m_path.filename().string();

		loginfo("Parsing layer: " + m_name + "...");

		return true;
	}
}