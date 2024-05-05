#pragma once

//#include <filesystem>
#include <ostream>
#include "../odbdesign_export.h"

namespace Odb::Lib::FileModel
{
	class IStreamSaveable
	{
	public:
		virtual bool Save(std::ostream& os) = 0;

	protected:
		IStreamSaveable() = default;
	};
}