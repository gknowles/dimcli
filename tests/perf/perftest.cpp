// Copyright Glen Knowles 2016 - 2021.
// Distributed under the Boost Software License, Version 1.0.
//
// perftest.cpp - dimcli test perf

#include "pch.h"
#pragma hdrstop

using namespace std::chrono;

//===========================================================================
static bool doubleequals(const double a, const double b) {
    static const double delta = 0.0001;
    const double diff = a - b;
    return diff < delta && diff > -delta;
}

//===========================================================================
int main(int argc, char * argv[]) {
    Dim::Cli cli;
    auto & test = cli.opt("test", false).desc("Run tests.");
    if (!cli.parse(argc, argv))
        return cli.printError(std::cerr);
    if (*test) {
        std::cout << "Run from automated testing framework, quick exit.";
        return 0;
    }

    std::vector<std::string> carguments({"-i", "7", "-c", "a", "2.7",
        "--char", "b", "8.4", "-c", "c", "8.8", "--char", "d"});
    std::vector<std::string> pcarguments({"progname", "-i", "7",
        "-c", "a", "2.7", "--char", "b", "8.4", "-c", "c", "8.8",
        "--char", "d"});
    for (int i = 0; i < 1000; ++i) {
        carguments.push_back("7");
        pcarguments.push_back("7");
    }

    // args
    {
        high_resolution_clock::time_point start = high_resolution_clock::now();
        args::ArgumentParser parser(
            "This is a test program.",
            "This goes after the options."
        );
        args::ValueFlag<int> integer(parser, "integer",
            "The integer flag", {'i', "int"});
        args::ValueFlagList<char> characters(parser, "characters",
            "The character flag", {'c', "char"});
        args::PositionalList<double> numbers(parser, "numbers",
            "The numbers position list");
        std::vector<std::string> arguments(carguments);
        for (int x = 0; x < 10'000; ++x)
        {
            parser.ParseArgs(arguments);
            const int i = args::get(integer);
            std::vector<char> const c(args::get(characters));
            std::vector<double> const n(args::get(numbers));
            assert(i == 7);
            assert(c[0] == 'a');
            assert(c[1] == 'b');
            assert(c[2] == 'c');
            assert(c[3] == 'd');
            assert(doubleequals(n[0], 2.7));
            assert(doubleequals(n[1], 8.4));
            assert(doubleequals(n[2], 8.8));
        }
        auto runtime = high_resolution_clock::now() - start;
        std::cout << "args seconds to run: "
            << duration_cast<duration<double>>(runtime).count() << std::endl;
    }
    // dimcli
    {
        auto start = high_resolution_clock::now();
        Dim::CliLocal cli;
        cli.desc("This is a test program.");
        cli.footer("This goes after the options.");
        auto & i = cli.opt<int>("i int").valueDesc("integer")
            .desc("The integer flag");
        auto & c = cli.optVec<char>("c char").valueDesc("characters")
            .desc("The character flag");
        auto & n = cli.optVec<double>("[numbers]")
            .desc("The numbers position list");
        std::vector<std::string> arguments(pcarguments);
        for (int x = 0; x < 10'000; ++x)
        {
            bool result = cli.parse(arguments);
            assert(result == true);
            assert(*i == 7);
            assert(c[0] == 'a');
            assert(c[1] == 'b');
            assert(c[2] == 'c');
            assert(c[3] == 'd');
            assert(doubleequals(n[0], 2.7));
            assert(doubleequals(n[1], 8.4));
            assert(doubleequals(n[2], 8.8));
        }
        auto runtime = high_resolution_clock::now() - start;
        std::cout << "dimcli seconds to run: "
            << duration_cast<duration<double>>(runtime).count() << std::endl;
    }

    return 0;
}
