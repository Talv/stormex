# stormex

Command-line application to list and extract files from the [CASC](https://wowdev.wiki/CASC) (Content
Addressable Storage Container) archives used in Blizzard games.

Tested on:

* StarCraft II
* Heroes of The Storm

## Building

Requires [cmake](http://www.cmake.org/) to build.

```sh
git submodule init
git submodule update
cd build && cmake ..
make
```

The executable will be put in `build/bin/stormex`

## Usage

```
stormex v1.1.0
  Usage: stormex [options]

This program can list and optionally extract files from a CASC storage container.

    -h, --help                Display this help

Options:
  Common:
    -i, --in <PATH>           Path to game installation folder
    -v, --verbose             Prints more information
    -q, --quiet               Prints nothing, nada, zip
    -s, --search <STRING>     Restrict results to full paths matching STRING
    --ignore-case             Case-insensitive pattern
    --include <PATTERN>       Include files matching ECMAScript regex PATTERN
    --exclude <PATTERN>       Exclude files matching ECMAScript regex PATTERN

  Extract:
    -x, --extract             Extract the files found
    -o, --out <PATH>          The folder where the files are extracted (extract only)
                                (default: current working directory)
```

### Examples

#### List content of CASC based on regex pattern

```sh
stormex -i '/mnt/s1/BnetGameLib/StarCraft II' --include '/BuildId.txt$'`
```

#### Extract content of CASC based on inclusion and exclusion patterns

```sh
stormex -i '/mnt/s1/BnetGameLib/StarCraft II' --ignore-case -v \
  --include '\/(DocumentInfo|Objects|Regions|Triggers)$' \
  --include '\.(fx|xml|txt|json|galaxy|SC2Style|SC2Hotkeys|SC2Lib|TriggerLib|SC2Interface|SC2Locale|SC2Components|SC2Layout)$' \
  --exclude '(dede|eses|esmx|frfr|itit|kokr|plpl|ptbr|ruru|zhcn|zhtw)\.sc2data' \
  --exclude '(PreloadAssetDB|TextureReductionValues)\.txt$' \
  -x -o './out'
```

## Credits

The library absolutely, unequivocably, could not be possible without
[ladislav-zezula's CascLib](https://github.com/ladislav-zezula/CascLib)
library. Many thanks to [ladislav-zezula](https://github.com/ladislav-zezula).

~~Most of the program was canibalized from
[Kanma's CASCExtractor](https://github.com/Kanma/CASCExtractor/) with the
purpose of customizing it for Heroes of the Storm and integration into
[NodeJS](https://www.nodejs.org).~~

Most of the program was canibalized from [storm-extract](https://github.com/nydus/storm-extract) with the purpose of reducing code complexity by removing NodeJS stuff. And transitioning it into more streamlined cli app.
