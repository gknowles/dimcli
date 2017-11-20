<!--
Copyright Glen Knowles 2016 - 2017.
Distributed under the Boost Software License, Version 1.0.
-->

# Change Log
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](http://keepachangelog.com/)
and this project adheres to [Semantic Versioning](http://semver.org/).

## Unreleased
- Changed - Default to user's preferred (instead of global) locale for parsing

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
