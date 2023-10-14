#include "OdbServerAppBase.h"
#include "Logger.h"

namespace Odb::Lib
{
    OdbAppBase::OdbAppBase(int argc, char* argv[])
        : m_designCache("designs")
        , m_commandLineArgs(argc, argv)
    {        
        GOOGLE_PROTOBUF_VERIFY_VERSION;
    }

    OdbAppBase::~OdbAppBase()
    {   
        Logger::instance()->stop();
        google::protobuf::ShutdownProtobufLibrary();
    }

    const CommandLineArgs& OdbAppBase::arguments() const
    {
        return m_commandLineArgs;
    }

    DesignCache& OdbAppBase::design_cache()
    {
        return m_designCache;
    }

    Utils::ExitCode OdbAppBase::Run()
    {                
        Logger::instance()->SetLogLevel(Utils::LogLevel::Info);
        Logger::instance()->start();

        return Utils::ExitCode::Success;
    }   
}