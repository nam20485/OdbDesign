#include <gtest/gtest.h>
#include "Fixtures/FileArchiveLoadFixture.h"
#include "OdbDesign.h"

//using namespace Odb::Lib::App;
using namespace Odb::Lib::FileModel;
using namespace Odb::Test::Fixtures;

namespace Odb::Test
{	

#ifndef RUN_ALL_TESTS
#	define RUN_ALL_TESTS 0
#endif
#ifndef RUN_SUCCEEDING_TESTS
#	define RUN_SUCCEEDING_TESTS 1
#endif
#ifndef RUN_FAILING_TESTS
#	define RUN_FAILING_TESTS 1
#endif

#if (RUN_SUCCEEDING_TESTS || RUN_ALL_TESTS)
	
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

#endif // (RUN_SUCCEEDING_TESTS || RUN_ALL_TESTS)	

	//
	// Fails
	//

#if (RUN_FAILING_TESTS || RUN_ALL_TESTS)

	TEST_F(FileArchiveLoadFixture, Load_FileArchive_Missing_Standard_Fonts_File_Throws_InvalidOdbError)
	{
		// assert throws invalid_odb_error b/c missing fonts/standard file
		EXPECT_THROW(
			{
				auto pFileArchive = m_pDesignCache->GetFileArchive("designodb_rigidflex - missing_fonts_standard");
			},
			invalid_odb_error);
	}	

	TEST_F(FileArchiveLoadFixture, Load_FileArchive_Missing_Misc_Info_File_Throws_InvalidOdbError)
	{
		// throws invalid_odb_error because missing misc/info file
		EXPECT_THROW(
			{
				auto pFileArchive = m_pDesignCache->GetFileArchive("designodb_rigidflex - missing_misc_info");
			},
			invalid_odb_error);
	}	
	
	TEST_F(FileArchiveLoadFixture, Load_FileArchive_InvalidPinElectricalType_Throws_ParseError)
	{
		// throws parse_error because:
		// 
		//Parse Error:
		//current file : [data:73383]
		//current line : [PIN PIN_61 T -14.1 0 0 N R ID=3996]
		//										~^~
		//current token : [N]
		//location : EdaDataFile.cpp : 824

		EXPECT_THROW(
			{
				auto pFileArchive = m_pDesignCache->GetFileArchive("designodb_rigidflex - pin_invalid_electricaltype");
			},
			parse_error);
	}

	TEST_F(FileArchiveLoadFixture, Load_FileArchive_InvalidPinType_Throws_ParseError)
	{
		// throws parse_error because:
		// 
		//Parse Error :
		//current file : [designs\Panel - Rotation Test Board\Panel - Rotation Test Board\steps\rotation - test - board\eda\data\data:180]
		//current line : [PIN 1 U -0.3984522 -0.0977426 0 U U ID=30]
		//					   ~^~
		//current token : [U]
		//location : EdaDataFile.cpp : 790

		EXPECT_THROW(
			{
				auto pFileArchive = m_pDesignCache->GetFileArchive("designodb_rigidflex - pin_invalid_type");
			},
			parse_error);		
	}

#endif // (RUN_FAILING_TESTS || RUN_ALL_TESTS)

}