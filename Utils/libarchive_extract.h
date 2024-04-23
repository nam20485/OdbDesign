#pragma once

#include "utils_export.h"

namespace Utils
{
	enum class CompressionType
	{
		TarGzip
	};

	bool extract(const char* filename, const char* destDir);
	UTILS_EXPORT bool compress(const char* srcDir, 
							   const char* destDir, 
							   const char* archiveName, 
							   std::string& fileOut,
							   CompressionType type = CompressionType::TarGzip);
}
