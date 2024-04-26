#pragma once

#include <vector>
#include <string>
#include <exception>
#include <cctype>

//bool case_insensitive_compare(const std::string& s1, const std::string& s2)
//{
//	return std::equal(
//		s1.begin(), s1.end(),
//		s2.begin(), s2.end(),
//		[](char c1, char c2)
//		{
//			return std::tolower(c1) == std::tolower(c2);
//		});
//}

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
		//auto find = std::find_if(m_names.begin(), m_names.end(), name, case_insensitive_compare);
		auto findIt = std::find(m_names.begin(), m_names.end(), name);
		if (findIt == m_names.end())
		{
			throw Exception("no value found for name");
		}
		return static_cast<E>(std::distance(m_names.begin(), findIt));
	}

	bool contains(const std::string& name) const
	{
		//return std::find_if(m_names.begin(), m_names.end(), name, case_insensitive_compare) != m_names.end();
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
