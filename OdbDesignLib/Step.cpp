#include "Step.h"
#include <filesystem>
#include "Layer.h"
#include "ComponentLayer.h"
#include <fstream>
#include <sstream>


namespace OdbDesign::Lib
{
    Step::Step(std::filesystem::path path)
        : m_path(path)
    {
    }

    Step::~Step()
    {
        m_layersByName.clear();
    }

    std::string Step::GetName()
    {
        return m_name;
    }

    std::filesystem::path Step::GetPath()
    {
        return m_path;
    }

    const EdaData& Step::GetEdaData() const { return m_edaData; }
    const Layer::StringMap& Step::GetLayersByName() const { return m_layersByName; }
    const Netlist::StringMap& Step::GetNetlistsByName() const { return m_netlistsByName; }

    bool Step::Parse()
    {
        if (!std::filesystem::exists(m_path)) return false;
        else if (!std::filesystem::is_directory(m_path)) return false;

        m_name = std::filesystem::path(m_path).filename().string();

        auto layersPath = m_path / "layers";
        if (!ParseLayers(layersPath)) return false;

        auto netlistsPath = m_path / "netlists";
        if (!ParseNetlists(netlistsPath)) return false;

        auto edaPath = m_path / "eda";
        if (!ParseEdaData(edaPath)) return false;

        return true;
    }

    bool Step::ParseLayers(std::filesystem::path layersPath)
    {
        if (!std::filesystem::exists(layersPath)) return false;
        else if (!std::filesystem::is_directory(layersPath)) return false;

        for (auto& d : std::filesystem::directory_iterator(layersPath))
        {
            if (std::filesystem::is_directory(d))
            {
                std::shared_ptr<Layer> pLayer;

                auto layerName = d.path().filename().string();
                if (layerName == Layer::TOP_COMPONENTS_LAYER_NAME ||
                    layerName == Layer::BOTTOM_COMPONENTS_LAYER_NAME)
                {
                    pLayer = std::make_shared<ComponentLayer>(d.path());
                }
                else
                {
                    pLayer = std::make_shared<Layer>(d.path());
                }

                if (pLayer->Parse())
                {
                    m_layersByName[pLayer->GetName()] = pLayer;
                }
                else
                {
                    return false;
                }
            }
        }

        return true;
    }

    bool Step::ParseEdaData(std::filesystem::path edaPath)
    {
        if (!std::filesystem::exists(edaPath)) return false;
        else if (!std::filesystem::is_directory(edaPath)) return false;

        // parse nets and packages definitions      
        return m_edaData.Parse(edaPath);
    }

    bool Step::ParseNetlists(std::filesystem::path netlistsPath)
    {
        if (!std::filesystem::exists(netlistsPath)) return false;
        else if (!std::filesystem::is_directory(netlistsPath)) return false;

        // parse net name records
        for (auto& d : std::filesystem::directory_iterator(netlistsPath))
        {
            if (std::filesystem::is_directory(d))
            {
                auto pNetlist = std::make_shared<Netlist>(d.path());
                if (pNetlist->Parse())
                {
                    m_netlistsByName[pNetlist->GetName()] = pNetlist;
                }
            }
        }

        return true;
    }
}