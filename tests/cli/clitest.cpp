// Copyright Glen Knowles 2016 - 2021.
// Distributed under the Boost Software License, Version 1.0.
//
// clitest.cpp - dimcli test cli

#include "pch.h"
#pragma hdrstop

using namespace std;

#if defined(_MSC_VER)
#elif defined(__clang__)
#pragma clang diagnostic ignored "-Wstring-plus-int"
#endif


/****************************************************************************
*
*   Globals
*
***/

#if defined(_WIN32)
const char kCommand[] = "test.exe";
#else
const char kCommand[] = "test";
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
#define EXPECT_CMDLINE(fn, ...) toCmdlineTest(__LINE__, fn, __VA_ARGS__)
#define EXPECT_ASSERT(text) assertTest(__LINE__, text)

//===========================================================================
void failed(int line, const char msg[]) {
    cerr << "Line " << line << ": EXPECT(" << msg << ") failed" << endl;
    s_errors += 1;
}

//===========================================================================
void parseTest(
    int line,
    Dim::Cli & cli,
    const string & cmdline = {},
    bool continueFlag = true,
    int exitCode = -1
) {
    auto args = cli.toWindowsArgv(cmdline);
    args.insert(args.begin(), kCommand);
    bool rc = cli.parse(args);
    if (exitCode == -1)
        exitCode = rc ? Dim::kExitOk : Dim::kExitUsage;
    if (rc != continueFlag || exitCode != cli.exitCode()) {
        if (exitCode) {
            cerr << cli.errMsg() << endl;
            if (!cli.errDetail().empty())
                cerr << cli.errDetail() << endl;
        }
        EXPECT(rc == continueFlag);
        EXPECT(exitCode == cli.exitCode());
    }
}

//===========================================================================
void errTest(int line, Dim::Cli & cli, const string & errText) {
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
    const string & cmd,
    const string & helpText
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
    const string & cmd,
    const string & usageText
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
    function<vector<string>(const string &)> fn,
    const string & cmdline,
    const vector<string> & argv
) {
    auto args = fn(cmdline);
    EXPECT(args == argv);
}

//===========================================================================
void toCmdlineTest(
    int line,
    function<string(size_t, char**)> fn,
    function<vector<string>(const string &)> fnv,
    const vector<string> & argv,
    const string & cmdline
) {
    auto pargs = Dim::Cli::toPtrArgv(argv);
    auto tmp = fn(pargs.size(), (char **) pargs.data());
    EXPECT(tmp == cmdline);
    EXPECT(argv == fnv(cmdline));
    if (tmp != cmdline || argv != fnv(cmdline))
        cerr << tmp << endl;

}

namespace {

class CliTest : public Dim::CliLocal {
public:
    CliTest() { maxWidth(80); }
};

}


/****************************************************************************
*
*   Improper usage assertions
*
***/

#ifdef DIMCLI_LIB_BUILD_COVERAGE

struct AssertInfo {
    const char * text;
    unsigned line;
};
vector<AssertInfo> s_asserts;

//===========================================================================
void Dim::assertHandler(const char expr[], unsigned line) {
    s_asserts.push_back({expr, line});
}

//===========================================================================
void assertTest(int line, const char text[]) {
    string tmp;
    for (auto && ai : s_asserts) {
        tmp += ai.text;
        tmp += '\n';
    }
    EXPECT(tmp == text);
    if (tmp != text)
        cerr << tmp;
    s_asserts.clear();
}

//===========================================================================
void assertTests() {
    CliTest cli;

    // bad usage
    enum MyType {} mval;
    cli.opt(&mval, "v");
    EXPECT_PARSE(cli, "-v x", false);
    EXPECT_ERR(cli, "Error: Invalid '-v' value: x\n");
    EXPECT_ASSERT(1 + R"(
!"no assignment from string or stream extraction operator"
)");
    cli.opt<bool>("t").implicitValue(false);
    EXPECT_ASSERT(1 + R"(
!"bool argument values can't be implicit"
)");

    // bad argument name
    cli = {};
    cli.opt<bool>("a=");
    EXPECT_ASSERT(1 + R"(
!"bad argument name, contains '='"
)");
    cli.opt<int>("[B] [C]");
    EXPECT_ASSERT(1 + R"(
!"argument with multiple operand names"
)");
    cli.opt<int>("-d");
    EXPECT_ASSERT(1 + R"(
!"bad argument name, contains '-'"
)");
    cli.opt<bool>("?e");
    EXPECT_ASSERT(1 + R"(
!"bad modifier '?' for bool argument"
)");
    cli.opt<bool>("f.");
    EXPECT_ASSERT(1 + R"(
!"bad modifier '.' for short name"
)");
    EXPECT_USAGE(cli, "", 1 + R"(
Usage: test [--help] [B]
)");
    EXPECT_ASSERT(1 + R"(
!"bad argument name, contains '='"
!"argument with multiple operand names"
!"bad argument name, contains '-'"
!"bad modifier '?' for bool argument"
!"bad modifier '.' for short name"
)");

    // bad vector size
    {
        cli = {};
        cli.optVec<string>("g")
            .size(-1)
            .size(-1, 0)
            .size(0, -10)
            .size(1, 0);
        EXPECT_ASSERT(1 + R"RAW(
!"bad optVec size, minimum must be >= 0"
!"bad optVec size, minimum must be >= 0"
!"bad optVec size, maximum must be >= 0 or -1 (unlimited)"
!"bad optVec size, min greater than max"
)RAW");
    }

    // exec usage
    {
        cli = {};
        cli.command("empty").action({});
        EXPECT_PARSE(cli, "empty");
        cli.exec();
        EXPECT_ERR(
            cli,
            "Error: Command 'empty' found by parse not defined.\n"
        );
        EXPECT_ASSERT(1 + R"(
!"command found by parse not defined"
)");
    }

    // finalOpt
    {
        cli = {};
        cli.opt<int>("[A]").finalOpt();
        cli.opt<int>("<B>");
        EXPECT_PARSE(cli, "1");
        EXPECT_ASSERT(1 + R"(
!"required operand after optional operand w/finalOpt."
)");

        cli = {};
        cli.optVec<int>("<A>");
        cli.opt<bool>("<B>").finalOpt();
        EXPECT_PARSE(cli, "1");
        EXPECT_ASSERT(1 + R"(
!"operand w/finalOpt after variable size operand."
)");

        cli = {};
        cli.opt<int>("[A]");
        cli.opt<int>("<B>").finalOpt();
        EXPECT_PARSE(cli, "1");
        EXPECT_ASSERT(1 + R"(
!"required operand w/finalOpt after optional operand."
)");
    }
}

#endif


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
void valueTests() {
    int line = 0;
    CliTest cli;

    // parse action
    {
        cli = {};
        auto & sum = cli.opt<int>("n number", 1)
            .desc("numbers to multiply")
            .parse([](auto & cli, auto & arg, const string & val) {
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
        auto & opt = cli.opt("[VALUE]", ExtractNoInsert::kBad)
            .desc("Value to attempt to parse.");
        EXPECT_PARSE(cli, "g");
        EXPECT(*opt == ExtractNoInsert::kGood);
        EXPECT_PARSE(cli, "b", false);
        EXPECT(*opt == ExtractNoInsert::kInvalid);
        EXPECT_HELP(cli, "", 1 + R"(
Usage: test [OPTIONS] [VALUE]
  VALUE     Value to attempt to parse.

Options:
  --help    Show this message and exit.
)");
    }

    // default render failure
    {
        cli = {};
        auto & opt = cli.opt("[VALUE]", ExtractWithInsert::kGood)
            .desc("Value to attempt to parse.");
        EXPECT_PARSE(cli);
        EXPECT(*opt == ExtractWithInsert::kGood);
        EXPECT_PARSE(cli, "b", false);
        EXPECT(*opt == ExtractWithInsert::kInvalid);
        EXPECT_HELP(cli, "", 1 + R"(
Usage: test [OPTIONS] [VALUE]
  VALUE     Value to attempt to parse. (default: g)

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
Usage: test [OPTIONS] [VALUE]
  VALUE     Value to attempt to parse.

Options:
  --help    Show this message and exit.
)");
    }

    // complex number
    {
        cli = {};
        auto & opt = cli.opt<complex<double>>("complex")
            .desc("Complex number to parse.");
        EXPECT_PARSE(cli, "--complex=(1.0,2)");
        EXPECT(*opt == complex<double>{1.0,2.0});
        EXPECT_HELP(cli, "", 1 + R"(
Usage: test [OPTIONS]

Options:
  --complex=VALUE  Complex number to parse. (default: (0,0))

  --help           Show this message and exit.
)");
    }
}


/****************************************************************************
*
*   Parsing failures
*
***/

//===========================================================================
void parseTests() {
    //int line = 0;
    CliTest cli;

    EXPECT_PARSE(cli, "-x", false);
    EXPECT_ERR(cli, "Error: Unknown option: -x\n");
    EXPECT_PARSE(cli, "--x", false);
    EXPECT_ERR(cli, "Error: Unknown option: --x\n");
    EXPECT_PARSE(cli, "--help=x", false);
    EXPECT_ERR(cli, "Error: Invalid '--help' value: x\n");

    cli.helpCmd();
    EXPECT_PARSE(cli, "x", false);
    EXPECT_ERR(cli, "Error: Unknown command: x\n");

    cli = {};
    EXPECT_PARSE(cli, "x", false);
    EXPECT_ERR(cli, "Error: Unexpected argument: x\n");

    cli.opt<int>("n", 1);
    cli.opt<int>("?o", 2).check([](auto & cli, auto & opt, auto & val) {
        return cli.badUsage("Malformed '"s + opt.from() + "' value: " + val);
    });
    EXPECT_PARSE(cli, "-na", false);
    EXPECT_ERR(cli, "Error: Invalid '-n' value: a\n");
    EXPECT_PARSE(cli, "-o", false);
    EXPECT_ERR(cli, "Error: Malformed '-o' value:\n");
    EXPECT_PARSE(cli, "-n", false);
    EXPECT_ERR(cli, "Error: No value given for -n\n");
    EXPECT_PARSE(cli, "-n a", false);
    EXPECT_ERR(cli, "Error: Invalid '-n' value: a\n");

    cli = {};
    cli.opt<int>("<n>", 1);
    EXPECT_PARSE(cli, "", false);
    EXPECT_ERR(cli, "Error: Option 'n' missing value.\n");
}


/****************************************************************************
*
*   Choice
*
***/

//===========================================================================
void choiceTests() {
    int line = 0;
    CliTest cli;
    enum class State { go, wait, stop };

    cli = {};
    auto & state = cli.opt("streetlight", State::wait)
        .desc("Color of street light.")
        .valueDesc("COLOR")
        .choice(State::go, "green", "Means go!")
        .choice(State::wait, "yellow", "Means wait, even if you're late.")
        .choice(State::stop, "red", "Means stop.");
    EXPECT_HELP(cli, "", 1 + R"(
Usage: test [OPTIONS]

Options:
  --streetlight=COLOR  Color of street light.
      green   Means go!
      yellow  Means wait, even if you're late. (default)
      red     Means stop.

  --help               Show this message and exit.
)");
    EXPECT_USAGE(cli, "", 1 + R"(
Usage: test [--streetlight=COLOR] [--help]
)");
    EXPECT_PARSE(cli, "--streetlight red");
    EXPECT(*state == State::stop);

    EXPECT_PARSE(cli, "--streetlight white", false);
    EXPECT_ERR(cli, 1 + R"(
Error: Invalid '--streetlight' value: white
Must be 'green', 'yellow', or 'red'.
)");

    state.defaultValue(State::go);
    EXPECT_HELP(cli, "", 1 + R"(
Usage: test [OPTIONS]

Options:
  --streetlight=COLOR  Color of street light.
      green   Means go! (default)
      yellow  Means wait, even if you're late.
      red     Means stop.

  --help               Show this message and exit.
)");

    cli = {};
    auto & state2 = cli.optVec<State>("[STREETLIGHTS]")
        .desc("Color of street lights.")
        .valueDesc("COLOR")
        .choice(State::stop, "red", "Means stop.", "3")
        .choice(State::wait, "yellow", "Means wait, even if you're late.", "2")
        .choice(State::go, "green", "Means go!", "1");
    EXPECT_HELP(cli, "", 1 + R"(
Usage: test [OPTIONS] [STREETLIGHTS...]
  STREETLIGHTS  Color of street lights.
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
Error: Invalid 'STREETLIGHTS' value: white
Must be 'green', 'yellow', or 'red'.
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
Must be 'one' or 'two'.
)");

    nums.choice(3, "three").choice(4, "four").choice(5, "five")
        .choice(6, "six").choice(7, "seven").choice(8, "eight")
        .choice(9, "nine").choice(10, "ten").choice(11, "eleven")
        .choice(12, "twelve");
    EXPECT_PARSE(cli, "-n white", false);
    EXPECT_ERR(cli, 1 + R"(
Error: Invalid '-n' value: white
Must be 'one', 'two', 'three', 'four', 'five', 'six', 'seven', 'eight', 'nine',
'ten', 'eleven', or 'twelve'.
)");
}


/****************************************************************************
*
*   Help and version
*
***/

//===========================================================================
void helpTextTests() {
    int line = 0;
    CliTest cli;
    ostringstream out;
    string raw;

    // wrap table cell indent
    raw.clear();
    out.str("");
    for (auto i = 0; i < 6; ++i) {
        raw += "\f\a10 10\a";   // column min/max width to 8, (10% of 80)
        raw.append(2 * i, ' ');
        raw.append(5, (char) i + 'A');
        raw += '\t';
        raw.append(8, (char) i + 'a');
        raw += '\n';
    }
    cli.printText(out, raw);
    auto tmp = out.str();
    EXPECT(tmp == 1 + R"(
AAAAA   aaaaaaaa
  BBBBB  bbbbbbbb
    CCCCC  cccccccc
      DDDDD
        dddddddd
  EEEEE  eeeeeeee
    FFFFF  ffffffff)");

    // passthru invalid column min/max width
    out.str("");
    raw = "\f\aone\a\ttwo\n";
    cli.printText(out, raw);
    tmp = out.str();
    EXPECT(tmp == "\aone\a       two");

    // indent and unindent wrapped lines
    out.str("");
    cli.maxWidth(50);
    raw = "\fnone\tThe quick brown fox jumped over the lazy dog.\n"
        "\fchild +2\t\v\vThe quick brown fox jumped over the lazy dog.\n"
        "\fpara +2\t\r\r  The quick brown fox jumped over the lazy dog.\n"
        "\n";
    cli.printText(out, raw);
    tmp = out.str();
    EXPECT(tmp == 1 + R"(
none      The quick brown fox jumped over the
          lazy dog.
child +2  The quick brown fox jumped over the
            lazy dog.
para +2     The quick brown fox jumped over the
          lazy dog.
)");

    // three columns
    out.str("");
    cli.maxWidth(80);
    raw = "one\t1\tfirst\n"
        "two\t2\tsecond\n"
        "three\t3\tthird\n"
        "\n";
    cli.printText(out, raw);
    tmp = out.str();
    EXPECT(tmp == 1 + R"(
one         1           first
two         2           second
three       3           third
)");
}

//===========================================================================
void helpTests() {
    int line = 0;
    CliTest cli;
    istringstream in;
    ostringstream out;

    // no options
    {
        cli = {};
        cli.helpOpt().show(false);
        EXPECT_PARSE(cli);
        EXPECT_HELP(cli, {}, 1 + R"(
Usage: test
)");
    }

    {
        cli = {};
        auto & num = cli.opt<int>("n quantity", 1).desc("quantity is an int");
        cli.opt(num, "c").desc("alias for quantity").valueDesc("COUNT");
        cli.opt<int>("n2", 2).desc("no defaultDesc").defaultDesc({});
        cli.opt<int>("n3", 3).desc("custom defaultDesc").defaultDesc("three");
        cli.opt<bool>("oversized-for-the-desc-col.")
            .desc("slightly too long option name that also has a long "
                  "description");
        cli.opt<bool>("s special !S", false).desc("snowflake");
        cli.group("name").title("Name options")
            .optVec<string>("name");
        EXPECT(cli.title() == "Name options");
        cli.group({}).optVec<string>("[KEY]").desc(
            "it's the key arguments with a very long description that "
            "wraps the line at least once, maybe more.");
        cli.title(
            "Long explanation of this very short set of options, it's so "
            "long that it even wraps around to the next line");
        EXPECT_HELP(cli, {}, 1 + R"(
Usage: test [OPTIONS] [KEY...]
  KEY       it's the key arguments with a very long description that wraps the
            line at least once, maybe more.

Long explanation of this very short set of options, it's so long that it even
wraps around to the next line:
  -c COUNT                    alias for quantity (default: 0)
  -n, --quantity=NUM          quantity is an int (default: 1)
  --n2=NUM                    no defaultDesc
  --n3=NUM                    custom defaultDesc (default: three)
  --oversized-for-the-desc-col  slightly too long option name that also has a
                              long description
  -s, --special / -S, --no-special
                              snowflake

Name options:
  --name=STRING

  --help                      Show this message and exit.
)");
        EXPECT_USAGE(cli, {}, 1 + R"(
Usage: test [-c COUNT] [-n, --quantity=NUM] [--n2=NUM] [--n3=NUM]
            [--name=STRING] [--oversized-for-the-desc-col] [-s, --special]
            [--help] [KEY...]
)");
        out.str("");
        EXPECT(cli.printUsage(out, kCommand, "") == Dim::kExitOk);
        EXPECT(out.str() == "Usage: test [OPTIONS] [KEY...]\n");
        out.str("");
        cli.printOperands(out, "");
        auto tmp = out.str();
        EXPECT(tmp == 1 + R"(
  KEY       it's the key arguments with a very long description that wraps the
            line at least once, maybe more.)"
        );
        out.str("");
        cli.printOptions(out, "");
        tmp = out.str();
        EXPECT(tmp == R"(
Long explanation of this very short set of options, it's so long that it even
wraps around to the next line:
  -c COUNT                    alias for quantity (default: 0)
  -n, --quantity=NUM          quantity is an int (default: 1)
  --n2=NUM                    no defaultDesc
  --n3=NUM                    custom defaultDesc (default: three)
  --oversized-for-the-desc-col  slightly too long option name that also has a
                              long description
  -s, --special / -S, --no-special
                              snowflake

Name options:
  --name=STRING

  --help                      Show this message and exit.)"
        );
        out.str("");
        cli.printCommands(out);
        tmp = out.str();
        EXPECT(tmp == "");

        // with maxWidth of 70
        cli.maxWidth(70, 10, 20); // for coverage of explicit desc col
        cli.maxWidth(70);
        EXPECT_HELP(cli, {}, 1 + R"(
Usage: test [OPTIONS] [KEY...]
  KEY      it's the key arguments with a very long description that
           wraps the line at least once, maybe more.

Long explanation of this very short set of options, it's so long that
it even wraps around to the next line:
  -c COUNT            alias for quantity (default: 0)
  -n, --quantity=NUM  quantity is an int (default: 1)
  --n2=NUM            no defaultDesc
  --n3=NUM            custom defaultDesc (default: three)
  --oversized-for-the-desc-col
                      slightly too long option name that also has a
                      long description
  -s, --special / -S, --no-special
                      snowflake

Name options:
  --name=STRING

  --help              Show this message and exit.
)");
        EXPECT_USAGE(cli, {}, 1 + R"(
Usage: test [-c COUNT] [-n, --quantity=NUM] [--n2=NUM] [--n3=NUM]
            [--name=STRING] [--oversized-for-the-desc-col]
            [-s, --special] [--help] [KEY...]
)");

        // with maxWidth of 50
        cli.maxWidth(50);
        EXPECT_HELP(cli, {}, 1 + R"(
Usage: test [OPTIONS] [KEY...]
  KEY     it's the key arguments with a very long
          description that wraps the line at
          least once, maybe more.

Long explanation of this very short set of
options, it's so long that it even wraps around
to the next line:
  -c COUNT       alias for quantity (default: 0)
  -n, --quantity=NUM
                 quantity is an int (default: 1)
  --n2=NUM       no defaultDesc
  --n3=NUM       custom defaultDesc (default:
                 three)
  --oversized-for-the-desc-col
                 slightly too long option name
                 that also has a long description
  -s, --special / -S, --no-special
                 snowflake

Name options:
  --name=STRING

  --help         Show this message and exit.
)");
        EXPECT_USAGE(cli, {}, 1 + R"(
Usage: test [-c COUNT] [-n, --quantity=NUM]
            [--n2=NUM] [--n3=NUM] [--name=STRING]
            [--oversized-for-the-desc-col]
            [-s, --special] [--help] [KEY...]
)");

    }

    // option with enough names to wrap
    {
        cli = {};
        cli.opt<bool>("option with excessively long list of key names")
            .desc("Why so many?");
        cli.opt<string>("< equal '=' in name >")
            .desc("Equals are sometimes reserved.");
        EXPECT_HELP(cli, "", 1 + R"(
Usage: test [OPTIONS] <equal '=' in name>
  equal '=' in name  Equals are sometimes reserved.

Options:
  --option, --with, --excessively, --long, --list, --of, --key, --names /
    --no-option, --no-with, --no-excessively, --no-long, --no-list, --no-of,
    --no-key, --no-names
            Why so many?

  --help    Show this message and exit.
)");
    }

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
        out.clear();
        out.str("");
        cli.iostreams(nullptr, &out);
        EXPECT_PARSE(cli, {}, false, Dim::kExitOk);
        cli.iostreams(nullptr, nullptr);
        EXPECT(out.str() == 1 + R"(
Usage: test [OPTIONS]

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

        cli = {};
        cli.opt<bool>("help. ?", false)
            .check([](auto & cli, auto & opt, auto &) {
                if (*opt) {
                    cli.printHelp(cli.conout(), {}, cli.commandMatched());
                    return false;
                }
                return true;
            });
        out.clear();
        out.str("");
        cli.iostreams(nullptr, &out);
        EXPECT_PARSE(cli, "-?", false, Dim::kExitOk);
        EXPECT(out.str() == 1 + R"(
Usage: test [OPTIONS]

Options:
  -?, --help
)");
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

Usage: test [OPTIONS]

Description.

Options:
  --help    Show this message and exit.

Multiline footer:
- first reference
- second reference
)");
        EXPECT_PARSE(cli, "--no-help");
        cli.helpOpt().parse([](auto &, auto & opt, auto &) {
            *opt = false;
            return true;
        });
        EXPECT_PARSE(cli, "--help=1");
    }

    // group sortKey
    {
        cli = {};
        cli.group("One").sortKey("1").opt("1", true).desc("First option.");
        EXPECT(cli.sortKey() == "1");
        cli.group("Two").sortKey("2").opt("2", true).desc("Second option.");
        cli.group("Three").sortKey("3").opt("3", true).desc("Third option.");
        EXPECT_HELP(cli, "", 1 + R"(
Usage: test [OPTIONS]

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
    CliTest cli;
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
        EXPECT(c2.commandMatched() == "one");
        EXPECT_PARSE(c1, "-a", false);
        EXPECT_ERR(c2, "Error: Unknown option: -a\n");
        EXPECT_PARSE(c1, "two -a", false);
        EXPECT_ERR(c2, "Error: Command 'two': No value given for -a\n");
        EXPECT(c1.resetValues().exec() == Dim::kExitUsage);
        EXPECT_ERR(c1, "Error: No command given.\n");
        EXPECT_PARSE(c1, "one");
        EXPECT(c1.exec() == Dim::kExitSoftware);
        EXPECT_ERR(c1, "Error: Command 'one' has not been implemented.\n");

        EXPECT_HELP(c1, "one", 1 + R"(
Usage: test one [OPTIONS]

First sentence of description. Rest of one's description.

Options:
  -a NUM    (default: 1)

  --help    Show this message and exit.
)");
        EXPECT_HELP(c1, "three", 1 + R"(
Usage: test three [OPTIONS]

Options:
  -b NUM    (default: 99)

  --help    Show this message and exit.
)");
        EXPECT_HELP(c1, "", 1 + R"(
Usage: test [OPTIONS] COMMAND [ARGS...]

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
Usage: test [OPTIONS] COMMAND [ARGS...]

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

    // allowed top level operands
    {
        cli = {};
        auto & p1 = cli.opt<int>("<1>");
        cli.command("one");
        EXPECT_PARSE(cli, "5 one");
        EXPECT(*p1 == 5);
        EXPECT(cli.commandMatched() == "one");
        EXPECT_HELP(cli, "", 1 + R"(
Usage: test [OPTIONS] COMMAND [ARGS...]

Commands:
  one

Options:
  --help    Show this message and exit.
)");
        EXPECT_HELP(cli, "one", 1 + R"(
Usage: test one [OPTIONS]

Options:
  --help    Show this message and exit.
)");
    }
        EXPECT_HELP(cli, "unknown", 1 + R"(
Usage: test unknown [ARGS...]
)");

    // unknownCommand
    {
        cli = {};
        cli.unknownCmd();
        cli.unknownCmd([](auto &) {
            return true;
        });
        EXPECT_PARSE(cli, "unknown a b c");
        EXPECT(cli.commandMatched() == "unknown");
        EXPECT(cli.unknownArgs() == vector<string>{"a", "b", "c"});
        EXPECT_PARSE(cli, "");
        EXPECT_HELP(cli, "", 1 + R"(
Usage: test [OPTIONS] [COMMAND] [ARGS...]

Options:
  --help    Show this message and exit.
)");
        EXPECT_HELP(cli, "unknown", 1 + R"(
Usage: test unknown [ARGS...]
)");
    }

    // helpCmd
    {
        cli = {};
        cli.helpCmd();
        EXPECT_HELP(cli, "", 1 + R"(
Usage: test [OPTIONS] COMMAND [ARGS...]

Commands:
  help      Show help for individual commands and exit.

Options:
  --help    Show this message and exit.
)");
        auto helpText = 1 + R"(
Usage: test help [OPTIONS] [COMMAND]

Show help for individual commands and exit. If no command is given the list of
commands and general options are shown.
  COMMAND   Command to show help information about.

Options:
  -u, --usage / --no-usage  Only show condensed usage.

  --help                    Show this message and exit.
)";
        EXPECT_HELP(cli, "help", helpText);
        EXPECT_PARSE(cli, "help help");
        out.str("");
        cli.iostreams(nullptr, &out);
        EXPECT(cli.exec() == Dim::kExitOk);
        cli.iostreams(nullptr, nullptr);
        EXPECT(out.str() == helpText);
        EXPECT_USAGE(cli, "", 1 + R"(
Usage: test [--help] COMMAND [ARGS...]
)");
        EXPECT_USAGE(cli, "help", 1 + R"(
Usage: test help [-u, --usage] [--help] [COMMAND]
)");
        EXPECT_PARSE(cli, "help notACmd");
        out.str("");
        EXPECT(cli.exec(out) == Dim::kExitUsage);
        EXPECT(out.str() == 1 + R"(
Error: Command 'help': Help requested for unknown command: notACmd
)");
        EXPECT_PARSE(cli, "help help --usage");
        out.str("");
        cli.iostreams(nullptr, &out);
        EXPECT(cli.exec() == Dim::kExitOk);
        cli.iostreams(nullptr, nullptr);
        EXPECT(out.str() == 1 + R"(
Usage: test help [-u, --usage] [--help] [COMMAND]
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
    int line = 0;
    CliTest cli;

    // windows style argument parsing
    {
        auto fn = cli.toWindowsCmdline;
        auto fnv = cli.toWindowsArgv;
        EXPECT_ARGV(fnv, R"( a "" "c )", {"a", "", "c "});
        EXPECT_ARGV(fnv, R"(a"" b ")", {"a", "b", ""});
        EXPECT_ARGV(fnv, R"("abc" d e)", {"abc", "d", "e"});
        EXPECT_ARGV(fnv, R"(a\\\b d"e f"g h)", {R"(a\\\b)", "de fg", "h"});
        EXPECT_ARGV(fnv, R"(a\\\"b c d)", {R"(a\"b)", "c", "d"});
        EXPECT_ARGV(fnv, R"(a\\\\"b c" d e)", {R"(a\\b c)", "d", "e"});
        EXPECT_ARGV(fnv, R"(\ "\"" )", {"\\", "\""});

        EXPECT_CMDLINE(fn, fnv, {}, "");
        EXPECT_CMDLINE(fn, fnv, {"a", "b", "c"}, "a b c");
        EXPECT_CMDLINE(fn, fnv, {"a", "b c", "d"}, "a \"b c\" d");
        EXPECT_CMDLINE(fn, fnv, {R"(\a)"}, R"(\a)");
        EXPECT_CMDLINE(fn, fnv, {R"(" \ " \")"}, R"("\" \ \" \\\"")");
    }

    // gnu style
    {
        auto fn = cli.toGnuCmdline;
        auto fnv = cli.toGnuArgv;
        EXPECT_ARGV(fnv, R"(\a'\b'  'c')", {"ab", "c"});
        EXPECT_ARGV(fnv, "a 'b", {"a", "b"});

        EXPECT_CMDLINE(fn, fnv, {}, "");
        EXPECT_CMDLINE(fn, fnv, {"a", "b", "c"}, "a b c");
        EXPECT_CMDLINE(fn, fnv, {"a", "b c", "d"}, "a b\\ c d");
    }

    // glib style
    {
        auto fn = cli.toGlibCmdline;
        auto fnv = cli.toGlibArgv;
        EXPECT_ARGV(fnv, 1 + R"(
\a\
b # c)", {"ab"});
        EXPECT_ARGV(fnv, "\\\n#\n", {});
        EXPECT_ARGV(fnv, " 'a''b", {"ab"});
        EXPECT_ARGV(fnv, 1 + R"(
"a"b"\$\
c\d)", {"ab$c\\d"});

        EXPECT_CMDLINE(fn, fnv, {}, "");
        EXPECT_CMDLINE(fn, fnv, {"a", "b", "c"}, "a b c");
        EXPECT_CMDLINE(fn, fnv, {"a", "b c", "d"}, "a b\\ c d");
    }

    // argv to/from cmdline
    const char cmdline[] = "a b c";
    auto a1 = cli.toArgv(cmdline);
    EXPECT(cli.toCmdline(a1) == cmdline);
    auto p1 = cli.toPtrArgv(a1);
    EXPECT(cli.toCmdline(p1.size(), p1.data()) == cmdline);
    const wchar_t * wargs[] = { L"a", L"b", L"c", NULL };
    a1 = cli.toArgv(sizeof wargs / sizeof *wargs - 1, (wchar_t **) wargs);
    EXPECT(cli.toCmdline(a1) == cmdline);
    EXPECT(cli.toCmdlineL("a", 'b', "c"s) == cmdline);
}


/****************************************************************************
*
*   Option validation helpers
*
***/

//===========================================================================
void optCheckTests() {
    int line = 0;
    CliTest cli;

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
        EXPECT_ERR(cli, 1 + R"(
Error: Out of range 'letter' value: 0
Must be between 'a' and 'z'.
)");
        EXPECT_PARSE(cli, "-- -5", false);
        EXPECT(*count == 1);
        EXPECT_ERR(cli, "Error: Option 'letter' missing value.\n");
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
    CliTest cli;

    // flagValue
    {
        cli = {};
        string fruit;
        cli.group("fruit").title("Type of fruit");
        auto & orange = cli.opt(&fruit, "o orange", "orange").flagValue();
        cli.opt(&fruit, "a", "apple").flagValue(true);
        cli.opt(orange, "p", "pear").flagValue();
        cli.group("~").title("Other");
        EXPECT_USAGE(cli, "", 1 + R"(
Usage: test [-o, --orange] [-p] [--help]
)");
        EXPECT_HELP(cli, "", 1 + R"(
Usage: test [OPTIONS]

Type of fruit:
  -a            (default)
  -o, --orange
  -p

Other:
  --help        Show this message and exit.
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
Usage: test [OPTIONS]

Options:
  / --notOn  (default)
  --on

  --help     Show this message and exit.
)");
    }

    {
        cli = {};
        auto & opt = cli.opt<int>("x", 1).flagValue(true);
        cli.opt(opt, "y !z", 2).flagValue();
        EXPECT_PARSE(cli, "-y");
        EXPECT(*opt == 2);
        EXPECT_PARSE(cli, "-z");
        EXPECT(*opt == 1);
        EXPECT_HELP(cli, "", 1 + R"(
Usage: test [OPTIONS]

Options:
  -x        (default)
  -y / -z

  --help    Show this message and exit.
)");
        EXPECT_PARSE(cli, "-w", false);
    }

    {
        cli = {};
        vector<int> vals;
        cli.optVec(&vals, "x").defaultValue(1).flagValue();
        cli.optVec(&vals, "y !z").defaultValue(2).flagValue(true);
        EXPECT_PARSE(cli, "-x");
        EXPECT(vals == vector<int>{1});
        EXPECT_PARSE(cli, "-z");
        EXPECT(vals.empty());
    }
}


/****************************************************************************
*
*   Response files
*
***/

//===========================================================================
template<typename T, int N>
void writeRsp(const char path[], T const (&data)[N]) {
    fstream f(path, ios::out | ios::trunc | ios::binary);
    f.write((char *) data, sizeof *data * (N - 1));
}

//===========================================================================
void responseTests() {
#ifdef FILESYSTEM
    namespace fs = FILESYSTEM;
    int line = 0;
    CliTest cli;

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
    writeRsp("test/none.rsp", " ");
    writeRsp("test/reA.rsp", "@reB.rsp");
    writeRsp("test/reB.rsp", "@reA.rsp");
    writeRsp("test/reX.rsp", "@reX.rsp");

    cli = {};
    auto & args = cli.optVec<string>("[ARGS]");
    EXPECT_PARSE(cli, "@test/a.rsp");
    EXPECT(*args == vector<string>{"1", "x", "y", "2"});

    EXPECT_PARSE(cli, "@test/does_not_exist.rsp", false);
    EXPECT_ERR(cli, "Error: Invalid response file: test/does_not_exist.rsp\n");
    cli.responseFiles(false);
    EXPECT_PARSE(cli, "@test/does_not_exist.rsp");
    EXPECT(args && args[0] == "@test/does_not_exist.rsp");
    cli.responseFiles(true);

    EXPECT_PARSE(cli, "@test/none.rsp");
    EXPECT(args.size() == 0);

    EXPECT_PARSE(cli, "@test/cL.rsp @test/f.rsp");
    EXPECT(*args == vector<string>{"c1", "c2", "f"});

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
#else
    {
        chmod("test/f.rsp", 0111);
        EXPECT_PARSE(cli, "@test/f.rsp", false);
        EXPECT_ERR(cli, "Error: Read error: test/f.rsp\n");
        chmod("test/f.rsp", 0555);
    }
#endif

#endif
}


/****************************************************************************
*
*   std::filesystem tests
*
***/

//===========================================================================
void filesystemTests() {
#ifdef FILESYSTEM
    int line = 0;
    CliTest cli;
    namespace fs = FILESYSTEM;

    {
        cli = {};
        fs::path path = "path";
        ostringstream os;
        os << path;
        cli.opt(&path, "path", path)
            .desc("std::filesystem::path");
        EXPECT_PARSE(cli, "--path one");
        EXPECT(path == "one");
        EXPECT_HELP(cli, "", 1 + R"(
Usage: test [OPTIONS]

Options:
  --path=FILE  std::filesystem::path (default: )"
        + os.str()
        + R"()

  --help       Show this message and exit.
)");
    }
#endif
}


/****************************************************************************
*
*   Parse and exec variations
*
***/

//===========================================================================
void execTests() {
    int line = 0;
    CliTest cli;
    ostringstream out;

    const char * argsNone[] = { "test", nullptr };
    auto nargsNone = sizeof argsNone / sizeof *argsNone - 1;
    const char * argsUnknown[] = { "test", "unknown", nullptr };
    auto nargsUnknown = sizeof argsUnknown / sizeof *argsUnknown - 1;

    auto vargsNone = vector<string>{argsNone, argsNone + nargsNone};
    auto vargsUnknown = vector<string>{argsUnknown, argsUnknown + nargsUnknown};

    {
        cli = {};
        auto rc = cli.parse(out, nargsUnknown, (char **) argsUnknown);
        EXPECT(rc == false && cli.exitCode() == Dim::kExitUsage);
        EXPECT(out.str() == "Error: Unexpected argument: unknown\n");
        out.clear();
        out.str({});
        rc = cli.parse(out, nargsNone, (char **) argsNone);
        EXPECT(rc && out.str() == "");
    }

    {
        cli = {};
        cli.action([](auto &) {});
        auto rc = cli.exec(nargsNone, (char **) argsNone);
        EXPECT(rc == Dim::kExitOk);
        out.clear();
        out.str({});
        rc = cli.exec(out, nargsNone, (char **) argsNone);
        EXPECT(rc == Dim::kExitOk && out.str() == "");
        rc = cli.exec(vargsNone);
        EXPECT(rc == Dim::kExitOk);
        out.clear();
        out.str({});
        cli = {};
        rc = cli.exec(out, vargsNone);
        EXPECT(rc == Dim::kExitUsage);
        EXPECT(out.str() == "Error: No command given.\n");
    }
}


/****************************************************************************
*
*   Vector options
*
***/

//===========================================================================
void vectorTests() {
    int line = 0;
    CliTest cli;

    // vector option
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
Usage: test [OPTIONS]

Options:
  -r, -s [STRING]  String array.

  --help           Show this message and exit.
)");

        cli.optVec(strs, "string").desc("Alternate for string array.");
        EXPECT_HELP(cli, "", 1 + R"(
Usage: test [OPTIONS]

Options:
  -r, -s [STRING]  String array.
  --string=STRING  Alternate for string array.

  --help           Show this message and exit.
)");
        EXPECT_PARSE(cli, "--string=a -sb");
        EXPECT(*strs == vector<string>({"a"s, "b"s}));
    }

    // optVec<bool>
    {
        cli = {};
        vector<bool> v0;
        cli.optVec<bool>(&v0, "[ZERO]").desc("External bool");
        auto & v1 = cli.optVec<bool>("[ONE]").desc("Internal bool.");
        EXPECT_PARSE(cli, "0");
        EXPECT(v0.size() == 1 && v0[0] == 0);
        EXPECT(v1.size() == 0);
    }

    // vector option with size
    {
        cli = {};
        auto & v0 = cli.optVec<int>("0").size(0).desc("None allowed.");
        EXPECT_PARSE(cli, "");
        EXPECT(v0.size() == 0);
        EXPECT_PARSE(cli, "-00", false);
        EXPECT_ERR(cli, 1 + R"(
Error: Too many '-0' values: 0
The maximum number of values is 0.
)");
        auto & v1 = cli.optVec<int>("1").size(1).desc("Not more than one.");
        auto & vn = cli.optVec<int>("N").desc("Unlimited.");
        EXPECT_PARSE(cli, "-11");
        EXPECT(v1.size() == 1 && v1[0] == 1);
        EXPECT(vn.size() == 0);
        EXPECT_HELP(cli, "", 1 + R"(
Usage: test [OPTIONS]

Options:
  -0 NUM    None allowed. (limit: 0)
  -1 NUM    Not more than one. (limit: 1)
  -N NUM    Unlimited.

  --help    Show this message and exit.
)");
    }

    // optional vector operand with size
    {
        cli = {};
        auto & v0 = cli.optVec<int>("[ONE]").size(1, 2);
        EXPECT_PARSE(cli, "1 2 3", false);
        EXPECT_ERR(cli, "Error: Unexpected argument: 3\n");
        EXPECT(v0.size() == 0);
    }

    // vector operand with 0 size (ignored)
    {
        cli = {};
        auto & v0 = cli.optVec<int>("<ONE>").size(0);
        EXPECT_PARSE(cli, "", true);
        EXPECT(v0.size() == 0);
    }

    // vector with too few values
    {
        cli = {};
        cli.optVec<int>("[FIRST]").size(2, -1);
        EXPECT_PARSE(cli, "1", false);
        EXPECT_ERR(cli, "Error: Unexpected argument: 1\n");

        cli = {};
        cli.optVec<int>("<SECOND>").size(2, -1);
        EXPECT_PARSE(cli, "1", false);
        EXPECT_ERR(cli, 1 + R"(
Error: Option 'SECOND' missing value.
Must have 2 or more values.
)");

        cli = {};
        cli.optVec<int>("3").size(2);
        EXPECT_PARSE(cli, "-3 1", false);
        EXPECT_ERR(cli, 1 + R"(
Error: Option '-3' missing value.
Must have 2 values.
)");

        cli = {};
        cli.optVec<int>("fourth").size(2, 3);
        EXPECT_PARSE(cli, "--fourth=1", false);
        EXPECT_ERR(cli, 1 + R"(
Error: Option '--fourth' missing value.
Must have 2 to 3 values.
)");
    }

    // required vector operand with size
    {
        cli = {};
        auto & v0 = cli.optVec<int>("<ONE>").size(1, 2)
            .desc("The one and only?");
        EXPECT_PARSE(cli, "", false);
        EXPECT_ERR(cli, 1 + R"(
Error: Option 'ONE' missing value.
Must have 1 to 2 values.
)");
        EXPECT(v0.size() == 0);
        EXPECT_HELP(cli, "", 1 + R"(
Usage: test [OPTIONS] ONE...
  ONE       The one and only? (limit: 1 to 2)

Options:
  --help    Show this message and exit.
)");

        v0.size(2, -1);
        EXPECT_PARSE(cli, "", false);
        EXPECT_ERR(cli, 1 + R"(
Error: Option 'ONE' missing value.
Must have 2 or more values.
)");
        EXPECT_HELP(cli, "", 1 + R"(
Usage: test [OPTIONS] ONE...
  ONE       The one and only? (limit: 2+)

Options:
  --help    Show this message and exit.
)");
    }

    // positional argument matching
    {
        cli = {};
        auto & v1 = cli.optVec<int>("<ONE>");
        auto & v2 = cli.optVec<int>("<TWO>");
        EXPECT_PARSE(cli, "1 2 3");
        EXPECT(*v1 == vector<int>{1, 2});
        EXPECT(*v2 == vector<int>{3});
    }
    {
        cli = {};
        auto & v0 = cli.optVec<int>("[ZERO]").size(2);
        auto & v1 = cli.optVec<int>("[ONE]");
        auto & v2 = cli.optVec<int>("<TWO>").size(1);
        auto & v3 = cli.optVec<int>("<THREE>").size(2);
        EXPECT_PARSE(cli, "1 2 3");
        EXPECT(v0->empty());
        EXPECT(v1->empty());
        EXPECT(*v2 == vector<int>{1});
        EXPECT(*v3 == vector<int>{2, 3});

        EXPECT_PARSE(cli, "1 2", false);
        EXPECT_ERR(cli, 1 + R"(
Error: Option 'THREE' missing value.
Must have 2 values.
)");
        EXPECT(v0->empty());
        EXPECT(*v1 == vector<int>{1});
        EXPECT(*v2 == vector<int>{2});
        EXPECT(v3->empty());

        EXPECT_PARSE(cli, "1 2 3 4");
        EXPECT(v0->empty());
        EXPECT(*v1 == vector<int>{1});
        EXPECT(*v2 == vector<int>{2});
        EXPECT(*v3 == vector<int>{3, 4});

        EXPECT_PARSE(cli, "1 2 3 4 5");
        EXPECT(*v0 == vector<int>{1, 2});
        EXPECT(*v1 == vector<int>{});
        EXPECT(*v2 == vector<int>{3});
        EXPECT(*v3 == vector<int>{4, 5});
    }
}


/****************************************************************************
*
*   Basic
*
***/

//===========================================================================
void basicTests() {
    int line = 0;
    CliTest cli;

    // assignment operator
    {
        Dim::Cli tmp{cli};
        tmp = cli;
    }

    {
        cli = {};
        auto & num = cli.opt<int>(" n number ", 1);
        cli.opt(num, "c");
        cli.opt<int>("n2", 2);
        cli.opt<int>("n3", 3);
        auto & special = cli.opt<bool>("s special !S", false).desc("snowflake");
        auto & name = cli.group("name").title("Name options")
            .optVec<string>("name");
        EXPECT(cli.title() == "Name options");
        auto & keys = cli.group({}).optVec<string>("[KEY]");
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
        EXPECT_USAGE(cli, {}, 1 + R"(
Usage: test [--help] [ARG1]
)");
    }
}


/****************************************************************************
*
*   Numerical values with units
*
***/

enum EnumAB { a = 1, b = 2 };
istream & operator>>(istream & is, EnumAB & val) {
    int ival;
    is >> ival;
    val = (EnumAB) ival;
    return is;
}

//===========================================================================
void unitsTests() {
    int line = 0;
    CliTest cli;

    // si units
    {
        cli = {};
        auto & dbls = cli.optVec<double>("[V]").siUnits("b");
        EXPECT_PARSE(cli, "1 1k 1b 1kb 1Mb 1kib 1000mb");
        EXPECT(dbls[0] == 1
            && dbls[1] == 1000
            && dbls[2] == 1
            && dbls[3] == 1000
            && dbls[4] == 1'000'000
            && dbls[5] == 1024
            && dbls[6] == 1
        );
        EXPECT_PARSE(cli, "b", false);
        EXPECT_ERR(cli, "Error: Invalid 'V' value: b\n");
        EXPECT_PARSE(cli, "1B", false);
        EXPECT_ERR(cli,
            "Error: Invalid 'V' value: 1B\n"
            "Units symbol 'B' not recognized.\n"
        );

        dbls.siUnits("b", cli.fUnitBinaryPrefix);
        EXPECT_PARSE(cli, "1k 1ki");
        EXPECT(dbls[0] == 1024 && dbls[1] == 1024);
        EXPECT_PARSE(cli, "1000m", false);
        EXPECT_ERR(cli, 1 + R"(
Error: Invalid 'V' value: 1000m
Units symbol 'm' not recognized.
)");

        dbls.siUnits("b", cli.fUnitInsensitive);
        EXPECT_PARSE(cli, "1 1b 1B 1kB 1Kb");
        EXPECT(dbls[0] == 1
            && dbls[1] == 1
            && dbls[2] == 1
            && dbls[3] == 1000
            && dbls[4] == 1000
        );
        EXPECT_PARSE(cli, "1000u", false);
        EXPECT_ERR(cli, 1 + R"(
Error: Invalid 'V' value: 1000u
Units symbol 'u' not recognized.
)");

        // with fUnitRequire
        dbls.siUnits("b", cli.fUnitRequire);
        EXPECT_PARSE(cli, "1b 1kb");
        EXPECT(dbls[0] == 1 && dbls[1] == 1000);
        EXPECT_PARSE(cli, "1", false);
        EXPECT_ERR(cli, 1 + R"(
Error: Invalid 'V' value: 1
Value requires suffix specifying the units.
)");
        EXPECT_PARSE(cli, "1k", false);
        EXPECT_ERR(cli, 1 + R"(
Error: Invalid 'V' value: 1k
Units symbol 'k' not recognized.
)");
        EXPECT_PARSE(cli, "b", false);
        EXPECT_ERR(cli, "Error: Invalid 'V' value: b\n");
        EXPECT_PARSE(cli, "k", false);
        EXPECT_ERR(cli, "Error: Invalid 'V' value: k\n");
        EXPECT_PARSE(cli, "kb", false);
        EXPECT_ERR(cli, "Error: Invalid 'V' value: kb\n");
        EXPECT_PARSE(cli, "1x23kb", false);
        EXPECT_ERR(cli, "Error: Invalid 'V' value: 1x23kb\n");
        dbls.siUnits("", cli.fUnitRequire);
        EXPECT_PARSE(cli, "1k");
        EXPECT(dbls[0] == 1000);

        // to int, double, string, complex
        auto & sv = cli.opt<string>("s").siUnits();
        EXPECT_PARSE(cli, "-s 1M");
        EXPECT(*sv == "1000000");

        auto & si = cli.opt<int>("i").siUnits();
        EXPECT_PARSE(cli, "-i2G");
        EXPECT(*si == 2'000'000'000);
        EXPECT_PARSE(cli, "-i6G", false);
        EXPECT_ERR(cli, 1 + R"(
Error: Out of range '-i' value: 6G
Must be between '-2,147,483,648' and '2,147,483,647'.
)");
        EXPECT_PARSE(cli, "-iNaN(1)k", false);
        EXPECT_ERR(cli, "Error: Invalid '-i' value: NaN(1)k\n");

        auto & sd = cli.opt<double>("d").siUnits();
        EXPECT_PARSE(cli, "-d2.k");
        EXPECT(*sd == 2000);

        EnumAB seRaw;
        auto & se = cli.opt(&seRaw, "e").siUnits();
        EXPECT_PARSE(cli, "-e500m", false);
        EXPECT_ERR(cli, "Error: Invalid '-e' value: 500m\n");
        EXPECT(se);

        EXPECT_HELP(cli, "", 1 + R"(
Usage: test [OPTIONS] [V...]

Options:
  -d FLOAT[<units>]   (default: 0)
  -e VALUE[<units>]   (default: 0)
  -i NUM[<units>]     (default: 0)
  -s STRING[<units>]

  --help              Show this message and exit.
)");
    }

    // time units
    {
        cli = {};
        auto & sht = cli.opt<uint16_t>("s").timeUnits();
        EXPECT_HELP(cli, "", 1 + R"(
Usage: test [OPTIONS]

Options:
  -s NUM[<units>]  (default: 0)

  --help           Show this message and exit.
)");
        EXPECT_PARSE(cli, "-s 1.5m");
        EXPECT(*sht == 90);
        EXPECT_PARSE(cli, "-s100ms");
        EXPECT(*sht == 0);
        EXPECT_PARSE(cli, "-s1y", false);
        EXPECT_ERR(cli, 1 + R"(
Error: Out of range '-s' value: 1y
Must be between '0' and '65,535'.
)");
        auto & lng = cli.opt<long>("l").timeUnits();
        EXPECT_PARSE(cli, "-l1y");
        EXPECT(*lng == 31'536'000);
        auto & dbl = cli.opt<double>("d").timeUnits();
        EXPECT_PARSE(cli, "-d 1.5 -s 1.4");
        EXPECT(*dbl == 1.5 && *sht == 1);
        EXPECT_PARSE(cli, "-s 1.6");
        EXPECT(*sht == 2);
        auto & dbls = cli.optVec<double>("v").timeUnits();
        EXPECT_PARSE(cli, "-v1s -v1m -v1h -v1d -v1w -v1y");
        EXPECT(dbls[0] == 1
            && dbls[1] == 60 * dbls[0]
            && dbls[2] == 60 * dbls[1]
            && dbls[3] == 24 * dbls[2]
            && dbls[4] == 7 * dbls[3]
            && dbls[5] == 365 * dbls[3]
        );
        EXPECT_PARSE(cli, "-v1ms -v1us -v1ns");
        EXPECT(dbls[0] == 1e-3 && dbls[1] == 1e-6 && dbls[2] == 1e-9);
    }

    // any units
    {
        cli = {};
        auto & length = cli.opt<double>("l length")
            .anyUnits({{"yd", 36}, {"ft", 12}, {"in", 1}, {"mil", 0.001}})
            .desc("Length, in inches");
        EXPECT_PARSE(cli, "-l 1yd");
        EXPECT(*length == 36);
        EXPECT_PARSE(cli, "-l 3ft");
        EXPECT(*length == 36);
        EXPECT_PARSE(cli, "-l 36");
        EXPECT(*length == 36);
    }
}


/****************************************************************************
*
*   Prompting
*
***/

//===========================================================================
void promptTests(bool prompt) {
    int line = 0;
    CliTest cli;
    istringstream in;
    ostringstream out;

    // password prompt
    {
        cli = {};
        auto & pass = cli.passwordOpt(true);
        EXPECT_HELP(cli, "", 1 + R"(
Usage: test [OPTIONS]

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


/****************************************************************************
*
*   Before action
*
***/

//===========================================================================
void beforeTests() {
    CliTest cli;
    cli.before([](auto & cli, auto & args) {
        if (args.size() > 1)
            return cli.fail(Dim::kExitUsage, "Too many args");
        return true;
    });
    EXPECT_PARSE(cli, "one two", false);
    EXPECT_ERR(cli, "Error: Too many args\n");
}


/****************************************************************************
*
*   Environment
*
***/

//===========================================================================
void envTests() {
#if !defined(DIMCLI_LIB_NO_ENV)
    int line = 0;
    CliTest cli;
    int result;

    cli = {};
    auto & args = cli.optVec<string>("[ARGS]");
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
*   finalOpt tests
*
***/

//===========================================================================
void finalOptTests() {
    int line = 0;
    CliTest cli;

    // finalOpt operand after optional operand
    {
        cli = {};
        cli.opt<int>("[A]");
        auto & b = cli.optVec<int>("[B]").finalOpt();
        EXPECT_PARSE(cli, "1 2 3");
        EXPECT(*b == vector<int>{2, 3});
    }

    // finalOpt option
    {
        cli = {};
        cli.opt<bool>("a lname").finalOpt();
        auto & b = cli.opt<int>("b").finalOpt();
        auto & c = cli.optVec<string>("[C]");
        EXPECT_PARSE(cli, "-a -b");
        EXPECT(!b);
        EXPECT(*c == vector<string>{"-b"});
        EXPECT_PARSE(cli, "-b 1 -b");
        EXPECT(*b == 1);
        EXPECT(*c == vector<string>{"-b"});
        EXPECT_PARSE(cli, "--lname -a");
        EXPECT(*c == vector<string>{"-a"});
    }
}


/****************************************************************************
*
*   Main
*
***/

//===========================================================================
static int runTests(bool prompt) {
    // consoleWidth() call for code coverage.
    unsigned width = Dim::Cli::consoleWidth(false);
    (void) width;

    Dim::Cli cli;
    cli.maxWidth(80);

#ifdef DIMCLI_LIB_BUILD_COVERAGE
    assertTests();
#endif

    valueTests();
    parseTests();
    choiceTests();
    helpTests();
    helpTextTests();
    cmdTests();
    argvTests();
    optCheckTests();
    flagTests();
    responseTests();
    filesystemTests();
    execTests();
    vectorTests();
    basicTests();
    unitsTests();
    promptTests(prompt);
    beforeTests();
    envTests();
    finalOptTests();

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

    CliTest cli;
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
