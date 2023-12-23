#include "OdbServerAppBase.h"
#include "Logger.h"
#include "CrowHttpServer.h"
#include <memory>

using namespace Utils;
using namespace std::filesystem;

namespace Odb::Lib::App
{
	OdbServerAppBase::OdbServerAppBase(int argc, char* argv[])
		: OdbAppBase(argc, argv)
		, m_pHttpServer(std::make_unique<CrowHttpServer>())
	{
	}

	OdbServerAppBase::~OdbServerAppBase()
	{
		m_vecControllers.clear();
	}

	ExitCode OdbServerAppBase::Run()
	{
		// print usage and exit w/ success
		if (args().help())
		{
			args().printUsage();
			return ExitCode::Success;
		}

		// call base class
		if (ExitCode::Success != OdbAppBase::Run()) return ExitCode::FailedInit;

		// set Crow's log level
		http_server().setLogLevel(IHttpServer::LogLevel::Info);
		//m_crowApp.loglevel(crow::LogLevel::Info);

		// enable HTTP compression
		http_server().useCompression(IHttpServer::CompressionType::GZip);
		//m_crowApp.use_compression(crow::compression::algorithm::GZIP);

		try
		{
			if (args().useHttps())
			{
				path sslDirPath(args().sslDir());
				if (!exists(sslDirPath) || !is_directory(sslDirPath))
				{
					logerror("SSL was specified but the directory does not exist, exiting...");
					return ExitCode::FailedInitSslDirDoesNotExist;
				}				

				// enable SSL/HTTPS
				http_server().setSslKeyFiles((sslDirPath / SSL_CERT_FILE).string(),
											 (sslDirPath / SSL_KEY_FILE).string());
				//m_crowApp.ssl_file((sslDirPath / SSL_CERT_FILE).string(),
				//				   (sslDirPath / SSL_KEY_FILE).string());
			}
		}
		catch (boost::wrapexcept<boost::system::system_error>& e)
		{
			// log the error
			logexception(e);
			logerror("SSL was specified but it failed to initialize, exiting...");			
			return ExitCode::FailedInitSsl;
		}

		// let subclasses add controller types
		add_controllers();

		// register all added controllers' routes
		register_routes();

		// set port to passed-in port or default if none supplied
		http_server().setPort(args().port());
		//m_crowApp.port(static_cast<unsigned short>(args().port()));

		// set server to use multiple threads
		http_server().setMultithreaded(true);
		//m_crowApp.multithreaded();

		// run the Crow server
		http_server().run();
		//m_crowApp.run();

		// success!
		return ExitCode::Success;
	}

	//crow::SimpleApp& OdbServerAppBase::crow_app()
	//{
	//	//return m_crowApp;
	//}

	IHttpServer& OdbServerAppBase::http_server()
	{
		return *m_pHttpServer;
	}

	//void OdbServerAppBase::add_controllers(const RouteController::Vector& controllers)
	//{
	//	m_vecControllers.insert(m_vecControllers.end(), controllers.begin(), controllers.end());
	//}

	void OdbServerAppBase::register_routes()
	{
		for (const auto& pController : m_vecControllers)
		{
			pController->register_routes();
		}
	}
}