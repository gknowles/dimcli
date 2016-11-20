[![AppVeyor Status](https://ci.appveyor.com/api/projects/status/02i9uq9asqlb6opy/branch/master?svg=true)](https://ci.appveyor.com/project/gknowles/dimcli/branch/master)
# [![Travis Status](https://travis-ci.org/gknowles/dimcli.svg?branch=master)](https://travis-ci.org/gknowles/dimcli)
# dimcli

Making command line interface implementation fun for kids of all ages.

- parses directly to supplied (or implicitly created) variables
- supports parsing to any type that is:
  - default constructable
  - copyable
  - assignable from std::string or has an istream extraction operator
- help page generation
- works with exceptions and RTTI disabled

## Documentation
Check out the [wiki](https://github.com/gknowles/dimcli/wiki), you'll be glad 
you did! Contains thorough documentation including many examples.

## Build
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

## Include in your project
- Copy the includes files
- Link to the library, or just add "cli.cpp" to your project
