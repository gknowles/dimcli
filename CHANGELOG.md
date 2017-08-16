<!--
Copyright Glen Knowles 2016 - 2017.
Distributed under the Boost Software License, Version 1.0.
-->

# Change Log
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](http://keepachangelog.com/) 
and this project adheres to [Semantic Versioning](http://semver.org/).

## Unreleased
- Added - support for clang >= 3.6
- Added - support for gcc >= 6.2
- Added - show default option values in help text
- Added - Cli() copy constructor
- Fixed - cli.toPtrArgv() trailing nullptr improperly added
- Changed - rename cli.run() to cli.exec() and invert the return value
- Fixed - internal group name relies on static init order

## dimcli 2.0.0 (2017-02-25)
- Added - optVec.operator[]
- Changed - renamed opt.write*() functions to opt.print*()
- Added - reduced footprint to just two files (cli.h & cli.cpp)
- Added - opt.writeUsageEx() includes option names in usage text
- Fixed - opt.choice() should be usable when no string conversion exists
- Fixed - help text for choices not aligned
- Fixed - option groups sorted by name instead of sort key

## dimcli 1.0.3 (2016-12-03)
First public release
