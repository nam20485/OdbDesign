#pragma once

#include "../FileModel/Design/FileArchive.h"
#include "../ProductModel/Design.h"
#include "../odbdesign_export.h"
#include "StringVector.h"
#include <memory>
#include <vector>
#include <string>


namespace Odb::Lib::App
{
	class ODBDESIGN_EXPORT DesignCache
	{
	public:		
		DesignCache(std::string directory);
		~DesignCache();
		
		std::shared_ptr<ProductModel::Design> GetDesign(const std::string& designName);
		std::shared_ptr<FileModel::Design::FileArchive> GetFileArchive(const std::string& designName);

		void AddFileArchive(const std::string& designName, std::shared_ptr<FileModel::Design::FileArchive> fileArchive, bool save);

		bool SaveFileArchive(const std::string& designName);

		std::vector<std::string> getLoadedDesignNames(const std::string& filter = "") const;
		std::vector<std::string> getLoadedFileArchiveNames(const std::string& filter = "") const;
		std::vector<std::string> getUnloadedDesignNames(const std::string& filter = "") const;

		int loadAllFileArchives(bool stopOnError);
		int loadAllDesigns(bool stopOnError);
		int loadFileArchives(const Utils::StringVector& names);
		int loadDesigns(const Utils::StringVector& names);

		void setDirectory(const std::string& directory);
		const std::string& getDirectory() const;		

		void Clear();		
		
	private:
		std::string m_directory;

		FileModel::Design::FileArchive::StringMap m_fileArchivesByName;
		ProductModel::Design::StringMap m_designsByName;

		std::shared_ptr<ProductModel::Design> LoadDesign(const std::string& designName);
		std::shared_ptr<FileModel::Design::FileArchive> LoadFileArchive(const std::string& designName);

		constexpr inline static const char* DESIGN_EXTENSIONS[] = { "zip", "tgz", "tar.gz", "tar", "gzip" , "gz" };

	};
}
