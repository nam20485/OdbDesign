#pragma once

#include <string>
#include <vector>
#include <memory>
#include "../../odbdesign_export.h"
#include "../../IProtoBuffable.h"
#include "../../ProtoBuf/common.pb.h"
#include "../../odbdesign_export.h"


namespace Odb::Lib::FileModel::Design
{
	struct ODBDESIGN_EXPORT ContourPolygon : public IProtoBuffable<Odb::Lib::Protobuf::ContourPolygon>
	{
		~ContourPolygon()
		{
			m_polygonParts.clear();
		}

		struct ODBDESIGN_EXPORT PolygonPart : public IProtoBuffable<Odb::Lib::Protobuf::ContourPolygon::PolygonPart>
		{
			enum class Type
			{
				Segment,
				Arc
			};

			Type type;

			// Segment/Arc
			double endX, endY;

			// Arc
			double xCenter, yCenter;
			bool isClockwise;

			// Inherited via IProtoBuffable
			std::unique_ptr<Odb::Lib::Protobuf::ContourPolygon::PolygonPart> to_protobuf() const override;
			void from_protobuf(const Odb::Lib::Protobuf::ContourPolygon::PolygonPart& message) override;

			typedef std::vector<std::shared_ptr<PolygonPart>> Vector;

			inline static const char* SEGMENT_RECORD_TOKEN = "OS";
			inline static const char* ARC_RECORD_TOKEN = "OC";

		}; // struct PolygonPart

		enum class Type
		{
			Island,
			Hole
		};		

		Type type;
		double xStart, yStart;

		PolygonPart::Vector m_polygonParts;

		// Inherited via IProtoBuffable
		std::unique_ptr<Odb::Lib::Protobuf::ContourPolygon> to_protobuf() const override;
		void from_protobuf(const Odb::Lib::Protobuf::ContourPolygon& message) override;

		typedef std::vector<std::shared_ptr<ContourPolygon>> Vector;

		inline static const char* BEGIN_RECORD_TOKEN = "OB";
		inline static const char* END_RECORD_TOKEN = "OE";
		inline static const char* ISLAND_TYPE_TOKEN = "I";
		inline static const char* HOLE_TYPE_TOKEN = "H";

	}; // struct ContourPolygon
}