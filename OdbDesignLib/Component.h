#pragma once

#include "odbdesign_export.h"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include "Pin.h"
#include "Package.h"
#include "BoardSide.h"


namespace Odb::Lib::ProductModel
{
	class ODBDESIGN_EXPORT Component
	{
	public:
		Component(std::string refDes, std::string partName, std::shared_ptr<Package> pPackage, unsigned int index, BoardSide side);
		~Component();

		std::string GetRefDes() const;
		std::string GetPartName() const;
		std::shared_ptr<Package> GetPackage() const;
		unsigned int GetIndex() const;
		BoardSide GetSide() const;

		typedef std::vector<std::shared_ptr<Component>> Vector;
		typedef std::map<std::string, std::shared_ptr<Component>> StringMap;

	private:
		std::string m_refDes;
		std::string m_partName;		
		std::shared_ptr<Package> m_pPackage;
		unsigned int m_index;
		BoardSide m_side;
	};
}
