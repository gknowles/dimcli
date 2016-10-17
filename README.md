[![Build status](https://ci.appveyor.com/api/projects/status/02i9uq9asqlb6opy/branch/master?svg=true)](https://ci.appveyor.com/project/gknowles/dimcli/branch/master)

# dimcli

Making command line interfaces fun for kids of all ages.

Main features:
- parses directly to c++ variables (or makes proxies for them)
- supports parsing to any type that is:
  - default constructable
  - copyable
  - assignable from std::string or has an istream extraction operator
- help page generation
- light weight

## Documentation
Check out the [wiki](https://github.com/gknowles/dimcli/wiki), you'll be glad 
you did!

## Build
- Prerequisites
  - install cmake >= 3.6
  - install Visual Studio 2015 
    - include the "Github Extension for Visual Studio"
    - include git
- Make the library
  - git clone https://github.com/gknowles/dimcli.git
  - cd dimcli
  - md build & cd build
  - cmake -G "Visual Studio 14 2015 Win64" ..
  - cmake --build .
- Test
  - cd dimcli\build\bin
  - clitest 
    - should print some random stuff ending with "All tests passed"
- Visual Studio 2015
  - open dimcli\dimcli.sln (not the one in dimcli\build\dimcli.sln)

## Include in your project
- Copy the source files
