#include "FileReader.h"
#include "FileReader.h"
#include <filesystem>
#include <ios>
#include <vector>
#include <fstream>

using namespace std::filesystem;

namespace Utils
{
	FileReader::FileReader(const std::filesystem::path& filePath)
		: m_filePath(filePath)
	{
	}

	//FileReader::FileReader(const std::filesystem::path& filePath, std::ios::openmode mode)
	//	: FileReader(filePath, mode, false)
	//{
	//}

	//FileReader::FileReader(const std::filesystem::path& filePath, std::ios::openmode mode, bool unbuffered)
	//	: m_filePath(filePath)
	//	, m_mode(mode)
	//	, m_unbuffered(unbuffered)
	//{
	//}

	long long FileReader::Read(BufferStrategy bufferStrategy /*= BufferStrategy::Buffered*/, std::ios::openmode mode /*= DEFAULT_OPENMODE*/)
	{
		auto read = 0LL;

		std::ifstream file(m_filePath, mode);
		if (file.is_open())
		{
			if (bufferStrategy == BufferStrategy::Unbuffered)
			{
				long long size = file_size(m_filePath);
				m_buffer.resize(size);
				file.read(m_buffer.data(), size);
				if (file.gcount() == size)
				{
					read = size;
				}
			}
			else
			{
				char szBuffer[BUFFER_SIZE]{ 0 };
				while (true)
				{
					file.read(szBuffer, sizeof(szBuffer));
					auto readin = file.gcount();
					if (readin < 1)
					{
						break;
					}
					m_buffer.insert(m_buffer.end(), szBuffer, szBuffer + readin);
					read += readin;
				}
			}
			m_buffer.push_back(0);
			m_buffer.resize(read);
			file.close();
		}

		return read;
	}

	void FileReader::Clear()
	{
		m_buffer.clear();
	}

	const std::vector<char>& Utils::FileReader::GetBuffer() const
	{
		return m_buffer;
	}
}