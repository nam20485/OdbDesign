#include "LayerDirectory.h"
#include "LayerDirectory.h"
#include "LayerDirectory.h"
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
		if (!ParseAttrListFile(m_path)) return false;

		loginfo("Parsing layer directory: " + m_name + " complete");

		return true;
	}

	bool LayerDirectory::ParseAttrListFile(std::filesystem::path directory)
	{
		return m_attrListFile.Parse(directory);
	}

	bool Odb::Lib::FileModel::Design::LayerDirectory::ParseComponentsFile(std::filesystem::path directory)
	{
		return m_componentsFile.Parse(directory);
	}

	bool Odb::Lib::FileModel::Design::LayerDirectory::ParseFeaturesFile(std::filesystem::path directory)
	{
		return m_featuresFile.Parse(directory);
	}

	const ComponentsFile& Odb::Lib::FileModel::Design::LayerDirectory::GetComponentsFile() const
	{
		return m_componentsFile;
	}

	const FeaturesFile& LayerDirectory::GetFeaturesFile() const
	{
		return m_featuresFile;
	}

	const AttrListFile& LayerDirectory::GetAttrListFile() const
	{
		return m_attrListFile;
	}

	std::unique_ptr<Odb::Lib::Protobuf::LayerDirectory> Odb::Lib::FileModel::Design::LayerDirectory::to_protobuf() const
	{		
		std::unique_ptr<Odb::Lib::Protobuf::LayerDirectory> pLayerDirectoryMessage(new Odb::Lib::Protobuf::LayerDirectory);
		pLayerDirectoryMessage->set_name(m_name);
		pLayerDirectoryMessage->set_path(m_path.string());
		pLayerDirectoryMessage->mutable_components()->CopyFrom(*m_componentsFile.to_protobuf());
		return pLayerDirectoryMessage;
	}

	void Odb::Lib::FileModel::Design::LayerDirectory::from_protobuf(const Odb::Lib::Protobuf::LayerDirectory& message)
	{
		m_name = message.name();
		m_path = message.path();
		m_componentsFile.from_protobuf(message.components());
	}
}