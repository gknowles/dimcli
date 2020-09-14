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
#elif defined(__DEPRECATED)
#undef __DEPRECATED
#include <strstream>
#define __DEPRECATED 1
#endif
