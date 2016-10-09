// clitest.cpp - dim test cli
#include "pch.h"
#pragma hdrstop

using namespace Dim;
using namespace std;

//===========================================================================
bool parseTest(Cli & cli, vector<char *> args) {
    args.insert(args.begin(), "test.exe");
    if (cli.parse(size(args), data(args)))
        return true;
    cerr << cli.errMsg();
    return false;
}

//===========================================================================
int main(int argc, char * argv[]) {
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    _set_error_mode(_OUT_TO_MSGBOX);

    bool result;
    Cli cli;
    auto & num = cli.arg<int>("n number", 1);
    auto & special = cli.arg<bool>("s special", false);
    auto & name = cli.argVec<string>("name");
    cli.argVec<string>("[key]");
    result = parseTest(cli, {"-n3"});
    result = parseTest(cli, {"--name", "two"});
    result = parseTest(cli, {"--name=three"});
    result = parseTest(cli, {"-s-name=four", "key", "--name", "four"});
    result = parseTest(cli, {"key", "extra"});
    *num += 2;
    *special = name->empty();

    cli = {};
    int count;
    bool help;
    cli.arg(&count, "c count");
    cli.arg(&help, "? h help");
    result = parseTest(cli, {"-hc2", "-?"});
    if (result) {
        cout << "Last test passed" << endl;
        return EX_OK;
    }
    return EX_SOFTWARE;
}
