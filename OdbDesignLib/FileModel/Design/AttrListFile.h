//
// Created by nmill on 10/13/2023.
//
#ifndef ODBDESIGN_ATTRLISTFILE_H
#define ODBDESIGN_ATTRLISTFILE_H

#include <string>
#include <map>
#include <filesystem>

namespace Odb::Lib::FileModel::Design
{
    class AttrListFile
    {
    public:
        AttrListFile();

        typedef std::map<std::string, std::string> AttributeMap;

        std::string GetUnits() const;
        const AttributeMap& GetAttributes() const;

        bool Parse(std::filesystem::path directory);

    private:
        std::filesystem::path m_directory;
        std::filesystem::path m_path;
        std::string m_units;    
        AttributeMap m_attributes;

        bool attributeValueIsOptional(const std::string& attributeName) const;

        inline static const auto ATTRLIST_FILENAMES = { "attrlist" };
        inline static const char* OPTIONAL_ATTRIBUTES[] = { "" };
    };

}

#endif //ODBDESIGN_ATTRLISTFILE_H
