#pragma once

#include <map>
#include <vector>
#include <string>
#include <memory>
#include <filesystem>
#include "export.h"


namespace OdbDesign::Lib::FileModel
{
	class DECLSPEC NetlistFile
	{
	public:
		enum class Staggered
		{
			Yes,
			No,
			Unknown
		};

		NetlistFile(std::filesystem::path path);
		~NetlistFile();

		std::filesystem::path GetPath() const;
		std::string GetName() const;
		std::string GetUnits() const;

		bool GetOptimized() const;
		Staggered GetStaggered() const;

		const std::vector<std::string>& GetNetNames() const;

		bool Parse();

		typedef std::vector<std::shared_ptr<NetlistFile>> Vector;
		typedef std::map<std::string, std::shared_ptr<NetlistFile>> StringMap;

	private:
		std::filesystem::path m_path;
		std::string m_name;
		std::string m_units;
		bool m_optimized;
		Staggered m_staggered;

		std::vector<std::string> m_netNames;

		inline static const std::string UNITS_TOKEN = "UNITS";
		inline static const std::string COMMENT_TOKEN = "#";
		inline static const std::string NET_TOKEN = "$";
		inline static const std::string HEADER_TOKEN = "H";

	};
}