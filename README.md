# stormex

Command-line application to enumerate and extract files from [CASC](https://wowdev.wiki/CASC) (Content Addressable Storage Container) used in Blizzard games.

Tested on:

* StarCraft II
* Heroes of The Storm
* Warcraft III Classic & Reforged

## Installation

### Download

* [**Compiled binaries**](https://github.com/Talv/stormex/releases/latest) (currently only available for __Windows__).

### Building on Linux / macOS

```sh
git submodule update --init
cd build && cmake ..
make
```

> Executable will be put in `build/bin/stormex`

### Building on Windows

* Requires `Visual Studio 15 2017 Build Tools`

```sh
git submodule update --init
cd build
cmake -G "Visual Studio 15 2017 Win64" ..
MSBuild STORMEXTRACT.sln /p:Configuration=Release
```

> Executable will be put in `build\bin\Release\stormex.exe`

## Usage

```
Command-line application to enumerate and extract files from CASC (Content Addressable Storage Container) used in Blizzard games.

Regex pattern is expected to follow ECMAScript syntax

Usage:
  stormex [OPTION...] [STORAGE]

 Common options:
  -h, --help     Print help.
  -v, --verbose  Verbose output.
  -q, --quiet    Supresses output entirely.
      --version  Print version.

 Base options:
  -S, --storage [PATH]  Path to directory with CASC.

 List options:
  -l, --list     List files inside CASC.
  -d, --details  Show details about each file - such as its size.

 Filter options:
  -s, --search [SEARCH...]      Search for files using a substring.
      --smart-case              Searches case insensitively if the pattern is
                                all lowercase. Search case sensitively
                                otherwise. (default: true)
  -i, --in-regex [PATTERN...]   Include files matching regex.
  -I, --in-iregex [PATTERN...]  Include files matching regex case
                                insensitively.
  -e, --ex-regex [PATTERN...]   Exclude files matching regex.
  -E, --ex-iregex [PATTERN...]  Exclude files matching regex case
                                insensitively.

 Extract options:
  -x, --extract-all             Extract all files matching search filters.
  -X, --extract-file [FILE...]  Extract file(s) matching exactly.
  -o, --outdir [PATH]           Output directory for extracted files.
                                (default: .)
  -p, --stdout                  Pipe content of a file(s) to stdout instead
                                writing it to the filesystem.
  -P, --progress                Notify about progress during extraction.
  -n, --dry-run                 Simulate extraction process without writing
                                any data to the filesystem.

 Mount options:
  -m, --mount [MOUNTPOINT]  Mount CASC as a filesystem
```

### Examples

#### List content

List based on a search phrase

```sh
stormex '/mnt/s1/BnetGameLib/StarCraft II' -s 'buildid' -l
```

List all files with details and sort by filesize.

```sh
stormex '/mnt/s1/BnetGameLib/StarCraft II' -ld | sort -h
```

#### Extract files based on inclusion and exclusion patterns

```sh
stormex '/mnt/s1/BnetGameLib/StarCraft II' \
  -I '\\(DocumentInfo|Objects|Regions|Triggers)$' \
  -I '\.(fx|xml|txt|json|galaxy|SC2Style|SC2Hotkeys|SC2Lib|TriggerLib|SC2Interface|SC2Locale|SC2Components|SC2Layout)$' \
  -E '(dede|eses|esmx|frfr|itit|kokr|plpl|ptbr|ruru|zhcn|zhtw)\.sc2data' \
  -E '(PreloadAssetDB|TextureReductionValues)\.txt$' \
  -x -o './out'
```

#### Extract to stdout

Extract specific file to `stdout` and pipe the stream to another program. For example convert dds to png and display it with `imagick`.

```sh
stormex -S '/mnt/s1/BnetGameLib/StarCraft II' -X 'mods/core.sc2mod/base.sc2data/EditorData/Images/HeroesEditor_Logo.tga' -p | display tga:
```

```sh
stormex -S '/mnt/s1/BnetGameLib/StarCraft II' -X 'mods/core.sc2mod/base.sc2data/EditorData/Images/EditorLogo.dds' -p | magick dds: png: | display png:
```

#### Mount as FUSE filesystem

```sh
mkdir -p cascfs
stormex -v -S '/mnt/s1/BnetGameLib/StarCraft II' -m ./cascfs
```

Result:

```sh
$ ls -l ./cascfs
dr-xr-xr--   - root  1 Jan  1970 campaigns
dr-xr-xr--   - root  1 Jan  1970 CKEY
.r-xr-xr-- 17M root  1 Jan  1970 DOWNLOAD
.r-xr-xr-- 41M root  1 Jan  1970 ENCODING
.r-xr-xr-- 59k root  1 Jan  1970 INSTALL
dr-xr-xr--   - root  1 Jan  1970 mods
.r-xr-xr-- 20M root  1 Jan  1970 ROOT
dr-xr-xr--   - root  1 Jan  1970 versions.osxarchive
dr-xr-xr--   - root  1 Jan  1970 versions.winarchive
```

##### Windows support via Dokany

[Dokany](https://github.com/dokan-dev) provides a FUSE wrapper for Windows. You've to install [Dokany's system driver](https://github.com/dokan-dev/dokany/wiki/Installation) in order for this feature to work.

As `[MOUNTPOINT]` argument provide a free drive letter. In following example CASC will be mounted at `S:\`.

```sh
stormex.exe -v -S X:\shared\SC2.4.8.4.73286 -m S
```

[![](https://i.imgur.com/1y7zCTL.png)](https://i.imgur.com/1y7zCTL.png)

## Credits

* Powered by [CascLib](https://github.com/ladislav-zezula/CascLib)
