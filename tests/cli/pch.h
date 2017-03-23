// Copyright Glen Knowles 2016 - 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// pch.h - dim test cli

#define DIM_LIB_KEEP_MACROS
#include "dimcli/cli.h"

#include <cstdlib>
#include <cstring>
#include <experimental/filesystem>
#include <iostream>

// getenv/putenv trigger the visual c++ security warning
#if (_MSC_VER >= 1400)
#pragma warning(disable : 4996) // this function or variable may be unsafe.
#endif
