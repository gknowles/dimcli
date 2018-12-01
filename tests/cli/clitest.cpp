// Copyright Glen Knowles 2016 - 2018.
// Distributed under the Boost Software License, Version 1.0.
//
// clitest.cpp - dimcli test cli
#include "pch.h"
#pragma hdrstop

using namespace std;


/****************************************************************************
*
*   Globals
*
***/

#if defined(_WIN32)
char const kCommand[] = "test.exe";
#else
char const kCommand[] = "test";
#endif

static int s_errors;


/****************************************************************************
*
*   Helpers
*
***/

#define EXPECT(...) \
    if (!bool(__VA_ARGS__)) \
    failed(line ? line : __LINE__, #__VA_ARGS__)
#define EXPECT_PARSE(...) parseTest(__LINE__, __VA_ARGS__)
#define EXPECT_ERR(cli, text) errTest(__LINE__, cli, text)
#define EXPECT_HELP(cli, cmd, text) helpTest(__LINE__, cli, cmd, text)
#define EXPECT_USAGE(cli, cmd, text) usageTest(__LINE__, cli, cmd, text)
#define EXPECT_ARGV(fn, cmdline, ...) \
    toArgvTest(__LINE__, fn, cmdline, __VA_ARGS__)

//===========================================================================
void failed(int line, char const msg[]) {
    cerr << "Line " << line << ": EXPECT(" << msg << ") failed" << endl;
    s_errors += 1;
}

//===========================================================================
void parseTest(
    int line,
    Dim::Cli & cli,
    string const & cmdline = {},
    bool continueFlag = true,
    int exitCode = -1
) {
    auto args = cli.toWindowsArgv(cmdline);
    args.insert(args.begin(), kCommand);
    bool rc = cli.parse(args);
    if (exitCode == -1)
        exitCode = rc ? Dim::kExitOk : Dim::kExitUsage;
    if (rc != continueFlag || exitCode != cli.exitCode()) {
        if (exitCode)
            cerr << cli.errMsg() << endl;
        EXPECT(rc == continueFlag);
        EXPECT(exitCode == cli.exitCode());
    }
}

//===========================================================================
void errTest(int line, Dim::Cli & cli, string const & errText) {
    ostringstream os;
    cli.printError(os);
    auto tmp = os.str();
    EXPECT(tmp == errText);
    if (tmp != errText)
        cerr << tmp;
}

//===========================================================================
void helpTest(
    int line,
    Dim::Cli & cli,
    string const & cmd,
    string const & helpText
) {
    ostringstream os;
    cli.printHelp(os, kCommand, cmd);
    auto tmp = os.str();
    EXPECT(tmp == helpText);
    if (tmp != helpText)
        cerr << tmp;
}

//===========================================================================
void usageTest(
    int line,
    Dim::Cli & cli,
    string const & cmd,
    string const & usageText
) {
    ostringstream os;
    cli.printUsageEx(os, kCommand, cmd);
    auto tmp = os.str();
    EXPECT(tmp == usageText);
    if (tmp != usageText)
        cerr << tmp;
}

//===========================================================================
void toArgvTest(
    int line,
    function<vector<string>(string const &)> fn,
    string const & cmdline,
    vector<string> const & argv
) {
    auto args = fn(cmdline);
    EXPECT(args == argv);
}


/****************************************************************************
*
*   Value parsing
*
***/

enum class ExtractNoInsert {
    kInvalid,
    kGood,
    kBad,
};
istream & operator>>(istream & is, ExtractNoInsert & out) {
    auto ch = is.get();
    switch (ch) {
    case 'g': out = ExtractNoInsert::kGood; break;
    case 'b': out = ExtractNoInsert::kBad; break;
    default: out = ExtractNoInsert::kInvalid; break;
    }
    if (out != ExtractNoInsert::kGood)
        is.setstate(ios::failbit);
    return is;
}

enum class ExtractWithInsert {
    kInvalid,
    kGood,
    kInOnly,
    kBad,
};
istream & operator>>(istream & is, ExtractWithInsert & out) {
    auto ch = is.get();
    switch (ch) {
    case 'g': out = ExtractWithInsert::kGood; break;
    case 'i': out = ExtractWithInsert::kInOnly; break;
    case 'b': out = ExtractWithInsert::kBad; break;
    default: out = ExtractWithInsert::kInvalid; break;
    }
    if (out == ExtractWithInsert::kBad || out == ExtractWithInsert::kInvalid)
        is.setstate(ios::failbit);
    return is;
}
ostream & operator<<(ostream & os, const ExtractWithInsert & in) {
    switch (in) {
    case ExtractWithInsert::kGood: os << 'g'; break;
    case ExtractWithInsert::kInOnly: os << 'i'; break;
    case ExtractWithInsert::kBad: os << 'b'; break;
    default: break;
    }
    if (in != ExtractWithInsert::kGood)
        os.setstate(ios::failbit);
    return os;
}

//===========================================================================
void parseTests() {
    int line = 0;
    Dim::CliLocal cli;

    // parse action
    {
        cli = {};
        auto & sum = cli.opt<int>("n number", 1)
            .desc("numbers to multiply")
            .parse([](auto & cli, auto & arg, string const & val) {
                int tmp = *arg;
                if (!arg.parseValue(val))
                    return cli.badUsage(
                        "Bad '" + arg.from() + "' value: " + val);
                *arg *= tmp;
                return true;
            });
        EXPECT_PARSE(cli, "-n2 -n3");
        EXPECT(*sum == 6);
    }

    // parsing failure
    {
        cli = {};
        auto & opt = cli.opt("[value]", ExtractNoInsert::kBad)
            .desc("Value to attempt to parse.");
        EXPECT_PARSE(cli, "g");
        EXPECT(*opt == ExtractNoInsert::kGood);
        EXPECT_PARSE(cli, "b", false);
        EXPECT(*opt == ExtractNoInsert::kInvalid);
        EXPECT_HELP(cli, "", 1 + R"(
usage: test [OPTIONS] [value]
  value     Value to attempt to parse.

Options:
  --help    Show this message and exit.
)");
    }

    // default render failure
    {
        cli = {};
        auto & opt = cli.opt("[value]", ExtractWithInsert::kGood)
            .desc("Value to attempt to parse.");
        EXPECT_PARSE(cli);
        EXPECT(*opt == ExtractWithInsert::kGood);
        EXPECT_PARSE(cli, "b", false);
        EXPECT(*opt == ExtractWithInsert::kInvalid);
        EXPECT_HELP(cli, "", 1 + R"(
usage: test [OPTIONS] [value]
  value     Value to attempt to parse. (default: g)

Options:
  --help    Show this message and exit.
)");

        opt.defaultValue(ExtractWithInsert::kInOnly);
        EXPECT_PARSE(cli);
        EXPECT(*opt == ExtractWithInsert::kInOnly);
        EXPECT_PARSE(cli, "g");
        EXPECT(*opt == ExtractWithInsert::kGood);
        EXPECT_PARSE(cli, "i");
        EXPECT(*opt == ExtractWithInsert::kInOnly);
        EXPECT_HELP(cli, "", 1 + R"(
usage: test [OPTIONS] [value]
  value     Value to attempt to parse.

Options:
  --help    Show this message and exit.
)");
    }
}


/****************************************************************************
*
*   Choice
*
***/

//===========================================================================
void choiceTests() {
    int line = 0;
    Dim::CliLocal cli;
    enum class State { go, wait, stop };

    cli = {};
    auto & state = cli.opt("streetlight", State::wait)
        .desc("Color of street light.")
        .valueDesc("COLOR")
        .choice(State::go, "green", "Means go!")
        .choice(State::wait, "yellow", "Means wait, even if you're late.")
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
    EXPECT_PARSE(cli, "--streetlight red");
    EXPECT(*state == State::stop);

    EXPECT_PARSE(cli, "--streetlight white", false);
    EXPECT_ERR(cli, 1 + R"(
Error: Invalid '--streetlight' value: white
Must be "green", "yellow", or "red".
)");

    state.defaultValue(State::go);
    EXPECT_HELP(cli, "", 1 + R"(
usage: test [OPTIONS]

Options:
  --streetlight=COLOR  Color of street light.
      green   Means go! (default)
      yellow  Means wait, even if you're late.
      red     Means stop.

  --help               Show this message and exit.
)");

    cli = {};
    auto & state2 = cli.optVec<State>("[streetlights]")
        .desc("Color of street lights.")
        .valueDesc("COLOR")
        .choice(State::stop, "red", "Means stop.", "3")
        .choice(State::wait, "yellow", "Means wait, even if you're late.", "2")
        .choice(State::go, "green", "Means go!", "1");
    EXPECT_HELP(cli, "", 1 + R"(
usage: test [OPTIONS] [streetlights...]
  streetlights  Color of street lights.
      green   Means go!
      yellow  Means wait, even if you're late.
      red     Means stop.

Options:
  --help    Show this message and exit.
)");
    EXPECT_PARSE(cli, "red");
    EXPECT(state2.size() == 1 && state2[0] == State::stop);
    EXPECT_PARSE(cli, "white", false);
    EXPECT_ERR(cli, 1 + R"(
Error: Invalid 'streetlights' value: white
Must be "green", "yellow", or "red".
)");

    EXPECT(state2.defaultValue() == State::go);
    state2.defaultValue(State::wait);
    EXPECT(state2.defaultValue() == State::wait);

    cli = {};
    auto & nums = cli.optVec<unsigned>("n")
        .desc("List of numbers")
        .valueDesc("NUMBER")
        .choice(1, "one").choice(2, "two");
    EXPECT_PARSE(cli, "-n white", false);
    EXPECT_ERR(cli, 1 + R"(
Error: Invalid '-n' value: white
Must be "one" or "two".
)");

    nums.choice(3, "three").choice(4, "four").choice(5, "five")
        .choice(6, "six").choice(7, "seven").choice(8, "eight")
        .choice(9, "nine").choice(10, "ten").choice(11, "eleven")
        .choice(12, "twelve");
    EXPECT_PARSE(cli, "-n white", false);
    EXPECT_ERR(cli, 1 + R"(
Error: Invalid '-n' value: white
Must be "one", "two", "three", "four", "five", "six", "seven", "eight", "nine",
"ten", "eleven", or "twelve".
)");
}


/****************************************************************************
*
*   Help and version
*
***/

//===========================================================================
void helpTests() {
    int line = 0;
    Dim::CliLocal cli;
    istringstream in;
    ostringstream out;

    // version option
    {
        cli = {};
        cli.versionOpt("1.0");
        in.str("");
        out.str("");
        cli.iostreams(&in, &out);
        EXPECT_PARSE(cli, "--version", false, Dim::kExitOk);
        auto tmp = out.str();
        string expect = "test version 1.0\n";
        EXPECT(tmp == expect);
        if (tmp != expect)
            cout << tmp;
    }

    // helpNoArgs (aka before action)
    {
        cli = {};
        cli.helpNoArgs();
        out.str("");
        cli.iostreams(nullptr, &out);
        EXPECT_PARSE(cli, {}, false, Dim::kExitOk);
        cli.iostreams(nullptr, nullptr);
        EXPECT(out.str() == 1 + R"(
usage: test [OPTIONS]

Options:
  --help    Show this message and exit.
)");
    }

    // implicit value
    // helpOpt override
    {
        cli = {};
        int count;
        bool help;
        cli.opt(&count, "c ?count").implicitValue(3);
        cli.opt(&help, "? h help");
        EXPECT_PARSE(cli, "-hc2 -?");
        EXPECT(count == 2);
        EXPECT_PARSE(cli, "--count");
        EXPECT(count == 3);
    }

    // multiline footer
    {
        cli = {};
        cli.opt<bool>(cli.helpOpt(), "no-help.", false).show(false);
        cli.header("");
        EXPECT(cli.header().size() == 1 && cli.header()[0] == '\0');
        cli.header("Multiline header:\n"
                   "- second line\n");
        cli.footer("");
        EXPECT(cli.footer().size() == 1 && cli.footer()[0] == '\0');
        cli.footer("Multiline footer:\n"
                   "- first reference\n"
                   "- second reference\n");
        cli.desc("Description.");
        EXPECT(cli.desc() == "Description.");
        EXPECT_HELP(cli, "", 1 + R"(
Multiline header:
- second line

usage: test [OPTIONS]

Description.

Options:
  --help    Show this message and exit.

Multiline footer:
- first reference
- second reference
)");
        EXPECT_PARSE(cli, "--no-help");
    }

    // group sortKey
    {
        cli = {};
        cli.group("One").sortKey("1").opt("1", true).desc("First option.");
        EXPECT(cli.sortKey() == "1");
        cli.group("Two").sortKey("2").opt("2", true).desc("Second option.");
        cli.group("Three").sortKey("3").opt("3", true).desc("Third option.");
        EXPECT_HELP(cli, "", 1 + R"(
usage: test [OPTIONS]

One:
  -1        First option.

Two:
  -2        Second option.

Three:
  -3        Third option.

  --help    Show this message and exit.
)");
    }
}


/****************************************************************************
*
*   Subcommands
*
***/

//===========================================================================
void cmdTests() {
    int line = 0;
    Dim::CliLocal cli;
    istringstream in;
    ostringstream out;

    // subcommands
    {
        Dim::Cli c1;
        auto & a1 = c1.command("one").cmdTitle("Primary").opt<int>("a", 1);
        EXPECT(c1.cmdTitle() == "Primary");
        c1.desc("First sentence of description. Rest of one's description.");
        Dim::Cli c2;
        auto & a2 = c2.command("two").cmdGroup("Additional").opt<int>("a", 2);

        // create option and hide it underneath an undefined command
        c2.opt<int>("b", 99).command("three");

        EXPECT_PARSE(c1, "one -a3");
        EXPECT(*a1 == 3);
        EXPECT(*a2 == 2);
        EXPECT(c2.runCommand() == "one");
        EXPECT_PARSE(c1, "-a", false);
        EXPECT_ERR(c2, "Error: Unknown option: -a\n");
        EXPECT_PARSE(c1, "two -a", false);
        EXPECT_ERR(c2, "Error: Command 'two': Option requires value: -a\n");
        EXPECT(c1.resetValues().exec() == false);
        EXPECT_ERR(c1, "Error: No command given.\n");
        EXPECT_PARSE(c1, "one");
        EXPECT(c1.exec() == false);
        EXPECT_ERR(c1, "Error: Command 'one' has not been implemented.\n");

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

Primary:
  one       First sentence of description.
  three

Additional:
  two

Options:
  --help    Show this message and exit.
)");
    }

    // subcommand groups
    {
        cli = {};
        cli.command("1a").cmdGroup("First").cmdSortKey("1");
        EXPECT(cli.cmdSortKey() == "1");
        cli.command("1b");
        cli.command("2a").cmdGroup("Second").cmdSortKey("2");
        cli.command("3a").cmdGroup("Third").cmdSortKey("3");
        EXPECT_HELP(cli, "", 1 + R"(
usage: test [OPTIONS] command [args...]

First:
  1a
  1b

Second:
  2a

Third:
  3a

Options:
  --help    Show this message and exit.
)");
    }

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
        EXPECT_PARSE(cli, "help help");
        out.str("");
        cli.iostreams(nullptr, &out);
        EXPECT(cli.exec());
        cli.iostreams(nullptr, nullptr);
        EXPECT(out.str() == helpText);
        EXPECT_USAGE(cli, "", 1 + R"(
usage: test [--help] command [args...]
)");
        EXPECT_USAGE(cli, "help", 1 + R"(
usage: test help [-u, --usage] [--help] [command]
)");
        EXPECT_PARSE(cli, "help notACmd");
        EXPECT(!cli.exec());
        EXPECT_ERR(cli, 1 + R"(
Error: Command 'help': Help requested for unknown command: notACmd
)");
        EXPECT_PARSE(cli, "help help --usage");
        out.str("");
        cli.iostreams(nullptr, &out);
        EXPECT(cli.exec());
        cli.iostreams(nullptr, nullptr);
        EXPECT(out.str() == 1 + R"(
usage: test help [-u, --usage] [--help] [command]
)");
    }
}


/****************************************************************************
*
*   Argv to/from command line
*
***/

//===========================================================================
void argvTests() {
    Dim::CliLocal cli;

    // windows style argument parsing
    {
        auto fn = cli.toWindowsArgv;
        EXPECT_ARGV(fn, R"( a "" "c )", {"a", "", "c "});
        EXPECT_ARGV(fn, R"(a"" b ")", {"a", "b", ""});
        EXPECT_ARGV(fn, R"("abc" d e)", {"abc", "d", "e"});
        EXPECT_ARGV(fn, R"(a\\\b d"e f"g h)", {R"(a\\\b)", "de fg", "h"});
        EXPECT_ARGV(fn, R"(a\\\"b c d)", {R"(a\"b)", "c", "d"});
        EXPECT_ARGV(fn, R"(a\\\\"b c" d e)", {R"(a\\b c)", "d", "e"});
        EXPECT_ARGV(fn, R"(\ "\"" )", {"\\", "\""});
    }
}


/****************************************************************************
*
*   Option validation helpers
*
***/

//===========================================================================
void optCheckTests() {
    int line = 0;
    Dim::CliLocal cli;

    // require
    {
        cli = {};
        auto & count = cli.opt<int>("c", 1).require();
        EXPECT_PARSE(cli, "-c10");
        EXPECT(*count == 10);
        EXPECT_PARSE(cli, {}, false);
        EXPECT(*count == 1);
        EXPECT_ERR(cli, "Error: No value given for -c\n");
        cli = {};
        auto & imp = cli.opt<int>("?index i").require().implicitValue(5);
        EXPECT_PARSE(cli, "--index=10");
        EXPECT(*imp == 10);
        EXPECT_PARSE(cli, "--index");
        EXPECT(*imp == 5);
        EXPECT_PARSE(cli, {}, false);
        EXPECT_ERR(cli, "Error: No value given for --index\n");
    }

    // clamp and range
    {
        cli = {};
        auto & count = cli.opt<int>("<count>", 2).clamp(1, 10);
        auto & letter = cli.opt<char>("<letter>").range('a', 'z');
        EXPECT_PARSE(cli, "20 a");
        EXPECT(*count == 10);
        EXPECT(*letter == 'a');
        EXPECT_PARSE(cli, "5 0", false);
        EXPECT(*count == 5);
        EXPECT_ERR(cli, "Error: Out of range 'letter' value [a - z]: 0\n");
        EXPECT_PARSE(cli, "-- -5", false);
        EXPECT(*count == 1);
        EXPECT_ERR(cli, "Error: Missing argument: letter\n");
    }
}


/****************************************************************************
*
*   Flag values
*
***/

//===========================================================================
void flagTests() {
    int line = 0;
    Dim::CliLocal cli;

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
        EXPECT_PARSE(cli, "-o");
        EXPECT(*orange == "orange");
        EXPECT(orange.from() == "-o");
        EXPECT(orange.pos() == 1 && orange.size() == 1);
    }

    {
        cli = {};
        auto & on = cli.opt<bool>("on.", true).flagValue();
        auto & notOn = cli.opt(on, "!notOn.").flagValue(true);
        EXPECT_PARSE(cli, "--on");
        EXPECT(*notOn);
        EXPECT_HELP(cli, "", 1 + R"(
usage: test [OPTIONS]

Options:
  / --notOn  (default)
  --on

  --help     Show this message and exit.
)");
    }

    {
        cli = {};
        auto & opt = cli.opt<int>("x", 1).flagValue(true);
        cli.opt(opt, "y", 2).flagValue();
        EXPECT_PARSE(cli, "-y");
        EXPECT(*opt == 2);
        EXPECT_HELP(cli, "", 1 + R"(
usage: test [OPTIONS]

Options:
  -x        (default)
  -y

  --help    Show this message and exit.
)");
        EXPECT_PARSE(cli, "-w", false);
    }

    {
        cli = {};
        vector<int> vals;
        cli.optVec(&vals, "x").defaultValue(1).flagValue();
        cli.optVec(&vals, "y").defaultValue(2).flagValue(true);
        EXPECT_PARSE(cli, "-x");
        EXPECT(vals == vector<int>{1});
    }
}


/****************************************************************************
*
*   Response files
*
***/

//===========================================================================
template<typename T, int N>
void writeRsp(char const path[], T const (&data)[N]) {
    fstream f(path, ios::out | ios::trunc | ios::binary);
    f.write((char *) data, sizeof(*data) * (N - 1));
}

//===========================================================================
void responseTests() {
#ifdef FILESYSTEM
    namespace fs = FILESYSTEM;
    int line = 0;
    Dim::CliLocal cli;

    if (!fs::is_directory("test"))
        fs::create_directories("test");
    writeRsp("test/a.rsp", "1 @bu8.rsp 2\n");
    writeRsp("test/bu8.rsp", u8"\ufeffx\ny\n");
    writeRsp("test/cL.rsp", L"\ufeffc1 c2");
    writeRsp("test/du.rsp", u"\ufeffd1 d2");
    writeRsp("test/eBad.rsp", "\xff\xfe\x00\xd8\x20\x20");
    writeRsp("test/f.rsp", "f");
    writeRsp("test/gBad.rsp", "@eBad.rsp");
    writeRsp("test/hU.rsp", U"\ufeffh1 h2");
    writeRsp("test/reA.rsp", "@reB.rsp");
    writeRsp("test/reB.rsp", "@reA.rsp");
    writeRsp("test/reX.rsp", "@reX.rsp");

    cli = {};
    auto & args = cli.optVec<string>("[args]");
    EXPECT_PARSE(cli, "@test/a.rsp");
    EXPECT(*args == vector<string>{"1", "x", "y", "2"});

    EXPECT_PARSE(cli, "@test/does_not_exist.rsp", false);
    EXPECT_ERR(cli, "Error: Invalid response file: test/does_not_exist.rsp\n");
    cli.responseFiles(false);
    EXPECT_PARSE(cli, "@test/does_not_exist.rsp");
    EXPECT(args && args[0] == "@test/does_not_exist.rsp");
    cli.responseFiles(true);

#ifdef _MSC_VER
    EXPECT_PARSE(cli, "@test/cL.rsp @test/du.rsp @test/f.rsp");
    EXPECT(*args == vector<string>{"c1", "c2", "d1", "d2", "f"});
#else
    EXPECT_PARSE(cli, "@test/f.rsp");
    EXPECT(*args == vector<string>{"f"});
#endif

    EXPECT_PARSE(cli, "@test/gBad.rsp", false);
    EXPECT_ERR(cli, "Error: Invalid encoding: eBad.rsp\n");

    EXPECT_PARSE(cli, "@test/reA.rsp", false);
    EXPECT_ERR(cli, "Error: Recursive response file: reA.rsp\n");

    EXPECT_PARSE(cli, "@test/reX.rsp", false);
    EXPECT_ERR(cli, "Error: Recursive response file: reX.rsp\n");

#ifdef _MSC_VER
    {
        fstream f("test/f.rsp", ios::in, _SH_DENYRW);
        EXPECT_PARSE(cli, "@test/f.rsp", false);
        EXPECT_ERR(cli, "Error: Read error: test/f.rsp\n");
    }
#endif

#endif
}


/****************************************************************************
*
*   Basic
*
***/

//===========================================================================
void basicTests() {
    int line = 0;
    Dim::CliLocal cli;

    (void) cli.imbue(locale{});

    // assignment operator
    {
        Dim::Cli tmp{cli};
        tmp = cli;
    }

    {
        cli = {};
        cli.helpOpt().show(false);
        EXPECT_PARSE(cli);
        EXPECT_HELP(cli, "", 1 + R"(
usage: test
)");
    }

    {
        cli = {};
        auto & num = cli.opt<int>(" n number ", 1).desc("number is an int");
        cli.opt(num, "c").desc("alias for number").valueDesc("COUNT");
        cli.opt<int>("n2", 2).desc("no defaultDesc").defaultDesc("");
        cli.opt<int>("n3", 3).desc("custom defaultDesc").defaultDesc("three");
        auto & special =
            cli.opt<bool>("s special !S", false).desc("snowflake");
        auto & name = cli.group("name").title("Name options")
            .optVec<string>("name");
        EXPECT(cli.title() == "Name options");
        auto & keys = cli.group("")
            .optVec<string>("[key]").desc(
                "it's the key arguments with a very long description that "
                "wraps the line at least once, maybe more.");
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
        EXPECT_PARSE(cli, "-n3");
        EXPECT(*num == 3);
        EXPECT(!*special);
        EXPECT(!name);
        EXPECT(!keys);

        EXPECT_PARSE(cli, "--name two");
        EXPECT(*num == 0);
        EXPECT(name.size() == 1 && (*name)[0] == "two");

        EXPECT_PARSE(cli, "--name=three");
        EXPECT(name.size() == 1 && (*name)[0] == "three");

        EXPECT_PARSE(cli, "--name= key");
        EXPECT(*name == vector<string>({""s}));
        EXPECT(*keys == vector<string>({"key"s}));

        EXPECT_PARSE(cli, "-s-name=four key --name four");
        EXPECT(*special);
        EXPECT(*name == vector<string>({"four"s, "four"s}));
        EXPECT(*keys == vector<string>({"key"s}));

        EXPECT_PARSE(cli, "key extra");
        EXPECT(*keys == vector<string>({"key"s, "extra"s}));

        EXPECT_PARSE(cli, "- -- -s");
        EXPECT(!special && !*special);
        *num += 2;
        EXPECT(*num == 2);
        *special = name->empty();
        EXPECT(*special);
    }

    {
        cli = {};
        cli.opt<int>("[]", 1);
        EXPECT_USAGE(cli, "", 1 + R"(
usage: test [--help] [arg1]
)");
    }

    {
        cli = {};
        cli.opt<bool>("option with excessively long list of key names")
            .desc("Why so many?");
        cli.opt<string>("< equal '=' in name >")
            .desc("Equals are sometimes reserved.");
        EXPECT_HELP(cli, "", 1 + R"(
usage: test [OPTIONS] <equal '=' in name>
  equal '=' in name  Equals are sometimes reserved.

Options:
  --option, --with, --excessively, --long, --list, --of, --key, --names /
     --no-option, --no-with, --no-excessively, --no-long, --no-list, --no-of,
     --no-key, --no-names    Why so many?

  --help                     Show this message and exit.
)");
    }

    // optVec
    {
        cli = {};
        auto & strs = cli.optVec<string>("r ?s").implicitValue("a")
            .desc("String array.");
        EXPECT_PARSE(cli, "-s1 -s -r 2 -s3");
        EXPECT(strs.size() == 4);
        EXPECT(strs.pos(2) == 4);
        EXPECT(strs.pos() == 5);
        EXPECT(*strs == vector<string>({"1"s, "a"s, "2"s, "3"s}));
        EXPECT_HELP(cli, "", 1 + R"(
usage: test [OPTIONS]

Options:
  -r, -s [STRING]  String array.

  --help           Show this message and exit.
)");

        cli.optVec(strs, "string").desc("Alternate for string array.");
        EXPECT_HELP(cli, "", 1 + R"(
usage: test [OPTIONS]

Options:
  -r, -s [STRING]  String array.
  --string=STRING  Alternate for string array.

  --help           Show this message and exit.
)");
        EXPECT_PARSE(cli, "--string=a -sb");
        EXPECT(*strs == vector<string>({"a"s, "b"s}));
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
        EXPECT_PARSE(cli, "--path one");
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
        EXPECT_PARSE(cli, "--password=hi");
        EXPECT(*pass == "hi");
        in.clear();
        in.str("secret\nsecret\n");
        out.str("");
        cli.iostreams(&in, &out);
        EXPECT_PARSE(cli);
        EXPECT(*pass == "secret");
        EXPECT(out.str() == "Password: \nEnter again to confirm: \n");
        EXPECT(in.get() == EOF);
        in.clear();
        in.str("secret\nmistype_secret\n");
        out.str("");
        EXPECT_PARSE(cli, {}, false);
        EXPECT_ERR(cli, "Error: Confirm failed, entries not the same.\n");
        EXPECT(in.get() == EOF);
        if (prompt) {
            cli.iostreams(nullptr, nullptr);
            cout << "Expects password to be confirmed (empty is ok)." << endl
                 << "What you type should *NOT* be visible!" << endl;
            EXPECT_PARSE(cli);
            cout << "Entered password was '" << *pass << "'" << endl;
        }
    }

    // confirmation prompt
    {
        cli = {};
        auto & ask = cli.confirmOpt();
        EXPECT_PARSE(cli, "-y");
        EXPECT(*ask);
        in.clear();
        in.str("n\n");
        out.str("");
        cli.iostreams(&in, &out);
        EXPECT_PARSE(cli, {}, false, Dim::kExitOk);
        EXPECT(!*ask);
        EXPECT(out.str() == "Are you sure? [y/N]: ");
        EXPECT(in.get() == EOF);
        if (prompt) {
            cli.iostreams(nullptr, nullptr);
            cout << "Expects answer to be no." << endl
                 << "What you type should be visible." << endl;
            EXPECT_PARSE(cli, {}, false, Dim::kExitOk);
            EXPECT(!*ask);
        }
    }

    // prompt for array
    {
        cli = {};
        auto & ask = cli.optVec<string>("name").prompt(Dim::Cli::fPromptHide);
        in.clear();
        in.str("jack\n");
        out.str("");
        cli.iostreams(&in, &out);
        EXPECT_PARSE(cli);
        EXPECT(out.str() == "Name: \n");
        EXPECT(ask.size() && ask[0] == "jack");
    }

    // prompt with default
    {
        cli = {};
        auto & ask = cli.opt<string>("name", "jill").prompt();
        in.clear();
        in.str("jack\n");
        out.str("");
        cli.iostreams(&in, &out);
        EXPECT_PARSE(cli);
        EXPECT(out.str() == "Name [jill]: ");
        EXPECT(*ask == "jack");
    }
}

//===========================================================================
void envTests() {
#if !defined(DIMCLI_LIB_NO_ENV)
    int line = 0;
    Dim::CliLocal cli;
    int result;

    cli = {};
    auto & args = cli.optVec<string>("[args]");
    cli.envOpts("TEST_OPTS");
    result = putenv((char *) "TEST_OPTS=");
    EXPECT(!result);
    EXPECT_PARSE(cli, "c d");
    EXPECT(*args == vector<string>{"c", "d"});
    result = putenv((char *) "TEST_OPTS=a b");
    EXPECT(!result);
    EXPECT_PARSE(cli, "c d");
    EXPECT(*args == vector<string>{"a", "b", "c", "d"});
#endif
}


/****************************************************************************
*
*   Main
*
***/

//===========================================================================
static int runTests(bool prompt) {
    basicTests();
    flagTests();
    parseTests();
    choiceTests();
    helpTests();
    cmdTests();
    envTests();
    argvTests();
    optCheckTests();
    responseTests();
    promptTests(prompt);

    if (s_errors) {
        cerr << "*** TESTS FAILED ***" << endl;
        return Dim::kExitSoftware;
    }
    cout << "All tests passed" << endl;
    return 0;
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
    cli.helpNoArgs();
    cli.opt<bool>("test").desc("Run tests.");
    auto & prompt = cli.opt<bool>("prompt").desc("Run tests with prompting.");
    if (cli.parse(argc, argv))
        return runTests(*prompt);

    // Exiting without error (i.e. help or version)?
    if (!cli.exitCode())
        return 0;

    cout << "Number of args: " << argc << "\n";
    for (auto i = 0; i < argc; ++i)
        cout << i << ": {" << argv[i] << "}\n";
    cout << "Line: " << cli.toCmdline(argc, argv) << '\n';
    return 0;
}
