// util.h - dim core
#pragma once

#include "config.h"

#include <sstream>

namespace Dim {


/****************************************************************************
*
*   String conversions
*
***/

//===========================================================================
// stringTo - converts from string to T
//===========================================================================
template <typename T>
auto stringTo_impl(T & out, const std::string & src, int)
    -> decltype(out = src, bool) {
    out = src;
    return true;
}

//===========================================================================
template <typename T>
bool stringTo_impl(T & out, const std::string & src, long) {
    std::stringstream interpreter;
    if (!(interpreter << src) || !(interpreter >> out) ||
        !(interpreter >> std::ws).eof()) {
        out = {};
        return false;
    }
    return true;
}

//===========================================================================
template <typename T> bool stringTo(T & out, const std::string & src) {
    // prefer the version of stringTo_impl taking an int as it's third
    // parameter, if that doesn't exist for T (because no out=src assignment
    // operator exists) the version taking a long is called.
    return stringTo_impl(out, src, 0);
}

//===========================================================================
} // namespace
