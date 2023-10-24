#pragma once

#include "FileArchive.h"
#include "Design.h"
#include "odbdesign_export.h"


namespace Odb::Lib
{
	class ODBDESIGN_EXPORT DesignCache
	{
	public:
		DesignCache();
		DesignCache(std::string directory);
		~DesignCache();
		
		std::shared_ptr<ProductModel::Design> GetDesign(const std::string& designName);
		std::shared_ptr<FileModel::Design::FileArchive> GetFileArchive(const std::string& designName);

		std::vector<std::string> getLoadedDesignNames(const std::string& filter = "") const;
		std::vector<std::string> getLoadedFileArchiveNames(const std::string& filter = "") const;
		std::vector<std::string> getUnloadedDesignNames(const std::string& filter = "") const;

		bool isQueryValid(const std::string& query) const;

		void Clear();
		
	private:
		std::string m_directory;

		FileModel::Design::FileArchive::StringMap m_fileArchivesByName;
		ProductModel::Design::StringMap m_designsByName;

		std::shared_ptr<ProductModel::Design> LoadDesign(const std::string& designName);
		std::shared_ptr<FileModel::Design::FileArchive> LoadFileArchive(const std::string& designName);

	};
}
