#include "StandardFontsFile.h"

#include "Logger.h"
#include <fstream>
#include "str_trim.h"
#include "Constants.h"

namespace Odb::Lib::FileModel::Design
{
	bool StandardFontsFile::Parse(std::filesystem::path path)
	{
		//Utils::Logger::instance()->operator<<("Parsing standard fonts file");

        if (!OdbFile::Parse(path)) return false;

        auto fontsStandardFile = path / "standard";
        if (!std::filesystem::exists(fontsStandardFile)) return false;

        std::ifstream standardFile;
        standardFile.open(fontsStandardFile, std::ios::in);
        if (!standardFile.is_open()) return false;

        std::shared_ptr<CharacterBlock> pCurrentCharacterBlock;
        std::shared_ptr<CharacterBlock::LineRecord> pCurrentLineRecord;
        bool beginTokenFound = false;

        std::string line;
        while (std::getline(standardFile, line))
        {
            // trim whitespace from beginning and end of line
            Utils::str_trim(line);
            if (!line.empty())
            {
                std::stringstream lineStream(line);
                if (line.find(Constants::COMMENT_TOKEN) == 0)
                {
                    // comment line
                }
                else if (line.find("XSIZE") == 0)
                {
                    std::string token;
                    if (!(lineStream >> token)) return false;
                    if (token != "XSIZE") return false;

                    if (!(lineStream >> token)) return false;
                    m_xSize = std::stof(token);
                }
                else if (line.find("YSIZE") == 0)
                {
                    std::string token;
                    if (!(lineStream >> token)) return false;
                    if (token != "YSIZE") return false;

                    if (!(lineStream >> token)) return false;
                    m_ySize = std::stof(token);
                }
                else if (line.find("OFFSET") == 0)
                {
                    std::string token;
                    if (!(lineStream >> token)) return false;
                    if (token != "OFFSET") return false;

                    if (!(lineStream >> token)) return false;
                    m_offset = std::stof(token);
                }
                else if (line.find(CharacterBlock::BEGIN_TOKEN) == 0)
                {
                    pCurrentCharacterBlock = std::make_shared<CharacterBlock>();
                    beginTokenFound = true;

                    std::string token;
                    if (!(lineStream >> token)) return false;
                    if (token != CharacterBlock::BEGIN_TOKEN) return false;

                    if (!(lineStream >> token)) return false;
                    Utils::str_trim(token);
                    if (token.length() != 1) return false;
                    pCurrentCharacterBlock->character = token[0];
                }
                else if (line.find(CharacterBlock::END_TOKEN) == 0)
                {
                    if (pCurrentCharacterBlock != nullptr && beginTokenFound)
                    {
                        m_characterBlocks.push_back(pCurrentCharacterBlock);
                        beginTokenFound = false;
                        pCurrentCharacterBlock.reset();
                    }
                    else
                    {
                        return false;
                    }
                }
                else if (line.find(CharacterBlock::LineRecord::LINE_RECORD_TOKEN) == 0)
                {
                    std::string token;
                    if (!(lineStream >> token)) return false;
                    if (token != CharacterBlock::LineRecord::LINE_RECORD_TOKEN) return false;

                    if (pCurrentCharacterBlock == nullptr || !beginTokenFound) return false;

                    auto pLineRecord = std::make_shared<CharacterBlock::LineRecord>();

                    if (!(lineStream >> token)) return false;
                    pLineRecord->xStart = std::stof(token);

                    if (!(lineStream >> token)) return false;
                    pLineRecord->yStart = std::stof(token);

                    if (!(lineStream >> token)) return false;
                    pLineRecord->xEnd = std::stof(token);

                    if (!(lineStream >> token)) return false;
                    pLineRecord->yEnd = std::stof(token);

                    // polarity
                    if (!(lineStream >> token)) return false;
                    Utils::str_trim(token);
                    if (token.length() != 1) return false;  

                    switch (token[0])
                    {
                    case 'P': pLineRecord->polarity = Polarity::Positive; break;
                    case 'N': pLineRecord->polarity = Polarity::Negative; break;
                    default: return false;
                    }
                    
                    // shape
                    if (!(lineStream >> token)) return false;
                    Utils::str_trim(token);
                    if (token.length() != 1) return false;

                    switch (token[0])
                    {
                    case 'R': pLineRecord->shape = LineShape::Round; break;
                    case 'S': pLineRecord->shape = LineShape::Square; break;
                    default: return false;
                    }

                    // width
                    if (!(lineStream >> token)) return false;
                    pLineRecord->width = std::stof(token);
                    
                    pCurrentCharacterBlock->m_lineRecords.push_back(pLineRecord);
                }
                else
                {
                    return false;
                }
            }
        }

		return true;
	}
}