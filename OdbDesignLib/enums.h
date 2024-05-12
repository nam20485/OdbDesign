#pragma once

#include "EnumMap.h"

namespace Odb::Lib
{
	enum class BoardSide
	{
		BsNone,
		Top,
		Bottom		
	};

	enum class LineShape
	{
		Square,
		Round
	};

	enum class Polarity
	{
		Positive,
		Negative
	};

	enum class UnitType
	{
		None,
		Metric,
		Imperial
	};

	enum class DesignType
	{
		FileArchive,
		Design
	};

	static const Utils::EnumMap<DesignType> designTypeMap{
		{
			"FileArchive",
			"Design"
		}
	};
}
