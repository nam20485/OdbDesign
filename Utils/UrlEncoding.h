#pragma once

#include <string>	
#include "utils_export.h"

namespace Utils
{
	// inspired by: https://github.com/davisking/dlib/blob/f7d99ae0dc97c00c1690863881709b7a8b89bb40/dlib/server/server_http.cpp		

	class UTILS_EXPORT UrlEncoding
	{	
	public:
		static std::string encode(const std::string& unencoded);
		static std::string decode(const std::string& encoded);

		// static-only class
		UrlEncoding() = delete;	
		
	};
}
