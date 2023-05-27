#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>


class Pin
{
public:
	Pin();
	~Pin();

	std::string GetName() const;

	typedef std::vector<std::shared_ptr<Pin>> Vector;
	typedef std::map<std::string, std::shared_ptr<Pin>> StringMap;

private:
	std::string m_name;

};
