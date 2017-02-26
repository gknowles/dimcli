# Change Log
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](http://keepachangelog.com/) 
and this project adheres to [Semantic Versioning](http://semver.org/).

## Unreleased
- Added - show default option values in help text

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
