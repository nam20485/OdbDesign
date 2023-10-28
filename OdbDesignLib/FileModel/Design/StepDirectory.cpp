#include "StepDirectory.h"
#include <filesystem>
#include "LayerDirectory.h"
#include "ComponentLayerDirectory.h"
#include <fstream>
#include <sstream>


namespace Odb::Lib::FileModel::Design
{
    StepDirectory::StepDirectory(std::filesystem::path path)
        : m_path(path)
    {
    }

    StepDirectory::~StepDirectory()
    {
        m_layersByName.clear();
    }

    std::string StepDirectory::GetName()
    {
        return m_name;
    }

    std::filesystem::path StepDirectory::GetPath()
    {
        return m_path;
    }

    const EdaDataFile& StepDirectory::GetEdaDataFile() const { return m_edaData; }
    const LayerDirectory::StringMap& StepDirectory::GetLayersByName() const { return m_layersByName; }
    const NetlistFile::StringMap& StepDirectory::GetNetlistsByName() const { return m_netlistsByName; }

    bool StepDirectory::Parse()
    {
        if (!std::filesystem::exists(m_path)) return false;
        else if (!std::filesystem::is_directory(m_path)) return false;

        m_name = std::filesystem::path(m_path).filename().string();

        auto layersPath = m_path / "layers";
        if (!ParseLayerFiles(layersPath)) return false;

        auto netlistsPath = m_path / "netlists";
        if (!ParseNetlistFiles(netlistsPath)) return false;

        auto edaPath = m_path / "eda";
        if (!ParseEdaDataFiles(edaPath)) return false;

        return true;
    }

    bool StepDirectory::ParseLayerFiles(std::filesystem::path layersPath)
    {
        if (!std::filesystem::exists(layersPath)) return false;
        else if (!std::filesystem::is_directory(layersPath)) return false;

        for (auto& d : std::filesystem::directory_iterator(layersPath))
        {
            if (std::filesystem::is_directory(d))
            {
                std::shared_ptr<LayerDirectory> pLayer;

                auto layerName = d.path().filename().string();
                if (layerName == LayerDirectory::TOP_COMPONENTS_LAYER_NAME ||
                    layerName == LayerDirectory::BOTTOM_COMPONENTS_LAYER_NAME)
                {
                    auto boardSide = layerName == LayerDirectory::TOP_COMPONENTS_LAYER_NAME ?
                        BoardSide::Top :
                        BoardSide::Bottom;
                    pLayer = std::make_shared<ComponentLayerDirectory>(d.path(), boardSide);
                }
                else
                {
                    pLayer = std::make_shared<LayerDirectory>(d.path());
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

    bool StepDirectory::ParseEdaDataFiles(std::filesystem::path edaPath)
    {
        if (!std::filesystem::exists(edaPath)) return false;
        else if (!std::filesystem::is_directory(edaPath)) return false;

        // parse nets and packages definitions      
        return m_edaData.Parse(edaPath);
    }

    bool StepDirectory::ParseNetlistFiles(std::filesystem::path netlistsPath)
    {
        if (!std::filesystem::exists(netlistsPath)) return false;
        else if (!std::filesystem::is_directory(netlistsPath)) return false;

        // parse net name records
        for (auto& d : std::filesystem::directory_iterator(netlistsPath))
        {
            if (std::filesystem::is_directory(d))
            {
                auto pNetlist = std::make_shared<NetlistFile>(d.path());
                if (pNetlist->Parse())
                {
                    m_netlistsByName[pNetlist->GetName()] = pNetlist;
                }
            }
        }

        return true;
    }

    std::shared_ptr<ComponentLayerDirectory> StepDirectory::GetTopComponentLayerDir() const
    {
        auto findIt = m_layersByName.find(LayerDirectory::TOP_COMPONENTS_LAYER_NAME);
        if (findIt != m_layersByName.end())
        {
			return std::dynamic_pointer_cast<ComponentLayerDirectory>(findIt->second);
		}
        else
        {
			return nullptr;
		}
    }

    std::shared_ptr<ComponentLayerDirectory> StepDirectory::GetBottomComponentLayerDir() const
    {
        auto findIt = m_layersByName.find(LayerDirectory::BOTTOM_COMPONENTS_LAYER_NAME);
        if (findIt != m_layersByName.end())
        {
            return std::dynamic_pointer_cast<ComponentLayerDirectory>(findIt->second);
        }
        else
        {
			return nullptr;
		}
    }
}