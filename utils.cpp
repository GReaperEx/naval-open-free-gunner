#include "utils.h"

#include <iostream>

using namespace std;

#if defined(_WIN32) || defined(__WIN32__)

#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#include <windows.h>

void setTermForeColor(TermColor fg)
{
    static const int colorCodes[] = {
        0,
        FOREGROUND_BLUE,
        FOREGROUND_GREEN,
        FOREGROUND_GREEN | FOREGROUND_BLUE,
        FOREGROUND_RED,
        FOREGROUND_RED | FOREGROUND_BLUE,
        FOREGROUND_RED | FOREGROUND_GREEN,
        FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,
        FOREGROUND_INTENSITY,
        FOREGROUND_INTENSITY | FOREGROUND_BLUE,
        FOREGROUND_INTENSITY | FOREGROUND_GREEN,
        FOREGROUND_INTENSITY | FOREGROUND_GREEN | FOREGROUND_BLUE,
        FOREGROUND_INTENSITY | FOREGROUND_RED,
        FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_BLUE,
        FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN,
        FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,
    };
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), colorCodes[(int)fg]);
}

#else

void setTermForeColor(TermColor fg)
{
    static const int colorCodes[] = {
        30, 31, 32, 33, 34, 35, 36, 37,
        90, 91, 92, 93, 94, 95, 96, 97
    };
    cout << "\e[" << colorCodes[(int)fg] << "m";
    cout.flush();
}

#endif
