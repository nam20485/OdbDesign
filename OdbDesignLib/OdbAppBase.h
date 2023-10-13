#pragma once

#include "IOdbServerApp.h"
#include "DesignCache.h"
#include "Logger.h"
#include "CommandLineArgs.h"
#include "odbdesign_export.h"

using namespace Utils;

namespace Odb::Lib
{
	class ODBDESIGN_EXPORT OdbAppBase : public virtual IOdbApp
	{
	public:
		OdbAppBase(int argc, char* argv[]);
		virtual ~OdbAppBase();

		static Logger m_logger;
		
		const CommandLineArgs& arguments() const override;
		DesignCache& design_cache() override;

		virtual Utils::ExitCode Run() override;

	protected:
		DesignCache m_designCache;		
		CommandLineArgs m_commandLineArgs;		

	};
}
