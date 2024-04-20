#pragma once

namespace Utils
{
	bool extract(const char* filename, const char* destDir);
	bool compress(const char* srcDir, const char* destDir, const char* archiveName);
}
