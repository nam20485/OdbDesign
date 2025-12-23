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

TEST(FeatureRecordSurfaceTests, ToProtobufHandlesSurfaceWithNoContourPolygons)
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
    // No contour polygons added

    auto message = feature->to_protobuf();
    ASSERT_TRUE(message);
    EXPECT_EQ(message->contourpolygons_size(), 0);
}

TEST(FeatureRecordSurfaceTests, ToProtobufHandlesSurfaceWithMultipleContourPolygons)
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

    // Add first contour polygon (Island)
    auto contour1 = std::make_shared<ContourPolygon>();
    contour1->type = ContourPolygon::Type::Island;
    contour1->xStart = 0.0;
    contour1->yStart = 0.0;
    auto part1 = std::make_shared<ContourPolygon::PolygonPart>();
    part1->type = ContourPolygon::PolygonPart::Type::Segment;
    part1->endX = 1.0;
    part1->endY = 0.0;
    contour1->m_polygonParts.push_back(part1);
    feature->m_contourPolygons.push_back(contour1);

    // Add second contour polygon (Hole)
    auto contour2 = std::make_shared<ContourPolygon>();
    contour2->type = ContourPolygon::Type::Hole;
    contour2->xStart = 2.0;
    contour2->yStart = 2.0;
    auto part2 = std::make_shared<ContourPolygon::PolygonPart>();
    part2->type = ContourPolygon::PolygonPart::Type::Segment;
    part2->endX = 3.0;
    part2->endY = 2.0;
    contour2->m_polygonParts.push_back(part2);
    feature->m_contourPolygons.push_back(contour2);

    auto message = feature->to_protobuf();
    ASSERT_TRUE(message);
    EXPECT_EQ(message->contourpolygons_size(), 2);
    
    // Verify first polygon (Island)
    const auto& poly1 = message->contourpolygons(0);
    EXPECT_EQ(poly1.type(), Odb::Lib::Protobuf::ContourPolygon::Island);
    EXPECT_DOUBLE_EQ(poly1.xstart(), 0.0);
    EXPECT_DOUBLE_EQ(poly1.ystart(), 0.0);
    
    // Verify second polygon (Hole)
    const auto& poly2 = message->contourpolygons(1);
    EXPECT_EQ(poly2.type(), Odb::Lib::Protobuf::ContourPolygon::Hole);
    EXPECT_DOUBLE_EQ(poly2.xstart(), 2.0);
    EXPECT_DOUBLE_EQ(poly2.ystart(), 2.0);
}

TEST(FeatureRecordSurfaceTests, ToProtobufHandlesDifferentPolygonTypes)
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

    // Add Island polygon
    auto island = std::make_shared<ContourPolygon>();
    island->type = ContourPolygon::Type::Island;
    island->xStart = 0.0;
    island->yStart = 0.0;
    auto islandPart = std::make_shared<ContourPolygon::PolygonPart>();
    islandPart->type = ContourPolygon::PolygonPart::Type::Segment;
    islandPart->endX = 1.0;
    islandPart->endY = 0.0;
    island->m_polygonParts.push_back(islandPart);
    feature->m_contourPolygons.push_back(island);

    // Add Hole polygon
    auto hole = std::make_shared<ContourPolygon>();
    hole->type = ContourPolygon::Type::Hole;
    hole->xStart = 5.0;
    hole->yStart = 5.0;
    auto holePart = std::make_shared<ContourPolygon::PolygonPart>();
    holePart->type = ContourPolygon::PolygonPart::Type::Segment;
    holePart->endX = 6.0;
    holePart->endY = 5.0;
    hole->m_polygonParts.push_back(holePart);
    feature->m_contourPolygons.push_back(hole);

    auto message = feature->to_protobuf();
    ASSERT_TRUE(message);
    ASSERT_EQ(message->contourpolygons_size(), 2);
    
    // Verify Island type
    EXPECT_EQ(message->contourpolygons(0).type(), Odb::Lib::Protobuf::ContourPolygon::Island);
    
    // Verify Hole type
    EXPECT_EQ(message->contourpolygons(1).type(), Odb::Lib::Protobuf::ContourPolygon::Hole);
}
