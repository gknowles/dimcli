// clitest.cpp - dim test cli
#include "pch.h"
#pragma hdrstop

using namespace std;

static int s_errors;

//===========================================================================
bool parseTest(Dim::Cli & cli, vector<char *> args) {
    args.insert(args.begin(), "test.exe");
    if (cli.parse(cerr, size(args), data(args)))
        return true;
    if (cli.exitCode())
        s_errors += 1;
    return false;
}

//===========================================================================
int main(int argc, char * argv[]) {
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    _set_error_mode(_OUT_TO_MSGBOX);

    Dim::Cli cli;
    auto & sum = cli.arg<int>("n number", 1)
                     .desc("numbers to multiply")
                     .action([](auto & cli, auto & arg, const string & val) {
                         int tmp = *arg;
                         if (!arg.parseValue(val))
                             return cli.badUsage(
                                 "Bad '" + arg.from() + "' value: " + val);
                         *arg *= tmp;
                         return true;
                     });
    parseTest(cli, {"-n2", "-n3"});
    cout << "The sum is: " << *sum << endl;

    cli = {};
    auto & num = cli.arg<int>("n number", 1).desc("number is an int");
    auto & special = cli.arg<bool>("s special !S", false).desc("snowflake");
    auto & name = cli.argVec<string>("name");
    cli.argVec<string>("[key]").desc(
        "it's the key argument with a very "
        "long description that wraps the line at least once, maybe more.");
    parseTest(cli, {"-n3"});
    parseTest(cli, {"--name", "two"});
    parseTest(cli, {"--name=three"});
    parseTest(cli, {"-s-name=four", "key", "--name", "four"});
    parseTest(cli, {"key", "extra"});
    parseTest(cli, {"-", "--", "-s"});
    *num += 2;
    *special = name->empty();
    parseTest(cli, {"--help"});

    cli = {};
    parseTest(cli, {"--help"});
    string fruit;
    cli.arg(&fruit, "o", "orange").flagValue();
    cli.arg(&fruit, "a", "apple").flagValue(true);
    parseTest(cli, {"-o"});

    cli = {};
    int count;
    bool help;
    cli.arg(&count, "c ?count").implicitValue(3);
    cli.arg(&help, "? h help");
    parseTest(cli, {"-hc2", "-?"});
    parseTest(cli, {"--count"});

    if (s_errors) {
        cerr << "*** TESTS FAILED ***" << endl;
        return EX_SOFTWARE;
    }
    cout << "All tests passed" << endl;
    return EX_OK;
}
