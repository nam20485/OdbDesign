#pragma once

#include <string>
#include <vector>
#include <memory>
#include "../../odbdesign_export.h"


namespace Odb::Lib::FileModel::Design
{
	struct ODBDESIGN_EXPORT ContourPolygon
	{
		struct ODBDESIGN_EXPORT PolygonPart
		{
			enum class Type
			{
				Segment,
				Arc
			};

			Type type;

			// Segment/Arc
			float endX, endY;

			// Arc
			float xCenter, yCenter;
			bool isClockwise;

			typedef std::vector<std::shared_ptr<PolygonPart>> Vector;

			inline static const char* SEGMENT_RECORD_TOKEN = "OS";
			inline static const char* ARC_RECORD_TOKEN = "OC";

		}; // struct PolygonPart

		enum class Type
		{
			Island,
			Hole
		};

		~ContourPolygon()
		{
			m_polygonParts.clear();
		}

		Type type;
		float xStart, yStart;

		PolygonPart::Vector m_polygonParts;

		typedef std::vector<std::shared_ptr<ContourPolygon>> Vector;

		inline static const char* BEGIN_RECORD_TOKEN = "OB";
		inline static const char* END_RECORD_TOKEN = "OE";
		inline static const char* ISLAND_TYPE_TOKEN = "I";
		inline static const char* HOLE_TYPE_TOKEN = "H";

	}; // struct ContourPolygon
}