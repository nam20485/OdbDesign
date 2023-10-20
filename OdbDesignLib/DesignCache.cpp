#include "DesignCache.h"
#include <filesystem>
#include <utility>
#include "Logger.h"
#include <exception>

using namespace Utils;

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

    void DesignCache::Clear()
    {
        m_fileArchivesByName.clear();
        m_designsByName.clear();
    }

    std::shared_ptr<ProductModel::Design> DesignCache::LoadDesign(const std::string& designName)
    {
        std::filesystem::path dir(m_directory);

        for (const auto& entry : std::filesystem::directory_iterator(dir))
        {
            if (entry.is_regular_file())
            {
                if (entry.path().stem() == designName)
                {
                    auto pDesign = std::make_shared<ProductModel::Design>();
                    if (pDesign->Build(entry.path().string()))
                    {
                        m_designsByName.emplace(designName, pDesign);
                        return pDesign;
                    }
                }
            }
        }

        return nullptr;
    }

    std::shared_ptr<FileModel::Design::FileArchive> DesignCache::LoadFileArchive(const std::string& designName)
    {
        try
        {
            std::filesystem::path dir(m_directory);

            for (const auto& entry : std::filesystem::directory_iterator(dir))
            {
                if (entry.is_regular_file())
                {
                    if (entry.path().stem() == designName)
                    {
                        auto pFileArchive = std::make_shared<FileModel::Design::FileArchive>(entry.path().string());
                        if (pFileArchive->ParseFileModel())
                        {
                            m_fileArchivesByName.emplace(designName, pFileArchive);
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
