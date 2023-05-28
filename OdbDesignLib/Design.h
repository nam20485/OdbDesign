#pragma once

#include <string>
#include "Net.h"
#include "Component.h"
#include "FileModel.h"


class Design
{
public:
	//Design(const FileModel& fileModel);
	~Design();	

private:
	std::string m_productModel;
	std::string m_name;

	Net::StringMap m_netsByName;
	Component::StringMap m_componentsByName;

	bool BuildFromFileModel();

};
