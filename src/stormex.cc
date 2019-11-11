#include <iostream>
#include <string>
#include <vector>
#include <exception>
#include <stdlib.h>
#include <stdio.h>
#include <fstream>
#include <algorithm>

#include "cxxopts.hpp"
#include "common.hpp"
#include "util.hpp"
#include "storage.hpp"
#include "cascfuse.hpp"
#include "common/Common.h"

class StormexContext {
public:
    struct {
        std::string storageSrc;
        std::string listfileSrc;
    } m_base;

    struct {
        bool listFiles;
        bool showDetails;
    } m_list;

    struct {
        std::vector<std::string> searchPhrase;
        bool searchSmartCast;
        std::vector<std::regex> includePatterns;
        std::vector<std::regex> excludePatterns;
    } m_filters;

    struct {
        bool doExtractAll;
        std::vector<std::string> xFilenames;
        std::string outDir;
        bool stdOut;
        bool progress;
        bool dryRun;
    } m_extract;

    struct {
        std::string mountPoint;
    } m_mount;

    void scanExtraArgs(cxxopts::ParseResult pResult)
    {
        if (pResult.count("in-regex")) {
            parseRegex(m_filters.includePatterns, pResult["in-regex"].as<std::vector<std::string>>(), false);
        }
        if (pResult.count("in-iregex")) {
            parseRegex(m_filters.includePatterns, pResult["in-iregex"].as<std::vector<std::string>>(), true);
        }
        if (pResult.count("ex-regex")) {
            parseRegex(m_filters.excludePatterns, pResult["ex-regex"].as<std::vector<std::string>>(), false);
        }
        if (pResult.count("ex-iregex")) {
            parseRegex(m_filters.excludePatterns, pResult["ex-iregex"].as<std::vector<std::string>>(), true);
        }
    }

private:
    void parseRegex(std::vector<std::regex>& patterns, const std::string input, bool icase)
    {
        try {
            patterns.push_back(std::regex(input, (icase == true ? std::regex::ECMAScript | std::regex::icase : std::regex::ECMAScript)));
        } catch (const std::regex_error& e) {
            std::cerr << e.what() << ": " << input << std::endl;
            exit(-1);
        }
    }

    void parseRegex(std::vector<std::regex>& patterns, const std::vector<std::string> input, bool icase)
    {
        for (const auto& value : input) {
            parseRegex(patterns, value, icase);
        }
    }
};

StormexContext appCtx;

void parseArguments(int argc, char* argv[])
{
    try {
        cxxopts::Options options(argv[0],
            "stormex v" + stormexVersion + "\n"
            "\n"
            "Command-line application to enumerate and extract files from CASC (Content Addressable Storage Container) used in Blizzard games.\n"
            "\n"
            "Regex pattern is expected to follow ECMAScript syntax\n");
        options
            .positional_help("[STORAGE]")
            .show_positional_help();

        options.add_options("Common")
            ("h,help", "Print help.")
            ("v,verbose", "Verbose output.", cxxopts::value<bool>())
            ("q,quiet", "Supresses output entirely.", cxxopts::value<bool>())
            ("version", "Print version.");

        options.add_options("Base")
            ("S,storage", "Path to directory with CASC.", cxxopts::value<std::string>(appCtx.m_base.storageSrc), "[PATH]");
            // ("L,listfile",
            //     "Map filenames from provided newline delimeted (LF or CRLF) textfile, instead of enumerating content of the archive, "
            //     "which is an extensive operation. It combines well when extracting single files, or a small group that matches given substring or regex pattern.", cxxopts::value<std::string>(appCtx.m_base.listfileSrc), "[FILE]");

        options.add_options("List")
            ("l,list", "List files inside CASC.", cxxopts::value<bool>(appCtx.m_list.listFiles))
            ("d,details", "Show details about each file - such as its size.", cxxopts::value<bool>(appCtx.m_list.showDetails));

        options.add_options("Filter")
            ("s,search", "Search for files using a substring.", cxxopts::value<std::vector<std::string>>(appCtx.m_filters.searchPhrase), "[SEARCH...]")
            ("smart-case",
                "Searches case insensitively if the pattern is all lowercase. Search case sensitively otherwise.",
                cxxopts::value<bool>(appCtx.m_filters.searchSmartCast)->default_value("true"))
            ("i,in-regex", "Include files matching regex.", cxxopts::value<std::vector<std::string>>(), "[PATTERN...]")
            ("I,in-iregex", "Include files matching regex case insensitively.", cxxopts::value<std::vector<std::string>>(), "[PATTERN...]")
            ("e,ex-regex", "Exclude files matching regex.", cxxopts::value<std::vector<std::string>>(), "[PATTERN...]")
            ("E,ex-iregex", "Exclude files matching regex case insensitively.", cxxopts::value<std::vector<std::string>>(), "[PATTERN...]");

        options.add_options("Extract")
            ("x,extract-all",
                "Extract all files matching search filters.",
                cxxopts::value<bool>(appCtx.m_extract.doExtractAll))
            ("X,extract-file",
                "Extract file(s) matching exactly.",
                cxxopts::value<std::vector<std::string>>(appCtx.m_extract.xFilenames), "[FILE...]")
            ("o,outdir", "Output directory for extracted files.", cxxopts::value<std::string>(appCtx.m_extract.outDir)->default_value("."), "[PATH]")
            ("p,stdout", "Pipe content of a file(s) to stdout instead writing it to the filesystem.", cxxopts::value<bool>(appCtx.m_extract.stdOut))
            ("P,progress", "Notify about progress during extraction.", cxxopts::value<bool>(appCtx.m_extract.progress))
            ("n,dry-run", "Simulate extraction process without writing any data to the filesystem.", cxxopts::value<bool>(appCtx.m_extract.dryRun));

        options.add_options("Mount")
            ("m,mount",
                "Mount CASC as a filesystem", cxxopts::value<std::string>(appCtx.m_mount.mountPoint), "[MOUNTPOINT]");

        options.parse_positional({"storage"});

        auto result = options.parse(argc, argv);

        if (result.count("help")) {
            std::cerr << options.help({ "Common", "Base", "List", "Filter", "Extract", "Mount" }) << std::endl;
            exit(0);
        }

        if (result.count("version")) {
            std::cerr << "stormex v" << stormexVersion << " | CascLib v" << CASCLIB_VERSION_STRING << std::endl;
            exit(0);
        }

        // logging verbosity
        plog::Severity level = plog::Severity::warning;
        if (result.count("verbose")) {
            auto vLvl = plog::Severity::verbose - plog::Severity::warning;
            if (result.count("verbose") > vLvl) {
                std::cerr << "Maximum level of logging verbosity has been reached [" << plog::Severity::debug << "]" << std::endl;
                exit(1);
            }
            level = static_cast<plog::Severity>(plog::Severity::warning + result.count("verbose"));
        }
        else if (result.count("quiet")) {
            plog::Severity level = plog::Severity::none;
        }
        plog::init(level, &consoleAppender);

        // validate required arguments
        if (!result.count("storage")) {
            // TODO: scan positional args
            std::cerr << "missing required argument --storage" << std::endl;
            exit(1);
        }

        appCtx.scanExtraArgs(result);
    } catch (const cxxopts::OptionException& e) {
        std::cerr << "error parsing options: " << e.what() << std::endl;
        exit(1);
    }
}

void extractFilenames(StorageExplorer& stExplorer, const std::vector<std::string>& filesToExtract)
{
    PLOG_DEBUG << "Preparing to extract " << filesToExtract.size() << " files..";
    if (appCtx.m_extract.dryRun) {
        PLOG_INFO << "Dry mode is active..";
    }

    if (appCtx.m_extract.stdOut) {
        setvbuf(stdout, NULL, _IONBF, 0);
        for (const auto& storedFilename : filesToExtract) {
            stExplorer.extractFileData(storedFilename, stdout);
        }
    }
    else if (!appCtx.m_extract.outDir.empty()) {
        if (!pathExists(appCtx.m_extract.outDir)) {
            PLOG_FATAL << "Specified output directory doesn't exist or cannot be opened: " << appCtx.m_extract.outDir;
            exit(-3);
        }

        PLOG_DEBUG << "Output directory set to: " << appCtx.m_extract.outDir;

        for (const auto& storedFilename : filesToExtract) {
            std::string targetFile = appCtx.m_extract.outDir;
            if (targetFile.at(targetFile.size() - 1) != '\\' && targetFile.at(targetFile.size() - 1) != '/') {
                targetFile += PATH_SEP_STR;
            }
            targetFile += storedFilename;

            if (appCtx.m_extract.progress) {
                // TODO: display progress
            }

            PLOG_INFO << "Extracting file " << storedFilename;
            size_t fileSize = 0;
            if (!appCtx.m_extract.dryRun) {
                // normalize slashes in the paths received from CASC and force '/'
                std::replace(targetFile.begin(), targetFile.end(), '\\', '/');

                // replace colon with backslash for compatibility purposes
                // internally in CASC, colon is used on directories that act as mount points
                std::replace(targetFile.begin(), targetFile.end(), ':', '/');

                fileSize = stExplorer.extractFileToPath(storedFilename, targetFile);
                PLOG_DEBUG << "Written " << formatFileSize(fileSize) << " to " << targetFile;
            }
            else {
            }
        }
    }
}

bool searchRegexMulti(const std::string& filename, const std::vector<std::regex>& patterns)
{
    for (const auto& current : patterns) {
        if (regex_search(filename, current)) {
            return true;
        }
    }

    return false;
}

std::vector<STORAGE_SEARCH_RESULT*> filterFiles(const std::vector<STORAGE_SEARCH_RESULT*>& inputList)
{
    std::vector<STORAGE_SEARCH_RESULT*> filteredList;

    for (const auto& entry : inputList) {
        if (appCtx.m_filters.searchPhrase.size()) {
            bool c = false;
            for (const auto& needle : appCtx.m_filters.searchPhrase) {
                c = stringFindIC(entry->filename, needle);
                if (c) break;
            }
            if (!c) continue;
        }

        if (appCtx.m_filters.includePatterns.size() && !searchRegexMulti(entry->filename, appCtx.m_filters.includePatterns)) continue;
        if (appCtx.m_filters.excludePatterns.size() && searchRegexMulti(entry->filename, appCtx.m_filters.excludePatterns)) continue;
        filteredList.push_back(entry);
    }

    return filteredList;
}

std::vector<std::string> readListFile(const std::string& filename)
{
    std::vector<std::string> filelist;
    std::ifstream ifs(filename, std::ifstream::in);

    std::string line;
    while (std::getline(ifs, line)) {
        filelist.push_back(line);
    }

    ifs.close();

    return filelist;
}

std::vector<STORAGE_SEARCH_RESULT*> enumerateFiles(StorageExplorer& stExplorer)
{
    PLOG_INFO << "Enumerating all files in storage..";
    std::vector<STORAGE_SEARCH_RESULT*> inputList;
    if (!stExplorer.enumerateFiles(inputList)) {
        return inputList;
    }
    auto filteredList = inputList;

    if (appCtx.m_filters.searchPhrase.size() || appCtx.m_filters.includePatterns.size() || appCtx.m_filters.excludePatterns.size()) {
        PLOG_INFO << "Filtering list..";
        filteredList = filterFiles(inputList);
    }

    PLOG_DEBUG << "list count " << inputList.size() << " : " << filteredList.size();
    return filteredList;
}

int main(int argc, char* argv[])
{
    parseArguments(argc, argv);

    StorageExplorer stExplorer;
    int tmp;

    LOG_DEBUG << "Opening storage..";
    if ((tmp = stExplorer.openStorage(appCtx.m_base.storageSrc)) != 0) {
        PLOG_FATAL << "Failed to open the storage: " << appCtx.m_base.storageSrc << " E(" << tmp << ")";
        exit(-1);
    }
    PLOG_INFO << "Storage opened " << static_cast<void*>(stExplorer.getHandle());

    try {
        if (appCtx.m_mount.mountPoint.length()) {
            return cascfs_mount(appCtx.m_mount.mountPoint, stExplorer.getHandle());
        }

        auto fResults = enumerateFiles(stExplorer);

        if (appCtx.m_list.listFiles) {
            for (const auto& entry : fResults) {
                char keyBuff[MD5_STRING_SIZE + 1];
                std::string tmps;
                if (appCtx.m_list.showDetails) {
                    tmps = formatFileSize(entry->fileSize);
                    std::cout << std::setfill(' ') << std::setw(8) << tmps << "  ";
                    formatBytes(std::cout, entry->CKey, sizeof(entry->CKey), false);
                    std::cout << "  ";
                }
                std::cout << entry->filename;
                std::cout << std::endl;
            }
        }
        else if (appCtx.m_extract.doExtractAll) {
            std::vector<std::string> fList;
            for (const auto& item : fResults) {
                fList.push_back(item->filename);
            }
            extractFilenames(stExplorer, fList);
        }
        else if (appCtx.m_extract.xFilenames.size()) {
            for (auto& item : appCtx.m_extract.xFilenames) {
                // force backslashes regardless of the platform
                // that's the expected output from CASC anyway, and it'll get normalized later
                std::replace(item.begin(), item.end(), '/', '\\');
            }
            extractFilenames(stExplorer, appCtx.m_extract.xFilenames);
        }
    } catch (const std::exception& e) {
        stExplorer.closeStorage();
        PLOG_FATAL << e.what();
        throw;
    }

    return 0;
}
