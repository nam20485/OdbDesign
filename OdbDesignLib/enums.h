#pragma once

namespace Odb::Lib
{
	enum class BoardSide
	{
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
}
