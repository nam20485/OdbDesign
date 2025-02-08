#pragma once

#include "../../odbdesign_export.h"
#include <string>
#include "StepDirectory.h"
#include <map>
#include <vector>
#include "MiscInfoFile.h"
#include "MatrixFile.h"
#include "StandardFontsFile.h"
#include <filesystem>
#include "../../IProtoBuffable.h"
#include "../../ProtoBuf/filearchive.pb.h"
#include "SymbolsDirectory.h"
#include "AttrListFile.h"
#include "../ISaveable.h"
#include <memory>


namespace Odb::Lib::FileModel::Design
{
	class ODBDESIGN_EXPORT FileArchive : public IProtoBuffable<Odb::Lib::Protobuf::FileArchive>, public ISaveable
	{
	public:	
		FileArchive();
		FileArchive(const std::string& path);
		~FileArchive();

		std::string GetRootDir() const;
		std::string GetProductName() const;
		std::string GetFilename() const;
		std::string GetFilePath() const;

		const StepDirectory::StringMap& GetStepsByName() const;
		const SymbolsDirectory::StringMap& GetSymbolsDirectoriesByName() const;
        const MiscInfoFile& GetMiscInfoFile() const;
		const MatrixFile& GetMatrixFile() const;
		const StandardFontsFile& GetStandardFontsFile() const;
		const AttrListFile& GetMiscAttrListFile() const;

		std::shared_ptr<StepDirectory> GetStepDirectory(const std::string& stepName = "") const;

		// TODO: fix these to use pointer return types
		//const EdaDataFile& GetStepEdaDataFile(std::string stepName) const;
		//const EdaDataFile& GetFirstStepEdaDataFile() const;		
				
		bool ParseFileModel();
		bool SaveFileModel(const std::filesystem::path& directory);
		bool SaveFileModel(const std::filesystem::path& directory, const std::string& archiveName);
		//bool SaveFileModel(const std::string& directory, const std::string& archiveName);				

		// Inherited via IProtoBuffable
		std::unique_ptr<Odb::Lib::Protobuf::FileArchive> to_protobuf() const override;
		void from_protobuf(const Odb::Lib::Protobuf::FileArchive& message) override;

		typedef std::vector<std::shared_ptr<FileArchive>> Vector;
		typedef std::map<std::string, std::shared_ptr<FileArchive>> StringMap;

	private:
		std::string m_rootDir;
		std::string m_productName;
		std::string m_filename;
		std::string m_filePath;

		MiscInfoFile m_miscInfoFile;
		MatrixFile m_matrixFile;
		StandardFontsFile m_standardFontsFile;
		AttrListFile m_miscAttrListFile;

		StepDirectory::StringMap m_stepsByName;
		SymbolsDirectory::StringMap m_symbolsDirectoriesByName;
		
		bool ParseDesignDirectory(const std::filesystem::path& path);
		bool ParseStepDirectories(const std::filesystem::path& path);
        bool ParseMiscInfoFile(const std::filesystem::path& path);
		bool ParseMatrixFile(const std::filesystem::path& path);
		bool ParseStandardFontsFile(const std::filesystem::path& path);		
		bool ParseSymbolsDirectories(const std::filesystem::path& path);
		bool ParseMiscAttrListFile(const std::filesystem::path& path);

		bool Save(const std::filesystem::path& directory);

		bool ExtractDesignArchive(const std::filesystem::path& path, std::filesystem::path& extractedPath);

		static std::string findRootDir(const std::filesystem::path& extractedPath);
		static bool pathContainsTopLevelDesignDirs(const std::filesystem::path& path);

		static inline constexpr const char* TOPLEVEL_DESIGN_DIR_NAMES[] =
		{ 
			"fonts", 
			"misc", 
			"matrix",
			"steps"
		};

		// REQUIRED (spec pg. 23):
		//• <product_model_name> / matrix / matrix
		//• <product_model_name> / misc	  / info
		//• <product_model_name> / fonts  / standard
		//• <product_model_name> / steps  / <step_name> / stephdr
		//• <product_model_name> / steps  / <step_name> / layers / <layer_name> / features(or features.Z)

		//• The length of an entity name must not exceed 64 characters.
		//• An entity name may contain only these characters :
		//	o Lower case letters(a through z).
		//	o Digits(0 through 9).
		//	o Punctuation—dash(-), underscore(_), dot(.) and plus(+).
		//• Entity names must not start with a dot(.), hyphen(-), or plus(+).
		//	The exception is system attribute names, which start with a dot.Names of user - defined
		//	attributes must not start with a dot.
		//• Entity names must not end with a dot(.).

		//The default units of measurement for the product model are as defined in the UNITS directive in
		//the file misc / info of the product model.If the default is not defined for the product model, the
		//default is imperial.


		// Attriubute Lookup Tables spec pg. 30
		//
		//Symbol Feature / symbols / <symbol_name> / features
		//	“<symbol_name> / features (Symbol Features)” on page 97

		//Net / steps / <step_name> / eda / data
		//	“eda / data(EDA Data)” on page 111

		//Feature / steps / <step_name> / layers / <layer_name> / features
		//	“<layer_name> / features(Graphic Features)” on page 172

		//Component / steps / <step_name> / layers / //<layer_name> / components
		//	“<layer_name> / components (Components)” on page 155

	};
}
