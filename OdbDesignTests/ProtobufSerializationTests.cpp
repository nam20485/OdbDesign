#include <gtest/gtest.h>
#include "Fixtures/FileArchiveLoadFixture.h"
#include <gmock/gmock-matchers.h>
#include "FileModel/Design/FileArchive.h"
#include "FileModel/Design/StepHdrFile.h"
#include <fstream>
#include <memory>
#include <string>
#include "ProductModel/Component.h"
#include "ProductModel/Design.h"


//using namespace Odb::Lib::App;
using namespace Odb::Lib::FileModel;
using namespace Odb::Test::Fixtures;
using namespace std::filesystem;
using namespace testing;
using namespace Odb::Lib::ProductModel;

namespace Odb::Test
{
	TEST_F(FileArchiveLoadFixture, Component_ProtoBuf_RoundTrip_sample_design_Succeeds)
	{		
		auto pDesign = m_pDesignCache->GetDesign("sample_design");
		ASSERT_THAT(pDesign, NotNull());

		auto pComponent = pDesign->GetComponent("U19");
		ASSERT_THAT(pComponent, NotNull());

		auto pComponentMsg = pComponent->to_protobuf();
		ASSERT_THAT(pComponentMsg, NotNull());

		auto pComponentFromMsg = std::make_unique<Component>();
		pComponentFromMsg->from_protobuf(*pComponentMsg);	

		ASSERT_EQ(pComponent->GetRefDes(), pComponentFromMsg->GetRefDes());
		ASSERT_EQ(pComponent->GetIndex(), pComponentFromMsg->GetIndex());
		ASSERT_EQ(pComponent->GetSide(), pComponentFromMsg->GetSide());
		ASSERT_EQ(pComponent->GetPackage()->GetName(), pComponentFromMsg->GetPackage()->GetName());
		ASSERT_EQ(pComponent->GetPart()->GetName(), pComponentFromMsg->GetPart()->GetName());
	}
	
	TEST_F(FileArchiveLoadFixture, Design_ProtoBuf_RoundTrip_sample_design_Succeeds)
	{
		auto pDesign = m_pDesignCache->GetDesign("sample_design");
		ASSERT_THAT(pDesign, NotNull());

		auto pDesignMsg = pDesign->to_protobuf();
		ASSERT_THAT(pDesignMsg, NotNull());
		
		auto pDesignFromMsg = std::make_unique<Odb::Lib::ProductModel::Design>();
		pDesignFromMsg->from_protobuf(*pDesignMsg);

		ASSERT_EQ(pDesign->GetName(), pDesignFromMsg->GetName());
		ASSERT_EQ(pDesign->GetProductModel(), pDesignFromMsg->GetProductModel());
		ASSERT_EQ(pDesign->GetComponents().size(), pDesignFromMsg->GetComponents().size());
		//ASSERT_THAT(pDesign->GetComponents(), ContainerEq(pDesignFromMsg->GetComponents()));
	}

	TEST_F(FileArchiveLoadFixture, FileArchive_ProtoBuf_RoundTrip_sample_design_Succeeds)
	{
		auto pFileArchive = m_pDesignCache->GetFileArchive("sample_design");
		ASSERT_THAT(pFileArchive, NotNull());

		auto pFileArchiveMsg = pFileArchive->to_protobuf();
		ASSERT_THAT(pFileArchiveMsg, NotNull());

		auto pFileArchiveFromMsg = std::make_unique<Odb::Lib::FileModel::Design::FileArchive>("");
		pFileArchiveFromMsg->from_protobuf(*pFileArchiveMsg);

		ASSERT_EQ(pFileArchive->GetProductName(), pFileArchiveFromMsg->GetProductName());
		//ASSERT_EQ(pFileArchive->GetFilename(), pFileArchiveFromMsg->GetFilename());
		ASSERT_EQ(pFileArchive->GetSymbolsDirectoriesByName().size(), pFileArchiveFromMsg->GetSymbolsDirectoriesByName().size());
		//ASSERT_THAT(pDesign->GetComponents(), ContainerEq(pDesignFromMsg->GetComponents()));
	}

	TEST_F(FileArchiveLoadFixture, Component_ProtoBuf_RoundTrip_designodb_rigidflex_Succeeds)
	{
		auto pDesign = m_pDesignCache->GetDesign("designodb_rigidflex");
		ASSERT_THAT(pDesign, NotNull());

		auto pComponent = pDesign->GetComponent("R56");
		ASSERT_THAT(pComponent, NotNull());

		auto pComponentMsg = pComponent->to_protobuf();
		ASSERT_THAT(pComponentMsg, NotNull());

		auto pComponentFromMsg = std::make_unique<Component>();
		pComponentFromMsg->from_protobuf(*pComponentMsg);

		ASSERT_EQ(pComponent->GetRefDes(), pComponentFromMsg->GetRefDes());
		ASSERT_EQ(pComponent->GetIndex(), pComponentFromMsg->GetIndex());
		ASSERT_EQ(pComponent->GetSide(), pComponentFromMsg->GetSide());
		ASSERT_EQ(pComponent->GetPackage()->GetName(), pComponentFromMsg->GetPackage()->GetName());
		ASSERT_EQ(pComponent->GetPart()->GetName(), pComponentFromMsg->GetPart()->GetName());
	}

	TEST_F(FileArchiveLoadFixture, Design_ProtoBuf_RoundTrip_designodb_rigidflex_Succeeds)
	{
		auto pDesign = m_pDesignCache->GetDesign("designodb_rigidflex");
		ASSERT_THAT(pDesign, NotNull());

		auto pDesignMsg = pDesign->to_protobuf();
		ASSERT_THAT(pDesignMsg, NotNull());

		auto pDesignFromMsg = std::make_unique<Odb::Lib::ProductModel::Design>();
		pDesignFromMsg->from_protobuf(*pDesignMsg);

		ASSERT_EQ(pDesign->GetName(), pDesignFromMsg->GetName());
		ASSERT_EQ(pDesign->GetProductModel(), pDesignFromMsg->GetProductModel());
		ASSERT_EQ(pDesign->GetComponents().size(), pDesignFromMsg->GetComponents().size());
		//ASSERT_THAT(pDesign->GetComponents(), ContainerEq(pDesignFromMsg->GetComponents()));
	}

	TEST_F(FileArchiveLoadFixture, FileArchive_ProtoBuf_RoundTrip_designodb_rigidflex_Succeeds)
	{
		auto pFileArchive = m_pDesignCache->GetFileArchive("designodb_rigidflex");
		ASSERT_THAT(pFileArchive, NotNull());

		auto pFileArchiveMsg = pFileArchive->to_protobuf();
		ASSERT_THAT(pFileArchiveMsg, NotNull());

		auto pFileArchiveFromMsg = std::make_unique<Odb::Lib::FileModel::Design::FileArchive>("");
		pFileArchiveFromMsg->from_protobuf(*pFileArchiveMsg);

		ASSERT_EQ(pFileArchive->GetProductName(), pFileArchiveFromMsg->GetProductName());
		//ASSERT_EQ(pFileArchive->GetFilename(), pFileArchiveFromMsg->GetFilename());
		ASSERT_EQ(pFileArchive->GetSymbolsDirectoriesByName().size(), pFileArchiveFromMsg->GetSymbolsDirectoriesByName().size());
		//ASSERT_THAT(pDesign->GetComponents(), ContainerEq(pDesignFromMsg->GetComponents()));
	}	

	namespace
	{
		// Writes a minimal steps/<step>/stephdr that omits every optional
		// attribute the Turbot design omits (notably AFFECTING_BOM and
		// AFFECTING_BOM_CHANGED) and returns the step directory path.
		path WriteStepHdrWithoutOptionalAttributes()
		{
			std::error_code ec;
			const auto base = temp_directory_path() / "OdbDesignTests" / "stephdr-no-optional-attrs";
			remove_all(base, ec);
			create_directories(base, ec);
			EXPECT_FALSE(ec) << "create_directories failed: " << ec.message();

			std::ofstream out(base / "stephdr", std::ios::out | std::ios::trunc);
			out << "UNITS=INCH\n"
				<< "X_DATUM=0\n"
				<< "Y_DATUM=0\n"
				<< "X_ORIGIN=0\n"
				<< "Y_ORIGIN=0\n"
				<< "TOP_ACTIVE=0\n"
				<< "BOTTOM_ACTIVE=0\n"
				<< "RIGHT_ACTIVE=0\n"
				<< "LEFT_ACTIVE=0\n"
				<< "ID=37\n"
				<< "ONLINE_NET_STAT=GREEN\n";
			out.close();
			EXPECT_TRUE(out.good());

			return base;
		}
	}

	// Regression (Turbot GetDesign corrupt protobuf): every stephdr attribute is
	// optional per the ODB++ spec, so any POD member may go unset at parse time.
	// to_protobuf() then serialized an uninitialized bool, whose raw non-0/1
	// byte reached the wire as a multi-byte varint and desynced the whole
	// serialized stream. The in-class member initializers are the fix; this
	// guards the observable behavior (wire round trip) for a stephdr that omits
	// the optional attributes. Runs meaningfully in both debug and release
	// presets (release is where the raw byte hit the wire).
	TEST(StepHdrFileTests, WireRoundTrip_MissingOptionalAttributes_SerializesValidProtobuf)
	{
		const auto stepDirectory = WriteStepHdrWithoutOptionalAttributes();

		Odb::Lib::FileModel::Design::StepHdrFile stepHdrFile;
		ASSERT_TRUE(stepHdrFile.Parse(stepDirectory));

		auto pMessage = stepHdrFile.to_protobuf();
		ASSERT_THAT(pMessage, NotNull());

		// Unset optional attributes must fall back to their defaults, not leak
		// whatever the heap happened to hold.
		EXPECT_FALSE(pMessage->affectingbomchanged());
		EXPECT_EQ(pMessage->id(), 37u);
		EXPECT_EQ(pMessage->xdatum(), 0.0);

		std::string wire;
		ASSERT_TRUE(pMessage->SerializeToString(&wire));
		ASSERT_FALSE(wire.empty());

		Odb::Lib::Protobuf::StepHdrFile reparsed;
		const auto parsedOk = reparsed.ParseFromString(wire);
		EXPECT_TRUE(parsedOk) << "stephdr wire bytes must re-parse (invalid varint => uninitialized member leaked onto the wire)";
		ASSERT_TRUE(parsedOk);
		EXPECT_EQ(reparsed.id(), 37u);
		EXPECT_FALSE(reparsed.affectingbomchanged());
		EXPECT_EQ(reparsed.onlinevalues().at("ONLINE_NET_STAT"), "GREEN");
	}

	// The other round-trip tests in this file stop at from_protobuf(); the wire
	// format itself (tags, lengths, varints) is only exercised by serializing
	// and re-parsing the bytes.
	TEST_F(FileArchiveLoadFixture, FileArchive_WireRoundTrip_sample_design_Succeeds)
	{
		auto pFileArchive = m_pDesignCache->GetFileArchive("sample_design");
		ASSERT_THAT(pFileArchive, NotNull());

		auto pFileArchiveMsg = pFileArchive->to_protobuf();
		ASSERT_THAT(pFileArchiveMsg, NotNull());

		std::string wire;
		ASSERT_TRUE(pFileArchiveMsg->SerializeToString(&wire));
		ASSERT_FALSE(wire.empty());

		Odb::Lib::Protobuf::FileArchive reparsed;
		EXPECT_TRUE(reparsed.ParseFromString(wire));
		EXPECT_EQ(reparsed.ByteSizeLong(), pFileArchiveMsg->ByteSizeLong());
	}
}