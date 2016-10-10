// clitest.cpp - dim test cli
#include "pch.h"
#pragma hdrstop

using namespace Dim;
using namespace std;

static int s_errors;

//===========================================================================
bool parseTest(Cli & cli, vector<char *> args) {
    args.insert(args.begin(), "test.exe");
    if (cli.parse(cerr, size(args), data(args)))
        return true;
    s_errors += 1;
    return false;
}

//===========================================================================
int main(int argc, char * argv[]) {
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    _set_error_mode(_OUT_TO_MSGBOX);

    Cli cli;
    auto & num = cli.arg<int>("n number", 1);
    auto & special = cli.arg<bool>("s special", false);
    auto & name = cli.argVec<string>("name");
    cli.argVec<string>("[key]");
    parseTest(cli, {"-n3"});
    parseTest(cli, {"--name", "two"});
    parseTest(cli, {"--name=three"});
    parseTest(cli, {"-s-name=four", "key", "--name", "four"});
    parseTest(cli, {"key", "extra"});
    *num += 2;
    *special = name->empty();

    cli = {};
    int count;
    bool help;
    cli.arg(&count, "c count");
    cli.arg(&help, "? h help");
    parseTest(cli, {"-hc2", "-?"});

    if (s_errors) {
        cerr << "*** TESTS FAILED ***" << endl;
        return EX_SOFTWARE;
    }
    cout << "All tests passed" << endl;
    return EX_OK;
}
