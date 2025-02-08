#include "SymbolsDirectory.h"
#include "Logger.h"
#include <filesystem>
#include <string>
#include "FeaturesFile.h"
#include "AttrListFile.h"
#include <memory>

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

	std::unique_ptr<Odb::Lib::Protobuf::SymbolsDirectory> SymbolsDirectory::to_protobuf() const
	{
		auto message = std::make_unique<Odb::Lib::Protobuf::SymbolsDirectory>();
		message->set_name(m_name);
		message->set_path(m_path.string());
		message->mutable_attrlistfile()->CopyFrom(*m_attrListFile.to_protobuf());
		message->mutable_featurefile()->CopyFrom(*m_featuresFile.to_protobuf());
		return message;
	}

	void SymbolsDirectory::from_protobuf(const Odb::Lib::Protobuf::SymbolsDirectory& message)
	{
		m_name = message.name();
		m_path = message.path();
		m_attrListFile.from_protobuf(message.attrlistfile());
		m_featuresFile.from_protobuf(message.featurefile());
	}
	
	bool SymbolsDirectory::ParseFeaturesFile(const std::filesystem::path& directory)
	{
		return m_featuresFile.Parse(directory);
	}

	bool SymbolsDirectory::ParseAttrListFile(const std::filesystem::path& directory)
	{
		return m_attrListFile.Parse(directory);
	}

	bool SymbolsDirectory::Save(const std::filesystem::path& directory)
	{
		auto symbolDir = directory / m_name;
		if (!create_directory(symbolDir)) return false;

		//FeaturesFile m_featuresFile;
		std::ofstream featuresFile(symbolDir / "features");
		if (!featuresFile.is_open()) return false;
		if (!m_featuresFile.Save(featuresFile)) return false;
		featuresFile.close();

		//AttrListFile m_attrListFile;
		std::ofstream attrlistFile(symbolDir / "attrlist");
		if (!attrlistFile.is_open()) return false;
		if (!m_attrListFile.Save(attrlistFile)) return false;
		attrlistFile.close();

		return true;
	}
}