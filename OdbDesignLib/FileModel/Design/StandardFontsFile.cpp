#include "StandardFontsFile.h"
#include "StandardFontsFile.h"

#include "Logger.h"
#include <fstream>
#include "str_utils.h"
#include "../../Constants.h"
#include "../parse_error.h"
#include "../invalid_odb_error.h"
#include "../../ProtoBuf/enums.pb.h"


namespace Odb::Lib::FileModel::Design
{
    StandardFontsFile::~StandardFontsFile()
    {
        m_characterBlocks.clear();
    }

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
                throw invalid_odb_error(message.c_str());
            }

            standardFile.open(fontsStandardFile, std::ios::in);
            if (!standardFile.is_open())
            {
                auto message = "unable to open fonts/standard file: [" + fontsStandardFile.string() + "]";
                throw invalid_odb_error(message.c_str());
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
        catch (invalid_odb_error& ioe)
        {
            parse_info pi(m_path, line, lineNumber);
            const auto m = pi.toString();
            logexception_msg(ioe, m);
            // cleanup file
            standardFile.close();
            throw ioe;
        }

		return true;
	}

    std::unique_ptr<Odb::Lib::Protobuf::StandardFontsFile> StandardFontsFile::to_protobuf() const
    {
        auto pStandardFontsFileMessage = std::make_unique<Odb::Lib::Protobuf::StandardFontsFile>();
        pStandardFontsFileMessage->set_xsize(m_xSize);
        pStandardFontsFileMessage->set_ysize(m_ySize);
        pStandardFontsFileMessage->set_offset(m_offset);
        for (const auto& characterBlock : m_characterBlocks)
        {
            pStandardFontsFileMessage->add_m_characterblocks()->CopyFrom(*characterBlock->to_protobuf());
        }
        return pStandardFontsFileMessage;        
    }

    void StandardFontsFile::from_protobuf(const Odb::Lib::Protobuf::StandardFontsFile& message)
    {
        m_xSize = message.xsize();
        m_ySize = message.ysize();
        m_offset = message.offset();
        for (const auto& characterBlockMessage : message.m_characterblocks())
        {
            auto pCharacterBlock = std::make_shared<CharacterBlock>();
            pCharacterBlock->from_protobuf(characterBlockMessage);
            m_characterBlocks.push_back(pCharacterBlock);
        }
    }

    bool StandardFontsFile::Save(std::ostream& os)
    {
        return true;
    }

    StandardFontsFile::CharacterBlock::~CharacterBlock()
    {
        m_lineRecords.clear();
    }

    std::unique_ptr<Odb::Lib::Protobuf::StandardFontsFile::CharacterBlock> StandardFontsFile::CharacterBlock::to_protobuf() const
    {
        std::unique_ptr<Odb::Lib::Protobuf::StandardFontsFile::CharacterBlock> pCharacterBlockMessage(new Odb::Lib::Protobuf::StandardFontsFile::CharacterBlock);
        pCharacterBlockMessage->set_character(std::string(1, character));
        for (const auto& lineRecord : m_lineRecords)
        {
            pCharacterBlockMessage->add_m_linerecords()->CopyFrom(*lineRecord->to_protobuf());
        }
        return pCharacterBlockMessage;
    }

    void StandardFontsFile::CharacterBlock::from_protobuf(const Odb::Lib::Protobuf::StandardFontsFile::CharacterBlock& message)
    {
        if (! message.character().empty())  character = message.character()[0];

        for (const auto& lineRecordMessage : message.m_linerecords())
        {
            auto pLineRecord = std::make_shared<LineRecord>();
            pLineRecord->from_protobuf(lineRecordMessage);
            m_lineRecords.push_back(pLineRecord);
        }
    }

    std::unique_ptr<Odb::Lib::Protobuf::StandardFontsFile::CharacterBlock::LineRecord> StandardFontsFile::CharacterBlock::LineRecord::to_protobuf() const
    {
        std::unique_ptr<Odb::Lib::Protobuf::StandardFontsFile::CharacterBlock::LineRecord> pLineRecordMessage(new Odb::Lib::Protobuf::StandardFontsFile::CharacterBlock::LineRecord);
        pLineRecordMessage->set_xstart(xStart);
        pLineRecordMessage->set_ystart(yStart);
        pLineRecordMessage->set_xend(xEnd);
        pLineRecordMessage->set_yend(yEnd);
        pLineRecordMessage->set_polarity(static_cast<Odb::Lib::Protobuf::Polarity>(polarity));
        pLineRecordMessage->set_shape(static_cast<Odb::Lib::Protobuf::LineShape>(shape));
        pLineRecordMessage->set_width(width);
        return pLineRecordMessage;
    }

    void StandardFontsFile::CharacterBlock::LineRecord::from_protobuf(const Odb::Lib::Protobuf::StandardFontsFile::CharacterBlock::LineRecord& message)
    {
        xStart = message.xstart();
        yStart = message.ystart();
        xEnd = message.xend();
        yEnd = message.yend();
        polarity = static_cast<Odb::Lib::Polarity>(message.polarity());
        shape = static_cast<LineShape>(message.shape());
        width = message.width();
    }
  
}