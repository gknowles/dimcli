[![AppVeyor Status](https://ci.appveyor.com/api/projects/status/02i9uq9asqlb6opy/branch/master?svg=true)](https://ci.appveyor.com/project/gknowles/dimcli/branch/master)
# dimcli

C++ command line parser toolkit for kids of all ages.

- can parse directly to any supplied (or implicitly created) variables 
  that are:
  - default constructable
  - copyable
  - assignable from std::string or have an istream extraction operator
- help generation
- options don't have to be defined all in one file
- git style subcommands
- response files
- works with exceptions and/or RTTI enabled or disabled

## Documentation
Check out the [wiki](https://github.com/gknowles/dimcli/wiki), you'll be glad 
you did! Contains thorough documentation with many examples. Or click 
[here](https://github.com/gknowles/dimcli/blob/master/docs/README.md) if you 
prefer it all on a single page.

## Include in Your Project
### Using [vcpkg](https://github.com/Microsoft/vcpkg) on Windows
- vcpkg install dimcli

### Copy source directly into your project
- all you need is cli.h and cli.cpp

### Using cmake
Get the latest snapshot: 
[dimcli 1.0.3](https://https://github.com/gknowles/dimcli/archive/v1.0.3.zip)

Build it (this example uses Visual C++ 2015 to install a 64-bit build to 
c:\dimcli on a windows machine):
- cmake .. -DCMAKE_INSTALL_PREFIX=c:\dimcli -G "Visual Studio 14 2015 Win64"
- cmake --build .
- ctest -C Debug
- cmake --build . --target install

## Working on the dimcli Project
- Prerequisites
  - install cmake >= 3.6
  - install Visual Studio 2015 
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
- Visual Studio 2015
  - open dimcli\dimcli.sln (not the one in dimcli\build\dimcli.sln) for github
    integration to work

# Random Thoughts
Sources of inspiration:

- LLVM's CommandLine module
- click - Python composable command line utility
- My own bad experiences

Things that were harder than expected:

- parsing command lines with bash style quoting
- response files - because of the need to transcode utf-16 on windows
- password prompting - there's no standard way to disable console echo :(
- build system - you can do a lot with cmake, but it's not always easy
