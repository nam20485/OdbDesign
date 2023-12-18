#pragma once

#include "../../odbdesign_export.h"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include "../../IProtoBuffable.h"
#include "../../ProtoBuf/stephdrfile.pb.h"
#include "../OdbFile.h"

namespace Odb::Lib::FileModel::Design
{
	class ODBDESIGN_EXPORT StepHdrFile : public OdbFile, public IProtoBuffable<Odb::Lib::Protobuf::StepHdrFile>
	{
	public:
		virtual ~StepHdrFile();		

		struct StepRepeatRecord : public IProtoBuffable<Odb::Lib::Protobuf::StepHdrFile::StepRepeatRecord>
		{
			std::string name;
			float x;
			float y;
			float dx;
			float dy;
			int nx;
			int ny;
			float angle;
			bool flip;
			bool mirror;

			typedef std::vector<std::shared_ptr<StepRepeatRecord>> Vector;

			constexpr static const char* ARRAY_HEADER_TOKEN = "STEP-REPEAT";

			// Inherited via IProtoBuffable
			std::unique_ptr<Odb::Lib::Protobuf::StepHdrFile::StepRepeatRecord> to_protobuf() const override;
			void from_protobuf(const Odb::Lib::Protobuf::StepHdrFile::StepRepeatRecord& message) override;
		};

		bool Parse(std::filesystem::path path) override;

		// Inherited via IProtoBuffable
		std::unique_ptr<Odb::Lib::Protobuf::StepHdrFile> to_protobuf() const override;
		void from_protobuf(const Odb::Lib::Protobuf::StepHdrFile& message) override;

	private:	
		std::string m_units;
		float xDatum;
		float yDatum;
		unsigned id;
		float xOrigin;
		float yOrigin;
		float topActive;
		float bottomActive;
		float rightActive;
		float leftActive;
		std::string affectingBom;
		bool affectingBomChanged;		
		std::map<std::string, std::string> m_onlineValues;

		StepRepeatRecord::Vector m_stepRepeatRecords;

	};
}
