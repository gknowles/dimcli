// Copyright Glen Knowles 2016 - 2021.
// Distributed under the Boost Software License, Version 1.0.
//
// pch.h - dimcli test perf

#include "dimcli/cli.h"

#pragma warning(disable : \
    4100    /* unreferenced formal parameter */ \
    4456    /* declaration hides previous local declaration */ \
    4458    /* declaration hides class member */ \
)
#include "args.h"

#include <iostream>
#include <chrono>

#undef NDEBUG
#include <cassert>

// getenv/putenv trigger the visual c++ security warning
#if (_MSC_VER >= 1400)
#pragma warning(disable : \
    4189    /* local variable is initialized but not referenced */ \
    4996    /* this function or variable may be unsafe. */ \
)
#endif
