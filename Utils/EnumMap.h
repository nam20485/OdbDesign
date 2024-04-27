#pragma once

#include <vector>
#include <string>
#include <exception>

namespace Utils
{
	template<typename E>
	class EnumMap
	{
	public:
		EnumMap(const std::vector<std::string>& names)
			: m_names(names)
		{}

		const std::string& getValue(const E e) const
		{
			auto index = static_cast<size_t>(e);
			if (index < 0 || index >= m_names.size())
			{
				std::string msg = "no name found for value: " + std::to_string(index);
				throw Exception(msg.c_str());
			}
			return m_names[index];
		}

		E getValue(const std::string& name) const
		{
			auto findIt = std::find(m_names.begin(), m_names.end(), name);			
			if (findIt == m_names.end())
			{
				std::string msg = "no value found for name: (" + name + ")";
				throw Exception(msg.c_str());
			}
			return static_cast<E>(std::distance(m_names.begin(), findIt));
		}

		bool contains(const std::string& name) const
		{
			return std::find(m_names.begin(), m_names.end(), name) != m_names.end();
		}

		bool contains(const E e) const
		{
			auto index = static_cast<size_t>(e);
			return index >= 0 && index < m_names.size();
		}

		typedef std::exception Exception;

	private:
		const std::vector<std::string> m_names;

	};
}
