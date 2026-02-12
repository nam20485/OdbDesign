#include "DesignCache.h"
#include "DesignCache.h"
#include "ArchiveExtractor.h"
#include "DesignCache.h"
#include "Logger.h"
#include <exception>
#include <filesystem>
#include <vector>
#include "../FileModel/Design/FileArchive.h"
#include <memory>
#include "../ProductModel/Design.h"
#include <iosfwd>
#include <string>
#include <type_traits>
#include <StringVector.h>
#include <shared_mutex>

using namespace Utils;
using namespace std::filesystem;

namespace Odb::Lib::App
{    
    DesignCache::DesignCache(std::string directory) :
        m_directory(std::move(directory))
    {
		ensureDirectoryExists();
    }

    DesignCache::~DesignCache()
    {
        Clear();
    }

    std::shared_ptr<ProductModel::Design> DesignCache::GetDesign(const std::string& designName)
    {
        // Fast path: shared (read) lock for cache hit
        {
            std::shared_lock readLock(m_cacheMutex);
            auto findIt = m_designsByName.find(designName);
            if (findIt != m_designsByName.end())
            {
                return findIt->second;
            }
        }

        // Slow path: load from disk without holding lock (LoadDesign may call GetFileArchive)
        auto pDesign = LoadDesign(designName);

        // Insert into cache under exclusive lock (double-check another thread didn't load it)
        if (pDesign != nullptr)
        {
            std::unique_lock writeLock(m_cacheMutex);
            auto findIt = m_designsByName.find(designName);
            if (findIt != m_designsByName.end())
            {
                return findIt->second;  // Another thread loaded it first
            }
            m_designsByName[designName] = pDesign;
        }
        return pDesign;
    }

    std::shared_ptr<FileModel::Design::FileArchive> DesignCache::GetFileArchive(const std::string& designName)
    {
        std::stringstream ss;
        ss << "Retrieving design \"" << designName << "\" from cache... ";
        logdebug(ss.str());

        // Fast path: shared (read) lock for cache hit
        {
            std::shared_lock readLock(m_cacheMutex);
            auto findIt = m_fileArchivesByName.find(designName);
            if (findIt != m_fileArchivesByName.end())
            {
                logdebug("Found. Returning from cache.");
                return findIt->second;
            }
        }

        loginfo("Not found in cache, attempting to load from file...");

        // Slow path: load from disk without holding lock
        auto pFileArchive = LoadFileArchive(designName);

        if (pFileArchive == nullptr)
        {
            logwarn("Failed loading from file");
        }
        else
        {
            loginfo("Loaded from file");

            // Insert into cache under exclusive lock (double-check)
            std::unique_lock writeLock(m_cacheMutex);
            auto findIt = m_fileArchivesByName.find(designName);
            if (findIt != m_fileArchivesByName.end())
            {
                logdebug("Found. Returning from cache (loaded by another thread).");
                return findIt->second;  // Another thread loaded it first
            }
            m_fileArchivesByName[designName] = pFileArchive;
        }
        return pFileArchive;
    }

    void DesignCache::AddFileArchive(const std::string& designName, std::shared_ptr<FileModel::Design::FileArchive> fileArchive, bool save)
    {
        std::unique_lock writeLock(m_cacheMutex);
        m_fileArchivesByName[designName] = fileArchive;
        if (save)
        {
            // SaveFileArchive calls GetFileArchive internally, which takes the lock.
            // We must release before calling to avoid deadlock.
            writeLock.unlock();
            SaveFileArchive(designName);
        }
    }

    bool DesignCache::SaveFileArchive(const std::string& designName)
    {
        auto fileArchive = GetFileArchive(designName);
        if (fileArchive != nullptr)
        {
            std::string directory;
            {
                std::shared_lock readLock(m_cacheMutex);
                directory = m_directory;
            }
            return fileArchive->SaveFileModel(directory);
        }
        return false;
    }

    std::vector<std::string> DesignCache::getLoadedDesignNames(const std::string& filter) const
    {
        std::shared_lock readLock(m_cacheMutex);
        std::vector<std::string> loadedDesigns;
        for (const auto& kv : m_designsByName)
        {
            loadedDesigns.push_back(kv.first);
        }
        return loadedDesigns;
    }

    std::vector<std::string> DesignCache::getLoadedFileArchiveNames(const std::string& filter) const
    {
        std::shared_lock readLock(m_cacheMutex);
        std::vector<std::string> loadedFileArchives;
        for (const auto& kv : m_fileArchivesByName)
		{
			loadedFileArchives.push_back(kv.first);
		}
        return loadedFileArchives;
    }

    std::vector<std::string> DesignCache::getUnloadedDesignNames(const std::string& filter) const
    {        
        std::string directory;
        {
            std::shared_lock readLock(m_cacheMutex);
            directory = m_directory;
        }

        std::vector<std::string> unloadedNames;

        //try
        {
            path dir(directory);
            for (const auto& entry : directory_iterator(dir))
            {
                if (entry.is_regular_file())
                {
                    unloadedNames.push_back(entry.path().stem().string());
                }
            }
        }
        //catch (std::filesystem::filesystem_error& fe)
        //{
        //    logexception(fe);
        //    // re-throw it so we get a HTTP 500 response to the client
        //    throw fe;
        //}

        return unloadedNames;
    }

    int DesignCache::loadAllFileArchives(bool stopOnError)
    {
        int loaded = 0;

        std::string directory;
        {
            std::shared_lock readLock(m_cacheMutex);
            directory = m_directory;
        }

        for (const auto& entry : directory_iterator(directory))
        {
            if (entry.is_regular_file())
            {
                if (ArchiveExtractor::IsArchiveTypeSupported(entry.path().filename()))
                {
                    try
                    {
                        auto pFileArchive = GetFileArchive(entry.path().stem().string());
                        if (pFileArchive != nullptr)
                        {
                            loaded++;
                        }
                    }
                    catch (std::exception& e)
                    {
                        // continue if we encounter an error loading one
                        logexception(e);
                        if (stopOnError) throw e;
                    }
                }
            }
       }

        return loaded;
    }

    int DesignCache::loadAllDesigns(bool stopOnError)
    {
        int loaded = 0;

        std::string directory;
        {
            std::shared_lock readLock(m_cacheMutex);
            directory = m_directory;
        }

        for (const auto& entry : directory_iterator(directory))
        {
            if (entry.is_regular_file())
            {
                if (ArchiveExtractor::IsArchiveTypeSupported(entry.path().filename()))
                {
                    try
                    {
                        auto pDesign = GetDesign(entry.path().stem().string());
                        if (pDesign != nullptr)
                        {
                            loaded++;
                        }
                    }
                    catch (std::exception& e)
                    {
                        logexception(e);
                        if (stopOnError)
                        {
                            throw;
                        }
                    }
                }
            }
        }

        return loaded;
    }

    int DesignCache::loadFileArchives(const StringVector& names)
    {
        int loaded = 0;

        for (const auto& name : names)
        {
            try
            {
                auto pFileArchive = GetFileArchive(name);
                if (pFileArchive != nullptr)
                {
                    loaded++;
                }
            }
            catch (std::exception& e)
            {
                // continue on error
                logexception(e);
            }
        }

        return loaded;
    }

    int DesignCache::loadDesigns(const StringVector& names)
    {
        int loaded = 0;

        for (const auto& name : names)
        {
            try
            {
                auto pDesign = GetDesign(name);
                if (pDesign != nullptr)
                {
                    loaded++;
                }
            }
            catch (std::exception& e)
            {
                // continue on error
                logexception(e);
            }
        }

        return loaded;
    }

    void DesignCache::setDirectory(const std::string& directory)
    {
        std::unique_lock writeLock(m_cacheMutex);
        m_directory = directory;
    }

    const std::string& DesignCache::getDirectory() const
    {
        std::shared_lock readLock(m_cacheMutex);
        return m_directory;
    }

    void DesignCache::Clear()
    {
        std::unique_lock writeLock(m_cacheMutex);
        m_fileArchivesByName.clear();
        m_designsByName.clear();
    }

    std::shared_ptr<ProductModel::Design> DesignCache::LoadDesign(const std::string& designName)
    {
        // NOTE: This is a lock-free I/O helper. It does NOT modify the cache maps.
        // Cache insertion is handled by GetDesign().
        std::string directory;
        {
            std::shared_lock readLock(m_cacheMutex);
            directory = m_directory;
        }

        for (const auto& entry : directory_iterator(directory))
        {
            if (entry.is_regular_file())
            {
                if (entry.path().stem() == designName)
                {
                    // GetFileArchive is thread-safe, manages its own locking
                    auto pFileModel = GetFileArchive(designName);
                    if (pFileModel != nullptr)
                    {
                        auto pDesign = std::make_shared<ProductModel::Design>();
                        if (pDesign->Build(pFileModel))
                        {
                            return pDesign;
                        }
                        else
                        {
                            break;
                        }
                    }
                    else
                    {
                        break;
                    }
                }
            }
        }    

        return nullptr;
    }

    std::shared_ptr<FileModel::Design::FileArchive> DesignCache::LoadFileArchive(const std::string& designName)
    {
        // NOTE: This is a lock-free I/O helper. It does NOT modify the cache maps.
        // Cache insertion is handled by GetFileArchive().
        auto fileFound = false;

        std::string directory;
        {
            std::shared_lock readLock(m_cacheMutex);
            directory = m_directory;
        }

        // skip inaccessible files and do not follow symlinks
        const auto options = directory_options::skip_permission_denied;
        for (const auto& entry : directory_iterator(directory, options))
        {
            if (entry.is_regular_file())
            {
                if (entry.path().stem() == designName)
                {                    
                    fileFound = true;

                    loginfo("file found: [" + entry.path().string() + "], attempting to parse...");

                    auto pFileArchive = std::make_shared<FileModel::Design::FileArchive>(entry.path().string());
                    if (pFileArchive->ParseFileModel())
                    {
                        return pFileArchive;
                    }
                    else
                    {
                        break;
                    }
                }
            }
        }       

        if (!fileFound)
        {
            logwarn("Failed to find file for design \"" + designName + "\"");
            
            // DEBUG: List all files in directory
            logwarn("DEBUG: Listing all files in directory: " + directory);
            for (const auto& entry : directory_iterator(directory, options))
            {
                if (entry.is_regular_file())
                {
                    logwarn("DEBUG: Found file: " + entry.path().filename().string() + " (stem: " + entry.path().stem().string() + ")");
                }
            }
        }

        return nullptr;
    }

    void DesignCache::ensureDirectoryExists() const
    {
        if (!std::filesystem::exists(m_directory))
        {
            // create directory
            try
            {
                std::filesystem::create_directories(m_directory);
            }
            catch (const std::exception& e)
            {
                std::string msg = "Failed to create design cache directory: " + m_directory;				
                logexception_msg(e, msg);                
                throw e;
			}
        }
    }
}
