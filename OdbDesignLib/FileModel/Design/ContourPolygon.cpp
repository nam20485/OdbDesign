#include "ContourPolygon.h"

namespace Odb::Lib::FileModel::Design
{
	std::unique_ptr<Odb::Lib::Protobuf::ContourPolygon> ContourPolygon::to_protobuf() const
	{
		std::unique_ptr<Odb::Lib::Protobuf::ContourPolygon> pContourPolygonMessage(new Odb::Lib::Protobuf::ContourPolygon);
		pContourPolygonMessage->set_type((Odb::Lib::Protobuf::ContourPolygon::Type) type);
		pContourPolygonMessage->set_xstart(xStart);
		pContourPolygonMessage->set_ystart(yStart);
		for (const auto& pPolygonPart : m_polygonParts)
		{
			pContourPolygonMessage->add_polygonparts()->CopyFrom(*pPolygonPart->to_protobuf());
		}
		return pContourPolygonMessage;
	}

	void ContourPolygon::from_protobuf(const Odb::Lib::Protobuf::ContourPolygon& message)
	{
		type = (Type) message.type();
		xStart = message.xstart();
		yStart = message.ystart();
		for (const auto& polygonPartMessage : message.polygonparts())
		{
			std::shared_ptr<PolygonPart> pPolygonPart(new PolygonPart);
			pPolygonPart->from_protobuf(polygonPartMessage);
			m_polygonParts.push_back(pPolygonPart);
		}
	}

	// Inherited via IProtoBuffable
	std::unique_ptr<Odb::Lib::Protobuf::ContourPolygon::PolygonPart> ContourPolygon::PolygonPart::to_protobuf() const
	{
		std::unique_ptr<Odb::Lib::Protobuf::ContourPolygon::PolygonPart> pPolygonPartMessage(new Odb::Lib::Protobuf::ContourPolygon::PolygonPart);
		pPolygonPartMessage->set_endx(endX);
		pPolygonPartMessage->set_endy(endY);
		pPolygonPartMessage->set_xcenter(xCenter);
		pPolygonPartMessage->set_ycenter(yCenter);
		pPolygonPartMessage->set_isclockwise(isClockwise);
		return pPolygonPartMessage;
	}

	void ContourPolygon::PolygonPart::from_protobuf(const Odb::Lib::Protobuf::ContourPolygon::PolygonPart& message)
	{
		endX = message.endx();
		endY = message.endy();
		xCenter = message.xcenter();
		yCenter = message.ycenter();
		isClockwise = message.isclockwise();
	}
}