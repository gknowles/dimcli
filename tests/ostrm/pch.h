// Copyright Glen Knowles 2019.
// Distributed under the Boost Software License, Version 1.0.
//
// pch.h - dimcli test ostrm

#define _SILENCE_CXX17_STRSTREAM_DEPRECATION_WARNING

#include <chrono>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>

#if defined(_MSC_VER)
#include <strstream>
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated"
#include <strstream>
#pragma GCC diagnostic pop
#endif
