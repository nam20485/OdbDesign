#pragma once

#include "IJsonable.h"
#include "CrowReturnable.h"

namespace Utils
{
	template<typename TJsonable>
	class JsonCrowReturnable : public Utils::CrowReturnable<TJsonable>
	{
	public:
		JsonCrowReturnable(const TJsonable& odbObject)
			: Utils::CrowReturnable<TJsonable>(odbObject, CONTENT_TYPE)
		{}

		inline static const std::string CONTENT_TYPE = "application/json";

	protected:
		std::string to_string() const override;

		// TJsonable MUST derive from IJsonConvertable (must use this until template type contraints support is added)
		static_assert(std::is_base_of<Utils::IJsonable, TJsonable>::value, "template parameter type TJsonable must derive from IJsonConvertable interface class");

	};

	template<typename TJsonable>
	inline std::string JsonCrowReturnable<TJsonable>::to_string() const
	{
		return this->m_t.to_json();
	}
}
