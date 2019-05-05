# Overview

C++ command line parser toolkit for kids of all ages.

* GNU style command lines \(-o, --output=FILE, etc.\)
* parses directly to any supplied \(or implicitly created\) variable that is:
  * default constructible
  * copyable
  * assignable from string, has an istream extraction operator, or has a

    specialization of Cli\::fromString\\(\)
* help generation
* option definitions can be scattered across multiple files
* git style subcommands
* response files \(requires `<filesystem>` support\)
* works whether exceptions and RTTI are enabled or disabled

How does it feel?

```cpp
#include "dimcli/cli.h"
#include <iostream>
using namespace std;

int main(int argc, char * argv[]) {
    Dim::Cli cli;
    auto & count = cli.opt<int>("c n count", 1).desc("times to say hello");
    auto & name = cli.opt<string>("name", "Unknown").desc("who to greet");
    if (!cli.parse(cerr, argc, argv))
        return cli.exitCode();
    if (!name)
        cout << "Greeting the unknown." << endl;
    for (unsigned i = 0; i < *count; ++i)
        cout << "Hello " << *name << "!" << endl;
    return 0;
}
```

What it does when run:

```text
$ a.out -x
Error: Unknown option: -x

$ a.out --help
Usage: a.out [OPTIONS]

Options:
  -c, -n, --count=INTEGER  times to say hello (default: 1)
  --name STRING            who to greet (default: Unknown)

  --help                   Show this message and exit.

$ a.out --count=3
Greeting the unknown.
Hello Unknown!
Hello Unknown!
Hello Unknown!

$ a.out --name John
Hello John!
```

