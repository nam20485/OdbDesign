#include "FileReader.h"
#include "FileReader.h"
#include <filesystem>
#include <ios>
#include <vector>
#include <iosfwd>

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

		if (bufferStrategy == BufferStrategy::Unbuffered)
		{
			std::ifstream file(m_filePath, mode);
			if (file.is_open())
			{
				long long size = file_size(m_filePath);
				m_buffer.resize(size);
				file.read(m_buffer.data(), size);
				if (file.gcount() == size)
				{
					read = size;					
				}
				m_buffer.push_back(0);
				m_buffer.resize(read);
				file.close();
			}
		}
		else
		{
			std::ifstream file(m_filePath, mode);
			if (file.is_open())
			{
				char szBuffer[BUFFER_SIZE]{ 0 };				
				while (true)
				{
					file.read(szBuffer, sizeof(szBuffer));
					auto readin = file.gcount();
					if (readin > 0)
					{
						m_buffer.insert(m_buffer.end(), szBuffer, szBuffer + readin);
						read += readin;
					}					
					else
					{
						break;
					}
				}
				// put a '/0' AFTER the end of the buffer
				m_buffer.push_back(0);
				// then resize it back so the '/0' isn't included, but visualizing as a C string still works
				m_buffer.resize(read);
				file.close();
			}
		}

		return read;
	}

	//const char* FileReader::GetBytes()
	//{
	//	if (m_buffer.size() > 0)
	//	{
	//		return m_buffer.data();
	//	}
	//	return nullptr;		
	//}

	void FileReader::Clear()
	{
		m_buffer.clear();
	}

	const std::vector<char>& Utils::FileReader::GetBuffer() const
	{
		return m_buffer;
	}
}