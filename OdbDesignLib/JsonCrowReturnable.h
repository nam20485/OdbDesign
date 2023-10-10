#pragma once

#include "CrowReturnable.h"

namespace Odb::Lib
{
	template<typename TJsonConvertable>
	class JsonCrowReturnable : public CrowReturnable<TJsonConvertable>
	{
	public:
		JsonCrowReturnable(const TJsonConvertable& odbObject)
			: CrowReturnable<TJsonConvertable>(odbObject, CONTENT_TYPE)
		{}

		inline static const std::string CONTENT_TYPE = "application/json";

	protected:
		std::string to_string() const override;

	};

	template<typename TJsonConvertable>
	inline std::string JsonCrowReturnable<TJsonConvertable>::to_string() const
	{
		return this->m_t.to_json();
	}
}
