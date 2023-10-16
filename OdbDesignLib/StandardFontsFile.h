#pragma once

#include "OdbFile.h"
#include "odbdesign_export.h"

namespace Odb::Lib::FileModel::Design
{
	class StandardFontsFile : public OdbFile
	{
	public:
		StandardFontsFile() = default;
		~StandardFontsFile() = default;
	
		bool Parse(std::filesystem::path path) override;

	private:


	};
}
