#include "Layer.h"

namespace OdbDesign::Lib::FileModel
{
	Layer::Layer(std::filesystem::path path)
		: m_path(path)
	{
	}

	Layer::~Layer()
	{
	}

	std::string Layer::GetName() const
	{
		return m_name;
	}

	std::filesystem::path Layer::GetPath() const
	{
		return m_path;
	}

	bool Layer::Parse()
	{
		m_name = std::filesystem::path(m_path).filename().string();
		return true;
	}
}