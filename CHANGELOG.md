# Change Log

## [2.1.0] - 2019-07-31

* Added basic support for mounting CASC as filesystem visible to the OS using [FUSE](https://github.com/libfuse/libfuse).
  * Linux will work out of the box.
  * MacOS is likely to work but hasn't been tested.
  * Under Windows app is compiled against `dokanfuse` wrapper library. In order for it to work `Dokany` must be installed on the system.

## [2.0.0] - 2019-07-26

* Refactored entire codebase..
* `CascLib` upgraded to `1.20`:
  * Enumerating MNDX storage is now much faster (SC2 & Storm data)
  * The library now outputs filepaths with backslashes, instead of forwardslashes as it did previously in case of SC2/Storm.
  * Applied [patch](https://github.com/Talv/CascLib/commit/b2646e578b43641a46df5725d951b093a7cefce0) to preserve original case sensitivity of filenames.
* The core functionality remains intact, however some of the existing options/arguments have been renamed and/or reorganized. Hopefully for the better.

## [1.4.0] - 2019-07-22

* Introduced compatibility with Windows

## [1.3.0] - 2019-05-26

* `CascLib` upgraded to [ef66d7bb46f0bb4dd782d3b68eb7dcc358d52a13](https://github.com/ladislav-zezula/CascLib/commit/ef66d7bb46f0bb4dd782d3b68eb7dcc358d52a13).

## [1.1.0] - 2019-05-26

* Forked [storm-extract#216812d7f91ab2ca72b04f2561c587c754273489](https://github.com/nydus/storm-extract/tree/216812d7f91ab2ca72b04f2561c587c754273489).
* Rebranded to `stormex`.
* Removed NodeJS bindings, and everything related.
* Enhanced cli app with regex pattern filters on a filelist.
