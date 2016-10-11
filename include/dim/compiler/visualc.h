// visualc.h - dim compiler config
#pragma once

#pragma warning(disable : 4100) // unreferenced formal parameter
#pragma warning( \
    disable : 4324) // structure was padded due to alignment specifier
#pragma warning( \
    disable : 4456) // declaration of 'elem' hides previous local declaration
#pragma warning(disable : 5030) // attribute 'identifier' is not recognized

#define _NO_LOCALES 0
#define _ITERATOR_DEBUG_LEVEL 0

// program exit codes
// On *nix the EX_* constants are defined in <sysexits.h>
#ifndef EX_OK
enum {
    EX_OK = 0,
    EX_USAGE = 64,       // bad command line
    EX_DATAERR = 65,     // bad input file
    EX_UNAVAILABLE = 69, // failed for some other reason
    EX_SOFTWARE = 70,    // internal software error
    EX_OSERR = 71,       // some os operation failed
    EX_IOERR = 74,       // console or file read/write error
    EX_NOPERM = 77,      // insufficient permission
    EX_CONFIG = 78,      // configuration error

    // Microsoft specific
    EX_ABORTED = 3, // assert() failure or direct call to abort()
};
#endif
