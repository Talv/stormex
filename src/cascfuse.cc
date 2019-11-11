#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#include <sstream>
#include <algorithm>
#include <unordered_set>
#include <unordered_map>
#include <map>
#include <cctype>

#define __CASCLIB_SELF__
#include "../CascLib/src/CascLib.h"
#include "../CascLib/src/CascCommon.h"
#include "common.hpp"
#include "util.hpp"

#ifndef WIN32
    #define FUSE_STAT struct stat
    #define FUSE_OFF_T off_t
#endif

enum class FsNodeKind {
    Unknown,
    Root,
    Folder,
    File,
};

class StringIHasher
{
public:
    size_t operator()(const std::string& k) const
    {
        std::string str = k;
        std::transform(str.begin(), str.end(), str.begin(), [](const char& ch) {
            switch (ch) {
                case '\\': return '/';
                default: return static_cast<char>(::tolower(ch));
            }
        });
        return CalcNormNameHash(str.c_str(), str.size());
    }
};

class StringIComparator
{
public:
    bool operator()(const std::string& str1, const std::string& str2) const
    {
        return str1.size() == str2.size() && std::equal(str1.begin(), str1.end(), str2.begin(), [](const char& ch1, const char& ch2) {
            if ((ch1 == '/' || ch1 == '\\') && (ch2 == '/' || ch2 == '\\')) {
                return true;
            }
            else {
                return std::tolower(ch1) == std::tolower(ch2);
            }
        });
    }
};

class FsNode {
    std::string m_name;
    std::string m_filename;
    FsNode* m_parent = NULL;
    std::unordered_map<std::string, FsNode*, StringIHasher, StringIComparator> m_children;
public:
    const FsNodeKind m_kind = FsNodeKind::Unknown;
    PCASC_CKEY_ENTRY ckeyEntry = NULL;

    FsNode(FsNodeKind nKind, std::string name, FsNode *parent)
        : m_kind(nKind), m_name(name), m_parent(parent)
    {
    }

    FsNode(FsNodeKind nKind = FsNodeKind::Root)
        : m_kind(nKind), m_name("/")
    {
    }

    void Insert(FsNode* childNode)
    {
        assert(m_kind != FsNodeKind::File);
        m_children[stringToLowerCopy(childNode->Name())] = childNode;
    }

    FsNode *Insert(FsNodeKind nKind, const std::string& name)
    {
        auto childNode = new FsNode(nKind, name, this);
        Insert(childNode);
        return childNode;
    }

    const std::unordered_map<std::string, FsNode*, StringIHasher, StringIComparator>& Children()
    {
        return m_children;
    }

    const std::string& Name()
    {
        return m_name;
    }

    FsNode* Parent()
    {
        return m_parent;
    }

    void SetParent(FsNode* newParent)
    {
        m_parent = newParent;
    }

    const std::string Filepath()
    {
        if (m_filename.length()) return m_filename;

        std::vector<const FsNode*> parentNodes;
        auto currParent = this;
        do {
            parentNodes.push_back(currParent);
            currParent = currParent->Parent();
        } while (currParent != NULL && (currParent->m_kind == FsNodeKind::Folder || currParent->m_kind == FsNodeKind::File));

        std::ostringstream osPath;
        for (auto it = parentNodes.rbegin(); it != parentNodes.rend(); it++) {
            if ((*it)->m_kind == FsNodeKind::Root) {
                osPath << (*it)->m_name;
            }
            else {
                osPath << '/' << (*it)->m_name;
            }
        }
        m_filename = osPath.str();

        return m_filename;
    }
};

class FsTree {
    const size_t m_openFileLimit = 128;
    FsNode m_rootNode;
    std::unordered_map<std::string, FsNode*, StringIHasher, StringIComparator> m_nodeMap;
    std::map<std::string, HANDLE> m_openFiles;

public:
    FsNode* GetRootNode() { return &m_rootNode; }
    std::unordered_map<std::string, FsNode*, StringIHasher, StringIComparator>& GetNodeMap() { return m_nodeMap; }
    HANDLE m_hStorage = NULL;

    FsTree()
        : m_rootNode(FsNodeKind::Root)
    {
    }

    void GenerateNodeHashMap(FsNode* fNode)
    {
        if (fNode->m_kind != FsNodeKind::Unknown) {
            m_nodeMap[fNode->Filepath()] = fNode;
        }

        for (auto childNode : fNode->Children()) {
            GenerateNodeHashMap(childNode.second);
        }
    }

    FsNode* GetNodeAtPath(const char* path)
    {
        auto fNode = m_nodeMap.find(path);
        if (fNode != m_nodeMap.end()) {
            return fNode->second;
        }
        return NULL;
    }

    FsNode* GetParentNodeOfFilename(const std::string& filename)
    {
        size_t pos_start = 0;
        size_t pos_end = std::string::npos;
        auto currentNode = GetRootNode();

        while ((pos_end = filename.find_first_of(":\\", pos_start)) != std::string::npos) {
            std::string dirname = filename.substr(pos_start, pos_end - pos_start);
            pos_start = pos_end + 1;
            auto folderNodeEntry = currentNode->Children().find(dirname);
            if (folderNodeEntry != currentNode->Children().end()) {
                currentNode = folderNodeEntry->second;
            }
            else {
                currentNode = currentNode->Insert(FsNodeKind::Folder, dirname);
            }
        }

        return currentNode;
    }

    HANDLE GetNodeHandle(FsNode* fNode)
    {
        auto result = m_openFiles.find(fNode->Filepath());
        if (result != m_openFiles.end()) {
            return result->second;
        }
        else {
            if (m_openFiles.size() >= m_openFileLimit) {
                LOG_DEBUG << "Open files limit reached (" << m_openFileLimit << "). Closing first half..";
                for (auto it = m_openFiles.cbegin(); it != m_openFiles.end();) {
                    if (m_openFiles.size() > m_openFileLimit / 2) {
                        LOG_VERBOSE << "Closing: " << it->first;
                        CascCloseFile(it->second);
                        it = m_openFiles.erase(it);
                    }
                    else {
                        // ++it;
                        break;
                    }
                }
            }

            HANDLE hFile;
            if (!CascOpenFile(m_hStorage, fNode->ckeyEntry->CKey, CASC_LOCALE_ALL, CASC_OPEN_BY_CKEY, &hFile)) {
                LOG_ERROR << "Couldn't open file " << fNode->Filepath();
                return NULL;
            }
            m_openFiles[fNode->Filepath()] = hFile;

            return hFile;
        }
    }
};

FsTree cfFileTree;

static int cascfs_getattr(const char *path, FUSE_STAT *stbuf)
{
    LOG_VERBOSE << path;
    int res = 0;
    memset(stbuf, 0, sizeof(*stbuf));

    auto fNode = cfFileTree.GetNodeAtPath(path);
    if (fNode != NULL) {
        switch (fNode->m_kind) {
            case FsNodeKind::Root:
            case FsNodeKind::Folder:
            {
                stbuf->st_mode = S_IFDIR | 0554;
                stbuf->st_nlink = 2;
                stbuf->st_size = 0;
                break;
            }

            case FsNodeKind::File:
            {
                stbuf->st_mode = S_IFREG | 0554;
                stbuf->st_nlink = 1;

                stbuf->st_size = fNode->ckeyEntry->ContentSize;
                break;
            }

            default:
            {
                res = -ENOENT;
                break;
            }
        }
    }
    else {
        res = -ENOENT;
    }

    return res;
}

static int cascfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, FUSE_OFF_T offset, struct fuse_file_info *fi)
{
    LOG_VERBOSE << path;

    auto fNode = cfFileTree.GetNodeAtPath(path);
    if (fNode == NULL || fNode->m_kind == FsNodeKind::File) {
        return -ENOENT;
    }

    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);

    for (auto childNode : fNode->Children()) {
        filler(buf, childNode.second->Name().c_str(), NULL, 0);
    }

    return 0;
}

static int cascfs_open(const char *path, struct fuse_file_info *fi)
{
    LOG_VERBOSE << path;

    auto fNode = cfFileTree.GetNodeAtPath(path);
    if (fNode == NULL || fNode->m_kind != FsNodeKind::File) {
        return -ENOENT;
    }

    if((fi->flags & 3) != O_RDONLY)
        return -EACCES;

    return 0;
}

static int cascfs_read(const char *path, char *buf, size_t size, FUSE_OFF_T offset, struct fuse_file_info *fi)
{
    LOG_VERBOSE << path << " at " << offset << " size " << size;

    auto fNode = cfFileTree.GetNodeAtPath(path);
    if (fNode == NULL) {
        return -ENOENT;
    }

    switch (fNode->m_kind) {
        case FsNodeKind::File:
        {
            DWORD readLen;
            auto fHandle = cfFileTree.GetNodeHandle(fNode);
            if (fHandle == NULL) {
                LOG_ERROR << "Failed to open " << fNode->Filepath() << " E" << GetLastError();
                return 0;
            }
            CascSetFilePointer(fHandle, offset, NULL, FILE_BEGIN);
            if (!CascReadFile(cfFileTree.GetNodeHandle(fNode), buf, size, &readLen)) {
                LOG_ERROR << "Failed to read " << fNode->Filepath() << " E" << GetLastError();
                return 0;
            }
            return readLen;
        }

        default:
        {
            return 0;
        }
    }
}

void cascfs_populate(HANDLE hStorage)
{
    auto hs = TCascStorage::IsValid(hStorage);
    cfFileTree.m_hStorage = hStorage;

    LOG_DEBUG << "Building file tree..";

    CASC_FIND_DATA findData;
    HANDLE handle = CascFindFirstFile(hStorage, "*", &findData, NULL);

    if (handle == INVALID_HANDLE_VALUE) {
        PLOG_FATAL << "CascFindFirstFile E(" << GetLastError() << ")";
        exit(GetLastError());
    }

    do {
        if (!findData.bFileAvailable) continue;

        FsNode *folderNode;

        if (findData.NameType == _CASC_NAME_TYPE::CascNameFull) {
            folderNode = cfFileTree.GetParentNodeOfFilename(findData.szFileName);
        }
        else if (findData.NameType == _CASC_NAME_TYPE::CascNameCKey) {
            std::string targetFilepath = "CKEY\\";
            targetFilepath += findData.szFileName;
            folderNode = cfFileTree.GetParentNodeOfFilename(targetFilepath);
        }
        else {
            LOG_WARNING << "findData.bCanOpenByCKey is false for " << findData.szFileName;
            continue;
        }

        auto fileNode = folderNode->Insert(FsNodeKind::File, findData.szPlainName);
        fileNode->ckeyEntry = FindCKeyEntry_CKey(hs, findData.CKey);
    } while (CascFindNextFile(handle, &findData));
    CascFindClose(handle);

    LOG_DEBUG << "Generating indexes..";
    cfFileTree.GenerateNodeHashMap(cfFileTree.GetRootNode());
}

static struct fuse_operations cascf_oper;

int cascfs_mount(const std::string& mountPoint, HANDLE hStorage)
{
    cascf_oper.getattr = cascfs_getattr;
    cascf_oper.open = cascfs_open;
    cascf_oper.read = cascfs_read;
    cascf_oper.readdir = cascfs_readdir;

    cascfs_populate(hStorage);

    LOG_DEBUG << "Preparing to mount..";

#ifndef WIN32
    if (!pathExists(mountPoint)) {
        LOG_FATAL << "Path doesn't exist or is not accessible" << mountPoint;
        return -2;
    }
#endif

    auto fChan = fuse_mount(mountPoint.c_str(), NULL);
    if (fChan != NULL) {
        auto fHandle = fuse_new(fChan, NULL, &cascf_oper, sizeof(cascf_oper), NULL);
        if (fHandle != NULL) {
            LOG_INFO << "cascfs " << static_cast<void*>(fHandle) << " mounted at " << mountPoint;
            struct fuse_session *se = fuse_get_session(fHandle);
            if (fuse_set_signal_handlers(se) == 0) {
                LOG_DEBUG << "Entering CASC-FS loop..";
                fuse_loop(fHandle);
                LOG_DEBUG << "Leaving CASC-FS loop..";

                fuse_remove_signal_handlers(se);
            }

            fuse_unmount(mountPoint.c_str(), fChan);
            fuse_destroy(fHandle);
        }
        else {
            LOG_FATAL << "fuse_new failed " << static_cast<void*>(fHandle);
            return -2;
        }
    }
    else {
        LOG_FATAL << "Couldn't mount to " << mountPoint;
        return -2;
    }

    return 0;
}
