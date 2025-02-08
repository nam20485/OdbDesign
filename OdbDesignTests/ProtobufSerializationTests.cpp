#include <gtest/gtest.h>
#include "Fixtures/FileArchiveLoadFixture.h"
#include <gmock/gmock-matchers.h>
#include "FileModel/Design/FileArchive.h"
#include <memory>
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
}