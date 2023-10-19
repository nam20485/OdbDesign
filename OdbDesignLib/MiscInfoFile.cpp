//
// Created by nmill on 10/13/2023.
//

#include <fstream>
#include "MiscInfoFile.h"
#include <string>
#include <sstream>
#include "str_trim.h"
#include "Constants.h"
#include "timestamp.h"
#include "Logger.h"

using namespace std::chrono;

namespace Odb::Lib::FileModel::Design
{
    bool MiscInfoFile::Parse(std::filesystem::path path)
    {
        if (!OdbFile::Parse(path)) return false;

        auto infoFilePath = path / "info";
        if (!std::filesystem::exists(infoFilePath)) return false;

        std::ifstream infoFile;
        infoFile.open(infoFilePath, std::ios::in);
        if (!infoFile.is_open()) return false;

        std::string line;
        while (std::getline(infoFile, line))
        {
            // trim whitespace from beginning and end of line
            Utils::str_trim(line);
            if (!line.empty())
            {
                std::stringstream lineStream(line);
                if (line.find(Constants::COMMENT_TOKEN) == 0)
                {
                    // comment line
                }
                else
                {
                    // attribute line
                    std::string attribute;
                    std::string value;

                    if (! std::getline(lineStream, attribute, '=')) return false;
                    if (! std::getline(lineStream, value)) return false;

                    Utils::str_trim(attribute);
                    Utils::str_trim(value);

                    if (attribute == "PRODUCT_MODEL_NAME" ||
                        attribute == "product_model_name")
                    {
                        m_productModelName = value;
                    }
                    else if (attribute == "JOB_NAME" ||
                             attribute == "job_name")
                    {
                        m_jobName = value;
                    }
                    else if (attribute == "odb_version_major" ||
                             attribute == "ODB_VERSION_MAJOR")
                    {
                        m_odbVersionMajor = value;
                    }
                    else if (attribute == "odb_version_minor" ||
                             attribute == "ODB_VERSION_MINOR")
                    {
                        m_odbVersionMinor = value;
                    }
                    else if (attribute == "odb_source" ||
                             attribute == "ODB_SOURCE")
                    {
                        m_odbSource = value;
                    }
                    else if (attribute == "creation_date" ||
                             attribute == "CREATION_DATE")
                    {
                        //std::istringstream iss(value);
                        //// yyyymmdd.hhmmss
                        //iss >> std::chrono::parse("%Y%m%d.%H%M%S", m_creationDateDate);
                        m_creationDateDate = Utils::parse_timestamp(value, "%Y%m%d.%H%M%S");

#if defined(_DEBUG)

                        auto createdDate = Utils::make_timestamp(m_creationDateDate);
                        std::stringstream ss;
                        ss << "value: " << value << ", parsed createdDate: " << createdDate;
                        loginfo(ss.str());
#endif // _DEBUG
                    }
                    else if (attribute == "save_date" ||
                             attribute == "SAVE_DATE")
                    {
                        //// yyyymmdd.hhmmss                        
                        //std::istringstream(value) >> std::chrono::parse("%Y%m%d.%H%M%S", m_saveDate);
                        m_saveDate = Utils::parse_timestamp(value, "%Y%m%d.%H%M%S");
#if defined(_DEBUG)
                        auto saveDate = Utils::make_timestamp(m_saveDate);

                        std::stringstream ss;
                        ss << "value: " << value << ", parsed saveDate: " << saveDate;
                        loginfo(ss.str());
#endif // _DEBUG                                                
                    }
                    else if (attribute == "save_app" ||
                             attribute == "SAVE_APP")
                    {
                        m_saveApp = value;
                    }
                    else if (attribute == "save_user" ||
                             attribute == "SAVE_USER")
                    {
                        m_saveUser = value;
                    }
                    else if (attribute == "units" ||
                             attribute == "UNITS")
                    {
                        m_units = value;
                    }
                    else if (attribute == "max_uid" ||
                             attribute == "MAX_UID")
                    {
                        m_maxUniqueId = std::stoi(value);
                    }
                    else
                    {
                        return false;
                    }
                }
            }
        }

        return true;
    }

    std::string MiscInfoFile::GetProductModelName() const
    {
        return m_productModelName;
    }

    std::string MiscInfoFile::GetJobName() const
    {
        return m_jobName;
    }

    std::string MiscInfoFile::GetOdbVersionMajor() const
    {
        return m_odbVersionMajor;
    }

    std::string MiscInfoFile::GetOdbVersionMinor() const
    {
        return m_odbVersionMinor;
    }

    std::string MiscInfoFile::GetOdbSource() const
    {
        return m_odbSource;
    }

    system_clock::time_point MiscInfoFile::GetCreationDate() const
    {
        return m_creationDateDate;
    }

    system_clock::time_point MiscInfoFile::GetSaveDate() const
    {
        return m_saveDate;
    }

    std::string MiscInfoFile::GetSaveApp() const
    {
        return m_saveApp;
    }

    std::string MiscInfoFile::GetSaveUser() const
    {
        return m_saveUser;
    }

    std::string MiscInfoFile::GetUnits() const
    {
        return m_units;
    }

    unsigned int MiscInfoFile::GetMaxUniqueId() const
    {
        return m_maxUniqueId;
    }
}