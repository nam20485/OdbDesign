#include "OdbServerAppBase.h"
// #include "Logger.h"
#include <App/RequestAuthenticationBase.h>
#include <crow_win.h>
// #include <boost/throw_exception.hpp>
// #include <boost/system/system_error.hpp>
#include <App/OdbAppBase.h>
#include <ExitCode.h>
#include <memory>
#include <csignal>
#include <iostream>
#include <string>
#include <thread>
#include <utility>
#include <App/DesignCache.h>
#include <crow/compression.h>
#include <crow/logging.h>

#if defined(_WIN32)
#include <io.h>
#define ISATTY _isatty
#define FILENO _fileno
#else
#include <unistd.h>
#define ISATTY isatty
#define FILENO fileno
#include <sys/select.h>
#include <sys/time.h>
#endif
#include <atomic>
#include <cctype>
#include <cstdio>
#include <crow/app.h>
#include <crow/http_response.h>
#include <grpcpp/server.h>

using namespace Utils;
using namespace std::filesystem;

namespace {

// Interactive console quit helper (TTY-only)
	std::atomic<bool> s_consoleLoopStarted{ false };
	std::atomic<bool> s_consoleLoopStop{ false };
	std::thread s_consoleThread;

	void start_interactive_quit_if_tty()
	{
		if (s_consoleLoopStarted.exchange(true)) return;
		if (!ISATTY(FILENO(stdin)))
		{
			return; // non-interactive (containers/k8s) -> no console loop
		}

		s_consoleLoopStop.store(false);
		s_consoleThread = std::thread([]
			{
				std::fprintf(stderr, "[interactive] Type 'q' then Enter to quit gracefully...\n");
				std::string line;
				while (!s_consoleLoopStop.load() && std::getline(std::cin, line))
				{
					for (auto& ch : line) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
					if (line == "q" || line == "quit" || line == "exit" || line == "shutdown")
					{
						// Unify with the SIGINT path your class already exposes
						Odb::Lib::App::OdbServerAppBase::HandleSignal(SIGINT);
						break;
					}
				}
			});
	}

	void stop_interactive_quit() { s_consoleLoopStop.store(true); }
	void join_interactive_quit() { if (s_consoleThread.joinable()) s_consoleThread.join(); }

} // anonymous namespace

namespace Odb::Lib::App
{
	std::atomic<OdbServerAppBase *> OdbServerAppBase::s_activeInstance{nullptr};

	OdbServerAppBase::OdbServerAppBase(int argc, char *argv[])
		: OdbAppBase(argc, argv), m_shutdownFlag(false)
	{
		// Initialize the design cache shared pointer to point to the base class's design cache
		m_pDesignCache = std::shared_ptr<DesignCache>(&m_designCache, [](DesignCache *) {});
		s_activeInstance.store(this, std::memory_order_release);
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
		stop_servers();
		m_vecControllers.clear();
		s_activeInstance.store(nullptr, std::memory_order_release);
	}

	void OdbServerAppBase::HandleSignal(int signum)
	{
		auto *instance = s_activeInstance.load(std::memory_order_acquire);
		if (instance != nullptr)
		{
			instance->signal_handler(signum);
		}
	}

	void OdbServerAppBase::RunGrpcServer(const std::string &server_address, std::shared_ptr<DesignCache> cache)
	{
		// Default implementation does nothing
		// Override in subclass to implement gRPC service
		std::cout << "gRPC server not implemented in base class" << std::endl;
	}

	void OdbServerAppBase::start_grpc_server()
	{
		if (args().grpcPort() > 0 && m_pDesignCache)
		{
			std::string grpc_server_address = "0.0.0.0:" + std::to_string(args().grpcPort());
			m_grpcThread = std::make_unique<std::thread>(&OdbServerAppBase::RunGrpcServer, this, grpc_server_address, m_pDesignCache);
			std::cout << "gRPC server thread started on port " << args().grpcPort() << std::endl;
		}
	}

	void OdbServerAppBase::set_grpc_server(std::shared_ptr<grpc::Server> server) {
		m_grpcServer = std::move(server);
	}

	// Accessor if needed by controllers/services
	inline std::shared_ptr<grpc::Server> OdbServerAppBase::grpc_server() const { return m_grpcServer; }

	void OdbServerAppBase::stop_servers()
	{
		if (m_shutdownFlag.exchange(true))
		{
			return; // Already shutting down
		}

		std::cout << "Shutting down servers..." << std::endl;

		// Stop Crow
		m_crowApp.stop();

		// Stop gRPC
		if (m_grpcServer)
		{
			m_grpcServer->Shutdown();
		}

		// Wait for gRPC thread to finish
		if (m_grpcThread && m_grpcThread->joinable())
		{
			m_grpcThread->join();
		}

		std::cout << "All servers shut down." << std::endl;
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
		if (ExitCode::Success != OdbAppBase::Run())
			return ExitCode::FailedInit;

		// set Crow's log level
		m_crowApp.loglevel(crow::LogLevel::Info);

		// enable HTTP compression
		m_crowApp.use_compression(crow::compression::algorithm::GZIP);

		// let subclasses add controller types
		add_controllers();

		// register all added controllers' routes
		register_routes();

		// minimal health/readiness routes
		CROW_ROUTE(m_crowApp, "/healthz")([] {
			return crow::response{ 200, "ok" };
			});
		CROW_ROUTE(m_crowApp, "/ready")([this] {
			if (m_shutdownFlag.load()) return crow::response{ 503, "not ready" };
			return crow::response{ 200, "ready" };
			});

		// enable optional per-request HTTP tracing
		Utils::Tracing::HttpTraceMiddleware::SetEnabled(args().httpTrace());

		// make the bind address explicit (avoid ambiguity across platforms/containers)
		m_crowApp.bindaddr(args().bindAddress());

		// set port to passed-in port or default if none supplied
		m_crowApp.port(static_cast<unsigned short>(args().port()));

		// set server to use multiple threads
		m_crowApp.multithreaded();		

		if (!preServerRun())
			return ExitCode::PreServerRunFailed;

		// Start gRPC server in a separate thread
		start_grpc_server();

		// Start interactive quit (TTY only). Type 'q' + Enter to stop.
		start_interactive_quit_if_tty();

		// run the Crow server (blocks until stop() is called)
		std::cout << "Crow REST server starting on " << args().bindAddress() << ":" << args().port()
				  << " (http-trace=" << (args().httpTrace() ? "true" : "false") << ")" << std::endl;
		m_crowApp.run();

		// Stop interactive watcher and join
		stop_interactive_quit();
		join_interactive_quit();

		// Stop gRPC
		if (m_grpcServer)
		{
			m_grpcServer->Shutdown();
		}

		// Wait for gRPC thread to finish
		if (m_grpcThread && m_grpcThread->joinable())
		{
			m_grpcThread->join();
		}

		if (!postServerRun())
			return ExitCode::PostServerRunFailed;

		// success!
		return ExitCode::Success;
	}

	CrowApp &OdbServerAppBase::crow_app()
	{
		return m_crowApp;
	}

	RequestAuthenticationBase &OdbServerAppBase::request_auth()
	{
		return *m_pRequestAuthentication;
	}

	void OdbServerAppBase::request_auth(std::unique_ptr<RequestAuthenticationBase> pRequestAuthentication)
	{
		m_pRequestAuthentication = std::move(pRequestAuthentication);
	}

	std::shared_ptr<DesignCache> OdbServerAppBase::design_cache()
	{
		return m_pDesignCache;
	}

	void OdbServerAppBase::design_cache(std::shared_ptr<DesignCache> pDesignCache)
	{
		m_pDesignCache = pDesignCache;
	}

	void OdbServerAppBase::register_routes()
	{
		for (const auto &pController : m_vecControllers)
		{
			pController->register_routes();
		}
	}

	void OdbServerAppBase::signal_handler(int signum)
	{
		std::cout << "\nSignal " << signum << " received. Initiating graceful shutdown..." << std::endl;
		stop_servers();
	}
}