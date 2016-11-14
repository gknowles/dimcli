// clitest.cpp - dim test cli
#include "pch.h"
#pragma hdrstop

using namespace std;

static int s_errors;

//===========================================================================
bool parseTest(Dim::Cli & cli, vector<const char *> args) {
    args.insert(args.begin(), "test.exe");
    args.push_back(nullptr);
    if (cli.parse(cerr, size(args) - 1, const_cast<char **>(data(args))))
        return true;
    if (cli.exitCode())
        s_errors += 1;
    return false;
}

//===========================================================================
bool toArgvTest(
    std::function<vector<string>(const string &)> fn,
    const std::string & line,
    const vector<string> & argv) {
    auto args = fn(line);
    if (args == argv)
        return true;
    s_errors += 1;
    return false;
}

//===========================================================================
int main(int argc, char * argv[]) {
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    _set_error_mode(_OUT_TO_MSGBOX);

    Dim::Cli c1;
    Dim::Cli c2;
    Dim::CliLocal cli;
    if (!cli.parse(cerr, argc, argv))
        return cli.exitCode();

    auto & sum = cli.opt<int>("n number", 1)
                     .desc("numbers to multiply")
                     .action([](auto & cli, auto & arg, const string & val) {
                         int tmp = *arg;
                         if (!arg.parseValue(val))
                             return cli.badUsage(
                                 "Bad '" + arg.from() + "' value: " + val);
                         *arg *= tmp;
                         return true;
                     });
    cli.versionOpt("1.0");
    parseTest(cli, {"-n2", "-n3"});
    cout << "The sum is: " << *sum << endl;
    parseTest(cli, {"--version"});

    cli = {};
    auto & num = cli.opt<int>("n number", 1).desc("number is an int");
    cli.opt(num, "c").desc("alias for number").valueDesc("COUNT");
    auto & special = cli.opt<bool>("s special !S", false).desc("snowflake");
    auto & name =
        cli.group("name").title("Name options").optVec<string>("name");
    cli.optVec<string>("[key]").desc(
        "it's the key argument with a very "
        "long description that wraps the line at least once, maybe more.");
    cli.title(
        "Long explanation of this very short set of options, it's so long "
        "that it even wraps around to the next line");
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
    auto & orange = cli.opt(&fruit, "o", "orange").flagValue();
    cli.opt(&fruit, "a", "apple").flagValue(true);
    cli.opt(orange, "p", "pear").flagValue();
    parseTest(cli, {"-o"});
    cout << "orange '" << *orange << "' from '" << orange.from() << "' at "
         << orange.pos() << endl;

    cli = {};
    int count;
    bool help;
    cli.opt(&count, "c ?count").implicitValue(3);
    cli.opt(&help, "? h help");
    parseTest(cli, {"-hc2", "-?"});
    parseTest(cli, {"--count"});

    auto fn = cli.toWindowsArgv;
    toArgvTest(fn, R"( a "" "c )", {"a", "", "c "});
    toArgvTest(fn, R"(a"" b ")", {"a", "b", ""});
    toArgvTest(fn, R"("abc" d e)", {"abc", "d", "e"});
    toArgvTest(fn, R"(a\\\b d"e f"g h)", {R"(a\\\b)", "de fg", "h"});
    toArgvTest(fn, R"(a\\\"b c d)", {R"(a\"b)", "c", "d"});
    toArgvTest(fn, R"(a\\\\"b c" d e)", {R"(a\\b c)", "d", "e"});

    if (s_errors) {
        cerr << "*** TESTS FAILED ***" << endl;
        return Dim::kExitSoftware;
    }
    cout << "All tests passed" << endl;
    return 0;
}
