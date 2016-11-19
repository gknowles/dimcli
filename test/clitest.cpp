// clitest.cpp - dim test cli
#include "pch.h"
#pragma hdrstop

using namespace std;

static int s_errors;

#define EXPECT(e) \
    if (!bool(e)) \
    failed(line ? line : __LINE__, #e)
#define EXPECT_HELP(cli, cmd, text) helpTest(__LINE__, cli, cmd, text)
#define EXPECT_PARSE(cli, ...) parseTest(__LINE__, cli, true, 0, __VA_ARGS__)
#define EXPECT_PARSE2(cli, cont, ec, ...) \
    parseTest(__LINE__, cli, cont, ec, __VA_ARGS__)
#define EXPECT_ARGV(fn, cmdline, ...) \
    toArgvTest(__LINE__, fn, cmdline, __VA_ARGS__)

//===========================================================================
void failed(int line, const char msg[]) {
    cerr << "Line " << line << ": EXPECT(" << msg << ") failed" << endl;
    s_errors += 1;
}

//===========================================================================
void helpTest(
    int line,
    Dim::Cli & cli,
    const string & cmd,
    const string & helpText) {
    ostringstream os;
    cli.writeHelp(os, "test.exe", cmd);
    auto tmp = os.str();
    EXPECT(os.str() == helpText);
}

//===========================================================================
void parseTest(
    int line,
    Dim::Cli & cli,
    bool continueFlag,
    int exitCode,
    vector<const char *> args) {
    args.insert(args.begin(), "test.exe");
    args.push_back(nullptr);
    bool rc = cli.parse(size(args) - 1, const_cast<char **>(data(args)));
    if (rc != continueFlag || exitCode != cli.exitCode()) {
        if (exitCode)
            cerr << cli.errMsg() << endl;
        EXPECT(rc == continueFlag);
        EXPECT(exitCode == cli.exitCode());
    }
}

//===========================================================================
void toArgvTest(
    int line,
    std::function<vector<string>(const string &)> fn,
    const std::string & cmdline,
    const vector<string> & argv) {
    auto args = fn(cmdline);
    EXPECT(args == argv);
}

//===========================================================================
int main(int argc, char * argv[]) {
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    _set_error_mode(_OUT_TO_MSGBOX);

    int line = 0;
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
    EXPECT_PARSE(cli, {"-n2", "-n3"});
    EXPECT(*sum == 6);
    EXPECT_PARSE2(cli, false, 0, {"--version"});

    cli = {};
    auto & num = cli.opt<int>("n number", 1).desc("number is an int");
    cli.opt(num, "c").desc("alias for number").valueDesc("COUNT");
    auto & special = cli.opt<bool>("s special !S", false).desc("snowflake");
    auto & name =
        cli.group("name").title("Name options").optVec<string>("name");
    cli.optVec<string>("[key]").desc(
        "it's the key argument with a very long description that wraps the "
        "line at least once, maybe more.");
    cli.title(
        "Long explanation of this very short set of options, it's so long "
        "that it even wraps around to the next line");
    EXPECT_HELP(cli, "", 1 + R"(
usage: test [OPTIONS] [key...]
  key       it's the key argument with a very long description that wraps the
            line at least once, maybe more.

Long explanation of this very short set of options, it's so long that it even
wraps around to the next line:
  -c COUNT                   alias for number
  -n, --number=NUM           number is an int
  -s, --special / -S, --no-special
                             snowflake

Name options:
  --name=STRING

  --help                     Show this message and exit.
)");
    EXPECT_PARSE(cli, {"-n3"});
    EXPECT(*num == 3);
    EXPECT_PARSE(cli, {"--name", "two"});
    EXPECT_PARSE(cli, {"--name=three"});
    EXPECT_PARSE(cli, {"-s-name=four", "key", "--name", "four"});
    EXPECT_PARSE(cli, {"key", "extra"});
    EXPECT_PARSE(cli, {"-", "--", "-s"});
    *num += 2;
    EXPECT(*num == 2);
    *special = name->empty();
    EXPECT(*special);

    cli = {};
    EXPECT_HELP(cli, "", 1 + R"(
usage: test [OPTIONS]

Options:
  --help    Show this message and exit.
)");

    string fruit;
    auto & orange = cli.opt(&fruit, "o", "orange").flagValue();
    cli.opt(&fruit, "a", "apple").flagValue(true);
    cli.opt(orange, "p", "pear").flagValue();
    EXPECT_PARSE(cli, {"-o"});
    EXPECT(*orange == "orange");
    EXPECT(orange.from() == "-o");
    EXPECT(orange.pos() == 1);

    cli = {};
    int count;
    bool help;
    cli.opt(&count, "c ?count").implicitValue(3);
    cli.opt(&help, "? h help");
    EXPECT_PARSE(cli, {"-hc2", "-?"});
    EXPECT_PARSE(cli, {"--count"});

    auto fn = cli.toWindowsArgv;
    EXPECT_ARGV(fn, R"( a "" "c )", {"a", "", "c "});
    EXPECT_ARGV(fn, R"(a"" b ")", {"a", "b", ""});
    EXPECT_ARGV(fn, R"("abc" d e)", {"abc", "d", "e"});
    EXPECT_ARGV(fn, R"(a\\\b d"e f"g h)", {R"(a\\\b)", "de fg", "h"});
    EXPECT_ARGV(fn, R"(a\\\"b c d)", {R"(a\"b)", "c", "d"});
    EXPECT_ARGV(fn, R"(a\\\\"b c" d e)", {R"(a\\b c)", "d", "e"});

    Dim::Cli c1;
    auto & a1 = c1.command("one").opt<int>("a", 1);
    Dim::Cli c2;
    auto & a2 = c2.command("two").opt<int>("a", 2);
    EXPECT_HELP(c1, "one", 1 + R"(
usage: test one [OPTIONS]

Options:
  -a NUM
)");
    EXPECT_PARSE(c1, {"one", "-a3"});
    EXPECT(*a1 == 3);
    EXPECT(*a2 == 2);
    EXPECT(c2.runCommand() == "one");
    EXPECT_PARSE2(c1, false, 64, {"-a"});
    EXPECT(c2.errMsg() == "Unknown option: -a");
    EXPECT_PARSE2(c1, false, 64, {"two", "-a"});
    EXPECT(c2.errMsg() == "Option requires value for 'two' command: -a");

    if (s_errors) {
        cerr << "*** TESTS FAILED ***" << endl;
        return Dim::kExitSoftware;
    }
    cout << "All tests passed" << endl;
    return 0;
}
