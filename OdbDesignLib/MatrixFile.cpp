//
// Created by nmill on 10/13/2023.
//

#include "MatrixFile.h"
#include <fstream>
#include "str_trim.h"
#include <string>
#include "constants.h"

namespace Odb::Lib::FileModel::Design
{
	bool MatrixFile::Parse(std::filesystem::path path)
	{
		if (!OdbFile::Parse(path)) return false;

        auto matrixFilePath = path / "matrix";
        if (!std::filesystem::exists(matrixFilePath)) return false;

        std::ifstream matrixFile;
        matrixFile.open(matrixFilePath, std::ios::in);
        if (!matrixFile.is_open()) return false;

        std::string line;
        while (std::getline(matrixFile, line))
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
                else
                {
                    return false;
                }
            }
        }

        return true;
	}
}