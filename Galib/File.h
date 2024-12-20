#ifndef _GALIB_FILE_H_
#define _GALIB_FILE_H_

#include <cstdlib>
#include <vector>
#include <fstream>

namespace galib {
    namespace file {
        typedef struct stat FileInfo;

        bool GetFileInfo(const char* const kPFilePath, FileInfo& file_info);

        bool ReadFile(const char* const kPFilePath, char* begin, const std::size_t kFileSize);
    
        bool AccessFolder(const char* const kPFolderPath);

        std::string FormatPath(const char* const kPPath);
    }
}

#endif // !_GALIB_FILE_H_
