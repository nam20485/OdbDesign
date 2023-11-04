#include "StandardFontsFile.h"

#include "Logger.h"
#include <fstream>
#include "str_trim.h"
#include "../../Constants.h"
#include "../parse_error.h"

namespace Odb::Lib::FileModel::Design
{
    bool StandardFontsFile::Parse(std::filesystem::path path)
    {
        std::ifstream standardFile;
        int lineNumber = 0;
        std::string line;

        try
        {
            if (!OdbFile::Parse(path)) return false;

            auto fontsStandardFile = path / "standard";
            if (!std::filesystem::exists(fontsStandardFile))
            {
                auto message = "fonts/standard file does not exist: [" + fontsStandardFile.string() + "]";
                throw std::exception(message.c_str());
            }

            standardFile.open(fontsStandardFile, std::ios::in);
            if (!standardFile.is_open())
            {
                auto message = "unable to open fonts/standard file: [" + fontsStandardFile.string() + "]";
                throw std::exception(message.c_str());
            }

            std::shared_ptr<CharacterBlock> pCurrentCharacterBlock;
            std::shared_ptr<CharacterBlock::LineRecord> pCurrentLineRecord;
            bool beginTokenFound = false;

            while (std::getline(standardFile, line))
            {
                lineNumber++;

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
                        if (!(lineStream >> token))
                        {
                            throw_parse_error(m_path, line, token, lineNumber);
                        }

                        if (token != "XSIZE")
                        {
                            throw_parse_error(m_path, line, token, lineNumber);
                        }

                        if (!(lineStream >> token))
                        {
                            throw_parse_error(m_path, line, token, lineNumber);
                        }

                        m_xSize = std::stof(token);
                    }
                    else if (line.find("YSIZE") == 0)
                    {
                        std::string token;
                        if (!(lineStream >> token))
                        {
                            throw_parse_error(m_path, line, token, lineNumber);
                        }

                        if (token != "YSIZE")
                        {
                            throw_parse_error(m_path, line, token, lineNumber);
                        }

                        if (!(lineStream >> token))
                        {
                            throw_parse_error(m_path, line, token, lineNumber);
                        }

                        m_ySize = std::stof(token);
                    }
                    else if (line.find("OFFSET") == 0)
                    {
                        std::string token;
                        if (!(lineStream >> token))
                        {
                            throw_parse_error(m_path, line, token, lineNumber);
                        }

                        if (token != "OFFSET")
                        {
                            throw_parse_error(m_path, line, token, lineNumber);
                        }

                        if (!(lineStream >> token))
                        {
                            throw_parse_error(m_path, line, token, lineNumber);
                        }

                        m_offset = std::stof(token);
                    }
                    else if (line.find(CharacterBlock::BEGIN_TOKEN) == 0)
                    {
                        pCurrentCharacterBlock = std::make_shared<CharacterBlock>();
                        beginTokenFound = true;

                        std::string token;
                        if (!(lineStream >> token))
                        {
                            throw_parse_error(m_path, line, token, lineNumber);
                        }

                        if (token != CharacterBlock::BEGIN_TOKEN)
                        {
                            throw_parse_error(m_path, line, token, lineNumber);
                        }

                        if (!(lineStream >> token))
                        {
                            throw_parse_error(m_path, line, token, lineNumber);
                        }

                        Utils::str_trim(token);
                        if (token.length() != 1)
                        {
                            throw_parse_error(m_path, line, token, lineNumber);
                        }

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
                            throw_parse_error(m_path, line, "", lineNumber);
                        }
                    }
                    else if (line.find(CharacterBlock::LineRecord::RECORD_TOKEN) == 0)
                    {
                        std::string token;
                        if (!(lineStream >> token))
                        {
                            throw_parse_error(m_path, line, token, lineNumber);
                        }

                        if (token != CharacterBlock::LineRecord::RECORD_TOKEN)
                        {
                            throw_parse_error(m_path, line, token, lineNumber);
                        }

                        if (pCurrentCharacterBlock == nullptr || !beginTokenFound)
                        {
                            throw_parse_error(m_path, line, token, lineNumber);
                        }

                        auto pLineRecord = std::make_shared<CharacterBlock::LineRecord>();

                        if (!(lineStream >> token))
                        {
                            throw_parse_error(m_path, line, token, lineNumber);
                        }
                        pLineRecord->xStart = std::stof(token);

                        if (!(lineStream >> token))
                        {
                            throw_parse_error(m_path, line, token, lineNumber);
                        }
                        pLineRecord->yStart = std::stof(token);

                        if (!(lineStream >> token))
                        {
                            throw_parse_error(m_path, line, token, lineNumber);
                        }
                        pLineRecord->xEnd = std::stof(token);

                        if (!(lineStream >> token))
                        {
                            throw_parse_error(m_path, line, token, lineNumber);
                        }
                        pLineRecord->yEnd = std::stof(token);

                        // polarity
                        if (!(lineStream >> token))
                        {
                            throw_parse_error(m_path, line, token, lineNumber);
                        }
                        Utils::str_trim(token);

                        if (token.length() != 1)
                        {
                            throw_parse_error(m_path, line, token, lineNumber);
                        }

                        switch (token[0])
                        {
                        case 'P': pLineRecord->polarity = Polarity::Positive; break;
                        case 'N': pLineRecord->polarity = Polarity::Negative; break;
                        default: throw_parse_error(m_path, line, token, lineNumber);
                        }

                        // shape
                        if (!(lineStream >> token))
                        {
                            throw_parse_error(m_path, line, token, lineNumber);
                        }
                        Utils::str_trim(token);

                        if (token.length() != 1)
                        {
                            throw_parse_error(m_path, line, token, lineNumber);
                        }

                        switch (token[0])
                        {
                        case 'R': pLineRecord->shape = LineShape::Round; break;
                        case 'S': pLineRecord->shape = LineShape::Square; break;
                        default: throw_parse_error(m_path, line, token, lineNumber);
                        }

                        // width
                        if (!(lineStream >> token))
                        {
                            throw_parse_error(m_path, line, token, lineNumber);
                        }
                        pLineRecord->width = std::stof(token);

                        pCurrentCharacterBlock->m_lineRecords.push_back(pLineRecord);
                    }
                    else
                    {
                        logwarn("unrecognized line: " + line);
                        throw_parse_error(m_path, line, "", lineNumber);
                    }
                }
            }

            standardFile.close();
        }
        catch (parse_error& pe)
        {
            auto m = pe.toString("Parse Error:");
            logerror(m);
            // cleanup file
            standardFile.close();
            throw pe;
        }
        catch (std::exception& e)
        {
            parse_info pi(m_path, line, lineNumber);
            const auto m = pi.toString();
            logexception_msg(e, m);
            // cleanup file
            standardFile.close();
            throw e;
        }

		return true;
	}
}