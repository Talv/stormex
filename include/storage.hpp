#ifndef __STORAGE_HPP__
#define __STORAGE_HPP__

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

#define __CASCLIB_SELF__
#include "../CascLib/src/CascLib.h"
#include "common.hpp"
#include "util.hpp"

// Based on CASC_FIND_DATA
struct STORAGE_SEARCH_RESULT
{
    // Full name of the found file. In case when this is CKey/EKey,
    // this will be just string representation of the key stored in 'FileKey'
    std::string filename;

    // Content key. This is present if the CASC_FEATURE_ROOT_CKEY is present
    BYTE CKey[MD5_HASH_SIZE];

    // Encoded key. This is always present.
    BYTE EKey[MD5_HASH_SIZE];

    // Size of the file, as retrieved from CKey entry or EKey entry
    DWORD fileSize;

    // If true the file is available locally
    DWORD fileAvailable:1;

    // Name type in 'szFileName'. In case the file name is not known,
    // CascLib can put FileDataId-like name or a string representation of CKey/EKey
    CASC_NAME_TYPE nameType;
};

/**
 * @brief CASC Storage Explorer
 */
class StorageExplorer {
protected:
    HANDLE m_hStorage = nullptr;

public:
    HANDLE getHandle() { return m_hStorage; }

    ~StorageExplorer();

    /**
     * @brief Open CASC
     *
     * @param src
     * @return non zero in case of failure
     */
    int openStorage(std::string src);

    /**
     * @brief Close CASC
     *
     * @return true
     * @return false
     */
    bool closeStorage();

    // TODO: CascGetStorageInfo

    bool enumerateFiles(std::vector<STORAGE_SEARCH_RESULT*>& searchResults);

    /**
     * @brief extract data of given file to location specified under filesystem
     *
     * @param storedFilename
     * @param targetFilename
     * @return size_t
     */
    size_t extractFileToPath(const std::string& storedFilename, const std::string& targetFilename);

    /**
     * @brief extract data of given file and write it to a FILE stream (not limited to files)
     *
     * @param storedFilename
     * @param outStream
     * @return size_t
     */
    size_t extractFileData(const std::string& storedFilename, FILE* outStream);
};

#endif // __STORAGE_HPP__
