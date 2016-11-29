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
void basicTests() {
    int line = 0;
    Dim::CliLocal cli;
    istringstream in;
    ostringstream out;

    // parse action
    {
        cli = {};
        auto & sum =
            cli.opt<int>("n number", 1)
                .desc("numbers to multiply")
                .parse([](auto & cli, auto & arg, const string & val) {
                    int tmp = *arg;
                    if (!arg.parseValue(val))
                        return cli.badUsage(
                            "Bad '" + arg.from() + "' value: " + val);
                    *arg *= tmp;
                    return true;
                });
        EXPECT_PARSE(cli, {"-n2", "-n3"});
        EXPECT(*sum == 6);
    }

    // version option
    {
        cli = {};
        cli.versionOpt("1.0");
        in.str("");
        out.str("");
        cli.iostreams(&in, &out);
        EXPECT_PARSE2(cli, false, 0, {"--version"});
        EXPECT(out.str() == "test version 1.0\n");
    }

    {
        cli = {};
        auto & num = cli.opt<int>("n number", 1).desc("number is an int");
        cli.opt(num, "c").desc("alias for number").valueDesc("COUNT");
        auto & special =
            cli.opt<bool>("s special !S", false).desc("snowflake");
        auto & name =
            cli.group("name").title("Name options").optVec<string>("name");
        auto & keys = cli.group("").optVec<string>("[key]").desc(
            "it's the key arguments with a very long description that wraps "
            "the line at least once, maybe more.");
        cli.title(
            "Long explanation of this very short set of options, it's so "
            "long that it even wraps around to the next line");
        EXPECT_HELP(cli, "", 1 + R"(
usage: test [OPTIONS] [key...]
  key       it's the key arguments with a very long description that wraps the
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
        EXPECT(!*special);
        EXPECT(!name);
        EXPECT(!keys);

        EXPECT_PARSE(cli, {"--name", "two"});
        EXPECT(*num == 0);
        EXPECT(name.size() == 1 && (*name)[0] == "two");

        EXPECT_PARSE(cli, {"--name=three"});
        EXPECT(name.size() == 1 && (*name)[0] == "three");

        EXPECT_PARSE(cli, {"-s-name=four", "key", "--name", "four"});
        EXPECT(*special);
        EXPECT(*name == vector<string>({"four"s, "four"s}));
        EXPECT(*keys == vector<string>({"key"s}));

        EXPECT_PARSE(cli, {"key", "extra"});
        EXPECT(*keys == vector<string>({"key"s, "extra"s}));

        EXPECT_PARSE(cli, {"-", "--", "-s"});
        EXPECT(!special && !*special);
        *num += 2;
        EXPECT(*num == 2);
        *special = name->empty();
        EXPECT(*special);
    }

    {
        cli = {};
        cli.footer("Multiline footer:\n"
                   "- first reference\n"
                   "- second reference\n");
        EXPECT_HELP(cli, "", 1 + R"(
usage: test [OPTIONS]

Options:
  --help    Show this message and exit.

Multiline footer:
- first reference
- second reference
)");
    }

    {
        cli = {};
        string fruit;
        auto & orange = cli.opt(&fruit, "o", "orange").flagValue();
        cli.opt(&fruit, "a", "apple").flagValue(true);
        cli.opt(orange, "p", "pear").flagValue();
        EXPECT_PARSE(cli, {"-o"});
        EXPECT(*orange == "orange");
        EXPECT(orange.from() == "-o");
        EXPECT(orange.pos() == 1);
    }

    {
        cli = {};
        int count;
        bool help;
        cli.opt(&count, "c ?count").implicitValue(3);
        cli.opt(&help, "? h help");
        EXPECT_PARSE(cli, {"-hc2", "-?"});
        EXPECT_PARSE(cli, {"--count"});
    }

    {
        auto fn = cli.toWindowsArgv;
        EXPECT_ARGV(fn, R"( a "" "c )", {"a", "", "c "});
        EXPECT_ARGV(fn, R"(a"" b ")", {"a", "b", ""});
        EXPECT_ARGV(fn, R"("abc" d e)", {"abc", "d", "e"});
        EXPECT_ARGV(fn, R"(a\\\b d"e f"g h)", {R"(a\\\b)", "de fg", "h"});
        EXPECT_ARGV(fn, R"(a\\\"b c d)", {R"(a\"b)", "c", "d"});
        EXPECT_ARGV(fn, R"(a\\\\"b c" d e)", {R"(a\\b c)", "d", "e"});
    }

    {
        Dim::Cli c1;
        auto & a1 = c1.command("one").opt<int>("a", 1);
        c1.desc("First sentence of description. Rest of one's description.");
        Dim::Cli c2;
        auto & a2 = c2.command("two").opt<int>("a", 2);
        EXPECT_HELP(c1, "one", 1 + R"(
usage: test one [OPTIONS]
First sentence of description. Rest of one's description.
Options:
  -a NUM
)");
        EXPECT_HELP(c1, "", 1 + R"(
usage: test [OPTIONS] command [args...]

Options:
  --help    Show this message and exit.

Commands:
  one       First sentence of description.
  two
)");
        EXPECT_PARSE(c1, {"one", "-a3"});
        EXPECT(*a1 == 3);
        EXPECT(*a2 == 2);
        EXPECT(c2.runCommand() == "one");
        EXPECT_PARSE2(c1, false, Dim::kExitUsage, {"-a"});
        EXPECT(c2.errMsg() == "Unknown option: -a");
        EXPECT_PARSE2(c1, false, Dim::kExitUsage, {"two", "-a"});
        EXPECT(c2.errMsg() == "Command 'two': Option requires value: -a");
    }

    {
        cli = {};
        auto & count = cli.opt<int>("<count>", 1).clamp(1, 10);
        auto & letter = cli.opt<char>("<letter>").range('a', 'z');
        EXPECT_PARSE(cli, {"20", "a"});
        EXPECT(*count == 10);
        EXPECT(*letter == 'a');
        EXPECT_PARSE2(cli, false, Dim::kExitUsage, {"5", "0"});
        EXPECT(*count == 5);
        EXPECT(cli.errMsg() == "Out of range 'letter' value [a - z]: 0");
    }
}

//===========================================================================
void promptTests(bool prompt) {
    int line = 0;
    Dim::CliLocal cli;

    cli = {};
    auto & pass = cli.passwordOpt(true);
    EXPECT_HELP(cli, "", 1 + R"(
usage: test [OPTIONS]

Options:
  --password=STRING  Password required for access.

  --help             Show this message and exit.
)");
    EXPECT_PARSE(cli, {"--password=hi"});
    EXPECT(*pass == "hi");
    istringstream in("secret\nsecret\n");
    ostringstream out;
    cli.iostreams(&in, &out);
    EXPECT_PARSE(cli, {});
    EXPECT(*pass == "secret");
    EXPECT(out.str() == "Password: \nEnter again to confirm: \n");
    EXPECT(in.get() == EOF);
    if (prompt) {
        cli.iostreams(nullptr, nullptr);
        cout << "Expects password to be confirmed (empty is ok)." << endl
             << "What you type should *NOT* be visible!" << endl;
        EXPECT_PARSE(cli, {});
        cout << "Entered password was '" << *pass << "'" << endl;
    }

    cli = {};
    auto & ask = cli.confirmOpt();
    EXPECT_PARSE(cli, {"-y"});
    EXPECT(*ask);
    in.str("n\n");
    out.str("");
    cli.iostreams(&in, &out);
    EXPECT_PARSE2(cli, false, 0, {});
    EXPECT(!*ask);
    EXPECT(out.str() == "Are you sure? [y/N]: ");
    EXPECT(in.get() == EOF);
    if (prompt) {
        cli.iostreams(nullptr, nullptr);
        cout << "Expects answer to be no." << endl
             << "What you type should be visible." << endl;
        EXPECT_PARSE2(cli, false, 0, {});
        EXPECT(!*ask);
    }
}

//===========================================================================
void envTests() {
    int line = 0;
    Dim::CliLocal cli;

    cli = {};
    auto & args = cli.optVec<string>("[args]");
    cli.envOpts("TEST_OPTS");
    putenv("");
    EXPECT_PARSE(cli, {"c", "d"});
    EXPECT(args->size() == 2);
    putenv("TEST_OPTS=a b");
    EXPECT_PARSE(cli, {"c", "d"});
    EXPECT(args->size() == 4);
}

//===========================================================================
int main(int argc, char * argv[]) {
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    _set_error_mode(_OUT_TO_MSGBOX);

    Dim::CliLocal cli;
    auto & prompt = cli.opt<bool>("prompt").desc("Run tests with prompting");
    if (!cli.parse(cerr, argc, argv))
        return cli.exitCode();
    basicTests();
    envTests();
    promptTests(*prompt);

    if (s_errors) {
        cerr << "*** TESTS FAILED ***" << endl;
        return Dim::kExitSoftware;
    }
    cout << "All tests passed" << endl;
    return 0;
}
