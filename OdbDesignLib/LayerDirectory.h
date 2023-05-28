#pragma once

#include <string>
#include <map>
#include <memory>
#include <filesystem>
#include "export.h"


namespace Odb::Lib::FileModel::Design
{
	class DECLSPEC LayerDirectory
	{
	public:
		LayerDirectory(std::filesystem::path path);
		~LayerDirectory();

		inline static const std::string TOP_COMPONENTS_LAYER_NAME = "comp_+_top";
		inline static const std::string BOTTOM_COMPONENTS_LAYER_NAME = "comp_+_bot";

		std::string GetName() const;
		std::filesystem::path GetPath() const;

		virtual bool Parse();

		typedef std::map<std::string, std::shared_ptr<LayerDirectory>> StringMap;

	protected: // TODO: do subclasses really need access to these (private instead)?
		std::string m_name;
		std::filesystem::path m_path;

	};
}