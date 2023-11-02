#include "OdbServerAppBase.h"
#include "Logger.h"

using namespace Utils;

namespace Odb::Lib::App
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

    const OdbDesignArgs& OdbAppBase::args() const
    {
        return m_commandLineArgs;
    }

    DesignCache& OdbAppBase::designs()
    {
        return m_designCache;
    }

    Utils::ExitCode OdbAppBase::Run()
    {                
        //Logger::instance()->logLevel(Logger::Level::Debug);
        Logger::instance()->logLevel(Logger::Level::Info);
        Logger::instance()->start();

        // load a design if specified via command line args
        if (!args().loadDesign().empty())
        {
            try
            {
                auto pFileArchive = designs().GetFileArchive(args().loadDesign());
                if (pFileArchive == nullptr)
                {
                    logerror("Failed to load design specified in arguments \"" + args().loadDesign() + "\"");
                    return Utils::ExitCode::FailedInitLoadDesign;
                }
            }
            catch (std::exception& e)
            {
                logerror("Failed to load design specified in arguments \"" + args().loadDesign() + "\"");
                return Utils::ExitCode::FailedInitLoadDesign;
            }
        }

        return Utils::ExitCode::Success;
    }   
}