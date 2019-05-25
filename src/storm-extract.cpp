/*****************************************************************************/
/* storm-extract.cpp                         Copyright 2016 Justin J. Novack */
/*---------------------------------------------------------------------------*/
/* list and extract files from the Heroes of the Storm CASC archives         */
/*****************************************************************************/

#if NODE
#include <nan.h>
#endif
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
string version = "1.1.0";

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
    OPT_SRC,
    OPT_DEST,
    OPT_FULLPATH,
    OPT_FILEEXT,
    OPT_FILEPTRN,
    OPT_SEARCH,
    OPT_LOWERCASE,
    OPT_LISTDIRS,
    OPT_REGEX_INCLUDE,
    OPT_REGEX_EXCLUDE,
    OPT_REGEX_ICASE
};

HANDLE hStorage;
string strSearchPattern = "/";
string strFilePattern;
std::vector<string> strFileExts;
std::vector<regex*> includePatterns;
std::vector<regex*> excludePatterns;
bool icasePatterns = false;
string strSource = "/Applications/Heroes of the Storm";
string strDestination = ".";
bool bUseFullPath = true;
// bool bLowerCase = false;
bool bExtract = false;
bool bPattern = false;
bool bFileExt = false;
// bool bDirectories = false;
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
    { OPT_SRC,              "-i",               SO_REQ_SEP },
    { OPT_SRC,              "--in",             SO_REQ_SEP },
    { OPT_DEST,             "-o",               SO_REQ_SEP },
    { OPT_DEST,             "--out",            SO_REQ_SEP },
    //{ OPT_FULLPATH,         "-p",               SO_NONE    },
    //{ OPT_FULLPATH,         "--path",           SO_NONE    },
    //{ OPT_LOWERCASE,        "-c",               SO_NONE    },
    //{ OPT_LOWERCASE,        "--lowercase",      SO_NONE    },
    { OPT_FILEPTRN,         "-f",               SO_REQ_SEP },
    { OPT_FILEPTRN,         "--filename",       SO_REQ_SEP },
    { OPT_FILEEXT,          "-t",               SO_REQ_SEP },
    { OPT_FILEEXT,          "--filetype",       SO_REQ_SEP },
    { OPT_SEARCH,           "-s",               SO_REQ_SEP },
    { OPT_SEARCH,           "--search",         SO_REQ_SEP },
    // { OPT_LISTDIRS,         "-d",               SO_NONE    },
    // { OPT_LISTDIRS,         "--directories",    SO_NONE    },
    { OPT_REGEX_ICASE,      "--ignore-case",    SO_NONE    },
    { OPT_REGEX_INCLUDE,    "--include",        SO_REQ_SEP },
    { OPT_REGEX_EXCLUDE,    "--exclude",        SO_REQ_SEP },

    SO_END_OF_OPTIONS
};

/* FUNCTIONS */
void showUsage(const std::string &pathToExecutable) {
    cout << "storm-extract v" << version << endl
         << "  Usage: " << pathToExecutable << " [options]" << endl
         << endl
         << "This program can list and optionally extract files from a Heroes of the Storm CASC storage container." << endl
         << endl
         << "    -h, --help                Display this help" << endl
         << endl
         << "Options:" << endl
         << "  Common:" << endl
         << "    -i, --in <PATH>           Directory where '/HeroesData' is" << endl
         << "                                (default: '/Applications/Heroes of the Storm')" << endl
         << "    -v, --verbose             Prints more information" << endl
         << "    -q, --quiet               Prints nothing, nada, zip" << endl
         << "    -s, --search <STRING>     Restrict results to full paths matching STRING" << endl
         << "    -f, --filename <STRING>   Search for filenames matching STRING" << endl
         << "    -t, --filetype <STRING>   Search for filenames having extension STRING" << endl
         << "    --ignore-case             Case-insensitive pattern" << endl
         << "    --include <PATTERN>       Include files matching regex PATTERN" << endl
         << "    --exclude <PATTERN>       Exclude files matching regex PATTERN" << endl
         << endl
         << "  Search:     storm-extract [options]" << endl
         << endl
         << "  Extract:    storm-extract -x [options]" << endl
         << "    -x, --extract             Extract the files found" << endl
         << "    -o, --out <PATH>          The folder where the files are extracted (extract only)" << endl
         << "                                (default: current working directory)" << endl
         // << "    -p, --path                During extraction, preserve the path hierarchy found" << endl
         // << "                                inside the storage (extract only)" << endl
         // << "    -c, --lowercase           Convert extracted file paths to lowercase (extract only)" <<endl
         // << endl
         // << "  Directory:  storm-extract -d [options]" << endl
         // << "    -d, --directories         Print all directories found" << endl
         << endl
         << "Examples:" << endl
         << endl
         << "  1) List all files in CASC storage container (this will take a while):" << endl
         << endl
         << "       ./storm-extract -i \"/Applications/Heroes of the Storm/\" -f ." << endl
         << endl
         << "  2) Extract a specific file:" << endl
         << endl
         << "       ./storm-extract -i \"/Applications/Heroes of the Storm/\" -f \"path/to/the/file\" -o out -x" << endl
         << endl
         << "  3) Extract specific filenames (preserving heirarchy):" << endl
         << endl
         << "       ./storm-extract -i \"/Applications/Heroes of the Storm/\" -f GameData.xml -o out -x" << endl
         << endl
         << "  4) Extract all English sounds (preserving heirarchy):" << endl
         << endl
         << "       ./storm-extract -i \"/Applications/Heroes of the Storm/\" -s enus -o out -e wav -x" << endl
         << "       ./storm-extract -i \"/Applications/Heroes of the Storm/\" -s enus -o out -e ogg -x" << endl
         << endl
         << "  5) Include and exclude patterns" << endl
         << endl
         << "       ./storm-extract -i 'StarCraft II' -v --ignore-case --include '\.SC2Map/' --exclude '(PreloadAssetDB|TextureReductionValues)\.txt$'" << endl
         << endl
         << "Copyright(c) 2016 Justin J. Novack <https://www.github.com/nydus/storm-extract>" << endl;
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

// Find out if the end of the string matches another string
bool hasExtension(const std::string filename, const std::vector<string> extensions) {
    for (size_t i = 0; i < extensions.size(); i++) {
        // http://stackoverflow.com/questions/874134/find-if-string-ends-with-another-string-in-c
        if (
            // Prevent exception of type std::out_of_range: basic_string because the file extension is greater than the filename
            (filename.length() > extensions[i].length()) &&
            // http://stackoverflow.com/questions/874134/find-if-string-ends-with-another-string-in-c
            (0 == filename.compare(filename.length() - extensions[i].length(), extensions[i].length(), extensions[i]))
        ) {
            return true;
        }
    }

    return false;
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
    if (!bQuiet && bVerbose) {
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
                if (
                    // No file pattern, No file type
                    (!bPattern && !bFileExt) ||
                    // No file pattern, Yes file type AND file type match.
                    (!bPattern && bFileExt && hasExtension(r.strFileName, strFileExts)) ||
                    // File name contains search pattern, No file type
                    (bPattern && r.strFileName.find(strFilePattern) != std::string::npos && !bFileExt) ||
                    // File name contains search pattern AND matches file type
                    (bPattern && r.strFileName.find(strFilePattern) != std::string::npos && bFileExt &&
                        hasExtension(r.strFileName, strFileExts))
                ) {
                    // Apply include/exclude filters
                    if (
                        (includePatterns.size() > 0 && !searchRegexMulti(r.strFullPath, includePatterns)) ||
                        (excludePatterns.size() > 0 && searchRegexMulti(r.strFullPath, excludePatterns))
                    ) {
                        continue;
                    }
                    // if ( bDirectories ) {
                    //     directoryResults.insert(r.strFullPath.substr(0,r.strFullPath.size()-r.strFileName.length()));
                    // } else {
                        ret.push_back(r.strFullPath);
                        // Debug
                        filesFound++;
                        //printCount(filesFound, " matches...");
                        verbose("  - ");
                        verbose(findData.szFileName);
                        verbose();
                    // }
                }
            }
        } while (CascFindNextFile(handle, &findData) && findData.szPlainName);

        CascFindClose(handle);
    }

    // if ( bDirectories && !directoryResults.empty() ) {
    //     for (dIter = directoryResults.begin(); dIter != directoryResults.end(); ++dIter) {
    //         ret.push_back(*dIter);
    //     }
    // }

    return ret;
}

size_t extractFile(string strFullPath) {
    char buffer[0x100000];  // 1MB buffer
    string strDestName = strDestination;

/*
    if (bUseFullPath)
    {
        // if (bLowerCase){
        //     transform(iter->strFullPath.begin(), iter->strFullPath.end(), iter->strFullPath.begin(), ::tolower);
        // }
*/
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
/*
    } else {
        // if (bLowerCase){
        //     transform(strFileName.begin(), strFileName.end(), strFileName.begin(), ::tolower);
        // }

        strDestName += strFileName;
    }
*/

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

/** main()
 */

int main(int argc, char** argv) {

    // TODO MOVE THESE
    int filesFound = 0;
    int filesDone = 0;

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

                case OPT_SRC:
                    strSource = args.OptionArg();
                    break;

                case OPT_DEST:
                    strDestination = args.OptionArg();
                    break;

                case OPT_SEARCH:
                    strSearchPattern = args.OptionArg();
                    break;

                case OPT_FILEPTRN:
                    bPattern = true;
                    strFilePattern = args.OptionArg();
                    break;

                case OPT_FILEEXT:
                    bFileExt = true;
                    strFileExts.push_back(args.OptionArg());
                    break;

                case OPT_FULLPATH:
                    bUseFullPath = true;
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

                // case OPT_LISTDIRS:
                //     bDirectories = true;
                //     break;

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

    // Remove trailing slashes at the end of the storage path (CascLib doesn't like that)
    if ((strSource[strSource.size() - 1] == '/') || (strSource[strSource.size() - 1] == '\\'))
        strSource = strSource.substr(0, strSource.size() - 1);

    // Open CASC Files
    if (!CascOpenStorage(strSource.c_str(), 0, &hStorage)) {
        cerr << "Failed to open the storage '" << strSource << "'" << endl;
        return -2;
    }

    // Explain what we want to do
    echo("Searching for files: \n");
    verbose("  * full paths matching '" + strSearchPattern + "'\n");
    if (bPattern) {
        verbose("  * filenames matching '" + strFilePattern + "'\n");
    }
    for (size_t i = 0; i < strFileExts.size(); i++) {
        verbose("  * extensions matching '" + strFileExts[i] + "'\n");
    }
    if (bFileExt || bPattern) {
        verbose();
    }

    // Search
    vector<string> results = searchArchive();
    filesFound = results.size();
    echo("  ");
    echo(filesFound);
    echo(" files found.\n");

    // if ( bDirectories ) {
    //    for (dIter=directoryResults.begin(); dIter!=directoryResults.end(); ++dIter)
    //        std::cout << ' ' << *dIter << endl;
    //    std::cout << '\n';
    // }

    // Extraction
    if (bExtract && !results.empty())
    {
        verbose("\n");
        int progress;
        echo("Extracting files:\n");

        if (strDestination.at(strDestination.size() - 1) != '/')
            strDestination += "/";

        vector<string>::iterator iter, iterEnd;
        for (iter = results.begin(), iterEnd = results.end(); iter != iterEnd; ++iter)
        {
            progress = int(filesDone * 100 / filesFound);
            printProgress(progress, iter->c_str());
            size_t bytesWritten = extractFile(iter->c_str());
            if (bytesWritten > 0) {
              filesDone++;
            }
            verbose(" ...done!\n");
        }
        verbose("\n");
        echo("  ");
        echo(filesDone);
        echo(" files extracted.\n");
    }

    CascCloseStorage(hStorage);
    echo();
    return 0;
}

#if NODE
/************************************/
/*******  NodeJS Functions  *********/
/************************************/

/* Get version
 *
 * Get the version of the application based on hard-coded value above.
 * @return (string) Version
 */
void nodeGetVersion(const Nan::FunctionCallbackInfo<v8::Value> &args) {
    // Allocate a new scope when we create v8 JavaScript objects.
    v8::Isolate *isolate = args.GetIsolate();
    Nan::HandleScope scope;

    // Convert to v8 String
    v8::Handle<v8::String> result = v8::String::NewFromUtf8(isolate, version.c_str());

    // Ship it out...
    args.GetReturnValue().Set(result);
    return;
}


/* List all files in a CASC archive.
 *
 * @param (string) Source directory of CASC files
 * @return (array) Full paths of files in the archive
 */
void nodeListFiles(const Nan::FunctionCallbackInfo<v8::Value> &args) {
    // Set API variables
    bQuiet = true;
    bVerbose = false;

    // Allocate a new scope when we create v8 JavaScript objects.
    v8::Isolate *isolate = args.GetIsolate();
    Nan::HandleScope scope;

    strSource = *v8::String::Utf8Value(args[0]->ToString());

    if ((strSource[strSource.size() - 1] == '/') || (strSource[strSource.size() - 1] == '\\'))
        strSource = strSource.substr(0, strSource.size() - 1);

    if (!CascOpenStorage(strSource.c_str(), 0, &hStorage)) {
        cerr << "Failed to open the storage '" << strSource << "'" << endl;
        return;
    }

    // Define variables
    CASC_FIND_DATA findData;
    HANDLE handle = CascFindFirstFile(hStorage, "*", &findData, NULL);

    // Let's get this party started..
    vector<string> results = searchArchive();

    // Convert returned vector of *chars to a v8::Array of v8::Strings
    v8::Handle<v8::Array> files = v8::Array::New(isolate);
    for (unsigned int i = 0; i < results.size(); i++ ) {
      v8::Handle<v8::String> result = v8::String::NewFromUtf8(isolate, results[i].c_str());
      files->Set(i, result);
    }

    // Clean it up...
    CascFindClose(handle);
    CascCloseStorage(hStorage);
    handle = NULL;
    hStorage = NULL;

    // Ship it out...
    args.GetReturnValue().Set(files);
    return;
}

/* Extract files into directory.
 *
 * Extract an array of files (args[0]) into a directory of choice (args[1]).
 * @param (string) Source directory of CASC archive
 * @param (string) Destination directory to extract files
 * @param (array) Array of files within the CASC archive
 * @return (int) Number of files successfully extracted.
 */
void nodeExtractFiles(const Nan::FunctionCallbackInfo<v8::Value> &args) {
    // Allocate a new scope when we create v8 JavaScript objects.
    v8::Isolate *isolate = args.GetIsolate();
    Nan::HandleScope scope;

    // strSource
    strSource = *v8::String::Utf8Value(args[0]->ToString());
    if ((strSource[strSource.size() - 1] == '/') || (strSource[strSource.size() - 1] == '\\'))
        strSource = strSource.substr(0, strSource.size() - 1);

    // strDestination
    strDestination = *v8::String::Utf8Value(args[1]->ToString());
    if (strDestination.at(strDestination.size() - 1) != '/')
        strDestination += "/";

    // Open CASC archive
    if (!CascOpenStorage(strSource.c_str(), 0, &hStorage)) {
        cerr << "Failed to open the storage '" << strSource << "'" << endl;
        args.GetReturnValue().Set(-1);
        return;
    }

    int filesDone = 0;

    if (args[2]->IsArray()) {
        v8::Handle<v8::Array> files = v8::Handle<v8::Array>::Cast(args[2]);
        for (uint32_t i = 0; i < files->Length(); i++) {
            v8::String::Utf8Value item(files->Get(i)->ToString());
            std::basic_string<char> file = std::string(*item);
            size_t bytesWritten = extractFile(file);
            if (bytesWritten > 0) {
              filesDone++;
            }
        }
    }

    // Clean it up...
    CascCloseStorage(hStorage);
    hStorage = NULL;

    // Ship it out...
    args.GetReturnValue().Set(filesDone);
    return;
}

/* Initialize and Register to Node */
void init(v8::Handle<v8::Object> exports) {
    Nan::Export(exports, "listFiles", nodeListFiles);
    Nan::Export(exports, "extractFiles", nodeExtractFiles);
    Nan::Export(exports, "getVersion", nodeGetVersion);
}

NODE_MODULE(StormExtractLib, init);
#endif
