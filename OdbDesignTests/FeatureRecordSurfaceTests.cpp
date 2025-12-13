#include <gtest/gtest.h>
#include <FileModel/Design/FeaturesFile.h>
#include <FileModel/Design/ContourPolygon.h>
#include <enums.h>

using namespace Odb::Lib::FileModel::Design;

TEST(FeatureRecordSurfaceTests, ToProtobufIncludesContourPolygons)
{
    auto feature = std::make_shared<FeaturesFile::FeatureRecord>();
    feature->type = FeaturesFile::FeatureRecord::Type::Surface;
    feature->xs = feature->ys = feature->xe = feature->ye = 0.0;
    feature->x = feature->y = 0.0;
    feature->apt_def_symbol_num = 0;
    feature->apt_def_resize_factor = 0.0;
    feature->xc = feature->yc = 0.0;
    feature->cw = false;
    feature->font = "";
    feature->xsize = feature->ysize = 0.0;
    feature->width_factor = 0.0;
    feature->text = "";
    feature->version = 0;
    feature->sym_num = 0;
    feature->polarity = Odb::Lib::Polarity::Positive;
    feature->dcode = 0;
    feature->id = 1;
    feature->orient_def = 0;
    feature->orient_def_rotation = 0.0;

    auto contour = std::make_shared<ContourPolygon>();
    contour->type = ContourPolygon::Type::Island;
    contour->xStart = 0.0;
    contour->yStart = 0.0;

    auto part = std::make_shared<ContourPolygon::PolygonPart>();
    part->type = ContourPolygon::PolygonPart::Type::Segment;
    part->endX = 1.0;
    part->endY = 0.0;
    part->xCenter = 0.0;
    part->yCenter = 0.0;
    part->isClockwise = false;

    contour->m_polygonParts.push_back(part);
    feature->m_contourPolygons.push_back(contour);

    auto message = feature->to_protobuf();
    ASSERT_TRUE(message);
    ASSERT_EQ(message->contourpolygons_size(), 1);

    const auto& poly = message->contourpolygons(0);
    EXPECT_EQ(poly.polygonparts_size(), 1);
    const auto& pbPart = poly.polygonparts(0);
    EXPECT_DOUBLE_EQ(pbPart.endx(), 1.0);
    EXPECT_DOUBLE_EQ(pbPart.endy(), 0.0);
    EXPECT_FALSE(pbPart.isclockwise());
}
