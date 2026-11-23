#include <gtest/gtest.h>
#include <memory>
#include <string>

#include "../OdbDesignLib/ProductModel/Via.h"
#include "../OdbDesignLib/enums.h"

using Odb::Lib::ProductModel::Via;
using ProtobufVia = Odb::Lib::Protobuf::ProductModel::Via;

namespace Odb::Test
{
    namespace
    {
        ProtobufVia makeViaMessage(const std::string& name, Odb::Lib::Protobuf::BoardSide side)
        {
            ProtobufVia message;
            message.set_name(name);
            message.set_boardside(side);
            return message;
        }
    }

    // the native <-> proto BoardSide mapping is a cast; the two enums must
    // stay value-aligned or every via changes sides across the boundary
    static_assert(static_cast<int>(Odb::Lib::BoardSide::BsNone) ==
                  static_cast<int>(Odb::Lib::Protobuf::BoardSide::BsNone));
    static_assert(static_cast<int>(Odb::Lib::BoardSide::Top) ==
                  static_cast<int>(Odb::Lib::Protobuf::BoardSide::Top));
    static_assert(static_cast<int>(Odb::Lib::BoardSide::Bottom) ==
                  static_cast<int>(Odb::Lib::Protobuf::BoardSide::Bottom));

    TEST(ViaTests, Defaults_NameEmpty_SideTop)
    {
        Via via;
        EXPECT_EQ(via.GetName(), "");
        EXPECT_EQ(via.GetSide(), Odb::Lib::BoardSide::Top);
    }

    TEST(ViaTests, FromProtobuf_PopulatesNameAndSide)
    {
        auto message = makeViaMessage("via1", Odb::Lib::Protobuf::BoardSide::Bottom);
        Via via;
        via.from_protobuf(message);
        EXPECT_EQ(via.GetName(), "via1");
        EXPECT_EQ(via.GetSide(), Odb::Lib::BoardSide::Bottom);
    }

    TEST(ViaTests, ToProtobuf_ReflectsPopulatedFields)
    {
        auto message = makeViaMessage("v2", Odb::Lib::Protobuf::BoardSide::Bottom);
        Via via;
        via.from_protobuf(message);

        auto pOut = via.to_protobuf();
        ASSERT_NE(pOut, nullptr);
        EXPECT_EQ(pOut->name(), "v2");
        EXPECT_EQ(pOut->boardside(), Odb::Lib::Protobuf::BoardSide::Bottom);
    }

    TEST(ViaTests, ProtobufWireRoundTrip_PreservesFields)
    {
        // full wire round trip: message -> Via -> serialize -> parse -> Via
        auto message = makeViaMessage("round_trip_via", Odb::Lib::Protobuf::BoardSide::Top);
        Via original;
        original.from_protobuf(message);

        auto pSerialized = original.to_protobuf();
        ASSERT_NE(pSerialized, nullptr);
        const std::string bytes = pSerialized->SerializeAsString();
        ASSERT_FALSE(bytes.empty());

        ProtobufVia reparsed;
        ASSERT_TRUE(reparsed.ParseFromString(bytes));

        Via restored;
        restored.from_protobuf(reparsed);
        EXPECT_EQ(restored.GetName(), original.GetName());
        EXPECT_EQ(restored.GetSide(), original.GetSide());
    }

    TEST(ViaTests, ToProtobuf_DefaultInstance_SerializesEmptyNameTopSide)
    {
        Via via;
        auto pOut = via.to_protobuf();
        ASSERT_NE(pOut, nullptr);
        EXPECT_EQ(pOut->name(), "");
        EXPECT_EQ(pOut->boardside(), Odb::Lib::Protobuf::BoardSide::Top);
    }
} // namespace Odb::Test
