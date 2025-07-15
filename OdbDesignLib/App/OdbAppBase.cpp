#include "OdbServerAppBase.h"
#include "Logger.h"
#include <filesystem>

using namespace Utils;
using namespace std::filesystem;

namespace Odb::Lib::App
{
    OdbAppBase::OdbAppBase(int argc, char* argv[])
        : m_designCache(DEFAULT_DESIGNS_DIR)
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

        // set designs dir if passed in
        if (!args().designsDir().empty())
        {
			designs().setDirectory(args().designsDir());
		}

        // load a design if specified via command line args
        if (!args().loadDesign().empty())
        {
            try
            {
                auto pFileArchive = 
                    //designs().GetDesign(args().loadDesign());
                    designs().GetFileArchive(args().loadDesign());
                if (pFileArchive == nullptr)
                {
                    logerror("Failed to load design specified in arguments \"" + args().loadDesign() + "\"");
                    return Utils::ExitCode::FailedInitLoadDesign;
                }
                else
                {
                    //std::string filename = 
                    pFileArchive->SaveFileModel(".", "notused");
                }
            }
            catch (filesystem_error& fe)
            {
                logexception(fe);
                logerror("filesystem_error: \"" + args().loadDesign() + "\" " + fe.what());
            }
            catch (std::exception& e)
            {
                logexception(e);
                logerror("Failed to load design specified in arguments \"" + args().loadDesign() + "\"");
                return Utils::ExitCode::FailedInitLoadDesign;
            }
        }
        
        // load all designs from designs-dir if specified
        if (args().loadAll())
        {
            designs().loadAllDesigns(false);
        }

        return Utils::ExitCode::Success;
    }   
}