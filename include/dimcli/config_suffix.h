// config_suffix.h - dim core
#pragma once

// Restore as many compiler settings as we can so they don't leak into
// the applications
#ifndef DIMCLI_LIB_SOURCE

// clear all dim header macros so they don't leak into the application
#ifdef DIMCLI_LIB_STANDALONE
    #undef DIMCLI_LIB_STANDALONE
    #undef DIMCLI_LIB_DYN_LINK

    #undef DIMCLI_LIB_SOURCE
    #undef DIMCLI_LIB_DECL
#endif

#ifdef _MSC_VER
    #pragma warning(pop)
#endif

#endif
