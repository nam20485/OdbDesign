#include "OdbServerAppBase.h"
#include <Logger.h>

using namespace Utils;

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

		try
		{
			if (args().useHttps())
			{
				// enable SSL/HTTPS
				m_crowApp.ssl_file("ssl/localhost.crt", "ssl/localhost.key");
			}
		}
		catch (boost::wrapexcept<boost::system::system_error>& e)
		{
			// log the error
			logexception(e.what());	
			logerror("SSL was specified but it failed to initialize, exiting...");			
			return ExitCode::FailedInitSsl;
		}

		// let subclasses add controller types
		add_controllers();

		// register all added controllers' routes
		register_routes();

		// set port to passed-in port or default if none supplied
		m_crowApp.port(static_cast<unsigned short>(args().port()));

		// set server to use multiple threads
		m_crowApp.multithreaded();

		// run the Crow server
		m_crowApp.run();

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