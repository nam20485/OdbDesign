#pragma once

#include <string>
#include <map>
#include <memory>
#include <filesystem>

#include "Layer.h"
#include "EdaData.h"
#include "Netlist.h"

namespace OdbDesign::Lib::FileModel
{
	class DECLSPEC Step
	{
	public:
		Step(std::filesystem::path path);
		~Step();

		std::string GetName();
		std::filesystem::path GetPath();

		const EdaData& GetEdaData() const;
		const Layer::StringMap& GetLayersByName() const;
		const Netlist::StringMap& GetNetlistsByName() const;

		bool Parse();

		typedef std::map<std::string, std::shared_ptr<Step>> StringMap;

	private:
		std::string m_name;
		std::filesystem::path m_path;

		Layer::StringMap m_layersByName;
		Netlist::StringMap m_netlistsByName;
		EdaData m_edaData;

		bool ParseLayers(std::filesystem::path layersPath);
		bool ParseNetlists(std::filesystem::path netlistsPath);
		bool ParseEdaData(std::filesystem::path edaPath);

	};
}

