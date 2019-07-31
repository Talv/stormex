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

    // Tag mask. Only valid if the storage supports tags, otherwise 0
    ULONGLONG TagBitMask;

    // File data ID. Only valid if the storage supports file data IDs, otherwise CASC_INVALID_ID
    DWORD dwFileDataId;

    // Size of the file, as retrieved from CKey entry or EKey entry
    DWORD dwFileSize;

    // Locale flags. Only valid if the storage supports locale flags, otherwise CASC_INVALID_ID
    DWORD dwLocaleFlags;

    // Content flags. Only valid if the storage supports content flags, otherwise CASC_INVALID_ID
    DWORD dwContentFlags;

    // Hints as for which open method is suitable
    DWORD bFileAvailable:1;                     // If true the file is available locally
    DWORD bCanOpenByName:1;
    DWORD bCanOpenByDataId:1;
    DWORD bCanOpenByCKey:1;
    DWORD bCanOpenByEKey:1;
    CASC_NAME_TYPE NameType;
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
