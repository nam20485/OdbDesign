#include "OdbServerAppBase.h"


namespace Odb::Lib
{
	Odb::Lib::OdbServerAppBase::OdbServerAppBase(int argc, char* argv[])
		: OdbAppBase(argc, argv)
	{
	}

	Odb::Lib::OdbServerAppBase::~OdbServerAppBase()
	{
		m_vecControllers.clear();
	}

	ExitCode Odb::Lib::OdbServerAppBase::Run()
	{
		auto result = OdbAppBase::Run();
		if (result != ExitCode::Success) return result;

		m_crowApp.use_compression(crow::compression::algorithm::GZIP);

		// let subclasses add controller types
		add_controllers();

		// register all added controllers' routes
		register_routes();

		// run the server
		m_crowApp.port(18080).multithreaded().run();

		return ExitCode::Success;
	}

	crow::SimpleApp& Odb::Lib::OdbServerAppBase::crow_app()
	{
		return m_crowApp;
	}

	void OdbServerAppBase::register_routes()
	{
		for (const auto& pController : m_vecControllers)
		{
			pController->register_routes();
		}
	}
}