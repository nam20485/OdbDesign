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
		// call base class
		if (ExitCode::Success != OdbAppBase::Run()) return ExitCode::FailedInit;

		// set Crow's log level
		m_crowApp.loglevel(crow::LogLevel::Info);

		// enable HTTP compression
		m_crowApp.use_compression(crow::compression::algorithm::GZIP);

		// enable SSL/HTTPS
		m_crowApp.ssl_file("ssl/localhost.crt", "ssl/localhost.key");

		// let subclasses add controller types
		add_controllers();

		// register all added controllers' routes
		register_routes();

		// run the Crow server
		m_crowApp.port(18080).multithreaded().run();

		// success!
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