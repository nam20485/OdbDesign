#pragma once

#include "../OdbFile.h"
#include "../../odbdesign_export.h"
#include "../../enums.h"
#include <vector>
#include "../../IProtoBuffable.h"
#include "../../ProtoBuf/standardfontsfile.pb.h"
#include "../IStreamSaveable.h"

namespace Odb::Lib::FileModel::Design
{
	class ODBDESIGN_EXPORT StandardFontsFile : public OdbFile, public IProtoBuffable<Odb::Lib::Protobuf::StandardFontsFile>, public IStreamSaveable
	{
	public:
		StandardFontsFile() = default;
		~StandardFontsFile();
	
		bool Parse(std::filesystem::path path) override;
		// Inherited via IStreamSaveable
		bool Save(std::ostream& os) override;

		// Inherited via IProtoBuffable
		std::unique_ptr<Odb::Lib::Protobuf::StandardFontsFile> to_protobuf() const override;
		void from_protobuf(const Odb::Lib::Protobuf::StandardFontsFile& message) override;

		struct CharacterBlock : public IProtoBuffable<Odb::Lib::Protobuf::StandardFontsFile::CharacterBlock>
		{
			~CharacterBlock();

			struct LineRecord : public IProtoBuffable<Odb::Lib::Protobuf::StandardFontsFile::CharacterBlock::LineRecord>
			{
				double xStart;
				double yStart;
				double xEnd;
				double yEnd;
				Polarity polarity;
				LineShape shape;
				double width;

				std::unique_ptr<Odb::Lib::Protobuf::StandardFontsFile::CharacterBlock::LineRecord> to_protobuf() const override;
				void from_protobuf(const Odb::Lib::Protobuf::StandardFontsFile::CharacterBlock::LineRecord& message) override;

				typedef std::vector<std::shared_ptr<LineRecord>> Vector;

				inline static constexpr const char* RECORD_TOKEN = "LINE";
			};

			inline static constexpr const char* BEGIN_TOKEN = "CHAR";
			inline static constexpr const char* END_TOKEN = "ECHAR";

			char character;
			LineRecord::Vector m_lineRecords;

			std::unique_ptr<Odb::Lib::Protobuf::StandardFontsFile::CharacterBlock> to_protobuf() const override;
			void from_protobuf(const Odb::Lib::Protobuf::StandardFontsFile::CharacterBlock& message) override;			

			typedef std::vector<std::shared_ptr<CharacterBlock>> Vector;

		};

	private:
		double m_xSize;
		double m_ySize;
		double m_offset;

		CharacterBlock::Vector m_characterBlocks;				
	};
}
