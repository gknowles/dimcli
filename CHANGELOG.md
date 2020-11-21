
<!--
Copyright Glen Knowles 2016 - 2020.
Distributed under the Boost Software License, Version 1.0.
-->

# Change Log

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](http://keepachangelog.com)
and this project adheres to [Semantic Versioning](http://semver.org).

## Unreleased
- Cosmetic - Converted docs from Markdown to AsciiDoc
- Added - Detail to errors parsing values with units
- Added - Variable argument cli.toArgvL() & cli.toCmdlineL()
- Changed - Moved fromString() & toString() to new Convert class
- Changed - Allowed long named booleans an optional value (e.g. "=true")
- Changed - Rename opt.unspecifiedValue() to opt.implicitValue()
- Added - fromString() support for types constructible from string
- Changed - Allow optional positionals before required ones
- Added - optVec.size(exact) and optVec.size(min, max)
- Removed - Argument 'nargs' of cli.optVec(), use optVec.size(...) instead
- Added - Allow unambiguous positionals before subcommands
- Added - Optionally capture arbitrary arguments of unknown subcommands
- Added - Option to change column at which errors and help text wraps
- Changed - Default text columns reduced on narrow console windows
- Changed - Moved opt.toValueDesc() to cli.valueDesc()
- Fixed - Compile fails with OptVec<bool> due to vector<bool> weirdness

## dimcli 5.0.2 (2020-01-14)
- Fixed - Name conflict with c++20 std::boolean

## dimcli 5.0.1 (2019-08-26)
- Added - Support and automated testing for VS2019

## dimcli 5.0.0 (2019-07-11)
- Added - Configuration option to unconditionally exclude &lt;filesystem>
- Added - Default valueDesc for floating point values ("FLOAT")
- Changed - Renamed cli.runCommand() to cli.commandMatched()
- Added - Optional error detail argument to cli.badUsage()
- Changed - Replaced opt.setValueDesc() with opt.toValueDesc()
- Fixed - Invalid array access using optVec.from() from parse action
- Changed - Default constructed T added to optVec&lt;T> before parse action
- Added - cli.badRange() helper
- Added - opt.siUnits(), opt.timeUnits(), and opt.anyUnits()
- Added - Allow limiting the number of values for vector options
- Changed - Moved fromString() & toString() from Cli to OptBase
- Changed - locale now set per option instead of for entire cli

## dimcli 4.1.0 (2018-12-05)
- Fixed - Order of keys in help text of multi-keyed options non-deterministic
- Fixed - optVec flag values with inverted keys aren't ignored
- Fixed - Nested response files not resolved relative to parent
- Fixed - Not all response files expanded when multiple appear
- Added - Trim spaces around positional argument names
- Fixed - Error detail for options with two choices missing "or"
- Fixed - Subcommand help summary terminated by '.' instead of "[.!?] "
- Added - Allow non-bool options to be inverted ('!' name prefix)
- Fixed - Failure loading wchar_t response files on linux

## dimcli 4.0.1 (2018-10-22)
- Changed - Default to user's preferred (instead of global) locale for parsing
- Changed - Rename kPrompt* flags to fPrompt*
- Added - Allow showing non-bool defaults in prompts
- Added - Conditionally use &lt;filesystem> instead of
          &lt;experimental/filesystem>
- Added - Allow limited operation when &lt;filesystem> not available
- Fixed - "--opt=" should use "" as value instead of the next arg
- Added - Word wrap list of choices in error detail
- Fixed - Parse errors for optVec report an empty string for the opt name
- Added - Command groups for grouping subcommands in help text
- Added - cli.toCmdline() static method

## dimcli 3.1.1 (2017-11-10)
- Added - cli.exec() overloads that both parse() and exec()
- Added - Before actions to adjust expanded args list before option assignment
- Added - cli.helpNoArgs() to print help when command line is empty
- Added - opt.command() and opt.group() access methods
- Added - Relax clamp and range to allow the low and high to be equal

## dimcli 3.0.0 (2017-10-17)
- Added - Support for clang >= 3.6
- Added - Support for gcc >= 6.2
- Added - Show default option values in help text
- Added - Cli() copy constructor
- Fixed - cli.toPtrArgv() trailing nullptr improperly added
- Changed - Rename cli.run() to cli.exec() and invert the return value
- Fixed - Internal group name relies on static init order
- Added - opt.require()
- Fixed - After actions of unselected commands should not be run
- Added - opt.defaultDesc() to modify "(default: )" clause
- Fixed - "No command given." should return kExitUsage
- Changed - Command header and footer default to the top level values
- Changed - cli.print*() functions now non-const
- Added - cli.helpCmd()

## dimcli 2.0.0 (2017-02-25)
- Added - optVec.operator[]
- Changed - Rename opt.write*() functions to opt.print*()
- Added - Reduce footprint to just two files (cli.h & cli.cpp)
- Added - opt.writeUsageEx() includes option names in usage text
- Fixed - opt.choice() should be usable when no string conversion exists
- Fixed - Help text for choices not aligned
- Fixed - Option groups sorted by name instead of sort key

## dimcli 1.0.3 (2016-12-03)
First public release
