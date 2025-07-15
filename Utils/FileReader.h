#pragma once

#include "utils_export.h"
#include <filesystem>
#include <vector>

namespace Utils
{
	class UTILS_EXPORT FileReader
	{
	public:
		enum class BufferStrategy
		{
			Unbuffered,
			Buffered
		};

		FileReader(const std::filesystem::path& filePath);		

		long long Read(BufferStrategy bufferStrategy = BufferStrategy::Buffered, std::ios::openmode mode = DEFAULT_OPENMODE);		
		void Clear();
		const std::vector<char>& GetBuffer() const;

	private:
		const std::filesystem::path m_filePath;	
		std::vector<char> m_buffer;

		static inline constexpr std::ios::openmode DEFAULT_OPENMODE = std::ios::in|std::ios::binary;
		static inline constexpr size_t BUFFER_SIZE = 1024;

	};
}
