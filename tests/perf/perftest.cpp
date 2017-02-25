// perftest.cpp - dim test perf
#include "pch.h"
#pragma hdrstop

using namespace std::chrono;

inline bool doubleequals(const double a, const double b)
{
    static const double delta = 0.0001;
    const double diff = a - b;
    return diff < delta && diff > -delta;
}
int main()
{
    const std::vector<std::string> carguments({"-i", "7", "-c", "a", "2.7", "--char", "b", "8.4", "-c", "c", "8.8", "--char", "d"});
    const std::vector<std::string> pcarguments({"progname", "-i", "7", "-c", "a", "2.7", "--char", "b", "8.4", "-c", "c", "8.8", "--char", "d"});
    // args
    {
        high_resolution_clock::time_point start = high_resolution_clock::now();
        for (int x = 0; x < 100'000; ++x)
        {
            args::ArgumentParser parser("This is a test program.", "This goes after the options.");
            args::ValueFlag<int> integer(parser, "integer", "The integer flag", {'i', "int"});
            args::ValueFlagList<char> characters(parser, "characters", "The character flag", {'c', "char"});
            args::PositionalList<double> numbers(parser, "numbers", "The numbers position list");
            std::vector<std::string> arguments(carguments);
            parser.ParseArgs(arguments);
            const int i = args::get(integer);
            const std::vector<char> c(args::get(characters));
            const std::vector<double> n(args::get(numbers));
            assert(i == 7);
            assert(c[0] == 'a');
            assert(c[1] == 'b');
            assert(c[2] == 'c');
            assert(c[3] == 'd');
            assert(doubleequals(n[0], 2.7));
            assert(doubleequals(n[1], 8.4));
            assert(doubleequals(n[2], 8.8));
        }
        high_resolution_clock::duration runtime = high_resolution_clock::now() - start;
        std::cout << "args seconds to run: " << duration_cast<duration<double>>(runtime).count() << std::endl;
    }
    // dimcli
    {
        high_resolution_clock::time_point start = high_resolution_clock::now();
        for (int x = 0; x < 100'000; ++x)
        {
            Dim::CliLocal cli;
            cli.header("This is a test program.");
            cli.footer("This goes after the options.");
            auto & i = cli.opt<int>("i int").valueDesc("integer").desc("The integer flag");
            auto & c = cli.optVec<char>("c char").valueDesc("characters").desc("The character flag");
            auto & n = cli.optVec<double>("[numbers]").desc("The numbers position list");
            std::vector<std::string> arguments(pcarguments);
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
        high_resolution_clock::duration runtime = high_resolution_clock::now() - start;
        std::cout << "dimcli seconds to run: " << duration_cast<duration<double>>(runtime).count() << std::endl;
    }
}
