#pragma once

#include <string>
#include <map>
#include <memory>
#include <filesystem>

class Layer
{
public:
	Layer(std::filesystem::path path);
	~Layer();

	inline static const std::string TOP_COMPONENTS_LAYER_NAME = "comp_+_top";
	inline static const std::string BOTTOM_COMPONENTS_LAYER_NAME = "comp_+_bot";

	std::string GetName() const;
	std::filesystem::path GetPath() const;

	virtual bool Parse();

	typedef std::map<std::string, std::shared_ptr<Layer>> StringMap;

protected: // TODO: do subclasses really need access to these?
	std::string m_name;
	std::filesystem::path m_path;

};

