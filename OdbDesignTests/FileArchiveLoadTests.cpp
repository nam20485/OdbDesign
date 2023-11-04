#include <gtest/gtest.h>
#include "FileArchiveLoadFixture.h"
#include "OdbDesign.h"

//using namespace Odb::Lib::App;
using namespace Odb::Lib::FileModel;

namespace Odb::Test
{	

#ifndef RUN_ALL_TESTS
#	define RUN_ALL_TESTS 1
#endif
#ifndef RUN_SUCCEEDING_TESTS
#	define RUN_SUCCEEDING_TESTS 0
#endif
#ifndef RUN_FAILING_TESTS
#	define RUN_FAILING_TESTS 0
#endif

#if (RUN_SUCCEEDING_TESTS || RUN_ALL_TESTS)

	TEST_F(FileArchiveLoadFixture, Load_FileArchive_Succeeds_Turbot_zip)
	{
		auto pFileArchive = m_pDesignCache->GetFileArchive("Turbot");
		ASSERT_NE(pFileArchive, nullptr);
	}

	TEST_F(FileArchiveLoadFixture, Load_FileArchive_Succeeds_35041017_rev1_odbjob_v7_tgz)
	{
		auto pFileArchive = m_pDesignCache->GetFileArchive("350-41017_rev1_odbjob_v7");
		ASSERT_NE(pFileArchive, nullptr);
	}

	TEST_F(FileArchiveLoadFixture, Load_FileArchive_Succeeds_sample_design_tgz)
	{
		auto pFileArchive = m_pDesignCache->GetFileArchive("sample_design");
		ASSERT_NE(pFileArchive, nullptr);
	}

	TEST_F(FileArchiveLoadFixture, Load_FileArchive_Succeeds_designodb_rigidflex_tgz)
	{
		auto pFileArchive = m_pDesignCache->GetFileArchive("designodb_rigidflex");
		ASSERT_NE(pFileArchive, nullptr);
	}

	TEST_F(FileArchiveLoadFixture, Load_FileArchive_Succeeds_200_40628_Rev1_v7_tgz)
	{
		auto pFileArchive = m_pDesignCache->GetFileArchive("200-40628_Rev1_v7");
		ASSERT_NE(pFileArchive, nullptr);
	}	

	TEST_F(FileArchiveLoadFixture, Load_FileArchive_Succeeds_odb_zip)
	{
		auto pFileArchive = m_pDesignCache->GetFileArchive("odb");
		ASSERT_NE(pFileArchive, nullptr);
	}

	TEST_F(FileArchiveLoadFixture, Load_FileArchive_Succeeds_Panel__Demo1210_zip)
	{
		auto pFileArchive = m_pDesignCache->GetFileArchive("Panel - Demo1210");
		ASSERT_NE(pFileArchive, nullptr);
	}

	TEST_F(FileArchiveLoadFixture, Load_FileArchive_Succeeds_Panel__g7162_31800_odb_tgz)
	{
		auto pFileArchive = m_pDesignCache->GetFileArchive("Panel - g7162-31800_odb");
		ASSERT_NE(pFileArchive, nullptr);
	}

	TEST_F(FileArchiveLoadFixture, Load_FileArchive_Succeeds_3_1712_xxa00odb_zip)
	{
		auto pFileArchive = m_pDesignCache->GetFileArchive("3.1712.xxa00odb");
		ASSERT_NE(pFileArchive, nullptr);
	}

	TEST_F(FileArchiveLoadFixture, Load_FileArchive_Succeeds_ap50132476_tgz)
	{
		auto pFileArchive = m_pDesignCache->GetFileArchive("ap50132476");
		ASSERT_NE(pFileArchive, nullptr);
	}

	TEST_F(FileArchiveLoadFixture, Load_FileArchive_Succeeds_comav331NEW_DFT_DFT_TGZ)
	{
		auto pFileArchive = m_pDesignCache->GetFileArchive("comav331NEW_DFT_DFT");
		ASSERT_NE(pFileArchive, nullptr);
	}

	TEST_F(FileArchiveLoadFixture, Load_FileArchive_Succeeds_design_1_no_bom_tgz)
	{
		auto pFileArchive = m_pDesignCache->GetFileArchive("design_1_no_bom");
		ASSERT_NE(pFileArchive, nullptr);
	}

	TEST_F(FileArchiveLoadFixture, Load_FileArchive_Succeeds_designodb_zip)
	{
		auto pFileArchive = m_pDesignCache->GetFileArchive("designodb");
		ASSERT_NE(pFileArchive, nullptr);
	}	

	TEST_F(FileArchiveLoadFixture, Load_FileArchive_Succeeds_kitara_tgz)
	{
		auto pFileArchive = m_pDesignCache->GetFileArchive("kitara");
		ASSERT_NE(pFileArchive, nullptr);
	}	

	TEST_F(FileArchiveLoadFixture, Load_FileArchive_Succeeds_PAB_SAYISAL_KART_RevX2_tgz)
	{
		auto pFileArchive = m_pDesignCache->GetFileArchive("PAB_SAYISAL_KART_RevX2");
		ASSERT_NE(pFileArchive, nullptr);
	}

#endif // (RUN_SUCCEEDING_TESTS || RUN_ALL_TESTS)

	//TEST_F(FileArchiveLoadFixture, Load_FileArchive_Succeeds_Panel__Demo1230_tgz)
	//{
	//	auto pFileArchive = m_pDesignCache->GetFileArchive("Panel - Demo1230");
	//	ASSERT_NE(pFileArchive, nullptr);
	//}

	//TEST_F(FileArchiveLoadFixture, Load_FileArchive_Succeeds_designodb_zip)
	//{
	//	auto pFileArchive = m_pDesignCache->GetFileArchive("designodb");
	//	ASSERT_NE(pFileArchive, nullptr);
	//}

	

	//
	// Fails
	//

#if (RUN_FAILING_TESTS || RUN_ALL_TESTS)

	TEST_F(FileArchiveLoadFixture, Load_FileArchive_Fails_pcb_1039N05890_v02_tar)
	{
		// assert throws invalid_odb_error b/c missing fonts/standard file
		EXPECT_THROW(
			{
				auto pFileArchive = m_pDesignCache->GetFileArchive("pcb_1039N05890_v02");
			},
			invalid_odb_error);
	}	

	TEST_F(FileArchiveLoadFixture, Load_FileArchive_Fails_odb1_zip)
	{
		// throws invalid_odb_error because missing misc/info file
		EXPECT_THROW(
			{
				auto pFileArchive = m_pDesignCache->GetFileArchive("odb1");
			},
			invalid_odb_error);
	}

	TEST_F(FileArchiveLoadFixture, Load_FileArchive_Fails_3687504951_zip)
	{
		// throws invalid_odb_error because missing misc/info file
		EXPECT_THROW(
			{
				auto pFileArchive = m_pDesignCache->GetFileArchive("3687504951");
			},
			invalid_odb_error);
	}	

	TEST_F(FileArchiveLoadFixture, Load_FileArchive_Fails_dft_zip)
	{
		// throws invalid_odb_error because missing misc/info file
		EXPECT_THROW(
			{
				auto pFileArchive = m_pDesignCache->GetFileArchive("dft");
			},
			invalid_odb_error);
	}	
	
	TEST_F(FileArchiveLoadFixture, Load_FileArchive_Fails_Panel__Demo1230_tgz)
	{
		// throws parse_error because:
		// 
		//Parse Error:
		//current file : [data:73383]
		//current line : [PIN PIN_61 T - 14.1 0 0 N R ID = 3996]
		//current token : [N]
		//location : EdaDataFile.cpp : 824

		EXPECT_THROW(
			{
				auto pFileArchive = m_pDesignCache->GetFileArchive("Panel - Demo1230");
			},
			parse_error);
	}

	TEST_F(FileArchiveLoadFixture, Load_FileArchive_Fails_Panel_Turbot_odb_panel_tgz)
	{
		// TODO: throws parse_error because ???
		EXPECT_THROW(
			{
				auto pFileArchive = m_pDesignCache->GetFileArchive("Turbot_odb_panel");
			},
			parse_error);
	}

#endif // (RUN_FAILING_TESTS || RUN_ALL_TESTS)

}