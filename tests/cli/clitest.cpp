// Copyright Glen Knowles 2016 - 2018.
// Distributed under the Boost Software License, Version 1.0.
//
// clitest.cpp - dim test cli
#include "pch.h"
#pragma hdrstop

using namespace std;


#define EXPECT(...) \
    if (!bool(__VA_ARGS__)) \
    failed(line ? line : __LINE__, #__VA_ARGS__)
#define EXPECT_HELP(cli, cmd, text) helpTest(__LINE__, cli, cmd, text)
#define EXPECT_USAGE(cli, cmd, text) usageTest(__LINE__, cli, cmd, text)
#define EXPECT_PARSE(cli, ...) parseTest(__LINE__, cli, true, 0, __VA_ARGS__)
#define EXPECT_PARSE2(cli, cont, ec, ...) \
    parseTest(__LINE__, cli, cont, ec, __VA_ARGS__)
#define EXPECT_ARGV(fn, cmdline, ...) \
    toArgvTest(__LINE__, fn, cmdline, __VA_ARGS__)


#if defined(_WIN32)
const char kCommand[] = "test.exe";
#else
const char kCommand[] = "test";
#endif

static int s_errors;


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
    const string & helpText
) {
    ostringstream os;
    cli.printHelp(os, kCommand, cmd);
    auto tmp = os.str();
    EXPECT(tmp == helpText);
    if (tmp != helpText)
        cout << tmp;
}

//===========================================================================
void usageTest(
    int line,
    Dim::Cli & cli,
    const string & cmd,
    const string & usageText
) {
    ostringstream os;
    cli.printUsageEx(os, kCommand, cmd);
    auto tmp = os.str();
    EXPECT(tmp == usageText);
    if (tmp != usageText)
        cout << tmp;
}

//===========================================================================
void parseTest(
    int line,
    Dim::Cli & cli,
    bool continueFlag,
    int exitCode,
    vector<const char *> args
) {
    args.insert(args.begin(), kCommand);
    args.push_back(nullptr);
    bool rc = cli.parse(args.size() - 1, const_cast<char **>(args.data()));
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
    function<vector<string>(const string &)> fn,
    const string & cmdline,
    const vector<string> & argv
) {
    auto args = fn(cmdline);
    EXPECT(args == argv);
}

//===========================================================================
void basicTests() {
    int line = 0;
    Dim::CliLocal cli;
    istringstream in;
    ostringstream out;

    cli.imbue(locale());

    // assignment operator
    {
        Dim::Cli tmp{cli};
        tmp = cli;
    }

    // choice
    {
        enum class State { go, wait, stop };
        cli = {};
        auto & state =
            cli.opt("streetlight", State::wait)
                .desc("Color of street light.")
                .valueDesc("COLOR")
                .choice(State::go, "green", "Means go!")
                .choice(
                    State::wait, "yellow", "Means wait, even if you're late.")
                .choice(State::stop, "red", "Means stop.");
        EXPECT_HELP(cli, "", 1 + R"(
usage: test [OPTIONS]

Options:
  --streetlight=COLOR  Color of street light.
      green   Means go!
      yellow  Means wait, even if you're late. (default)
      red     Means stop.

  --help               Show this message and exit.
)");
        EXPECT_USAGE(cli, "", 1 + R"(
usage: test [--streetlight=COLOR] [--help]
)");
        EXPECT_PARSE(cli, {"--streetlight", "red"});
        EXPECT(*state == State::stop);

        EXPECT_PARSE2(cli, false, Dim::kExitUsage, {"--streetlight", "white"});
        EXPECT(cli.errMsg() == "Invalid '--streetlight' value: white");
        EXPECT(cli.errDetail() == "Must be \"red\", \"green\", or \"yellow\"");
    }

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
        auto tmp = out.str();
        string expect = "test version 1.0\n";
        EXPECT(tmp == expect);
        if (tmp != expect)
            cout << tmp;
    }

    {
        cli = {};
        auto & num = cli.opt<int>("n number", 1).desc("number is an int");
        cli.opt(num, "c").desc("alias for number").valueDesc("COUNT");
        cli.opt<int>("n2", 2).desc("no defaultDesc").defaultDesc("");
        cli.opt<int>("n3", 3).desc("custom defaultDesc").defaultDesc("three");
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
  -c COUNT                   alias for number (default: 0)
  -n, --number=NUM           number is an int (default: 1)
  --n2=NUM                   no defaultDesc
  --n3=NUM                   custom defaultDesc (default: three)
  -s, --special / -S, --no-special
                             snowflake

Name options:
  --name=STRING

  --help                     Show this message and exit.
)");
        EXPECT_USAGE(cli, "", 1 + R"(
usage: test [-c COUNT] [-n, --number=NUM] [--n2=NUM] [--n3=NUM] [--name=STRING]
            [-s, --special] [--help] [key...]
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

        EXPECT_PARSE(cli, {"--name=", "key"});
        EXPECT(*name == vector<string>({""s}));
        EXPECT(*keys == vector<string>({"key"s}));

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

    // multiline footer
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

    // flagValue
    {
        cli = {};
        string fruit;
        cli.group("fruit").title("Type of fruit");
        auto & orange = cli.opt(&fruit, "o", "orange").flagValue();
        cli.opt(&fruit, "a", "apple").flagValue(true);
        cli.opt(orange, "p", "pear").flagValue();
        cli.group("~").title("Other");
        EXPECT_USAGE(cli, "", 1 + R"(
usage: test [-o] [-p] [--help]
)");
        EXPECT_HELP(cli, "", 1 + R"(
usage: test [OPTIONS]

Type of fruit:
  -a        (default)
  -o
  -p

Other:
  --help    Show this message and exit.
)");
        EXPECT_PARSE(cli, {"-o"});
        EXPECT(*orange == "orange");
        EXPECT(orange.from() == "-o");
        EXPECT(orange.pos() == 1);
    }

    // implicit value
    // helpOpt override
    {
        cli = {};
        int count;
        bool help;
        cli.opt(&count, "c ?count").implicitValue(3);
        cli.opt(&help, "? h help");
        EXPECT_PARSE(cli, {"-hc2", "-?"});
        EXPECT(count == 2);
        EXPECT_PARSE(cli, {"--count"});
        EXPECT(count == 3);
    }

    // windows style argument parsing
    {
        auto fn = cli.toWindowsArgv;
        EXPECT_ARGV(fn, R"( a "" "c )", {"a", "", "c "});
        EXPECT_ARGV(fn, R"(a"" b ")", {"a", "b", ""});
        EXPECT_ARGV(fn, R"("abc" d e)", {"abc", "d", "e"});
        EXPECT_ARGV(fn, R"(a\\\b d"e f"g h)", {R"(a\\\b)", "de fg", "h"});
        EXPECT_ARGV(fn, R"(a\\\"b c d)", {R"(a\"b)", "c", "d"});
        EXPECT_ARGV(fn, R"(a\\\\"b c" d e)", {R"(a\\b c)", "d", "e"});
    }

    // subcommands
    {
        Dim::Cli c1;
        auto & a1 = c1.command("one").opt<int>("a", 1);
        c1.desc("First sentence of description. Rest of one's description.");
        Dim::Cli c2;
        auto & a2 = c2.command("two").opt<int>("a", 2);

        // create option and hide it underneath an undefined command
        c2.opt<int>("b", 99).command("three");

        EXPECT_HELP(c1, "one", 1 + R"(
usage: test one [OPTIONS]

First sentence of description. Rest of one's description.

Options:
  -a NUM    (default: 1)

  --help    Show this message and exit.
)");
        EXPECT_HELP(c1, "three", 1 + R"(
usage: test three [OPTIONS]

Options:
  -b NUM    (default: 99)

  --help    Show this message and exit.
)");
        EXPECT_HELP(c1, "", 1 + R"(
usage: test [OPTIONS] command [args...]

Commands:
  one       First sentence of description.
  three
  two

Options:
  --help    Show this message and exit.
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

    // require
    {
        cli = {};
        auto & count = cli.opt<int>("c", 1).require();
        EXPECT_PARSE(cli, {"-c10"});
        EXPECT(*count == 10);
        EXPECT_PARSE2(cli, false, Dim::kExitUsage, {});
        EXPECT(*count == 1);
        EXPECT(cli.errMsg() == "No value given for -c");
        cli = {};
        auto & imp = cli.opt<int>("?index i").require().implicitValue(5);
        EXPECT_PARSE(cli, {"--index=10"});
        EXPECT(*imp == 10);
        EXPECT_PARSE(cli, {"--index"});
        EXPECT(*imp == 5);
        EXPECT_PARSE2(cli, false, Dim::kExitUsage, {});
        EXPECT(cli.errMsg() == "No value given for --index");
    }

    // clamp and range
    {
        cli = {};
        auto & count = cli.opt<int>("<count>", 2).clamp(1, 10);
        auto & letter = cli.opt<char>("<letter>").range('a', 'z');
        EXPECT_PARSE(cli, {"20", "a"});
        EXPECT(*count == 10);
        EXPECT(*letter == 'a');
        EXPECT_PARSE2(cli, false, Dim::kExitUsage, {"5", "0"});
        EXPECT(*count == 5);
        EXPECT(cli.errMsg() == "Out of range 'letter' value [a - z]: 0");
        EXPECT_PARSE2(cli, false, Dim::kExitUsage, {"--", "-5"});
        EXPECT(*count == 1);
        EXPECT(cli.errMsg() == "Missing argument: letter");
    }

    // filesystem
#ifdef FILESYSTEM
    {
        namespace fs = FILESYSTEM;
        cli = {};
        fs::path path = "path";
        ostringstream os;
        os << path;
        cli.opt(&path, "path", path)
            .desc("std::filesystem::path");
        EXPECT_PARSE(cli, {"--path", "one"});
        EXPECT(path == "one");
        EXPECT_HELP(cli, "", 1 + R"(
usage: test [OPTIONS]

Options:
  --path=FILE  std::filesystem::path (default: )"
        + os.str()
        + R"()

  --help       Show this message and exit.
)");
    }
#endif

    // helpCmd
    {
        cli = {};
        cli.helpCmd();
        EXPECT_HELP(cli, "", 1 + R"(
usage: test [OPTIONS] command [args...]

Commands:
  help      Show help for individual commands and exit.

Options:
  --help    Show this message and exit.
)");
        auto helpText = 1 + R"(
usage: test help [OPTIONS] [command]

Show help for individual commands and exit. If no command is given the list of
commands and general options are shown.
  command   Command to show help information about.

Options:
  -u, --usage / --no-usage  Only show condensed usage.

  --help                    Show this message and exit.
)";
        EXPECT_HELP(cli, "help", helpText);
        EXPECT_PARSE2(cli, true, Dim::kExitOk, {"help", "help"});
        out.str("");
        cli.iostreams(nullptr, &out);
        EXPECT(cli.exec());
        EXPECT(out.str() == helpText);
        cli.iostreams(nullptr, nullptr);
        EXPECT_USAGE(cli, "", 1 + R"(
usage: test [--help] command [args...]
)");
        EXPECT_USAGE(cli, "help", 1 + R"(
usage: test help [-u, --usage] [--help] [command]
)");
        EXPECT_PARSE(cli, {"help", "notACmd"});
        EXPECT(!cli.exec());
        EXPECT(cli.errMsg() == 1 + R"(
Command 'help': Help requested for unknown command: notACmd)");
    }

    // helpNoArgs (aka before action)
    {
        cli = {};
        cli.helpNoArgs();
        out.str("");
        cli.iostreams(nullptr, &out);
        EXPECT_PARSE2(cli, false, Dim::kExitOk, {});
        cli.iostreams(nullptr, nullptr);
        EXPECT(out.str() == 1 + R"(
usage: test [OPTIONS]

Options:
  --help    Show this message and exit.
)");
    }
}

//===========================================================================
void promptTests(bool prompt) {
    int line = 0;
    Dim::CliLocal cli;
    istringstream in;
    ostringstream out;

    // password prompt
    {
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
        in.str("secret\nsecret\n");
        out.str("");
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
    }

    // confirmation prompt
    {
        cli = {};
        auto & ask = cli.confirmOpt();
        EXPECT_PARSE(cli, {"-y"});
        EXPECT(*ask);
        in.str("n\n");
        out.str("");
        cli.iostreams(&in, &out);
        EXPECT_PARSE2(cli, false, Dim::kExitOk, {});
        EXPECT(!*ask);
        EXPECT(out.str() == "Are you sure? [y/N]: ");
        EXPECT(in.get() == EOF);
        if (prompt) {
            cli.iostreams(nullptr, nullptr);
            cout << "Expects answer to be no." << endl
                 << "What you type should be visible." << endl;
            EXPECT_PARSE2(cli, false, Dim::kExitOk, {});
            EXPECT(!*ask);
        }
    }
}

//===========================================================================
void envTests() {
#if !defined(DIMCLI_LIB_NO_CONSOLE)
    int line = 0;
    Dim::CliLocal cli;
    int result;

    cli = {};
    auto & args = cli.optVec<string>("[args]");
    cli.envOpts("TEST_OPTS");
    result = putenv((char *) "TEST_OPTS=");
    EXPECT(!result);
    EXPECT_PARSE(cli, {"c", "d"});
    EXPECT(args->size() == 2);
    result = putenv((char *) "TEST_OPTS=a b");
    EXPECT(!result);
    EXPECT_PARSE(cli, {"c", "d"});
    EXPECT(args->size() == 4);
#endif
}

//===========================================================================
int main(int argc, char * argv[]) {
#if defined(_MSC_VER)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF
        | _CRTDBG_LEAK_CHECK_DF
        | _CRTDBG_DELAY_FREE_MEM_DF);
    _set_error_mode(_OUT_TO_MSGBOX);
#endif

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
