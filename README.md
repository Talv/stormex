# storm-extract

A tool to list and extract files from the Heroes of the Storm CASC (Content
Addressable Storage Container) archives.

This package is BOTH a command-line application AND a NodeJS add-on module! You
can compile this project as a command-line application if you just need some
basic functionality.  You may also compile this as a NodeJS module if you need
to access the data programmatically.


## Extraction

The decision to only write directly to disk rather than buffer the contents and
return to Node was made early on, for two reasons.

1. The time it takes to find a particular file is rather long (seconds), and
there's little to no practical reason to retrieve a file, close the archive,
and then perform this process over again, when you can just extract all the
files you need during a single opening of the archive and save to disk.

2. Some files tip the scales coming in at over 120+MB, there is no practical
way to pass this file back to Node via a buffer, nor store it within an array
in C. Additionally, there is little point to making exceptions for certain
files.


## Cross-platform Compatability

The NodeJS module should work on all platforms.

The CLI works on MacOS X and Linux.  I have neither the time nor the resources
to build, compile or test on Windows.  Sorry.  Feel free to submit a pull-
request.  A day before I started this project I had no idea how to go about
building C++ applications into node.  I have faith that you can too.


## Dependencies

The following libraries are necessary to build **storm-extract**:

* CascLib (http://www.zezula.net/en/casc/main.html), MIT license
* SimpleOpt 3.4 (http://code.jellycan.com/simpleopt/), MIT License (included)

To download the CascLib submodule, do:

    ~$ cd <path/to/the/source/of/storm-extract>
    storm-extract$ git submodule init
    storm-extract$ git submodule update


## Building

### Command-Line Application

Requires <a href="http://www.cmake.org/">cmake</a> to build:

```sh
cmake .
cmake --build . && make
```

The executable will be put in `bin/storm-extract`

### NodeJS Module

If you already have `node-gyp`, just install the module:

    $ npm install -g storm-extract

Requires <a href="https://www.nodejs.org/>">NodeJS</a> and
<a href="https://www.npmjs.com/package/node-gyp">node-gyp</a> to build:

    $ npm install -g node-gyp
    $ npm install storm-extract


## Usage

### Command-Line Application

```
storm-extract v1.0.1
  Usage: storm-extract [options]

This program can list and optionally extract files from a Heroes of the Storm CASC storage container.

    -h, --help                Display this help

Options:
  Common:
    -i, --in <PATH>           Directory where '/HeroesData' is
                                (default: '/Applications/Heroes of the Storm')
    -v, --verbose             Prints more information
    -q, --quiet               Prints nothing, nada, zip
    -s, --search <STRING>     Restrict results to full paths matching STRING
    -f, --filename <STRING>   Search for filenames matching STRING
    -t, --filetype <STRING>   Search for filenames having extension STRING

  Search:     storm-extract [options]

  Extract:    storm-extract -x [options]
    -x, --extract             Extract the files found
    -o, --out <PATH>          The folder where the files are extracted (extract only)
                                (default: current working directory)

Examples:

  1) List all files in CASC storage container (this will take a while):

       ./storm-extract -i "/Applications/Heroes of the Storm/" -f .

  2) Extract a specific file:

       ./storm-extract -i "/Applications/Heroes of the Storm/" -f "path/to/the/file" -o out

  3) Extract specific filenames (preserving heirarchy):

       ./storm-extract -i "/Applications/Heroes of the Storm/" -f GameData.xml -o out -x

  4) Extract all English sounds (preserving heirarchy):

       ./storm-extract -i "/Applications/Heroes of the Storm/" -s enus -o out -t wav -x
       ./storm-extract -i "/Applications/Heroes of the Storm/" -s enus -o out -t ogg -x

Copyright(c) 2016 Justin J. Novack <https://www.github.com/nydus/storm-extract>
```

### NodeJS Module

    var stormExtract = require('storm-extract');
    var aFiles = stormExtract.listFiles('/Applications/Heroes of the Storm/');
    // console.log(aFiles);

    var files = [
        "mods/heroesdata.stormmod/base.stormdata/GameData.xml",
        "mods/core.stormmod/base.stormdata/GameData.xml"
    ];

    var count = storm.extractFiles('/Applications/Heroes of the Storm/', 'extract', files);
    console.log("Extracted " + count + " files.");

#### Caveats

Unfortunately, this module is entirely synchronous at the moment, it will STALL
your event-loop while it processes.  On my machine, to cycle through the entire
CASC archive takes three (3) seconds; that's three (3) seconds where nothing else
is happening.  You do not want to include this module into, example, a web
server application as it will NOT serve requests for that time.

You should probably run this module in an application that runs in an entirely
different thread, perhaps waiting for data from a socket.

If your application is synchronous (example, some utility application which runs
a process every interval), then there is no need to worry.

#### Bugs

I'm sure, as this is my second C++ to NodeJS module conversion.  Report them to
the [github issue tracker](https://github.com/jnovack/storm-extract/issues).


## Credits

**storm-extract** stands on the shoulders of giants.

The library absolutely, unequivocably, could not be possible without
[ladislav-zezula's CascLib](https://github.com/ladislav-zezula/CascLib)
library.  Many thanks to [ladislav-zezula](https://github.com/ladislav-zezula).

Most of the program was canibalized from
[Kanma's CASCExtractor](https://github.com/Kanma/CASCExtractor/) with the
purpose of customizing it for Heroes of the Storm and integration into
[NodeJS](https://www.nodejs.org).


## License

**storm-extract** is made available under the MIT License. The text of the
license is in the file `LICENSE`.

Under the MIT License you may use **storm-extract** for any purpose you wish,
without warranty, and modify it if you require, subject to one condition:

>   "The above copyright notice and this permission notice shall be included in
>   all copies or substantial portions of the Software."

In practice this means that whenever you distribute your application, whether
as binary or as source code, you must include somewhere in your distribution
the text in the file 'LICENSE'. This might be in the printed documentation, as
a file on delivered media, or even on the credits / acknowledgements of the
runtime application itself; any of those would satisfy the requirement.

Even if the license doesn't require it, please consider to contribute your
modifications back to the community.
