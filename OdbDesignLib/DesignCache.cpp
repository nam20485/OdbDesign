#include "DesignCache.h"
#include <filesystem>


namespace Odb::Lib
{
    DesignCache::DesignCache()
        : DesignCache(".")
    {
    }

    DesignCache::DesignCache(std::string directory) :
        m_directory(directory)
    {
    }

    DesignCache::~DesignCache()
    {
        m_fileArchivesByName.clear();
        m_designsByName.clear();
    }

    std::shared_ptr<ProductModel::Design> DesignCache::GetDesign(std::string designName)
    {
        auto findIt = m_designsByName.find(designName);
        if (findIt == m_designsByName.end())
        {
            auto pDesign = LoadDesign(designName);             
            return pDesign;
		}

        return m_designsByName[designName];
    }

    std::shared_ptr<FileModel::Design::FileArchive> DesignCache::GetFileArchive(std::string designName)
    {
        auto findIt = m_fileArchivesByName.find(designName);
        if (findIt == m_fileArchivesByName.end())
        {
            auto pFileArchive = LoadFileArchive(designName);
            return pFileArchive;
        }

        return m_fileArchivesByName[designName];        
    }

    std::shared_ptr<ProductModel::Design> DesignCache::LoadDesign(std::string designName)
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

    std::shared_ptr<FileModel::Design::FileArchive> DesignCache::LoadFileArchive(std::string designName)
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

        return nullptr;
    }
}
