#pragma once

#include "crow_win.h"

namespace Utils
{
	template <typename T>
	class CrowReturnable : public crow::returnable
	{
	public:
		CrowReturnable(const T& t, const std::string& contentType)
			: crow::returnable(contentType)
			, m_t(t)
		{}

	protected:
		const T& m_t;

		// Inherited via returnable
		std::string dump() const override;

		virtual std::string to_string() const = 0;

	};

	template<typename T>
	inline std::string CrowReturnable<T>::dump() const
	{
		return to_string();
	}
}
