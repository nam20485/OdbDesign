#include "Design.h"

namespace Odb::Lib::ProductModel
{	
	Design::Design()
	{
	}

	Design::~Design()
	{
	}

	bool Design::Build(std::string designDirectory)
	{
		auto pFileModel = std::make_shared<FileModel::Design::FileModel>(designDirectory);
		if (pFileModel->ParseFileModel())
		{
			return Build(pFileModel);
		}
		return false;
	}

	bool Design::Build(std::shared_ptr<FileModel::Design::FileModel> pFileModel)
	{
		m_pFileModel = pFileModel;
		return Build();		
	}

	bool Design::Build()
	{
		if (m_pFileModel == nullptr) return false;

		BuildNets();

		return true;
	}

	bool Design::BuildComponents()
	{
		return false;
	}

	bool Design::BuildNets()
	{
		return false;
	}

} // namespace Odb::Lib::ProductModel