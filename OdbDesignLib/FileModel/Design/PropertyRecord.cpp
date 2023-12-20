#include "PropertyRecord.h"

namespace Odb::Lib::FileModel::Design
{
    // Inherited via IProtoBuffable
    std::unique_ptr<Odb::Lib::Protobuf::PropertyRecord>
    PropertyRecord::to_protobuf() const
    {
        std::unique_ptr<Odb::Lib::Protobuf::PropertyRecord> pPropertyRecordMessage(new Odb::Lib::Protobuf::PropertyRecord);
        pPropertyRecordMessage->set_name(name);
        pPropertyRecordMessage->set_value(value);
        for (const auto& f : floatValues)
        {
            pPropertyRecordMessage->add_floatvalues(f);
        }
        return pPropertyRecordMessage;
    }

    void PropertyRecord::from_protobuf(const Odb::Lib::Protobuf::PropertyRecord& message)
    {
        name = message.name();
		value = message.value();
        for (int i = 0; i < message.floatvalues_size(); ++i)
        {
			floatValues.push_back(message.floatvalues(i));
		}
    }
}