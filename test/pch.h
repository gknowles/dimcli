// pch.h - dim test cli
#include "dim/cli.h"

#include <cstdlib>
#include <iostream>

// getenv/putenv trigger the visual c++ security warning
#if (_MSC_VER >= 1400)
#pragma warning(disable:4996) // this function or variable may be unsafe.
#endif
