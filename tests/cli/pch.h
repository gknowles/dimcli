// Copyright Glen Knowles 2016 - 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// pch.h - dim test cli

#include "dimcli/cli.h"

#include <cstdlib>
#include <cstring>
#include <iostream>

#if defined(_MSC_VER) && _MSC_VER < 1914
#include <experimental/filesystem>
#define FILESYSTEM std::experimental::filesystem
#elif defined(__has_include)
#if __has_include(<filesystem>)
#include <filesystem>
#define FILESYSTEM std::filesystem
#elif __has_include(<experimental/filesystem>)
#include <experimental/filesystem>
#define FILESYSTEM std::experimental::filesystem
#endif
#endif

// getenv/putenv trigger the visual c++ security warning
#if (_MSC_VER >= 1400)
#pragma warning(disable : 4996) // this function or variable may be unsafe.
#endif
