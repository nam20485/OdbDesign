#include "OdbServerAppBase.h"
//#include "Logger.h"
#include "RequestAuthenticationBase.h"
#include "crow_win.h"
//#include <boost/throw_exception.hpp>
//#include <boost/system/system_error.hpp>
#include "OdbAppBase.h"
#include <ExitCode.h>
#include <memory>
//#include <filesystem>

using namespace Utils;
using namespace std::filesystem;

namespace Odb::Lib::App
{
	OdbServerAppBase::OdbServerAppBase(int argc, char* argv[])
		: OdbAppBase(argc, argv)		
	{
	}

	bool OdbServerAppBase::preServerRun()
	{
		// override in extended class to configure server or run custom code
		return true;
	}

	bool OdbServerAppBase::postServerRun()
	{
		// override in extended class to cleanup server or run custom code
		return true;
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
		m_crowApp.loglevel(crow::LogLevel::Info);

		// enable HTTP compression
		m_crowApp.use_compression(crow::compression::algorithm::GZIP);

		//try
		//{
		//	if (args().useHttps())
		//	{
		//		path sslDirPath(args().sslDir());
		//		if (!exists(sslDirPath) || !is_directory(sslDirPath))
		//		{
		//			logerror("SSL was specified but the directory does not exist, exiting...");
		//			return ExitCode::FailedInitSslDirDoesNotExist;
		//		}				

		//		// enable SSL/HTTPS
		//		m_crowApp.ssl_file((sslDirPath / SSL_CERT_FILE).string(),
		//						   (sslDirPath / SSL_KEY_FILE).string());
		//	}
		//}
		//catch (boost::wrapexcept<boost::system::system_error>& e)
		//{
		//	// log the error
		//	logexception(e);
		//	logerror("SSL was specified but it failed to initialize, exiting...");			
		//	return ExitCode::FailedInitSsl;
		//}

		// let subclasses add controller types
		add_controllers();

		// register all added controllers' routes
		register_routes();

		// set port to passed-in port or default if none supplied
		m_crowApp.port(static_cast<unsigned short>(args().port()));

		// set server to use multiple threads
		m_crowApp.multithreaded();

		if (!preServerRun()) return ExitCode::PreServerRunFailed;

		// run the Crow server
		m_crowApp.run();

		if (!postServerRun()) return ExitCode::PostServerRunFailed;

		// success!
		return ExitCode::Success;
	}

	CrowApp& OdbServerAppBase::crow_app()
	{
		return m_crowApp;
	}

	RequestAuthenticationBase& OdbServerAppBase::request_auth()
	{
		return *m_pRequestAuthentication;
	}

	void OdbServerAppBase::request_auth(std::unique_ptr<RequestAuthenticationBase> pRequestAuthentication)
	{
		m_pRequestAuthentication = std::move(pRequestAuthentication);
	}

	void OdbServerAppBase::register_routes()
	{
		for (const auto& pController : m_vecControllers)
		{
			pController->register_routes();
		}
	}
}