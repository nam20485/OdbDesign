#pragma once

#include "IOdbServerApp.h"
#include "OdbAppBase.h"
#include "RouteController.h"
#include "crow_win.h"
#include "odbdesign_export.h"

namespace Odb::Lib
{
	class ODBDESIGN_EXPORT OdbServerAppBase : public OdbAppBase, public IOdbServerApp
	{
	public:
		OdbServerAppBase(int argc, char* argv[]);
		virtual ~OdbServerAppBase();

		crow::SimpleApp& crow_app() override;

		ExitCode Run() override;		

	protected:
		crow::SimpleApp m_crowApp;
		RouteController::Vector m_vecControllers;

		// implement in subclasses to add route controllers
		virtual void add_controllers() = 0;

	private:
		void register_routes();
	};
}
