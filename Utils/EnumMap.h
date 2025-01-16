#pragma once

#include <vector>
#include <string>
#include <exception>
#include "str_utils.h"
#include <algorithm>
#include <iterator>

namespace Utils
{
	template<typename E>
	class EnumMap
	{
	public:
		EnumMap(const std::vector<std::string>& names, bool caseInsensitive = false);

		const std::string& getValue(const E e) const;
		E getValue(const std::string& name) const;

		bool contains(const std::string& name) const;
		bool contains(const E e) const;

		//typedef std::exception Exception;

	private:
		const std::vector<std::string> m_names;
		const bool m_bCaseInsensitive;		
		
	};

	template<typename E>
	inline EnumMap<E>::EnumMap(const std::vector<std::string>& names, bool caseInsensitive)
		: m_names(names)
		, m_bCaseInsensitive(caseInsensitive)
	{}

	template<typename E>
	inline const std::string& EnumMap<E>::getValue(const E e) const
	{
		auto index = static_cast<size_t>(e);
		if (index < 0 || index >= m_names.size())
		{
			std::string msg = "no name found for value: " + std::to_string(index);
			throw std::invalid_argument(msg.c_str());
		}
		return m_names[index];
	}

	template<typename E>
	inline E EnumMap<E>::getValue(const std::string& name) const
	{
		std::vector<std::string>::const_iterator findIt;
		if (m_bCaseInsensitive)
		{
			findIt = Utils::find_str_icmp(m_names.begin(), m_names.end(), name);
		}
		else
		{
			findIt = std::find(m_names.begin(), m_names.end(), name);
		}

		if (findIt == m_names.end())
		{
			std::string msg = "no value found for name: (" + name + ")";
			throw std::invalid_argument(msg.c_str());
		}
		return static_cast<E>(std::distance(m_names.begin(), findIt));
	}

	template<typename E>
	inline bool EnumMap<E>::contains(const std::string& name) const
	{
		return std::find(m_names.begin(), m_names.end(), name) != m_names.end();
	}

	template<typename E>
	inline bool EnumMap<E>::contains(const E e) const
	{
		auto index = static_cast<size_t>(e);
		return index >= 0 && index < m_names.size();
	}
}
