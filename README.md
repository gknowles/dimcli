<!--
Copyright Glen Knowles 2016 - 2020.
Distributed under the Boost Software License, Version 1.0.
-->

# dimcli

| MSVC 2015, 2017, 2019 / CLANG 6, 9, 10 / GCC 7, 9, 10 | Test Coverage |
|:-----------------------------------------------------:|:-------------:|
| [![AppVeyor Status](https://ci.appveyor.com/api/projects/status/02i9uq9asqlb6opy/branch/master?svg=true)](https://ci.appveyor.com/project/gknowles/dimcli/branch/master) | [![codecov](https://codecov.io/gh/gknowles/dimcli/branch/master/graph/badge.svg)](https://codecov.io/gh/gknowles/dimcli) |

C++ command line parser toolkit for kids of all ages.

- GNU style command lines (-o, --output=FILE, etc.)
- parses directly to any supplied (or implicitly created) variable that is:
  - default constructible
  - copyable
  - assignable from string, has an istream extraction operator, or has a
    specialization of Cli&#58;:Convert::fromString\<T>()
- help generation
- option definitions can be scattered across multiple files
- git style subcommands
- response files (requires `<filesystem>` support)
- works whether exceptions and RTTI are enabled or disabled
- distributed under the Boost Software License, Version 1.0.

## Documentation
Check out the [documentation](https://gknowles.github.io/dimcli/), you'll be glad
you did! Thorough with many examples.

## Include in Your Project
### Copy source directly into your project
All you need is:
- libs/dimcli/cli.h
- libs/dimcli/cli.cpp

### Using [vcpkg](https://github.com/Microsoft/vcpkg)
- vcpkg install dimcli

### Using cmake
Get the latest snapshot:
[dimcli 5.0.2](https://github.com/gknowles/dimcli/archive/v5.0.2.zip)

Build it (this example uses Visual C++ 2015 to install a 64-bit build to
c:\dimcli on a windows machine):
- cmake .. -DCMAKE_INSTALL_PREFIX=c:\dimcli -G "Visual Studio 14 2015 Win64"
- cmake --build .
- ctest -C Debug
- cmake --build . --target install

## Working on the dimcli Project
- Prerequisites
  - install cmake >= 3.6
  - install Visual Studio >= 2015
    - include the "Github Extension for Visual Studio" (if you care)
    - include git
- Make the library
  - git clone https://github.com/gknowles/dimcli.git
  - cd dimcli
  - md build & cd build
  - cmake .. -G "Visual Studio 14 2015 Win64"
  - cmake . --build
- Test
  - ctest -C Debug
- Visual Studio
  - open dimcli\dimcli.sln (not the one in dimcli\build\dimcli.sln) for github
    integration to work

## Random Thoughts
Why not a single header file?

- On large projects with many binaries (tests, utilities, etc) it's good for
  compile times to move as much stuff out of the headers as you easily can.
- Inflicting <Windows.h> (and to a much lesser extent <termios.h> & <unistd.h>)
  on all clients seems a bridge too far.

Sources of inspiration:

- LLVM's CommandLine module
- click - Python command line interface creation kit
- My own bad experiences

Things that were harder than expected:

- parsing command lines with bash style quoting
- response files - because of the need to transcode UTF-16 on windows
- password prompting - there's no standard way to disable console echo :(
- build system - you can do a lot with cmake, but it's not always easy

Other interesting c++ command line parsers:

- [program_options](http://www.boost.org/doc/libs/release/libs/program_options/)
  \- from boost
- [gflags](https://gflags.github.io/gflags/) - from google
- [tclap](http://tclap.sourceforge.net) - header only
- [args](https://github.com/Taywee/args) - single header
- [cxxopts](https://github.com/jarro2783/cxxopts) - single header
- [CLI11](https://github.com/CLIUtils/CLI11) - header only
