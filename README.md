<!--
Copyright Glen Knowles 2016 - 2021.
Distributed under the Boost Software License, Version 1.0.
-->

# dimcli

| MSVC 2015, 2017, 2019 / CLANG 6, 10, 11 / GCC 7, 10, 11 | Test Coverage |
| :-----------------------------------------------------: | :-----------: |
| [![AppVeyor][av-image]][av-link] | [![Codecov][cc-image]][cc-link] |

[av-image]: https://ci.appveyor.com/api/projects/status/02i9uq9asqlb6opy/branch/master?svg=true "AppVeyor"
[av-link]: https://ci.appveyor.com/project/gknowles/dimcli/branch/master
[cc-image]: https://codecov.io/gh/gknowles/dimcli/branch/master/graph/badge.svg "Codecov"
[cc-link]: https://codecov.io/gh/gknowles/dimcli

C++ command line parser toolkit for kids of all ages.

- GNU style command lines (-o, --output=FILE, etc.)
- Parses directly to any supplied (or implicitly created) variable that is:
  - Default constructible
  - Copyable
  - Either assignable from string, constructible from string, has an istream
    extraction operator, or has a specialization of
    Cli&#58;:Convert::fromString&lt;T>().
- Help generation.
- Option definitions can be scattered across multiple files.
- Git style subcommands.
- Response files (requires `<filesystem>` support).
- Works whether or not exceptions and RTTI are disabled.
- Distributed under the Boost Software License, Version 1.0.

## Sample Usage

Check out the complete [documentation](https://gknowles.github.io/dimcli/),
you'll be glad you did! Thorough with many examples.

~~~ C++
#include "dimcli/cli.h"
#include <iostream>
using namespace std;

int main(int argc, char * argv[]) {
    Dim::Cli cli;
    // Populate existing variable
    int count;
    cli.opt(&count, "c n count", 1).desc("times to say hello");
    // Allocate option variable, returned object acts like a smart
    // pointer to underlying variable.
    auto & name = cli.opt<string>("name", "Unknown").desc("who to greet");
    if (!cli.parse(argc, argv))
        return cli.printError(cerr);
    if (!name)
        cout << "Greeting the unknown." << endl;
    for (int i = 0; i < count; ++i)
        cout << "Hello " << *name << "!" << endl;
    return 0;
}
~~~

What it does when run:

~~~ console
$ a.out -x
Error: Unknown option: -x

$ a.out --help
Usage: a.out [OPTIONS]

Options:
  -c, -n, --count=NUM  times to say hello (default: 1)
  --name=STRING        who to greet (default: Unknown)

  --help               Show this message and exit.

$ a.out --count=2
Greeting the unknown.
Hello Unknown!
Hello Unknown!
$ a.out --name John
Hello John!
~~~

## Include in Your Project
### Copy source directly into your project
All you need is:
- libs/dimcli/cli.h
- libs/dimcli/cli.cpp

### Using [vcpkg](https://github.com/Microsoft/vcpkg)
- vcpkg install dimcli

### Using cmake
Get the latest snapshot:
[dimcli 6.0.0](https://github.com/gknowles/dimcli/archive/v6.0.0.zip)

Build it (this example uses Visual C++ 2015 to install a 64-bit build to
c:\dimcli on a windows machine):
- md build & cd build
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
- Make the library (assuming VS 2015)
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

- LLVM CommandLine module
- click - Python command line interface creation kit
- My own bad experiences

Things that were harder than expected:

- Parsing command lines with bash style quoting
- Response files - because of the need to transcode UTF-16 on Windows
- Password prompting - there's no standard way to disable console echo :(
- Build system - you can do a lot with CMake, but it's not always easy

Other interesting c++ command line parsers:

- [program_options](http://www.boost.org/doc/libs/release/libs/program_options/)
  \- from boost
- [gflags](https://gflags.github.io/gflags/) - from google
- [tclap](http://tclap.sourceforge.net) - header only
- [args](https://github.com/Taywee/args) - single header
- [cxxopts](https://github.com/jarro2783/cxxopts) - single header
- [CLI11](https://github.com/CLIUtils/CLI11) - header only
