#include "DesignCache.h"
#include <filesystem>
#include <utility>
#include "Logger.h"
#include <exception>

using namespace Utils;
using namespace std::filesystem;

namespace Odb::Lib
{
    DesignCache::DesignCache()
        : DesignCache(".")
    {
    }

    DesignCache::DesignCache(std::string directory) :
        m_directory(std::move(directory))
    {
    }

    DesignCache::~DesignCache()
    {
        Clear();
    }

    std::shared_ptr<ProductModel::Design> DesignCache::GetDesign(const std::string& designName)
    {
        auto findIt = m_designsByName.find(designName);
        if (findIt == m_designsByName.end())
        {
            auto pDesign = LoadDesign(designName);             
            return pDesign;
		}

        return m_designsByName[designName];
    }

    std::shared_ptr<FileModel::Design::FileArchive> DesignCache::GetFileArchive(const std::string& designName)
    {
        std::stringstream ss;
        ss << "Retrieving \"" << designName << "\" from cache... ";
        //loginfo(ss.str());
        
        loginfo(ss.str());

        auto findIt = m_fileArchivesByName.find(designName);
        if (findIt == m_fileArchivesByName.end())
        {
            loginfo("Not found. Loading from file... ");

            auto pFileArchive = LoadFileArchive(designName);
            return pFileArchive;
        }

        loginfo("Found. Returning from cache.");

        return m_fileArchivesByName[designName];        
    }

    std::vector<std::string> DesignCache::getLoadedDesignNames(const std::string& filter) const
    {
        std::vector<std::string> loadedDesigns;
        for (const auto& kv : m_designsByName)
        {
            loadedDesigns.push_back(kv.second->GetFileModel()->GetProductName());
        }
        return loadedDesigns;
    }

    std::vector<std::string> DesignCache::getLoadedFileArchiveNames(const std::string& filter) const
    {
        std::vector<std::string> loadedFileArchives;
        for (const auto& kv : m_fileArchivesByName)
		{
			loadedFileArchives.push_back(kv.second->GetProductName());
		}
        return loadedFileArchives;
    }

    std::vector<std::string> DesignCache::getUnloadedDesignNames(const std::string& filter) const
    {
        std::vector<std::string> unloadedNames;

        path dir(m_directory);
        for (const auto& entry : directory_iterator(dir))
        {
            if (entry.is_regular_file())
            {
                unloadedNames.push_back(entry.path().stem().string());
            }
        }

        return unloadedNames;
    }

    bool DesignCache::isQueryValid(const std::string& query) const
    {
        return false;
    }

    void DesignCache::Clear()
    {
        m_fileArchivesByName.clear();
        m_designsByName.clear();
    }

    std::shared_ptr<ProductModel::Design> DesignCache::LoadDesign(const std::string& designName)
    {        
        try
        {
            // no FileArchive with the same name is loaded, so load the Design from file
            std::filesystem::path dir(m_directory);

            for (const auto& entry : std::filesystem::directory_iterator(dir))
            {
                if (entry.is_regular_file())
                {
                    if (entry.path().stem() == designName)
                    {
                        auto pFileModel = GetFileArchive(designName);
                        if (pFileModel != nullptr)
                        {
                            auto pDesign = std::make_shared<ProductModel::Design>();
                            if (pDesign->Build(pFileModel))
                            {
                                // overwrite any existing design with the same name
                                m_designsByName[pFileModel->GetProductName()] =  pDesign;
                                return pDesign;
                            }
                        }
                    }
                }
            }
        }
        catch (std::filesystem::filesystem_error& fe)
        {
            logexception(fe);
            // re-throw it so we get a HTTP 500 response to the client
            throw fe;
        }

        return nullptr;
    }

    std::shared_ptr<FileModel::Design::FileArchive> DesignCache::LoadFileArchive(const std::string& designName)
    {
        try
        {
            std::filesystem::path dir(m_directory);
            // skip inaccessible files and do not follow symlinks
            const auto options = std::filesystem::directory_options::skip_permission_denied;

            for (const auto& entry : std::filesystem::directory_iterator(dir, options))
            {
                if (entry.is_regular_file())
                {
                    if (entry.path().stem() == designName)
                    {
                        auto pFileArchive = std::make_shared<FileModel::Design::FileArchive>(entry.path().string());
                        if (pFileArchive->ParseFileModel())
                        {
                            // overwrite any existing file archive with the same name
                            m_fileArchivesByName[pFileArchive->GetProductName()] = pFileArchive;
                            return pFileArchive;
                        }
                    }
                }
            }
        }
        catch (std::filesystem::filesystem_error& fe)
        {
            logexception(fe);
            // re-throw it so we get a HTTP 500 response to the client
            throw fe;
        }

        return nullptr;
    }
}
