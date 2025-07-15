#pragma once

#include "utils_export.h"
#include <string>

namespace Utils
{
	enum class CompressionType
	{
		TarGzip
	};

	bool extract(const char* filename, const char* destDir);
	UTILS_EXPORT bool compress_dir(const char* srcDir, 
							   const char* destDir, 
							   const char* archiveName, 
							   std::string& fileOut,
							   CompressionType type = CompressionType::TarGzip);
}
