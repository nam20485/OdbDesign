#pragma once

#include "../../odbdesign_export.h"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include "../../IProtoBuffable.h"
#include "stephdrfile.pb.h"
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
			double x = 0.0;
			double y = 0.0;
			double dx = 0.0;
			double dy = 0.0;
			int nx = 0;
			int ny = 0;
			double angle = 0.0;
			bool flip = false;
			bool mirror = false;

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
		// POD members carry in-class initializers: ODB++ makes every stephdr
		// attribute optional, and to_protobuf() serializes whatever an unset
		// member holds. An uninitialized bool read in optimized builds puts a
		// non-0/1 raw byte on the wire as a multi-byte varint, desyncing the
		// whole serialized stream (the Turbot GetDesign corruption).
		std::string m_units;
		double xDatum = 0.0;
		double yDatum = 0.0;
		unsigned id = 0;
		double xOrigin = 0.0;
		double yOrigin = 0.0;
		double topActive = 0.0;
		double bottomActive = 0.0;
		double rightActive = 0.0;
		double leftActive = 0.0;
		std::string affectingBom;
		bool affectingBomChanged = false;
		std::map<std::string, std::string> m_onlineValues;

		StepRepeatRecord::Vector m_stepRepeatRecords;

	};
}

