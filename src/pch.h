// pch.h - dim cli
#include "dim/cli.h"
#include "dim/console.h"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <locale>
#include <unordered_set>

// getenv triggers the visual c++ security warning
#if (_MSC_VER >= 1400)
#pragma warning(disable:4996) // this function or variable may be unsafe.
#endif
