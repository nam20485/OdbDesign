//
// Created by nmill on 10/13/2023.
//

#include "../OdbFile.h"
#include <chrono>

#pragma once

namespace Odb::Lib::FileModel::Design
{

    class MiscInfoFile : public OdbFile
    {
    public:
        MiscInfoFile() = default;
        ~MiscInfoFile() = default;

        std::string GetProductModelName() const;
        std::string GetJobName() const;
        std::string GetOdbVersionMajor() const;
        std::string GetOdbVersionMinor() const;
        std::string GetOdbSource() const;        
        std::chrono::system_clock::time_point GetCreationDate() const;
        std::chrono::system_clock::time_point GetSaveDate() const;
        std::string GetSaveApp() const;
        std::string GetSaveUser() const;
        std::string GetUnits() const;
        unsigned int GetMaxUniqueId() const;

        bool Parse(std::filesystem::path path) override;

    private:
        std::string m_productModelName;
        std::string m_jobName;
        std::string m_odbVersionMajor;
        std::string m_odbVersionMinor;
        std::string m_odbSource;
        std::chrono::system_clock::time_point m_creationDateDate;   // "20161024.101454"
        std::chrono::system_clock::time_point m_saveDate;           // "20170810.132829"
        std::string m_saveApp;
        std::string m_saveUser;
        std::string m_units;
        unsigned int m_maxUniqueId;

        static inline bool attributeValueIsOptional(const std::string& attribute);

        constexpr inline static const char* OPTIONAL_ATTRIBUTES[] =
        {
           "ODB_SOURCE",
           "MAX_UID",
        };

    };
}
