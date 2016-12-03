// config.h - dim core
#pragma once

// DIMCLI_LIB_STANDALONE: Defines this as standalone library that is not 
// being included as part of the DIM framework.
#define DIMCLI_LIB_STANDALONE

// DIMCLI_LIB_DYN_LINK: Forces all libraries that have separate source, to be 
// linked as dll's rather than static libraries on Microsoft Windows (this 
// macro is used to turn on __declspec(dllimport) modifiers, so that the 
// compiler knows which symbols to look for in a dll rather than in a static 
// library). Note that there may be some libraries that can only be linked in 
// one way (statically or dynamically), in these cases this macro has no 
// effect.
//#define DIMCLI_LIB_DYN_LINK

#ifdef _MSC_VER
    #ifndef DIMCLI_LIB_SOURCE
        #pragma warning(push)
    #endif
    #pragma warning(disable : 4100) // unreferenced formal parameter
#endif

#ifdef DIMCLI_LIB_DYN_LINK
    #if defined(_MSC_VER)
        // 'identifier': class 'type' needs to have dll-interface to be used 
        // by clients of class 'type2'
        #pragma warning(disable: 4251) 
    #endif

    #if defined(_WIN32)
        #ifdef DIMCLI_LIB_SOURCE
            #define DIMCLI_LIB_DECL __declspec(dllexport)
        #else
            #define DIMCLI_LIB_DECL __declspec(dllimport)
        #endif
    #endif
#else
    #define DIMCLI_LIB_DECL
#endif

#ifndef _CPPUNWIND
    #define _HAS_EXCEPTIONS 0
#endif
