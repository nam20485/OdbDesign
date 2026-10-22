#include <gtest/gtest.h>
#include "Fixtures/FileArchiveLoadFixture.h"
#include "ProductModel/Design.h"
#include <google/protobuf/struct.pb.h>
#include <google/protobuf/util/json_util.h>
#include <filesystem>
#include <fstream>
#include <map>
#include <string>
#include <vector>

// =========================================================================
// Connectivity contract tests (M2.1/M2.2): the derived Connectivity view is
// differentially tested against the M0.3 golden fixtures —
// OdbDesignTests/Fixtures/Connectivity/<design>.golden.json, generated
// straight from the raw ODB++ sources by scripts/gen-connectivity-golden.py
// (see docs/plan/component-connectivity.md, docs/plan/component-connectivity-phase2-plan.md).
//
// Golden schema per design:
//   fidelity      "full" (every component) or "sampled" (deterministic subset)
//   nets          [{name, ordinal}] — the eda/data NET record roster
//   components    [{index, refDes, side ("Top"|"Bottom"), uid (number|null),
//                   pins: [{ordinal, pinNumber, name, netOrdinal (number|null),
//                           subnetOrdinal}]}]
//
// Comparator rules (the contract made executable):
//   - golden pin with netOrdinal null            -> UNCONNECTED, no net, no subnets
//   - golden pin on the $NONE$ net (DP4)          -> UNCONNECTED, no net, no subnets
//   - golden pin with any other netOrdinal        -> CONNECTED, net == netOrdinal,
//                                                    subnets == [subnetOrdinal]
//   - derived pin with no golden toeprint         -> UNCONNECTED (roster difference)
//   - golden uid present / null                   -> derived id set+equal / unset (D2)
//
// Only sample_design ships in TEST_DATA; the three legacy/panel designs live
// in the gitignored local corpora (designs/, new_designs/) and are SKIPPED
// when absent, so CI (which has no such corpora) runs the sample_design
// differential plus round-trip coverage.
// =========================================================================

using namespace Odb::Lib::ProductModel;
using namespace Odb::Test::Fixtures;

#ifndef ODBDESIGN_SOURCE_DIR
#error "ODBDESIGN_SOURCE_DIR must be defined by CMake (OdbDesignTests/CMakeLists.txt)"
#endif

namespace Odb::Test
{
	using ConnectivityMsg = Odb::Lib::Protobuf::ProductModel::Connectivity;

	namespace
	{
		using PBStruct = google::protobuf::Struct;
		using PBValue = google::protobuf::Value;

		// Struct.fields() lookups deliberately avoid Map::find/count/at: on this
		// toolchain (vcpkg protobuf 6.33, x64-linux-dynamic), map LOOKUP on a
		// Struct populated by JsonStringToMessage can return end() for keys
		// that iteration demonstrably contains (verified while writing this
		// test: find("nets") == end() while iteration yielded all 644 entries;
		// hand-built tiny maps look up fine). The linear scan below takes no
		// dependency on the lookup path, and these maps hold at most eight keys.
		const PBValue* field(const PBStruct& s, const std::string& key)
		{
			for (const auto& kv : s.fields())
			{
				if (kv.first == key)
				{
					return &kv.second;
				}
			}
			return nullptr;
		}

		bool isNull(const PBValue& v)
		{
			return v.kind_case() == PBValue::kNullValue;
		}

		unsigned int num(const PBValue& v)
		{
			return static_cast<unsigned int>(v.number_value());
		}

		const PBValue* at(const PBValue& list, int index)
		{
			const auto& values = list.list_value().values();
			return index < values.size() ? &values[index] : nullptr;
		}

		// Loads a design archive under the per-fixture scratch dir and returns
		// the fully built product model. An empty path means "archive not
		// present on this machine" and the caller skips.
		std::filesystem::path localArchiveFor(const std::string& goldenName, const std::filesystem::path& scratchDir)
		{
			static const std::map<std::string, std::string> localArchives = {
				{ "Panel-g7162-31800_odb", "new_designs/Panel - g7162-31800_odb.tgz" },
				{ "200-40628_Rev1_v7", "new_designs/200-40628_Rev1_v7.tgz" },
				{ "350-41017_rev1_odbjob_v7", "new_designs/350-41017_rev1_odbjob_v7.tgz" },
			};

			const auto findIt = localArchives.find(goldenName);
			if (findIt == localArchives.end()) return {};

			const auto source = std::filesystem::path(ODBDESIGN_SOURCE_DIR) / findIt->second;
			std::error_code ec;
			if (!std::filesystem::is_regular_file(source, ec)) return {};

			// Copy into the scratch dir so archive extraction never writes into
			// the (gitignored) source corpus or the shared TEST_DATA dir.
			const auto copy = scratchDir / source.filename();
			std::filesystem::copy_file(source, copy, std::filesystem::copy_options::overwrite_existing, ec);
			if (ec) return {};
			return copy;
		}

	}

	class ConnectivityContractTest : public FileArchiveLoadFixture
	{
	protected:
		void runDifferential(const std::string& goldenName, bool archiveFromTestData)
		{
			std::filesystem::path archive;
			if (archiveFromTestData)
			{
				archive = getIsolatedDesignPath("sample_design.tgz");
				ASSERT_FALSE(archive.empty()) << goldenName << ": archive path unresolved";
			}
			else
			{
				// An empty path here means the gitignored local corpus is not
				// present (CI) — skip, do not fail.
				archive = localArchiveFor(goldenName, m_scratchDir);
				if (archive.empty())
				{
					GTEST_SKIP() << goldenName << ": local-only design corpus not present on this machine";
				}
			}
			if (!std::filesystem::exists(archive))
			{
				GTEST_SKIP() << goldenName << ": design archive not present on this machine (local-only corpus)";
			}

			auto pDesign = std::make_shared<Design>();
			ASSERT_TRUE(pDesign->Build(archive.string())) << goldenName << ": Design::Build failed";

			const auto pDesignMsg = pDesign->to_protobuf(/*includeNormalizedLists=*/false);
			ASSERT_NE(pDesignMsg, nullptr);
			ASSERT_TRUE(pDesignMsg->has_connectivity()) << "default (pruned) flavour must carry connectivity (D4 always-on)";

			runAgainstGolden(goldenName, pDesignMsg->connectivity());
		}

		void runAgainstGolden(const std::string& goldenName, const ConnectivityMsg& connectivity)
		{
			const auto goldenPath = std::filesystem::path(ODBDESIGN_SOURCE_DIR)
				/ "OdbDesignTests" / "Fixtures" / "Connectivity" / (goldenName + ".golden.json");
			std::ifstream in(goldenPath);
			ASSERT_TRUE(in.good()) << "cannot open golden: " << goldenPath;

			const std::string json((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
			PBStruct golden;
			const auto jsonStatus = google::protobuf::util::JsonStringToMessage(json, &golden);
			ASSERT_TRUE(jsonStatus.ok()) << "golden JSON parse failed: " << jsonStatus.message();

			const auto* fidelity = field(golden, "fidelity");
			ASSERT_NE(fidelity, nullptr);
			const bool fullFidelity = fidelity->string_value() == "full";

			// Net roster: the golden nets ARE the eda-data NET sequence. In
			// "sampled" fidelity the roster is capped at 50 entries, so exact
			// roster equality is asserted for "full" only.
			const auto* goldenNets = field(golden, "nets");
			ASSERT_NE(goldenNets, nullptr);

			unsigned int noneNetOrdinal = 0;
			auto hasNoneNet = false;
			const auto goldenNetCount = static_cast<int>(goldenNets->list_value().values().size());
			ASSERT_LE(goldenNetCount, connectivity.nets().size());
			for (auto i = 0; i < goldenNetCount; ++i)
			{
				const auto* entry = at(*goldenNets, i);
				ASSERT_NE(entry, nullptr);
				const auto* name = field(entry->struct_value(), "name");
				const auto* ordinal = field(entry->struct_value(), "ordinal");
				ASSERT_NE(name, nullptr);
				ASSERT_NE(ordinal, nullptr);

				ASSERT_LT(i, connectivity.nets().size());
				const auto& netEntry = connectivity.nets(i);
				if (fullFidelity)
				{
					EXPECT_EQ(netEntry.ordinal(), num(*ordinal));
					EXPECT_EQ(netEntry.name(), name->string_value());
				}
				if (name->string_value() == "$NONE$")
				{
					if (!hasNoneNet)
					{
						noneNetOrdinal = num(*ordinal);
						hasNoneNet = true;
					}
					// DP4: the $NONE$ entry stays in the emitted roster, verbatim.
					EXPECT_EQ(netEntry.name(), "$NONE$");
				}
			}

			// Derived components by refDes.
			std::map<std::string, const ConnectivityMsg::PlacedComponent*> derivedByName;
			for (const auto& component : connectivity.components())
			{
				derivedByName[component.refdes()] = &component;
			}

			const auto* goldenComponents = field(golden, "components");
			ASSERT_NE(goldenComponents, nullptr);
			for (const auto& componentValue : goldenComponents->list_value().values())
			{
				const auto& componentStruct = componentValue.struct_value();
				const auto* refDes = field(componentStruct, "refDes");
				const auto* side = field(componentStruct, "side");
				const auto* uid = field(componentStruct, "uid");
				const auto* pins = field(componentStruct, "pins");
				ASSERT_NE(refDes, nullptr);
				ASSERT_NE(side, nullptr);
				ASSERT_NE(uid, nullptr);
				ASSERT_NE(pins, nullptr);

				const auto findIt = derivedByName.find(refDes->string_value());
				ASSERT_NE(findIt, derivedByName.end())
					<< "golden component " << refDes->string_value() << " missing from derived connectivity";
				const auto& derived = *findIt->second;

				EXPECT_EQ(derived.side(), side->string_value() == "Top"
					? Odb::Lib::Protobuf::Top
					: Odb::Lib::Protobuf::Bottom)
					<< refDes->string_value();

				if (isNull(*uid))
				{
					EXPECT_FALSE(derived.has_id()) << refDes->string_value() << ": uid absent in golden but set in derived (D2)";
				}
				else
				{
					ASSERT_TRUE(derived.has_id()) << refDes->string_value() << ": uid present in golden but unset in derived";
					EXPECT_EQ(derived.id(), num(*uid)) << refDes->string_value();
				}

				// Derived pins are package-roster ascending; index them by number.
				std::map<unsigned int, const ConnectivityMsg::ComponentPin*> derivedPins;
				for (const auto& pin : derived.pins())
				{
					derivedPins[pin.number()] = &pin;
				}

				// Every golden toeprint pin must match a derived pin (DP4
				// comparator rule); derived pins without a golden toeprint are
				// asserted UNCONNECTED afterwards (roster difference).
				std::map<unsigned int, bool> goldenPinSeen;
				for (const auto& pinValue : pins->list_value().values())
				{
					const auto& pinStruct = pinValue.struct_value();
					const auto* pinNumber = field(pinStruct, "pinNumber");
					const auto* pinName = field(pinStruct, "name");
					const auto* netOrdinal = field(pinStruct, "netOrdinal");
					const auto* subnetOrdinal = field(pinStruct, "subnetOrdinal");
					ASSERT_NE(pinNumber, nullptr);
					ASSERT_NE(pinName, nullptr);
					ASSERT_NE(netOrdinal, nullptr);
					ASSERT_NE(subnetOrdinal, nullptr);

					const auto pinNo = num(*pinNumber);
					goldenPinSeen[pinNo] = true;

					const auto pinFindIt = derivedPins.find(pinNo);
					ASSERT_NE(pinFindIt, derivedPins.end())
						<< refDes->string_value() << " pin " << pinNo << ": no derived pin";
					const auto& derivedPin = *pinFindIt->second;
					// Name is deliberately NOT asserted against the golden:
					// the golden "name" is the raw toeprint label (TOP record
					// field 8), which panelized designs namespace per placed
					// component (e.g. "C1418-1"), while the derived name is
					// the package-roster pin name ("1") — consistent with
					// packages[].pins[].name, which is what a client joins on.
					// Identity is pinned by pinNumber; connectivity by the
					// kind/net/subnets assertions below.

					const auto& connection = derivedPin.connection();
					if (isNull(*netOrdinal) || (hasNoneNet && num(*netOrdinal) == noneNetOrdinal))
					{
						// net_num -1, or membership of the $NONE$ net (DP4).
						EXPECT_EQ(connection.kind(), ConnectivityMsg::Connection::UNCONNECTED)
							<< refDes->string_value() << " pin " << pinNo;
						EXPECT_EQ(connection.net(), 0u)
							<< refDes->string_value() << " pin " << pinNo << ": UNCONNECTED must not carry a net";
						EXPECT_TRUE(connection.subnets().empty())
							<< refDes->string_value() << " pin " << pinNo << ": UNCONNECTED must not carry subnets";
					}
					else
					{
						EXPECT_EQ(connection.kind(), ConnectivityMsg::Connection::CONNECTED)
							<< refDes->string_value() << " pin " << pinNo;
						EXPECT_EQ(connection.net(), num(*netOrdinal))
							<< refDes->string_value() << " pin " << pinNo;
						ASSERT_EQ(connection.subnets().size(), 1u)
							<< refDes->string_value() << " pin " << pinNo;
						EXPECT_EQ(connection.subnets(0), num(*subnetOrdinal))
							<< refDes->string_value() << " pin " << pinNo;
					}
				}

				// Roster difference: derived pins with no golden toeprint must be UNCONNECTED.
				for (const auto& [pinNo, pin] : derivedPins)
				{
					if (goldenPinSeen.count(pinNo) != 0) continue;
					EXPECT_EQ(pin->connection().kind(), ConnectivityMsg::Connection::UNCONNECTED)
						<< refDes->string_value() << " pin " << pinNo
						<< ": no toeprint in golden, but derived claims CONNECTED";
				}
			}

			if (fullFidelity)
			{
				EXPECT_EQ(connectivity.components().size(), goldenComponents->list_value().values().size())
					<< "full-fidelity golden: derived component count must match exactly";
			}

			// netMembers mirrors exactly the CONNECTED edges (M2.2/DP4): its
			// total pin count equals the golden's connected-pin count minus
			// $NONE$-net members (the goldens count those as connected).
			// Asserted for "full" fidelity only - $NONE$ membership must be
			// measured design-wide, but a "sampled" golden records members
			// only for its subset (350-41017: 472 sampled vs 11,775
			// design-wide), so the arithmetic is impossible on a sample.
			if (fullFidelity)
			{
			const auto* aggregates = field(golden, "aggregates");
			ASSERT_NE(aggregates, nullptr);
			const auto* pinsConnected = field(aggregates->struct_value(), "pins_connected");
			ASSERT_NE(pinsConnected, nullptr);

			unsigned int derivedConnectedPins = 0;
			for (const auto& netMembers : connectivity.netmembers())
			{
				for (const auto& member : netMembers.members())
				{
					derivedConnectedPins += member.pin().size();
				}
			}

			unsigned int goldenNoneNetMembers = 0;
			if (hasNoneNet)
			{
				for (const auto& componentValue : goldenComponents->list_value().values())
				{
					const auto* pins = field(componentValue.struct_value(), "pins");
					ASSERT_NE(pins, nullptr);
					for (const auto& pinValue : pins->list_value().values())
					{
						const auto* netOrdinal = field(pinValue.struct_value(), "netOrdinal");
						ASSERT_NE(netOrdinal, nullptr);
						if (!isNull(*netOrdinal) && num(*netOrdinal) == noneNetOrdinal)
						{
							++goldenNoneNetMembers;
						}
					}
				}
			}

			EXPECT_EQ(derivedConnectedPins, num(*pinsConnected) - goldenNoneNetMembers)
			<< "netMembers must hold exactly the CONNECTED edges";
			}		}
	};

	TEST_F(ConnectivityContractTest, Connectivity_SampleDesign_DerivedMatchesGolden)
	{
		runDifferential("sample_design", /*archiveFromTestData=*/true);
	}

	TEST_F(ConnectivityContractTest, Connectivity_PanelG7162_ZeroConnectedPinsMatchesGolden)
	{
		runDifferential("Panel-g7162-31800_odb", /*archiveFromTestData=*/false);
	}

	TEST_F(ConnectivityContractTest, Connectivity_Legacy20040628_ZeroConnectedPinsMatchesGolden)
	{
		runDifferential("200-40628_Rev1_v7", /*archiveFromTestData=*/false);
	}

	TEST_F(ConnectivityContractTest, Connectivity_Legacy35041017_SampledSubsetMatchesGolden)
	{
		runDifferential("350-41017_rev1_odbjob_v7", /*archiveFromTestData=*/false);
	}

	TEST_F(ConnectivityContractTest, Connectivity_SampleDesign_PayloadWithinSizeBudget)
	{
		// M2.4's always-on gate on the CI-available design: the connectivity
		// block must stay <= 150 KB on sample_design (813 components, 2,811
		// connected pins). The 10%-of-fileModel leg on the 81k-component
		// legacy design runs via the opt-in benchmark (DesignFetchBenchmarkTests).
		auto archive = getIsolatedDesignPath("sample_design.tgz");
		ASSERT_TRUE(std::filesystem::exists(archive));

		auto pDesign = std::make_shared<Design>();
		ASSERT_TRUE(pDesign->Build(archive.string()));

		const auto pDesignMsg = pDesign->to_protobuf(false);
		ASSERT_TRUE(pDesignMsg->has_connectivity());
		EXPECT_LE(pDesignMsg->connectivity().ByteSizeLong(), static_cast<size_t>(150) * 1024)
			<< "connectivity payload exceeds the 150 KB budget (D4 merge gate)";
	}
}

