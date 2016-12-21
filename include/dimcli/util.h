// util.h - dim core
#pragma once

#include "config.h"

#include <cassert>
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

//===========================================================================
template <typename T>
auto stringTo_impl(T & out, const std::string & src, int, int)
    -> decltype(out = src, bool()) {
    out = src;
    return true;
}

//===========================================================================
template <typename T>
auto stringTo_impl(T & out, const std::string & src, int, long) 
    -> decltype(std::declval<std::stringstream&>() >> out, bool()) {
    std::stringstream interpreter(src);
    if (!(interpreter >> out) || !(interpreter >> std::ws).eof()) {
        out = {};
        return false;
    }
    return true;
}

//===========================================================================
template <typename T>
bool stringTo_impl(T & out, const std::string & src, long, long) {
    // In order to parse an argument there must be one of:
    //  - assignment operator for std::string to T
    //  - istream extraction operator for T
    //  - parse action attached to the Opt<T> instance that doesn't call 
    //    opt.parseValue(), such as opt.choice().
    assert(false && "no assignment from string or stream extraction operator");
    return false;
}

//===========================================================================
template <typename T> bool stringTo(T & out, const std::string & src) {
    // versions of stringTo_impl taking ints as extra parameters are 
    // preferred, if they don't exist for T (because no out=src assignment
    // operator exists) only then are versions taking a long considered.
    return stringTo_impl(out, src, 0, 0);
}

//===========================================================================
} // namespace
