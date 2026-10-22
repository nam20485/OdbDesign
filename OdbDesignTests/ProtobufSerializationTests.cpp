#include <gtest/gtest.h>
#include "Fixtures/FileArchiveLoadFixture.h"
#include <gmock/gmock-matchers.h>
#include "FileModel/Design/FileArchive.h"
#include "FileModel/Design/StepHdrFile.h"
#include "FileModel/Design/ComponentsFile.h"
#include "FileModel/Design/FeaturesFile.h"
#include "FileModel/Design/EdaDataFile.h"
#include <fstream>
#include <memory>
#include <cstdio>
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

	TEST_F(FileArchiveLoadFixture, Design_Connectivity_AlwaysOnAndRoundTrip_sample_design_Succeeds)
	{
		// M2.1/D4: connectivity rides EVERY flavour, never gated on
		// include_normalized_lists — the pruned default must carry it while the
		// normalized collections stay pruned.
		auto pDesign = m_pDesignCache->GetDesign("sample_design");
		ASSERT_THAT(pDesign, NotNull());

		const auto prunedMsg = pDesign->to_protobuf(/*includeNormalizedLists=*/false);
		ASSERT_THAT(prunedMsg, NotNull());
		ASSERT_TRUE(prunedMsg->has_connectivity()) << "pruned flavour must carry connectivity (D4 always-on)";
		EXPECT_GT(prunedMsg->connectivity().components_size(), 0);
		EXPECT_GT(prunedMsg->connectivity().nets_size(), 0);
		EXPECT_EQ(prunedMsg->nets().size(), 0);
		EXPECT_EQ(prunedMsg->components().size(), 0);

		// Round-trip: the full flavour restores m_nets/m_components/m_packages
		// (including the DP1-threaded UID), so a re-derive must reproduce the
		// same connectivity. Subnet ordinals are the documented exception
		// (DP2: in-memory only, not serialized in the normalized lists).
		const auto fullMsg = pDesign->to_protobuf();
		ASSERT_THAT(fullMsg, NotNull());
		ASSERT_TRUE(fullMsg->has_connectivity());

		auto pRoundTripped = std::make_unique<Odb::Lib::ProductModel::Design>();
		pRoundTripped->from_protobuf(*fullMsg);
		const auto rederivedMsg = pRoundTripped->to_protobuf();
		ASSERT_THAT(rederivedMsg, NotNull());
		ASSERT_TRUE(rederivedMsg->has_connectivity());

		const auto& original = fullMsg->connectivity();
		const auto& rederived = rederivedMsg->connectivity();
		ASSERT_EQ(rederived.components_size(), original.components_size());
		for (auto i = 0; i < original.components_size(); ++i)
		{
			const auto& o = original.components(i);
			const auto& r = rederived.components(i);
			EXPECT_EQ(r.refdes(), o.refdes());
			EXPECT_EQ(r.has_id(), o.has_id()) << o.refdes() << ": UID presence must survive the round-trip (DP1)";
			if (o.has_id()) EXPECT_EQ(r.id(), o.id());
			EXPECT_EQ(r.side(), o.side());
			EXPECT_EQ(r.package(), o.package());
			ASSERT_EQ(r.pins_size(), o.pins_size());
			for (auto p = 0; p < o.pins_size(); ++p)
			{
				EXPECT_EQ(r.pins(p).number(), o.pins(p).number()) << o.refdes() << " pin " << p;
				EXPECT_EQ(r.pins(p).name(), o.pins(p).name()) << o.refdes() << " pin " << p;
				EXPECT_EQ(r.pins(p).connection().kind(), o.pins(p).connection().kind()) << o.refdes() << " pin " << p;
				EXPECT_EQ(r.pins(p).connection().net(), o.pins(p).connection().net()) << o.refdes() << " pin " << p;
			}
		}
		EXPECT_EQ(rederived.nets_size(), original.nets_size());
		EXPECT_EQ(rederived.netmembers_size(), original.netmembers_size());
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
		// protobuf's map::at() CHECK-fails and abort()s the whole process when the key
		// is absent, so a regression here reports as "Subprocess aborted" with no gtest
		// diagnostic -- it cost four CI runs and a log hunt to name the missing key.
		// Look the entry up explicitly so the failure names it and lists what did arrive.
		const auto& reparsedOnlineValues = reparsed.onlinevalues();
		const auto onlineIt = reparsedOnlineValues.find("ONLINE_NET_STAT");
		// Sighting 2026-09-27 (intermittent, ~1 run in 3, not reproducible on demand):
		// this assertion fired while the map held exactly one entry whose key dumped
		// byte-identical to "ONLINE_NET_STAT" (len=15) with value "GREEN" -- i.e. the
		// entry was present under iteration and absent under find(), same object, same
		// thread, microseconds apart. Ruled out at the time: ASan and UBSan both silent
		// over a 40-attempt reproduction (so not heap corruption or detected UB), and
		// exactly one protobuf runtime in the process (single libprotobuf.so.33.4.0 for
		// both the test binary and libOdbDesign.so, no strong protobuf symbols of its
		// own), so not the dual-loaded-copies failure mode; and stale generated code --
		// a deep clean rebuild (--clean-first, stephdrfile.pb.{h,cc} regenerated, mtimes
		// verified) still failed 17 of 40 runs. Failure rate is ~40%, not rare.
		// Unexplained. The byte dump below stays so the next occurrence records the key
		// rather than just a count. Tracked in #599, which records the reproduction,
		// everything ruled out and how, and what has not been tried yet.
		ASSERT_TRUE(onlineIt != reparsedOnlineValues.end())
				<< "ONLINE_NET_STAT did not survive the wire round trip; onlineValues holds "
				<< reparsedOnlineValues.size() << " entry/entries: "
				<< [&reparsedOnlineValues] {
					std::string dump;
					for (const auto& kv : reparsedOnlineValues)
					{
						dump += "[len=" + std::to_string(kv.first.size()) + " bytes=";
						for (unsigned char c : kv.first)
						{
							char hex[4];
							snprintf(hex, sizeof(hex), "%02x ", c);
							dump += hex;
						}
						dump += "]=[" + kv.second + "] ";
					}
					return dump;
				}();
		EXPECT_EQ(onlineIt->second, "GREEN");
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

		        // The ODB++ record unique id is optional: "no id present" must survive the
		        // wire distinctly from id 0 (plan D2), on every record type that carries one.
		        TEST(ProtobufSerializationTests, ComponentRecord_IdRoundTrip_PresenceSurvivesWire)
		        {
		                Odb::Lib::FileModel::Design::ComponentsFile::ComponentRecord withId;
		                withId.compName = "U1";
		                withId.id = 1234;
		                auto pMsg = withId.to_protobuf();
		                ASSERT_THAT(pMsg, NotNull());
		                ASSERT_TRUE(pMsg->has_id());
		                EXPECT_EQ(pMsg->id(), 1234u);

		                Odb::Lib::FileModel::Design::ComponentsFile::ComponentRecord roundTripped;
		                roundTripped.from_protobuf(*pMsg);
		                ASSERT_TRUE(roundTripped.id.has_value());
		                EXPECT_EQ(*roundTripped.id, 1234u);

		                Odb::Lib::FileModel::Design::ComponentsFile::ComponentRecord withoutId;
		                withoutId.compName = "U2";
		                auto pMsg2 = withoutId.to_protobuf();
		                ASSERT_THAT(pMsg2, NotNull());
		                EXPECT_FALSE(pMsg2->has_id());

		                Odb::Lib::FileModel::Design::ComponentsFile::ComponentRecord roundTripped2;
		                roundTripped2.from_protobuf(*pMsg2);
		                EXPECT_FALSE(roundTripped2.id.has_value());

		                // explicit zero is present, not absent (D2)
		                Odb::Lib::FileModel::Design::ComponentsFile::ComponentRecord zeroId;
		                zeroId.compName = "U3";
		                zeroId.id = 0u;
		                auto pMsg3 = zeroId.to_protobuf();
		                ASSERT_THAT(pMsg3, NotNull());
		                ASSERT_TRUE(pMsg3->has_id());
		                EXPECT_EQ(pMsg3->id(), 0u);

		                Odb::Lib::FileModel::Design::ComponentsFile::ComponentRecord roundTripped3;
		                roundTripped3.from_protobuf(*pMsg3);
		                ASSERT_TRUE(roundTripped3.id.has_value());
		                EXPECT_EQ(*roundTripped3.id, 0u);
		        }

		        TEST(ProtobufSerializationTests, FeatureNetPackageRecord_IdRoundTrip_PresenceSurvivesWire)
		        {
		                Odb::Lib::FileModel::Design::FeaturesFile::FeatureRecord feature;
		                feature.id = 2718;
		                auto pFeatureMsg = feature.to_protobuf();
		                ASSERT_THAT(pFeatureMsg, NotNull());
		                ASSERT_TRUE(pFeatureMsg->has_id());
		                EXPECT_EQ(pFeatureMsg->id(), 2718u);
		                Odb::Lib::FileModel::Design::FeaturesFile::FeatureRecord featureRoundTripped;
		                featureRoundTripped.from_protobuf(*pFeatureMsg);
		                ASSERT_TRUE(featureRoundTripped.id.has_value());
		                EXPECT_EQ(*featureRoundTripped.id, 2718u);

		                Odb::Lib::FileModel::Design::EdaDataFile::NetRecord net;
		                net.id = 3141;
		                auto pNetMsg = net.to_protobuf();
		                ASSERT_THAT(pNetMsg, NotNull());
		                ASSERT_TRUE(pNetMsg->has_id());
		                EXPECT_EQ(pNetMsg->id(), 3141u);
		                Odb::Lib::FileModel::Design::EdaDataFile::NetRecord netRoundTripped;
		                netRoundTripped.from_protobuf(*pNetMsg);
		                ASSERT_TRUE(netRoundTripped.id.has_value());
		                EXPECT_EQ(*netRoundTripped.id, 3141u);

		                Odb::Lib::FileModel::Design::EdaDataFile::PackageRecord pkg;
		                pkg.id = 1618;
		                auto pPkgMsg = pkg.to_protobuf();
		                ASSERT_THAT(pPkgMsg, NotNull());
		                ASSERT_TRUE(pPkgMsg->has_id());
		                EXPECT_EQ(pPkgMsg->id(), 1618u);
		                Odb::Lib::FileModel::Design::EdaDataFile::PackageRecord pkgRoundTripped;
		                pkgRoundTripped.from_protobuf(*pPkgMsg);
		                ASSERT_TRUE(pkgRoundTripped.id.has_value());
		                EXPECT_EQ(*pkgRoundTripped.id, 1618u);

		                // explicit zero is present on the wire for the other three record
		                // types too: each has its own hand-written has_value() guard, and a
		                // guard that filtered 0 would pass every nonzero case above.
		                Odb::Lib::FileModel::Design::FeaturesFile::FeatureRecord zeroFeature;
		                zeroFeature.id = 0u;
		                auto pZeroFeatureMsg = zeroFeature.to_protobuf();
		                ASSERT_THAT(pZeroFeatureMsg, NotNull());
		                ASSERT_TRUE(pZeroFeatureMsg->has_id());
		                EXPECT_EQ(pZeroFeatureMsg->id(), 0u);
		                Odb::Lib::FileModel::Design::FeaturesFile::FeatureRecord zeroFeatureRoundTripped;
		                zeroFeatureRoundTripped.from_protobuf(*pZeroFeatureMsg);
		                ASSERT_TRUE(zeroFeatureRoundTripped.id.has_value());
		                EXPECT_EQ(*zeroFeatureRoundTripped.id, 0u);

		                Odb::Lib::FileModel::Design::EdaDataFile::NetRecord zeroNet;
		                zeroNet.id = 0u;
		                auto pZeroNetMsg = zeroNet.to_protobuf();
		                ASSERT_THAT(pZeroNetMsg, NotNull());
		                ASSERT_TRUE(pZeroNetMsg->has_id());
		                EXPECT_EQ(pZeroNetMsg->id(), 0u);
		                Odb::Lib::FileModel::Design::EdaDataFile::NetRecord zeroNetRoundTripped;
		                zeroNetRoundTripped.from_protobuf(*pZeroNetMsg);
		                ASSERT_TRUE(zeroNetRoundTripped.id.has_value());
		                EXPECT_EQ(*zeroNetRoundTripped.id, 0u);

		                Odb::Lib::FileModel::Design::EdaDataFile::PackageRecord zeroPkg;
		                zeroPkg.id = 0u;
		                auto pZeroPkgMsg = zeroPkg.to_protobuf();
		                ASSERT_THAT(pZeroPkgMsg, NotNull());
		                ASSERT_TRUE(pZeroPkgMsg->has_id());
		                EXPECT_EQ(pZeroPkgMsg->id(), 0u);
		                Odb::Lib::FileModel::Design::EdaDataFile::PackageRecord zeroPkgRoundTripped;
		                zeroPkgRoundTripped.from_protobuf(*pZeroPkgMsg);
		                ASSERT_TRUE(zeroPkgRoundTripped.id.has_value());
		                EXPECT_EQ(*zeroPkgRoundTripped.id, 0u);
		        }
		}