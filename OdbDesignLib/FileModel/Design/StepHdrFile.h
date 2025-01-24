#pragma once

#include "../../odbdesign_export.h"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include "../../IProtoBuffable.h"
#include "../../ProtoBuf/stephdrfile.pb.h"
#include "../OdbFile.h"
#include "../IStreamSaveable.h"

namespace Odb::Lib::FileModel::Design
{
	class ODBDESIGN_EXPORT StepHdrFile : public OdbFile, public IProtoBuffable<Odb::Lib::Protobuf::StepHdrFile>, public IStreamSaveable
	{
	public:
		virtual ~StepHdrFile();		

		struct StepRepeatRecord : public IProtoBuffable<Odb::Lib::Protobuf::StepHdrFile::StepRepeatRecord>
		{
			std::string name;
			double x;
			double y;
			double dx;
			double dy;
			int nx;
			int ny;
			double angle;
			bool flip;
			bool mirror;

			typedef std::vector<std::shared_ptr<StepRepeatRecord>> Vector;

			constexpr static const char* ARRAY_HEADER_TOKEN = "STEP-REPEAT";

			// Inherited via IProtoBuffable
			std::unique_ptr<Odb::Lib::Protobuf::StepHdrFile::StepRepeatRecord> to_protobuf() const override;
			void from_protobuf(const Odb::Lib::Protobuf::StepHdrFile::StepRepeatRecord& message) override;
		};

		bool Parse(std::filesystem::path path) override;
		// Inherited via IStreamSaveable
		bool Save(std::ostream& os) override;

		// Inherited via IProtoBuffable
		std::unique_ptr<Odb::Lib::Protobuf::StepHdrFile> to_protobuf() const override;
		void from_protobuf(const Odb::Lib::Protobuf::StepHdrFile& message) override;

	private:	
		std::string m_units;
		double xDatum;
		double yDatum;
		unsigned id;
		double xOrigin;
		double yOrigin;
		double topActive;
		double bottomActive;
		double rightActive;
		double leftActive;
		std::string affectingBom;
		bool affectingBomChanged;		
		std::map<std::string, std::string> m_onlineValues;

		StepRepeatRecord::Vector m_stepRepeatRecords;

	};
}
