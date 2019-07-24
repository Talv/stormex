#include <set>
#include "storage.hpp"

int StorageExplorer::openStorage(std::string src)
{
    // Remove trailing slash at the end of path (CascLib doesn't like that, supposedly)
    if (src[src.size() - 1] == '/' || src[src.size() - 1] == '\\') {
        src = src.substr(0, src.size() - 1);
    }

    if (!CascOpenStorage(src.c_str(), 0, &m_hStorage)) {
        return GetLastError();
    }

    return 0;
}

bool StorageExplorer::closeStorage()
{
    return CascCloseStorage(m_hStorage);
}

std::vector<std::string> StorageExplorer::enumerateFiles()
{
    std::vector<std::string> ret;

    CASC_FIND_DATA findData;
    HANDLE handle = CascFindFirstFile(m_hStorage, "*", &findData, NULL);

    do {
        findData.dwFileSize;
        findData.FileKey;
        findData.szFileName;
        ret.push_back(findData.szFileName);
    } while (CascFindNextFile(handle, &findData));

    CascFindClose(handle);

    return ret;
}

size_t StorageExplorer::extractFileToPath(const std::string& storedFilename, const std::string& targetFilename)
{
    int tmp;

    if ((tmp = ensureDirExists(targetFilename)) != 0) {
        PLOG_ERROR << "Couldn't create directory path for file: " << targetFilename << " E(" << tmp << ")";
        return 0;
    }

    FILE* fileStream = fopen(targetFilename.c_str(), "wb");
    if (fileStream) {
        size_t fileSize = extractFileData(storedFilename, fileStream);
        fclose(fileStream);
        return fileSize;
    }
    else {
        PLOG_ERROR << "Failed to open file for writing: " << targetFilename << " E(" << errno << ")";
        return 0;
    }
}

size_t StorageExplorer::extractFileData(const std::string& storedFilename, FILE* outStream)
{
    char buffer[0x1000];

    HANDLE hFile;
    size_t fileSize = 0;
    if (CascOpenFile(m_hStorage, storedFilename.c_str(), CASC_LOCALE_ALL, 0, &hFile)) {
        DWORD read;
        do {
            if (CascReadFile(hFile, &buffer, sizeof(buffer), &read)) {
                fwrite(&buffer, read, 1, outStream);
                fileSize += read;
            }
        } while (read > 0);

        CascCloseFile(hFile);
    }
    else {
        PLOG_ERROR << "Failed to extract: " << storedFilename << " to " << static_cast<void*>(outStream) << " E(" << errno << ")";
        return 0;
    }

    return fileSize;
}
