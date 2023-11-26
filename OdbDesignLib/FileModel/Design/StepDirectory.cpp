#include "StepDirectory.h"
#include "StepDirectory.h"
#include "StepDirectory.h"
#include <filesystem>
#include "LayerDirectory.h"
#include <fstream>
#include <sstream>
#include "Logger.h"


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

        loginfo("Parsing step directory: " + m_name + "...");

        auto layersPath = m_path / "layers";
        if (!ParseLayerFiles(layersPath)) return false;

        auto netlistsPath = m_path / "netlists";
        if (!ParseNetlistFiles(netlistsPath)) return false;

        auto edaPath = m_path / "eda";
        if (!ParseEdaDataFiles(edaPath)) return false;

        loginfo("Parsing step directory: " + m_name + " complete");

        return true;
    }

    bool StepDirectory::ParseLayerFiles(std::filesystem::path layersPath)
    {
        loginfo("Parsing layer directories...");

        if (!std::filesystem::exists(layersPath)) return false;
        else if (!std::filesystem::is_directory(layersPath)) return false;

        for (auto& d : std::filesystem::directory_iterator(layersPath))
        {
            if (std::filesystem::is_directory(d))
            {
                auto pLayer = std::make_shared<LayerDirectory>(d.path());               
                if (pLayer->Parse())
                {
                    loginfo("Parsing layer: " + pLayer->GetName() + " complete");

                    m_layersByName[pLayer->GetName()] = pLayer;
                }
                else
                {
                    return false;
                }
            }
        }

        loginfo("Parsing layer directories complete");

        return true;
    }

    bool StepDirectory::ParseEdaDataFiles(std::filesystem::path edaPath)
    {
        loginfo("Parsing eda/data file...");

        if (!std::filesystem::exists(edaPath)) return false;
        else if (!std::filesystem::is_directory(edaPath)) return false;

        // parse nets and packages definitions      
        auto success =  m_edaData.Parse(edaPath);

        loginfo("Parsing eda/data file complete");

        return success;
    }

    std::unique_ptr<Odb::Lib::Protobuf::StepDirectory> StepDirectory::to_protobuf() const
    {
        std::unique_ptr<Odb::Lib::Protobuf::StepDirectory> pStepDirectoryMessage(new Odb::Lib::Protobuf::StepDirectory);
        pStepDirectoryMessage->set_name(m_name);
        pStepDirectoryMessage->set_path(m_path.string());
        pStepDirectoryMessage->mutable_edadatafile()->CopyFrom(*m_edaData.to_protobuf());

        for (const auto& kvNetlistFile : m_netlistsByName)
        {
            (*pStepDirectoryMessage->mutable_netlistsbyname())[kvNetlistFile.first] = *kvNetlistFile.second->to_protobuf();
        }        

        for (const auto& kvLayerDirectoryRecord : m_layersByName)
        {
            (*pStepDirectoryMessage->mutable_layersbyname())[kvLayerDirectoryRecord.first] = *kvLayerDirectoryRecord.second->to_protobuf();
        }
        
        return pStepDirectoryMessage;
    }

    void StepDirectory::from_protobuf(const Odb::Lib::Protobuf::StepDirectory& message)
    {
        m_name = message.name();
		m_path = message.path();
		m_edaData.from_protobuf(message.edadatafile());

        for (const auto& kvNetlistFile : message.netlistsbyname())
        {
            auto pNetlistFile = std::make_shared<NetlistFile>(std::filesystem::path(kvNetlistFile.second.path()));
            pNetlistFile->from_protobuf(kvNetlistFile.second);
            m_netlistsByName[kvNetlistFile.first] = pNetlistFile;
        }

        for (const auto& kvLayerDirectoryRecord : message.layersbyname())
        {
			auto pLayerDirectory = std::make_shared<LayerDirectory>(std::filesystem::path(kvLayerDirectoryRecord.second.path()));
			pLayerDirectory->from_protobuf(kvLayerDirectoryRecord.second);
			m_layersByName[kvLayerDirectoryRecord.first] = pLayerDirectory;
		}
    }

    bool StepDirectory::ParseNetlistFiles(std::filesystem::path netlistsPath)
    {
        loginfo("Parsing netlist files...");

        std::size_t netListDirectoriesFound = 0;

        if (std::filesystem::exists(netlistsPath))
        {
            if (std::filesystem::is_directory(netlistsPath))
            {                
                // parse net name records
                for (auto& d : std::filesystem::directory_iterator(netlistsPath))
                {
                    if (std::filesystem::is_directory(d))
                    {
                        netListDirectoriesFound++;

                        auto pNetlist = std::make_shared<NetlistFile>(d.path());
                        if (pNetlist->Parse())
                        {
                            m_netlistsByName[pNetlist->GetName()] = pNetlist;
                        }
                        else
                        {
                            // pNetList will be freed when exiting the above scope
                            logerror("Failed to parse netlist directory: " + pNetlist->GetName());
                        }
                    }
                }
            }
        }
       
        if (netListDirectoriesFound == 0)   // netlist dirs found, but none parsed successfully
        {      
            logwarn("No netlist directories found");
        }
        else if (netListDirectoriesFound == m_netlistsByName.size())
        {
			loginfo("netlist directories parsed successfully");
		}

        return true;
    }

    const ComponentsFile* StepDirectory::GetTopComponentsFile() const
    {
        auto findIt = m_layersByName.find(ComponentsFile::TOP_COMPONENTS_LAYER_NAME);
        if (findIt != m_layersByName.end())
        {
            return &(findIt->second->GetComponentsFile());
		}
        else
        {
			return nullptr;
		}
    }

    const ComponentsFile* StepDirectory::GetBottomComponentsFile() const
    {
        auto findIt = m_layersByName.find(ComponentsFile::BOTTOM_COMPONENTS_LAYER_NAME);
        if (findIt != m_layersByName.end())
        {
            return &(findIt->second->GetComponentsFile());
        }
        else
        {
			return nullptr;
		}
    }
}