# stormex

Command-line application to enumerate and extract files from [CASC](https://wowdev.wiki/CASC) (Content Addressable Storage Container) used in Blizzard games.

Tested on:

* StarCraft II
* Heroes of The Storm

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
                                (default: ./)
  -p, --stdout                  Pipe content of a file(s) to stdout instead
                                writing it to the filesystem.
  -P, --progress                Notify about progress during extraction.
  -n, --dry-run                 Simulate extraction process without writing
                                any data to the filesystem.
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
  -I '\/(DocumentInfo|Objects|Regions|Triggers)$' \
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

## Credits

* Powered by [CascLib](https://github.com/ladislav-zezula/CascLib)
