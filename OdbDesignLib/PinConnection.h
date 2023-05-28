#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include "Component.h"


class PinConnection
{
public:
	PinConnection();
	~PinConnection();

	//std::string GetPinName() const;
	//std::string GetComponentRefDes() const;

	typedef std::vector<std::shared_ptr<PinConnection>> Vector;
	typedef std::map<std::string, std::shared_ptr<PinConnection>> StringMap;

private:
	std::string m_name;
	std::shared_ptr<Component> m_component;
	std::shared_ptr<Pin> m_pin;

};
