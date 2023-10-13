#pragma once

#include "CommandLineArgs.h"
#include "DesignCache.h"
#include "ExitCode.h"
#include "odbdesign_export.h"

using namespace Utils;

namespace Odb::Lib
{
	class ODBDESIGN_EXPORT IOdbApp
	{
	public:
		virtual ~IOdbApp() {}

		virtual const CommandLineArgs& arguments() const = 0;		
		virtual DesignCache& design_cache() = 0;

		virtual ExitCode Run() = 0;

	protected:
		// abstract class/interface
		IOdbApp() = default;
		
	};
}
