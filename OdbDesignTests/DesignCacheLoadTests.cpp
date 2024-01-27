#include <gtest/gtest.h>
#include "Fixtures/FileArchiveLoadFixture.h"
#include "OdbDesign.h"

//using namespace Odb::Lib::App;
using namespace Odb::Lib::FileModel;
using namespace Odb::Test::Fixtures;
using namespace std::filesystem;

namespace Odb::Test
{
	TEST_F(FileArchiveLoadFixture, Load_Design_Succeeds_sample_design_tgz)
	{
		//ASSERT_TRUE(exists(getDesignPath("sample_design.tgz")));
		auto pDesign = m_pDesignCache->GetDesign("sample_design");
		ASSERT_NE(pDesign, nullptr);
	}

	TEST_F(FileArchiveLoadFixture, Load_Design_Succeeds_designodb_rigidflex_tgz)
	{
		//ASSERT_TRUE(exists(getDesignPath("designodb_rigidflex.tgz")));
		auto pDesign = m_pDesignCache->GetDesign("designodb_rigidflex");
		ASSERT_NE(pDesign, nullptr);
	}	
}