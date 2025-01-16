//
// Created by nmill on 10/13/2023.
//
#ifndef ODBDESIGN_ATTRLISTFILE_H
#define ODBDESIGN_ATTRLISTFILE_H

#include <string>
#include <map>
#include <filesystem>
#include "../../IProtoBuffable.h"
#include "../../ProtoBuf/attrlistfile.pb.h"
#include "../../odbdesign_export.h"
#include "../IStreamSaveable.h"

namespace Odb::Lib::FileModel::Design
{
    class ODBDESIGN_EXPORT AttrListFile : public IProtoBuffable<Odb::Lib::Protobuf::AttrListFile>, public IStreamSaveable
    {
    public:
        AttrListFile();

        typedef std::map<std::string, std::string> AttributeMap;

        std::string GetUnits() const;
        const AttributeMap& GetAttributes() const;

        bool Parse(std::filesystem::path directory);
        // Inherited via IStreamSaveable
        bool Save(std::ostream& os) override;

        // Inherited via IProtoBuffable
        std::unique_ptr<Odb::Lib::Protobuf::AttrListFile> to_protobuf() const override;
        void from_protobuf(const Odb::Lib::Protobuf::AttrListFile& message) override;

    private:
        std::filesystem::path m_directory;
        std::filesystem::path m_path;
        std::string m_units;    
        AttributeMap m_attributesByName;

        bool attributeValueIsOptional(const std::string& attributeName) const;

        inline static const auto ATTRLIST_FILENAMES = { "attrlist" };
        inline static const char* OPTIONAL_ATTRIBUTES[] = { "" };       
    };
}

#endif //ODBDESIGN_ATTRLISTFILE_H
