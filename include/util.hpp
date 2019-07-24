#ifndef __UTIL_HPP__
#define __UTIL_HPP__

#include <dirent.h>

#if (defined(_WIN32) || defined(_WIN64))
    #include <direct.h>
    #define mkdir(name, chmod) _mkdir(name)
#endif
#if defined(WIN32) || defined(_WIN32)
    #define PATH_SEP_STR "\\"
    #define PATH_SEP_CHAR '\\'
#else
    #define PATH_SEP_STR "/"
    #define PATH_SEP_CHAR '/'
#endif

bool pathExists(const std::string& target);
int ensureDirExists(std::string strDestName);

std::string formatFileSize(size_t size);

/// Try to find in the Haystack the Needle - ignore case
bool findStringIC(const std::string& strHaystack, const std::string& strNeedle);

#endif // __UTIL_HPP__
