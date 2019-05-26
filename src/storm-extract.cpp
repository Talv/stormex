#include "../CascLib/src/CascLib.h"
#include "../include/SimpleOpt.h"

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <unistd.h>
#include <dirent.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <set>
#include <regex>

using namespace std;


// All the global variables
string version = "1.3.0";

struct tSearchResult {
    string strFileName;
    string strFullPath;
    DWORD lFileSize;
};

// Valid options
enum {
    OPT_HELP,
    OPT_VERBOSE,
    OPT_QUIET,
    OPT_EXTRACT,
    OPT_DEST,
    OPT_SEARCH,
    OPT_LOWERCASE,
    OPT_REGEX_INCLUDE,
    OPT_REGEX_EXCLUDE,
    OPT_REGEX_ICASE
};

HANDLE hStorage;
string strSearchPattern = "";
std::vector<regex*> includePatterns;
std::vector<regex*> excludePatterns;
bool icasePatterns = false;
string strSource;
string strDestination = ".";
bool bExtract = false;
bool bVerbose = false;      // Print extra information for logging
bool bQuiet = false;        // Do not print anything.

const CSimpleOpt::SOption COMMAND_LINE_OPTIONS[] = {
    { OPT_HELP,             "-h",               SO_NONE    },
    { OPT_HELP,             "--help",           SO_NONE    },
    { OPT_VERBOSE,          "-v",               SO_NONE    },
    { OPT_VERBOSE,          "--verbose",        SO_NONE    },
    { OPT_QUIET,            "-q",               SO_NONE    },
    { OPT_QUIET,            "--quiet",          SO_NONE    },
    { OPT_EXTRACT,          "-x",               SO_NONE    },
    { OPT_EXTRACT,          "--extract",        SO_NONE    },
    { OPT_DEST,             "-o",               SO_REQ_SEP },
    { OPT_DEST,             "--out",            SO_REQ_SEP },
    //{ OPT_LOWERCASE,        "-c",               SO_NONE    },
    //{ OPT_LOWERCASE,        "--lowercase",      SO_NONE    },
    { OPT_SEARCH,           "-s",               SO_REQ_SEP },
    { OPT_SEARCH,           "--search",         SO_REQ_SEP },
    { OPT_REGEX_ICASE,      "--ignore-case",    SO_NONE    },
    { OPT_REGEX_INCLUDE,    "--include",        SO_REQ_SEP },
    { OPT_REGEX_EXCLUDE,    "--exclude",        SO_REQ_SEP },

    SO_END_OF_OPTIONS
};

/* FUNCTIONS */
void showUsage(const std::string &pathToExecutable) {
    cout << "stormex v" << version << endl
         << "  Usage: " << pathToExecutable << " <PATH> [options]" << endl
         << endl
         << "This program can list and optionally extract files from a CASC storage container." << endl
         << endl
         << "    -h, --help                Display this help" << endl
         << endl
         << "Arguments:" << endl
         << "    <PATH>                    Path to game installation folder" << endl
         << endl
         << "Options:" << endl
         << "  General:" << endl
         << "    -v, --verbose             Prints more information" << endl
         << "    -q, --quiet               Prints nothing, nada, zip" << endl
         << endl
         << "  Common:" << endl
         << "    -s, --search <STRING>     Restrict results to full paths matching STRING" << endl
         << "    --ignore-case             Case-insensitive pattern" << endl
         << "    --include <PATTERN>       Include files matching ECMAScript regex PATTERN" << endl
         << "    --exclude <PATTERN>       Exclude files matching ECMAScript regex PATTERN" << endl
         << endl
         << "  Extract:" << endl
         << "    -x, --extract             Extract the files found" << endl
         << "    -o, --out <PATH>          The folder where the files are extracted (extract only)" << endl
         << endl;
}

// Overloaded echo command.
void echo() {
    if (!bQuiet) {
          cout << endl;
    }
}

void echo(const std::string &output) {
    if (!bQuiet) {
        cout << output;
    }
}

void echo(const int &output) {
    if (!bQuiet) {
        cout << output;
    }
}

// Overloaded verbose command.
void verbose() {
    if (!bQuiet && bVerbose) {
        cout << endl;
    }
}

void verbose(const std::string &output) {
    if (!bQuiet && bVerbose) {
        cout << output;
    }
}

void verbose(const int &output) {
    if (!bQuiet && bVerbose) {
        cout << output;
    }
}

bool searchRegexMulti(const std::string filename, const std::vector<regex*> patterns) {
    for (size_t i = 0; i < patterns.size(); i++) {
        if (regex_search(filename, *patterns[i], regex_constants::match_default)) {
            return true;
        }
    }
    
    return false;
}

void printCount( int count, string description ) {
    if (!bQuiet) {
        std::printf("%c[2K", 27);
        std::cout << "\r  ";
        std::cout.width( 7 );
        std::cout << count << description;
        std::cout << std::flush;
    }
}

void printProgress( int percent, string description ) {
    if (!bQuiet) {
        std::printf("%c[2K", 27);
        std::cout << "\r  ";
        std::cout.width( 6 );
        std::cout << percent << "% " << description;
        std::cout << std::flush;
    }
}

vector<string> searchArchive() {
    // Instantiate variables
    int filesFound = 0;
    vector<string> ret;
    std::set<string> directoryResults;
    std::set<string>::iterator dIter;

    // Let's do dis...
    CASC_FIND_DATA findData;
    HANDLE handle = CascFindFirstFile(hStorage, "*", &findData, NULL);

    // Looper
    if (handle) {
        do {
            tSearchResult r;
            r.strFileName = findData.szPlainName;
            r.strFullPath = findData.szFileName;
            r.lFileSize = findData.dwFileSize;

            if (r.strFullPath.find(strSearchPattern) != std::string::npos) {
                // Apply include/exclude filters
                if (
                    (includePatterns.size() > 0 && !searchRegexMulti(r.strFullPath, includePatterns)) ||
                    (excludePatterns.size() > 0 && searchRegexMulti(r.strFullPath, excludePatterns))
                ) {
                    continue;
                }
                ret.push_back(r.strFullPath);

                if (!bExtract && !bQuiet) {
                    cout << findData.szFileName << endl;
                }
            }
        } while (CascFindNextFile(handle, &findData) && findData.szPlainName);

        CascFindClose(handle);
    }

    return ret;
}

size_t extractFile(string strFullPath) {
    char buffer[0x100000];  // 1MB buffer
    string strDestName = strDestination;

    {
        strDestName += strFullPath;

        size_t offset = strDestName.find("\\");
        while (offset != string::npos)
        {
            strDestName = strDestName.substr(0, offset) + "/" + strDestName.substr(offset + 1);
            offset = strDestName.find("\\");
        }

        offset = strDestName.find_last_of("/");
        if (offset != string::npos)
        {
            string dest = strDestName.substr(0, offset + 1);

            size_t start = dest.find("/", 0);
            while (start != string::npos)
            {
                string dirname = dest.substr(0, start);

                DIR* d = opendir(dirname.c_str());
                if (!d)
                    mkdir(dirname.c_str(), S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
                else
                    closedir(d);

                start = dest.find("/", start + 1);
            }
        }
    }

    HANDLE hFile;
    size_t fileSize = 0;
    if (CascOpenFile(hStorage, strFullPath.c_str(), CASC_LOCALE_ALL, 0, &hFile))
    {
        DWORD read;
        FILE* dest = fopen(strDestName.c_str(), "wb");
        if (dest)
        {
            do {
                if (CascReadFile(hFile, &buffer, 0x100000, &read)) {
                    fileSize += fwrite(&buffer, read, 1, dest);
                }
            } while (read > 0);

            fclose(dest);
        }
        else
        {
            cerr << "NOFILE: (" << errno << ") Failed to extract '" << strFullPath << "' to " << strDestName << endl;
        return 0;
        }
        CascCloseFile(hFile);
    }
    else
    {
        cerr << "NOARCHIVE: (" << errno << ") Failed to extract '" << strFullPath << "' to " << strDestName << endl;
        return 0;
    }
    return fileSize;
}

int main(int argc, char** argv) {

    vector<tSearchResult> searchResults;
    std::set<string> directoryResults;
    std::set<string>::iterator dIter;

    // Parse the command-line parameters
    CSimpleOpt args(argc, argv, COMMAND_LINE_OPTIONS);
    while (args.Next())
    {
        if (args.LastError() == SO_SUCCESS)
        {
            switch (args.OptionId())
            {
                case OPT_HELP:
                    showUsage(argv[0]);
                    return 0;

                case OPT_DEST:
                    strDestination = args.OptionArg();
                    break;

                case OPT_SEARCH:
                    strSearchPattern = args.OptionArg();
                    break;

                // case OPT_LOWERCASE:
                //     bLowerCase = true;
                //     break;

                case OPT_QUIET:
                    bQuiet = true;
                    break;

                case OPT_VERBOSE:
                    bVerbose = true;
                    break;

                case OPT_EXTRACT:
                    bExtract = true;
                    break;

                case OPT_REGEX_ICASE:
                    icasePatterns = true;
                    break;

                case OPT_REGEX_INCLUDE:
                case OPT_REGEX_EXCLUDE:
                    try {
                        regex* p = new regex(
                            args.OptionArg(),
                            (icasePatterns == true ? regex::ECMAScript | regex::icase : regex::ECMAScript)
                        );
                        if (args.OptionId() == OPT_REGEX_INCLUDE) {
                            includePatterns.push_back(p);
                        }
                        else {
                            excludePatterns.push_back(p);
                        }
                    } catch(const std::regex_error& e) {
                        std::cerr << e.what() << ": " << args.OptionArg() << endl;
                        return -1;
                    }
                    break;
            }
        }
        else
        {
            cerr << "Invalid argument: " << args.OptionText() << endl;
            return -1;
        }
    }

    if (!args.FileCount()) {
        cerr << "Missing argument <PATH>" << endl;
        return -1;
    }
    strSource = args.File(0);

    // Remove trailing slashes at the end of the storage path (CascLib doesn't like that)
    if ((strSource[strSource.size() - 1] == '/') || (strSource[strSource.size() - 1] == '\\'))
        strSource = strSource.substr(0, strSource.size() - 1);

    // Open CASC Files
    if (!CascOpenStorage(strSource.c_str(), 0, &hStorage)) {
        cerr << "Failed to open the storage '" << strSource << "'" << endl;
        return -2;
    }

    vector<string> results = searchArchive();
    
    if (!bExtract) {
        verbose();
        echo(results.size());
        echo(" files found.\n");
    }

    // Extraction
    if (bExtract && !results.empty())
    {
        int filesDone = 0;
        int progress;
        echo("Extracting files:\n");

        if (strDestination.at(strDestination.size() - 1) != '/')
            strDestination += "/";

        vector<string>::iterator iter, iterEnd;
        for (iter = results.begin(), iterEnd = results.end(); iter != iterEnd; ++iter)
        {
            filesDone++;
            progress = int(filesDone * 100 / results.size());
            printProgress(progress, iter->c_str());
            size_t bytesWritten = extractFile(iter->c_str());
            if (bytesWritten <= 0) {
                // TODO: report warning? this might mean file is encrypted
            }
            verbose();
        }
        verbose("\n");
        echo("  ");
        echo(filesDone);
        echo(" files extracted.\n");
    }

    CascCloseStorage(hStorage);
    return 0;
}
