#pragma once

#include "IOdbServerApp.h"
#include "DesignCache.h"
#include "OdbDesignArgs.h"
#include "../odbdesign_export.h"

namespace Odb::Lib::App
{
	class ODBDESIGN_EXPORT OdbAppBase : public virtual IOdbApp
	{
	public:
		OdbAppBase(int argc, char* argv[]);
		virtual ~OdbAppBase();

		const OdbDesignArgs& args() const override;
		DesignCache& designs() override;

		virtual Utils::ExitCode Run() override;

		inline static const char* DEFAULT_DESIGNS_DIR = "designs";

	protected:
		DesignCache m_designCache;		
		const OdbDesignArgs m_commandLineArgs;

	};
}
