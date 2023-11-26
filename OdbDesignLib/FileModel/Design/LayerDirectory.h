#pragma once

#include <string>
#include <map>
#include <memory>
#include <filesystem>
#include "../../odbdesign_export.h"
#include "ComponentsFile.h"


namespace Odb::Lib::FileModel::Design
{
	class ODBDESIGN_EXPORT LayerDirectory
	{
	public:
		LayerDirectory(std::filesystem::path path);
		virtual ~LayerDirectory();		

		std::string GetName() const;
		std::filesystem::path GetPath() const;

		virtual bool Parse();

		bool ParseComponentsFile(std::filesystem::path directory);
		bool ParseFeaturesFile(std::filesystem::path directory);

		const ComponentsFile& GetComponentsFile() const;

		typedef std::map<std::string, std::shared_ptr<LayerDirectory>> StringMap;

	protected: // TODO: do subclasses really need access to these (private instead)?
		std::string m_name;
		std::filesystem::path m_path;

	private:
		ComponentsFile m_componentsFile;

	};
}