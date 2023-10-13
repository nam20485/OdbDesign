#include "OdbServerAppBase.h"

namespace Odb::Lib
{
    OdbAppBase::OdbAppBase(int argc, char* argv[])
        : m_designCache("designs")
        , m_commandLineArgs(argc, argv)
    {
    }

    OdbAppBase::~OdbAppBase()
    {       
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
        //m_crowApp.loglevel(crow::LogLevel::Debug)        

        return Utils::ExitCode::Success;
    }   
}