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

		// TJsonConvertable MUST derive from IJsonConvertable (must use this until template type contraints support is added)
		static_assert(std::is_base_of<IJsonConvertable, TJsonConvertable>::value, "template parameter type TJsonConvertable must derive from IJsonConvertable interface class");

	};

	template<typename TJsonConvertable>
	inline std::string JsonCrowReturnable<TJsonConvertable>::to_string() const
	{
		return this->m_t.to_json();
	}
}
