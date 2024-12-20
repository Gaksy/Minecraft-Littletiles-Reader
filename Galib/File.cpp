#include "File.h"
#include "Exception.h"

using galib::file::FileInfo;

using galib::Exception::GalibException;
using galib::Exception::GalibErrorCode;

bool galib::file::GetFileInfo(const char* const kPFilePath, FileInfo & file_info) {
    // Check File Name
    if (!kPFilePath) {
        GalibException::SetException(GalibErrorCode::galib_invalid_argument, "The file path is incorrect", "galib::file::GetFileInfo");
        return false;
    }
    // Get Info
    int result;
    if (result = stat(kPFilePath, &file_info)) {
        GalibException::SetException(GalibErrorCode::galib_failed_retrieve_file, ((std::string("The file path is \"") + kPFilePath) + "\" the stat return: " + std::to_string(result)).c_str(), "galib::file::GetFileInfo");
        return false;
    }
    return true;
}

bool galib::file::ReadFile(const char* const kPFilePath, char* begin, const std::size_t kFileSize) {
    // Check File Name
    if (!kPFilePath) {
        GalibException::SetException(GalibErrorCode::galib_invalid_argument, "The file path is incorrect", "galib::file::ReadFile");
        return false;
    }
    // Create input file stream
    std::ifstream file_stream;

    // Open File
    file_stream.open(kPFilePath, std::ios::binary);
    if (!file_stream.is_open()) {
        GalibException::SetException(GalibErrorCode::galib_failed_open_file, ((std::string("can't open \"") + kPFilePath) + "\" use std::ifstream").c_str(), "galib::file::ReadFile");
        return false;
    }

    try {
        // Read file
        file_stream.read(begin, kFileSize);

        // Check if reading was successful
        if (!file_stream) {
            GalibException::SetException(GalibErrorCode::galib_failed_open_file, "File stream was error.", "galib::file::ReadFile");
        }
    }
    catch (...){
        GalibException::SetException(GalibErrorCode::galib_failed_open_file, "File stream was error.", "galib::file::ReadFile");
        return false;
    }

    // Close file_stream
    file_stream.close();
    return true;
}

bool galib::file::AccessFolder(const char* const kPFolderPath)
{
    // Check File Name
    if (!kPFolderPath) {
        return false;
    }

    FileInfo file_info;
    if (GetFileInfo(kPFolderPath, file_info)) {
        return file_info.st_mode & S_IFDIR;
    }

    return false;
}

std::string galib::file::FormatPath(const char* const kPPath)
{
    // Check File Name
    if (!kPPath) {
        GalibException::SetException(GalibErrorCode::galib_invalid_argument, "The path is incorrect", "galib::file::FormatPath");
        return std::string();
    }

    std::string temp(kPPath);
    for (char& ch : temp) {
        if (ch == '\\') {
            ch = '/';
        }
    }
    return temp;
}
