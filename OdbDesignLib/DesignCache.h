#pragma once

#include "FileArchive.h"
#include "Design.h"
#include "export.h"


namespace Odb::Lib
{
	class DECLSPEC DesignCache
	{
	public:
		DesignCache();
		DesignCache(std::string directory);
		~DesignCache();
		
		std::shared_ptr<ProductModel::Design> GetDesign(std::string designName);
		std::shared_ptr<FileModel::Design::FileArchive> GetFileArchive(std::string designName);
		
	private:
		std::string m_directory;

		FileModel::Design::FileArchive::StringMap m_fileArchivesByName;
		ProductModel::Design::StringMap m_designsByName;

		std::shared_ptr<ProductModel::Design> LoadDesign(std::string designName);
		std::shared_ptr<FileModel::Design::FileArchive> LoadFileArchive(std::string designName);

	};
}
