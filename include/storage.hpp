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

struct StorageSearchResult
{
    size_t fileSize;
};

/**
 * @brief CASC Storage Explorer
 */
class StorageExplorer {
protected:
    HANDLE m_hStorage = nullptr;

public:
    const HANDLE& getHandle() { return m_hStorage; }

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

    std::vector<std::string> enumerateFiles();

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
