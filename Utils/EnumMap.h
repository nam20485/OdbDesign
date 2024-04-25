#pragma once

#include <vector>
#include <string>
#include <exception>

template<typename E>
class EnumMap
{
public:
	EnumMap(const std::vector<std::string>& names)
		: m_names(names)
	{}

	std::string getValue(const E e) const
	{		
		auto index = static_cast<size_t>(e);
		if (index < 0 || index >= m_names.size())
		{
			throw Exception("no name found for value");
		}
		return m_names[index];
	}

	E getValue(const std::string& name) const
	{
		auto find = std::find(m_names.begin(), m_names.end(), name);
		if (find == m_names.end())
		{
			throw Exception("no value found for name");
		}
		return static_cast<E>(std::distance(m_names.begin(), find));
	}

	typedef std::exception Exception;

private:
	const std::vector<std::string> m_names;

};
