#pragma once

#include "OdbFile.h"
#include "odbdesign_export.h"
#include "enums.h"
#include <vector>

namespace Odb::Lib::FileModel::Design
{
	class StandardFontsFile : public OdbFile
	{
	public:
		StandardFontsFile() = default;
		~StandardFontsFile() = default;
	
		bool Parse(std::filesystem::path path) override;

		struct CharacterBlock
		{
			struct LineRecord
			{
				float xStart;
				float yStart;
				float xEnd;
				float yEnd;
				Polarity polarity;
				LineShape shape;
				float width;

				typedef std::vector<std::shared_ptr<LineRecord>> Vector;

				inline static constexpr char* LINE_RECORD_TOKEN = "LINE";
			};

			inline static constexpr char* BEGIN_TOKEN = "CHAR";
			inline static constexpr char* END_TOKEN = "ECHAR";

			char character;
			LineRecord::Vector m_lineRecords;

			typedef std::vector<std::shared_ptr<CharacterBlock>> Vector;

		};

	private:
		float m_xSize;
		float m_ySize;
		float m_offset;

		CharacterBlock::Vector m_characterBlocks;

	};
}
