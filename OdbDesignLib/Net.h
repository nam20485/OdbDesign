#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>

class Net
{
public:
	Net();
	~Net();

	std::string GetName() const;

	typedef std::vector<std::shared_ptr<Net>> Vector;
	typedef std::map<std::string, std::shared_ptr<Net>> StringMap;

private:
	std::string m_name;


};
