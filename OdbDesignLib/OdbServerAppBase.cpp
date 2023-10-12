#include "OdbServerAppBase.h"

namespace Odb::Lib
{
    OdbServerAppBase::OdbServerAppBase(int argc, char* argv[])
        : m_designCache("designs")
        , m_commandLineArgs(argc, argv)
    {
    }

    OdbServerAppBase::~OdbServerAppBase()
    {
        m_vecControllers.clear();
    }

    Utils::ExitCode OdbServerAppBase::Run()
    {
        //m_crowApp.loglevel(crow::LogLevel::Debug)
        m_crowApp.use_compression(crow::compression::algorithm::GZIP);

        // let subclasses add controller types
        add_controllers();

        // register all controllers' routes
        register_routes();

        // run the server
        m_crowApp.port(18080).multithreaded().run();

        return Utils::ExitCode::Success;
    }

    void OdbServerAppBase::register_routes()
    {
        for (const auto& pController : m_vecControllers)
        {
            pController->register_routes();
        }
    }
}