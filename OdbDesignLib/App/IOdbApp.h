#pragma once

#include "OdbDesignArgs.h"
#include "DesignCache.h"
#include "ExitCode.h"
#include "../odbdesign_export.h"

namespace Odb::Lib::App
{
	class ODBDESIGN_EXPORT IOdbApp
	{
	public:
		virtual ~IOdbApp() {}

		virtual const OdbDesignArgs& args() const = 0;
		virtual DesignCache& designs() = 0;

		virtual Utils::ExitCode Run() = 0;

	protected:
		// abstract class/interface
		IOdbApp() = default;
		
	};
}
