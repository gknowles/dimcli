// Copyright Glen Knowles 2019.
// Distributed under the Boost Software License, Version 1.0.
//
// ostrmtest.cpp - dimcli test ostrm

#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace std::chrono;


class Timer {
    high_resolution_clock::time_point m_start;
    high_resolution_clock::time_point m_lap;
public:
    Timer() { start(); }

    void start() {
        m_start = m_lap = high_resolution_clock::now();
    }
    double lap() {
        auto now = high_resolution_clock::now();
        auto runtime = now - m_lap;
        m_lap = now;
        return duration_cast<duration<double>>(runtime).count();
    }
    double elapsed() const {
        auto runtime = high_resolution_clock::now() - m_start;
        return duration_cast<duration<double>>(runtime).count();
    }
};

//===========================================================================
int main() {
    stringstream os;
    auto loc = locale();
    auto loc2 = locale("");
    auto xloc = loc;
    string val = "100";
    Timer timer;
    auto now = high_resolution_clock::now();
    for (auto i = 0; i < 10'000'000; ++i) {
        // debug
        // stringstream tmp; // 15.8637, 15.9761
        // os.imbue(loc); // 3.95534, 3.90026
        // os.clear(); // 0.275655, 0.28051
        // os.str({}); // 8.76144, 8.72934
        // os.str(""); // 10.317, 10.2283
        // os.str(val); // 5.34961, 5.44129

        // release
        // stringstream tmp; // 2.73482, 2.81621
        // stringstream tmp(val); // 3.28218, 3.27231
        // os.imbue(loc); // 0.241516, 0.240185
        // os.clear(); // 0.0216514, 0.0203205
        // os.str({}); // 0.0286509, 0.0294231
        // os.str(""); // 0.0742336, 0.077
        // os.str(val); // 0.493, 0.484
        // (void) os.getloc(); // 0.1193, 0.1196
    }
    cout << "Seconds: " << timer.elapsed() << endl;
    return 0;
}
