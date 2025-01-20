// main.cpp

#include "FIleModel/Design/FileArchive.h"
//#include "FileArchive.h" // Include your FileArchive class
//#include "attrlistfile.pb.h" // Include Protobuf headers
//#include "standardfontsfile.pb.h"

#include <iostream>
#include <fstream>
#include <google/protobuf/util/time_util.h>
#include <memory>
#include <google/protobuf/message.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/reflection.h>
#include <cstdint> // For fixed-width integer types
#include <iomanip> // For std::setprecision

namespace Design = Odb::Lib::FileModel::Design;
namespace Protobuf = Odb::Lib::Protobuf;

// UTF-8 Validation Function
bool isValidUTF8(const std::string& str) {
    int32_t i = 0;
    int32_t len = static_cast<int32_t>(str.length());
    while (i < len) {
        unsigned char c = str[i];
        if (c <= 0x7F) {
            // ASCII character
            i += 1;
        }
        else if ((c & 0xE0) == 0xC0) {
            // 2-byte sequence
            if (i + 1 >= len) return false;
            unsigned char c1 = str[i + 1];
            if ((c1 & 0xC0) != 0x80) return false;
            i += 2;
        }
        else if ((c & 0xF0) == 0xE0) {
            // 3-byte sequence
            if (i + 2 >= len) return false;
            unsigned char c1 = str[i + 1];
            unsigned char c2 = str[i + 2];
            if (((c1 & 0xC0) != 0x80) || ((c2 & 0xC0) != 0x80)) return false;
            i += 3;
        }
        else if ((c & 0xF8) == 0xF0) {
            // 4-byte sequence
            if (i + 3 >= len) return false;
            unsigned char c1 = str[i + 1];
            unsigned char c2 = str[i + 2];
            unsigned char c3 = str[i + 3];
            if (((c1 & 0xC0) != 0x80) || 
                ((c2 & 0xC0) != 0x80) || 
                ((c3 & 0xC0) != 0x80)) return false;
            i += 4;
        }
        else {
            // Invalid first byte
            return false;
        }
    }
    return true;
}

// UTF-8 Sanitization Function
std::string sanitizeToUTF8(const std::string& str) {
    std::string sanitized;
    int32_t i = 0;
    int32_t len = static_cast<int32_t>(str.length());
    while (i < len) {
        unsigned char c = str[i];
        if (c <= 0x7F) {
            // ASCII character
            sanitized += c;
            i += 1;
        }
        else if ((c & 0xE0) == 0xC0) {
            // 2-byte sequence
            if (i + 1 < len) {
                unsigned char c1 = str[i + 1];
                if ((c1 & 0xC0) == 0x80) {
                    sanitized += c;
                    sanitized += c1;
                    i += 2;
                    continue;
                }
            }
            // Invalid sequence
            sanitized += '?';
            i += 1;
        }
        else if ((c & 0xF0) == 0xE0) {
            // 3-byte sequence
            if (i + 2 < len) {
                unsigned char c1 = str[i + 1];
                unsigned char c2 = str[i + 2];
                if (((c1 & 0xC0) == 0x80) && ((c2 & 0xC0) == 0x80)) {
                    sanitized += c;
                    sanitized += c1;
                    sanitized += c2;
                    i += 3;
                    continue;
                }
            }
            // Invalid sequence
            sanitized += '?';
            i += 1;
        }
        else if ((c & 0xF8) == 0xF0) {
            // 4-byte sequence
            if (i + 3 < len) {
                unsigned char c1 = str[i + 1];
                unsigned char c2 = str[i + 2];
                unsigned char c3 = str[i + 3];
                if (((c1 & 0xC0) == 0x80) && 
                    ((c2 & 0xC0) == 0x80) && 
                    ((c3 & 0xC0) == 0x80)) {
                    sanitized += c;
                    sanitized += c1;
                    sanitized += c2;
                    sanitized += c3;
                    i += 4;
                    continue;
                }
            }
            // Invalid sequence
            sanitized += '?';
            i += 1;
        }
        else {
            // Invalid first byte
            sanitized += '?';
            i += 1;
        }
    }
    return sanitized;
}

// Recursive function to traverse and sanitize PropertyRecord 'value' fields
void TraverseAndSanitize(google::protobuf::Message* message) {
    const google::protobuf::Descriptor* descriptor = message->GetDescriptor();
    const google::protobuf::Reflection* reflection = message->GetReflection();

    for (int32_t i = 0; i < descriptor->field_count(); ++i) { // Changed to int32_t
        const google::protobuf::FieldDescriptor* field = descriptor->field(i);

        if (field->type() == google::protobuf::FieldDescriptor::TYPE_MESSAGE) {
            if (field->is_repeated()) {
                int32_t count = reflection->FieldSize(*message, field); // Changed to int32_t
                for (int32_t j = 0; j < count; ++j) { // Changed to int32_t
                    google::protobuf::Message* subMessage = reflection->MutableRepeatedMessage(message, field, j);
                    TraverseAndSanitize(subMessage);
                }
            }
            else {
                if (reflection->HasField(*message, field)) {
                    google::protobuf::Message* subMessage = reflection->MutableMessage(message, field);
                    TraverseAndSanitize(subMessage);
                }
            }
        }
        else if (field->type() == google::protobuf::FieldDescriptor::TYPE_STRING &&
                 field->name() == "value") { // Ensure it's the 'value' field
            if (field->is_repeated()) {
                int32_t count = reflection->FieldSize(*message, field); // Changed to int32_t
                for (int32_t j = 0; j < count; ++j) { // Changed to int32_t
                    std::string value = reflection->GetRepeatedString(*message, field, j);
                    if (!isValidUTF8(value)) {
                        std::string sanitized = sanitizeToUTF8(value);
                        reflection->SetRepeatedString(message, field, j, sanitized);
                        std::cerr << "Warning: PropertyRecord 'value' field at index " << j 
                                  << " had invalid UTF-8 and has been sanitized." << std::endl;
                    }
                }
            }
            else {
                if (reflection->HasField(*message, field)) {
                    std::string value = reflection->GetString(*message, field);
                    if (!isValidUTF8(value)) {
                        std::string sanitized = sanitizeToUTF8(value);
                        reflection->SetString(message, field, sanitized);
                        std::cerr << "Warning: PropertyRecord 'value' field had invalid UTF-8 and has been sanitized." << std::endl;
                    }
                }
            }
        }
    }
}

int main(int argc, char* argv[]) {
    // Check for the correct number of command-line arguments
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <path_to_odb_archive> <output_protobuf_file>" << std::endl;
        return EXIT_FAILURE;
    }

    // Extract command-line arguments
    std::string archivePath = argv[1];
    std::string outputProtobufPath = argv[2];

    // Initialize Google Protobuf library (optional but recommended)
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    try {
        // Create a FileArchive instance with the provided archive path
        Design::FileArchive fileArchive(archivePath);
        std::cout << "Initialized FileArchive with path: " << archivePath << std::endl;

        // Parse the ODB++ archive
        std::cout << "Parsing the ODB++ archive..." << std::endl;
        bool parseSuccess = fileArchive.ParseFileModel();
        if (!parseSuccess) {
            std::cerr << "Error: Failed to parse the ODB++ archive." << std::endl;
            return EXIT_FAILURE;
        }
        std::cout << "Successfully parsed the ODB++ archive." << std::endl;

        // Access and display parsed data using existing getter methods
        std::cout << "----- Parsed Data -----" << std::endl;
        std::cout << "Root Directory: " << fileArchive.GetRootDir() << std::endl;
        std::cout << "Product Name: " << fileArchive.GetProductName() << std::endl;
        std::cout << "Filename: " << fileArchive.GetFilename() << std::endl;
        std::cout << "File Path: " << fileArchive.GetFilePath() << std::endl;

        // Display Steps
        const auto& steps = fileArchive.GetStepsByName();
        std::cout << "\nSteps (" << steps.size() << "):" << std::endl;
        for (const auto& [stepName, stepDir] : steps) {
            std::cout << " - " << stepName << std::endl;
            // Additional step-specific information can be accessed here
        }

        // Display Symbols Directories
        const auto& symbolsDirs = fileArchive.GetSymbolsDirectoriesByName();
        std::cout << "\nSymbols Directories (" << symbolsDirs.size() << "):" << std::endl;
        for (const auto& [symbolDirName, symbolDir] : symbolsDirs) {
            std::cout << " - " << symbolDirName << std::endl;
            // Additional symbols directory-specific information can be accessed here
        }

        // Access and display MiscInfoFile
        const Design::MiscInfoFile& miscInfo = fileArchive.GetMiscInfoFile();
        std::cout << "\n--- Misc Info File ---" << std::endl;
        std::cout << "Product Model Name: " << miscInfo.GetProductModelName() << std::endl;
        std::cout << "Job Name: " << miscInfo.GetJobName() << std::endl;
        std::cout << "ODB Version: " << miscInfo.GetOdbVersionMajor() << "." << miscInfo.GetOdbVersionMinor() << std::endl;
        std::cout << "ODB Source: " << miscInfo.GetOdbSource() << std::endl;

        // Convert creation and save dates to time_t for readable output
        std::time_t creationTime = std::chrono::system_clock::to_time_t(miscInfo.GetCreationDate());
        std::time_t saveTime = std::chrono::system_clock::to_time_t(miscInfo.GetSaveDate());

        std::cout << "Creation Date: " << std::ctime(&creationTime);
        std::cout << "Save Date: " << std::ctime(&saveTime);
        std::cout << "Save App: " << miscInfo.GetSaveApp() << std::endl;
        std::cout << "Save User: " << miscInfo.GetSaveUser() << std::endl;
        std::cout << "Units: " << miscInfo.GetUnits() << std::endl;
        std::cout << "Max Unique ID: " << miscInfo.GetMaxUniqueId() << std::endl;

        // Access and display MatrixFile
        const Design::MatrixFile& matrixFile = fileArchive.GetMatrixFile();
        std::cout << "\n--- Matrix File ---" << std::endl;

        // Serialize MatrixFile to Protobuf message
        std::unique_ptr<Protobuf::MatrixFile> protobufMatrix = matrixFile.to_protobuf();

        std::cout << "Number of Step Records: " << protobufMatrix->steps_size() << std::endl;
        std::cout << "Number of Layer Records: " << protobufMatrix->layers_size() << std::endl;

        // Example: Display first step record if available
        if (protobufMatrix->steps_size() > 0) {
            const auto& firstStep = protobufMatrix->steps(0);
            std::cout << "First Step Record - Column: " << firstStep.column()
                      << ", Name: " << firstStep.name()
                      << ", ID: " << firstStep.id() << std::endl;
        }

        // Example: Display first layer record if available
        if (protobufMatrix->layers_size() > 0) {
            const auto& firstLayer = protobufMatrix->layers(0);
            std::cout << "First Layer Record - Row: " << firstLayer.row()
                      << ", Name: " << firstLayer.name()
                      << ", ID: " << firstLayer.id() << std::endl;
        }

        // Access and display StandardFontsFile
        const Design::StandardFontsFile& fontsFile = fileArchive.GetStandardFontsFile();
        std::cout << "\n--- Standard Fonts File ---" << std::endl;

        // Serialize StandardFontsFile to Protobuf message
        std::unique_ptr<Protobuf::StandardFontsFile> protobufFonts = fontsFile.to_protobuf();

        // Corrected field access based on Protobuf-generated methods
        std::cout << "X Size: " << protobufFonts->xsize() << std::endl;
        std::cout << "Y Size: " << protobufFonts->ysize() << std::endl;
        std::cout << "Offset: " << protobufFonts->offset() << std::endl;
        std::cout << "Number of Character Blocks: " << protobufFonts->m_characterblocks_size() << std::endl;

        // Iterate over character blocks
        for (int32_t i = 0; i < protobufFonts->m_characterblocks_size(); ++i) { // Changed to int32_t
            const auto& block = protobufFonts->m_characterblocks(i);
            std::cout << "Character Block " << i+1 << ":" << std::endl;
            std::cout << "  Character: " << block.character() << std::endl;
            std::cout << "  Number of Line Records: " << block.m_linerecords_size() << std::endl;
            for (int32_t j = 0; j < block.m_linerecords_size(); ++j) { // Changed to int32_t
                const auto& line = block.m_linerecords(j);
                std::cout << "    Line Record " << j+1 << ":" << std::endl;
                std::cout << "      xStart: " << line.xstart() << std::endl;
                std::cout << "      yStart: " << line.ystart() << std::endl;
                std::cout << "      xEnd: " << line.xend() << std::endl;
                std::cout << "      yEnd: " << line.yend() << std::endl;
                std::cout << "      Polarity: " << line.polarity() << std::endl;
                std::cout << "      Shape: " << line.shape() << std::endl;
                std::cout << "      Width: " << line.width() << std::endl;
            }
        }

        // Access and display AttrListFile
        const Design::AttrListFile& attrListFile = fileArchive.GetMiscAttrListFile();
        std::cout << "\n--- Misc Attr List File ---" << std::endl;

        // Serialize AttrListFile to Protobuf message
        std::unique_ptr<Protobuf::AttrListFile> protobufAttrList = attrListFile.to_protobuf();

        std::cout << "Units: " << protobufAttrList->units() << std::endl;
        std::cout << "Number of Attributes: " << protobufAttrList->attributesbyname_size() << std::endl;

        // Display all attributes
        std::cout << "Attributes:" << std::endl;
        for (const auto& [key, value] : protobufAttrList->attributesbyname()) {
            std::cout << " - " << key << " = " << value << std::endl;
        }

        // Serialize the entire FileArchive to a Protobuf message
        std::cout << "\nSerializing parsed data to Protobuf..." << std::endl;
        std::unique_ptr<Protobuf::FileArchive> protobufMessage = fileArchive.to_protobuf();
        
        // **Traverse and Sanitize PropertyRecord 'value' Fields**
        TraverseAndSanitize(protobufMessage.get());

        // Open the output file in binary mode
        std::ofstream outputFile(outputProtobufPath, std::ios::out | std::ios::binary);
        if (!outputFile.is_open()) {
            std::cerr << "Error: Unable to open output file: " << outputProtobufPath << std::endl;
            return EXIT_FAILURE;
        }

        // Serialize the Protobuf message to the output file
        if (!protobufMessage->SerializeToOstream(&outputFile)) {
            std::cerr << "Error: Failed to serialize Protobuf message to file." << std::endl;
            outputFile.close();
            return EXIT_FAILURE;
        }

        outputFile.close();
        std::cout << "Successfully serialized FileArchive to " << outputProtobufPath << std::endl;

    } catch (const std::exception& ex) {
        std::cerr << "Exception encountered: " << ex.what() << std::endl;
        return EXIT_FAILURE;
    }

    // Optional: Clean up Protobuf library resources
    google::protobuf::ShutdownProtobufLibrary();

    return EXIT_SUCCESS;
}