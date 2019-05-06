<!--
Copyright Glen Knowles 2016 - 2019.
Distributed under the Boost Software License, Version 1.0.
-->

## Going Your Own Way
If generated help doesn't work for you, you can override the built-in help
with your own.

~~~ cpp
auto & help = cli.opt<bool>("help"); // or maybe "help." to suppress --no-help
if (!cli.parse(cerr, argv, argc))
    return cli.exitCode();
if (*help)
    return printMyHelp();
~~~

This works because the last definition for named options overrides any
previous ones.

Within your help printer you can use the existing functions to do some of the
work:

- cli.printHelp
- cli.printUsage / cli.printUsageEx
- cli.printPositionals
- cli.printOptions
- cli.printCommands
