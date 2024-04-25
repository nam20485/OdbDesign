#include "MiscInfoFile.h"
#include "MiscInfoFile.h"
//
// Created by nmill on 10/13/2023.
//

#include <fstream>
#include "MiscInfoFile.h"
#include <string>
#include <sstream>
#include "str_utils.h"
#include "../../Constants.h"
#include "timestamp.h"
#include "Logger.h"
#include "../parse_error.h"
#include "../invalid_odb_error.h"

using namespace std::chrono;

namespace Odb::Lib::FileModel::Design
{
    bool MiscInfoFile::Parse(std::filesystem::path path)
    {
        std::ifstream infoFile;
        int lineNumber = 0;
        std::string line;

        try
        {
            if (!OdbFile::Parse(path))
            {
                auto message = "\"misc\" directory does not exist: [" + path.string() + "]";
                throw invalid_odb_error(message);
            }

            auto infoFilePath = path / "info";
            if (!std::filesystem::exists(infoFilePath))
            {
                auto message = "misc/info file does not exist: [" + infoFilePath.string() + "]";
                throw invalid_odb_error(message);
            }

            infoFile.open(infoFilePath, std::ios::in);
            if (!infoFile.is_open())
            {
                auto message = "unable to open misc/info file: [" + infoFilePath.string() + "]";
                throw invalid_odb_error(message);
            }

            while (std::getline(infoFile, line))
            {
                lineNumber++;

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

                        if (!std::getline(lineStream, attribute, '=')) return false;
                        // attribute value may be blank?

                        if (!std::getline(lineStream, value))
                        {
                            if (!attributeValueIsOptional(attribute))
                            {
                                logwarn("misc/info file: no value for non-optional attribute: " + attribute);
                            }
                        }

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
                            // unknown attribute

                            // DO NOT fail parsing on unknown attributes- log instead 
                            //return false;

                            parse_info pi(m_path, line, attribute, lineNumber);
                            logwarn(pi.toString("unrecognized line in misc/info file:"));
                        }
                    }
                }
            }

            infoFile.close();
        }
        catch (invalid_odb_error& ioe)
        {
            parse_info pi(m_path, line, lineNumber);
            const auto m = pi.toString();
            logexception_msg(ioe, m);
            // cleanup file
            infoFile.close();
            throw ioe;
        }
        catch (std::exception& e)
        {
            parse_info pi(m_path, line, lineNumber);
            const auto m = pi.toString();
            logexception_msg(e, m);
            // cleanup file
            infoFile.close();
            throw e;
        }

        return true;
    }

    bool MiscInfoFile::Save(std::ostream& os)
    {
        os << PRODUCT_MODEL_NAME_KEY << "=" << m_productModelName << std::endl;
        os << JOB_NAME_KEY << "=" << m_jobName << std::endl;
        os << ODB_VERSION_MAJOR_KEY << "=" << m_odbVersionMajor << std::endl;
        os << ODB_VERSION_MINOR_KEY << "=" << m_odbVersionMinor << std::endl;
        os << ODB_SOURCE_KEY << "=" << m_odbSource << std::endl;
        os << CREATION_DATE_KEY << "=" << Utils::make_timestamp(m_creationDateDate) << std::endl;
        os << SAVE_DATE_KEY << "=" << Utils::make_timestamp(m_saveDate) << std::endl;
        os << SAVE_APP_KEY << "=" << m_saveApp << std::endl;
        os << SAVE_USER_KEY << "=" << m_saveUser << std::endl;
        os << UNITS_KEY << "=" << m_units << std::endl;
        os << MAX_UID_KEY << "=" << m_maxUniqueId << std::endl;               

        return true;
    }

    /*static*/ inline bool MiscInfoFile::attributeValueIsOptional(const std::string& attribute)
    {
        for (const auto& optionalAttribute : OPTIONAL_ATTRIBUTES)
        {
            if (attribute == optionalAttribute)
            {
                return true;
            }
        }
        return false;
    }

    std::unique_ptr<Odb::Lib::Protobuf::MiscInfoFile> MiscInfoFile::to_protobuf() const
    {
        std::unique_ptr<Odb::Lib::Protobuf::MiscInfoFile> pMiscInfoFileMessage(new Odb::Lib::Protobuf::MiscInfoFile);
        pMiscInfoFileMessage->set_jobname(m_jobName);
        pMiscInfoFileMessage->set_productmodelname(m_productModelName);
        pMiscInfoFileMessage->set_odbversionmajor(m_odbVersionMajor);
        pMiscInfoFileMessage->set_odbversionminor(m_odbVersionMinor);
        pMiscInfoFileMessage->set_odbsource(m_odbSource);
        auto seconds = std::chrono::duration_cast<std::chrono::seconds>(m_creationDateDate.time_since_epoch()).count();
        pMiscInfoFileMessage->mutable_creationdatedate()->set_seconds(seconds);
        pMiscInfoFileMessage->mutable_creationdatedate()->set_nanos(0);
        seconds = std::chrono::duration_cast<std::chrono::seconds>(m_saveDate.time_since_epoch()).count();
        pMiscInfoFileMessage->mutable_savedate()->set_seconds(seconds);
        pMiscInfoFileMessage->mutable_savedate()->set_nanos(0);
        pMiscInfoFileMessage->set_saveapp(m_saveApp);
        pMiscInfoFileMessage->set_saveuser(m_saveUser);
        pMiscInfoFileMessage->set_units(m_units);
        pMiscInfoFileMessage->set_maxuniqueid(m_maxUniqueId);
        return pMiscInfoFileMessage;
    }

    void MiscInfoFile::from_protobuf(const Odb::Lib::Protobuf::MiscInfoFile& message)
    {
        m_jobName = message.jobname();
		m_productModelName = message.productmodelname();
		m_odbVersionMajor = message.odbversionmajor();
		m_odbVersionMinor = message.odbversionminor();
		m_odbSource = message.odbsource();
		//m_creationDateDate = system_clock::from_time_t(message.creationdatedate().seconds());        
        m_creationDateDate = std::chrono::system_clock::time_point(std::chrono::seconds(message.creationdatedate().seconds()));
		m_saveDate = std::chrono::system_clock::time_point(std::chrono::seconds(message.savedate().seconds()));
		m_saveApp = message.saveapp();
		m_saveUser = message.saveuser();
		m_units = message.units();
		m_maxUniqueId = message.maxuniqueid();
    }

    MiscInfoFile::MiscInfoFile()
        : m_maxUniqueId((unsigned int)-1)
    {
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