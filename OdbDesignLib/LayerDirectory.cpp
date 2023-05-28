#include "LayerDirectory.h"

namespace OdbDesign::Lib::FileModel
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
		m_name = std::filesystem::path(m_path).filename().string();
		return true;
	}
}