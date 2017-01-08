// console.cpp - dim cli
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Public API
*
***/

#if defined(DIM_LIB_NO_CONSOLE)

//===========================================================================
void Cli::consoleEnableEcho(bool enable) {
    assert(enable && "disabling console echo not supported");
}

#elif defined(_WIN32)

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

//===========================================================================
void Cli::consoleEnableEcho(bool enable) {
    HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode = 0;
    GetConsoleMode(hInput, &mode);
    if (enable) {
        mode |= ENABLE_ECHO_INPUT;
    } else {
        mode &= ~ENABLE_ECHO_INPUT;
    }
    SetConsoleMode(hInput, mode);
}

#else

#include <termios.h>
#include <unistd.h>

//===========================================================================
void Cli::consoleEnableEcho(bool enable) {
    termios tty;
    tcgetattr(STDIN_FILENO, &tty);
    if (enable) {
        tty.c_lflag |= ECHO;
    } else {
        tty.c_lflag &= ~ECHO;
    }
    tcsetattr(STDIN_FILENO, TCSANOW, &tty);
}

#endif
