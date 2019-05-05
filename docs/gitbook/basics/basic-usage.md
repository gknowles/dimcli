# Basic Usage

After inspecting args cli.parse\(\) returns false if it thinks the program should exit, in which case cli.exitCode\(\) is either EX\_OK \(0\) or EX\_USAGE \(64\) for early exit \(like --help\) or bad arguments respectively. Otherwise the command line was valid, arguments have been parsed, and cli.exitCode\(\) is EX\_OK.

```cpp
#include "dimcli/cli.h"
#include <iostream>
#include <sysexits.h> // if you want the unix exit code macros (EX_*)
using namespace std;

int main(int argc, char * argv[]) {
    Dim::Cli cli;
    if (!cli.parse(cerr, argc, argv))
        return cli.exitCode();
    cout << "Does the apple have a worm? No!";
    return EX_OK;
}
```

And what it looks like:

```text
$ a.out --help
Usage: a.out [OPTIONS]

Options:
  --help    Show this message and exit.

$ a.out
Does the apple have a worm? No!
```

The EX\_\* constants \(along with standard values\) are in sysexits.h on most unixes, although it may not be in any standard. Equivalent enum values Dim\::kExitOk \(0\) and Dim\::kExitUsage \(64\) are defined, which can be useful on Windows where \ doesn't exist.

