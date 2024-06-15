<!--
Copyright Glen Knowles 2016 - 2024.
Distributed under the Boost Software License, Version 1.0.
-->

# dimcli

| Branch | MSVC 2015, 2017, 2019, 2022 / CLANG 6, 9-17 / GCC 7-13 | Test Coverage |
| :----: | :----------------------------------------------------: | :-----------: |
| master | [![Build Status][gh-image-master]][gh-link-master] | [![Coverage][cc-image-master]][cc-link-master] |
| dev    | [![Build Status][gh-image-dev]][gh-link-dev] | [![Coverage][cc-image-dev]][cc-link-dev] |

[gh-image-master]: https://github.com/gknowles/dimcli/actions/workflows/github-build.yml/badge.svg?branch=master "GitHub Actions"
[gh-link-master]: https://github.com/gknowles/dimcli/actions/workflows/github-build.yml?query=branch%3Amaster
[cc-image-master]: https://codecov.io/gh/gknowles/dimcli/branch/master/graph/badge.svg "Codecov"
[cc-link-master]: https://app.codecov.io/gh/gknowles/dimcli/tree/master

[gh-image-dev]: https://github.com/gknowles/dimcli/actions/workflows/github-build.yml/badge.svg?branch=dev "GitHub Actions"
[gh-link-dev]: https://github.com/gknowles/dimcli/actions/workflows/github-build.yml?query=branch%3Adev
[cc-image-dev]: https://codecov.io/gh/gknowles/dimcli/branch/dev/graph/badge.svg "Codecov"
[cc-link-dev]: https://app.codecov.io/gh/gknowles/dimcli/tree/dev

C++ command line parser toolkit for kids of all ages.

- GNU style command lines (-o, --output=FILE, etc.)
- Parses directly to any supplied (or implicitly created) variable that is:
  - Default constructible
  - Copyable
  - Either assignable or constructible from string, has an istream extraction
    operator, or has a specialization of Cli&#58;:Convert::fromString&lt;T>().
- Render help text
- Option definitions can be scattered across multiple files.
- Git style subcommands.
- Response files (requires `<filesystem>` support).
- Convert argv to/from command line with Windows, Posix, or GNU semantics.
- Wordwrap arbitrary paragraphs and simple text tables for console.
- Works whether or not exceptions and RTTI are disabled.
- Distributed under the Boost Software License, Version 1.0.

## Sample Usage

Check out the complete [documentation](https://gknowles.github.io/dimcli/),
contains many examples, and the quick
[reference](https://gknowles.github.io/dimcli/reference.html).

~~~ C++
#include "dimcli/cli.h"
#include <iostream>
#include <string>
using namespace std;

int main(int argc, char * argv[]) {
    Dim::Cli cli;

    // Define option that populates an existing variable.
    int count;
    cli.opt(&count, "c n count", 1).desc("Times to say hello.");

    // Or, define option without referencing an existing variable. The variable
    // to populate is then implicitly allocated and the returned object is used
    // like a smart pointer to access it.
    auto & name = cli.opt<string>("name", "Unknown")
        .desc("Who to greet.");

    // Parse command line.
    if (!cli.parse(argc, argv))
        return cli.printError(cerr);

    // Access the options.
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
  -c, -n, --count=NUM  Times to say hello. (default: 1)
  --name=STRING        Who to greet. (default: Unknown)

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
Get the latest dimcli [release](https://github.com/gknowles/dimcli/releases).

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
  - git submodule update --init
  - md build & cd build
  - cmake .. -G "Visual Studio 14 2015 Win64"
  - cmake . --build
- Test
  - ctest -C Debug
- Visual Studio
  - open dimcli\build\dimcli.sln

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

Other interesting C++ command line parsers:

- [program_options](http://www.boost.org/doc/libs/release/libs/program_options/)
  \- from boost
- [gflags](https://gflags.github.io/gflags/) - from google
- [tclap](http://tclap.sourceforge.net) - header only
- [args](https://github.com/Taywee/args) - single header
- [cxxopts](https://github.com/jarro2783/cxxopts) - single header
- [CLI11](https://github.com/CLIUtils/CLI11) - header only
