// Copyright Glen Knowles 2016 - 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// cli.cpp - dim cli
//
// For documentation and examples:
// https://github.com/gknowles/dimcli

#ifndef DIM_LIB_SOURCE
#define DIM_LIB_SOURCE
#endif
#include "cli.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <locale>
#include <sstream>
#include <unordered_set>

using namespace std;
using namespace Dim;
namespace fs = experimental::filesystem;

// getenv triggers the visual c++ security warning
#if (_MSC_VER >= 1400)
#pragma warning(disable : 4996) // this function or variable may be unsafe.
#endif


/****************************************************************************
*
*   Tuning parameters
*
***/

// column where description text starts
const size_t kMinDescCol = 11;
const size_t kMaxDescCol = 28;


/****************************************************************************
*
*   Declarations
*
***/

// Name of group containing --help, --version, etc
const char kInternalOptionGroup[] = "~";

namespace {
struct OptName {
    Cli::OptBase * opt;
    bool invert;   // set to false instead of true (only for bools)
    bool optional; // value doesn't have to be present? (non-bools only)
    string name;   // name of argument (only for positionals)
};

// Option name filters for opts that are externally bool
enum NameListType : int {
    kNameEnable,     // include names that enable the opt
    kNameDisable,    // include names that disable
    kNameAll,        // include all names
    kNameNonDefault, // include names that change from the default
};

struct CodecvtWchar : codecvt<wchar_t, char, mbstate_t> {
    // public destructor required for use with wstring_convert
    ~CodecvtWchar() {}
};
} // namespace

struct Cli::ArgKey {
    string sort; // sort key
    string list;
    OptBase * opt;
};

struct Cli::OptIndex {
    unordered_map<char, OptName> shortNames;
    unordered_map<string, OptName> longNames;
    vector<OptName> argNames;
    bool allowCommands{false};
};

struct Cli::GroupConfig {
    string name;
    string title;
    string sortKey;
};

struct Cli::CommandConfig {
    string name;
    string header;
    string desc;
    string footer;
    function<Cli::ActionFn> action;
    Opt<bool> * helpOpt{nullptr};
    unordered_map<string, GroupConfig> groups;
};

struct Cli::Config {
    bool constructed{false};

    unordered_map<string, CommandConfig> cmds;
    list<unique_ptr<OptBase>> opts;
    bool responseFiles{true};
    string envOpts;
    istream * conin{&cin};
    ostream * conout{&cout};

    int exitCode{kExitOk};
    string errMsg;
    string errDetail;
    string progName;
    string command;
};


/****************************************************************************
*
*   Helpers
*
***/

// forward declarations
static bool helpAction(Cli & cli, Cli::Opt<bool> & opt, const string & val);
static bool cmdAction(Cli & cli);

//===========================================================================
static Cli::GroupConfig &
findGrpAlways(Cli::CommandConfig & cmd, const string & name) {
    auto i = cmd.groups.find(name);
    if (i != cmd.groups.end())
        return i->second;
    auto & grp = cmd.groups[name];
    grp.name = grp.sortKey = grp.title = name;
    return grp;
}

//===========================================================================
static Cli::CommandConfig &
findCmdAlways(Cli::Config & cfg, const string & name) {
    auto i = cfg.cmds.find(name);
    if (i != cfg.cmds.end())
        return i->second;
    auto & cmd = cfg.cmds[name];
    cmd.name = name;
    cmd.action = cmdAction;
    auto & defGrp = findGrpAlways(cmd, "");
    defGrp.title = "Options";
    auto & intGrp = findGrpAlways(cmd, kInternalOptionGroup);
    intGrp.title.clear();
    return cmd;
}

//===========================================================================
static string displayName(const fs::path & file) {
#if defined(_WIN32)
    return file.stem().string();
#else
    return file.filename().string();
#endif
}

//===========================================================================
// Replaces a set of contiguous values in one vector with the entire contents
// of another, growing or shrinking it as needed.
template <typename T>
static void
replace(vector<T> & out, size_t pos, size_t count, vector<T> && src) {
    size_t srcLen = src.size();
    if (count > srcLen) {
        out.erase(out.begin() + pos + srcLen, out.begin() + pos + count);
    } else if (count < srcLen) {
        out.insert(out.begin() + pos + count, srcLen - count, {});
    }
    auto i = out.begin() + pos;
    for (auto && val : src)
        *i++ = move(val);
}


/****************************************************************************
*
*   CliLocal
*
***/

//===========================================================================
CliLocal::CliLocal()
    : Cli(make_shared<Config>()) {}


/****************************************************************************
*
*   Cli::OptBase
*
***/

//===========================================================================
Cli::OptBase::OptBase(const string & names, bool boolean)
    : m_bool{boolean}
    , m_names{names} {
    // set m_fromName and assert if names is malformed
    OptIndex ndx;
    index(ndx);
}

//===========================================================================
void Cli::OptBase::setNameIfEmpty(const string & name) {
    if (m_fromName.empty())
        m_fromName = name;
}

//===========================================================================
bool Cli::OptBase::parseValue(const std::string & value) {
    Cli cli;
    return fromString(cli, value);
}

//===========================================================================
string Cli::OptBase::defaultPrompt() const {
    string name = m_fromName;
    while (name.size() && name[0] == '-')
        name.erase(0, 1);
    if (name.size())
        name[0] = (char)toupper(name[0]);
    return name;
}

//===========================================================================
void Cli::OptBase::index(OptIndex & ndx) {
    const char * ptr = m_names.data();
    string name;
    char close;
    bool hasPos = false;
    for (;; ++ptr) {
        switch (*ptr) {
        case 0: return;
        case ' ': continue;
        case '[': close = ']'; break;
        case '<': close = '>'; break;
        default: close = ' '; break;
        }
        const char * b = ptr;
        bool hasEqual = false;
        while (*ptr && *ptr != close) {
            if (*ptr == '=')
                hasEqual = true;
            ptr += 1;
        }
        if (hasEqual && close == ' ') {
            assert(!hasEqual && "bad argument name");
        } else if (hasPos && close != ' ') {
            assert(!hasPos && "argument with multiple positional names");
        } else {
            if (close != ' ')
                hasPos = true;
            name = string(b, ptr - b);
            indexName(ndx, name);
        }
        if (!*ptr)
            return;
    }
}

//===========================================================================
void Cli::OptBase::indexName(OptIndex & ndx, const string & name) {
    const bool invert = true;
    const bool optional = true;

    auto where = ndx.argNames.end();
    switch (name[0]) {
    case '-': assert(name[0] != '-' && "bad argument name"); return;
    case '[':
        ndx.argNames.push_back({this, !invert, optional, name.data() + 1});
        where = ndx.argNames.end() - 1;
        goto INDEX_POS_NAME;
    case '<':
        where =
            find_if(ndx.argNames.begin(), ndx.argNames.end(), [](auto && key) {
                return key.optional;
            });
        where = ndx.argNames.insert(
            where, {this, !invert, !optional, name.data() + 1});
    INDEX_POS_NAME:
        setNameIfEmpty(where->name);
        if (m_command.empty())
            ndx.allowCommands = false;
        return;
    }
    if (name.size() == 1)
        return indexShortName(ndx, name[0], !invert, !optional);
    switch (name[0]) {
    case '!':
        if (!m_bool) {
            assert(!m_bool && "bad modifier '!' for non-bool argument");
            return;
        }
        if (name.size() == 2)
            return indexShortName(ndx, name[1], invert, !optional);
        indexLongName(ndx, name.substr(1), invert, !optional);
        return;
    case '?':
        if (m_bool) {
            assert(!m_bool && "bad modifier '?' for bool argument");
            return;
        }
        if (name.size() == 2)
            return indexShortName(ndx, name[1], !invert, optional);
        indexLongName(ndx, name.substr(1), !invert, optional);
        return;
    }
    indexLongName(ndx, name, !invert, !optional);
}

//===========================================================================
void Cli::OptBase::indexShortName(
    OptIndex & ndx,
    char name,
    bool invert,
    bool optional) {
    ndx.shortNames[name] = {this, invert, optional};
    setNameIfEmpty("-"s += name);
}

//===========================================================================
void Cli::OptBase::indexLongName(
    OptIndex & ndx,
    const string & name,
    bool invert,
    bool optional) {
    bool allowNo = true;
    string key{name};
    if (key.back() == '.') {
        allowNo = false;
        if (key.size() == 2) {
            assert(key.size() > 2 && "bad modifier '.' for short name");
            return;
        }
        key.pop_back();
    }
    setNameIfEmpty("--"s += key);
    ndx.longNames[key] = {this, invert, optional};
    if (m_bool && allowNo)
        ndx.longNames["no-"s += key] = {this, !invert, optional};
}


/****************************************************************************
*
*   Action callbacks
*
***/

//===========================================================================
static bool helpAction(Cli & cli, Cli::Opt<bool> & opt, const string & val) {
    cli.fromString(*opt, val);
    if (*opt) {
        cli.printHelp(cli.conout(), {}, cli.runCommand());
        return false;
    }
    return true;
}

//===========================================================================
bool Cli::defaultParse(OptBase & opt, const string & val) {
    if (opt.fromString(*this, val)) 
        return true;

    badUsage("Invalid '" + opt.from() + "' value", val);
    if (!opt.m_choiceDescs.empty()) {
        ostringstream os;
        os << "Must be ";
        size_t pos = 0;
        for (auto && cd : opt.m_choiceDescs) {
            pos += 1;
            os << '"' << cd.first << '"';
            auto num = opt.m_choiceDescs.size();
            if (pos == num)
                break;
            if (pos + 1 == num) {
                os << ((pos == 1) ? " or " : ", or ");
            } else {
                os << ", ";
            }
        }
        m_cfg->errDetail = os.str();
    }
    return false;
}

//===========================================================================
static bool cmdAction(Cli & cli) {
    ostringstream os;
    if (cli.runCommand().empty()) {
        os << "No command given.";
    } else {
        os << "Command '" << cli.runCommand() << "' has not been implemented.";
    }
    return cli.fail(kExitSoftware, os.str());
}


/****************************************************************************
*
*   Cli
*
***/

//===========================================================================
// Creates a handle to the shared configuration
Cli::Cli() {
    static auto s_cfg = make_shared<Config>();
    m_cfg = s_cfg;
    helpOpt();
}

//===========================================================================
Cli::Cli(const Cli & from) 
    : m_cfg(from.m_cfg)
    , m_command(from.m_command)
    , m_group(from.m_group)
{}

//===========================================================================
// protected
Cli::Cli(shared_ptr<Config> cfg)
    : m_cfg(cfg) {
    helpOpt();
}

//===========================================================================
Cli & Cli::operator=(const Cli & from) {
    m_cfg = from.m_cfg;
    m_command = from.m_command;
    m_group = from.m_group;
    return *this;
}


/****************************************************************************
*
*   Configuration
*
***/

//===========================================================================
Cli::Opt<bool> & Cli::confirmOpt(const string & prompt) {
    auto & ask = opt<bool>("y yes")
        .desc("Suppress prompting to allow execution.")
        .check([](auto &, auto & opt, auto &) { return *opt; })
        .prompt(prompt.empty() ? "Are you sure?" : prompt);
    return ask;
}

//===========================================================================
Cli::Opt<bool> & Cli::helpOpt() {
    auto & cmd = cmdCfg();
    if (cmd.helpOpt)
        return *cmd.helpOpt;

    auto & hlp = opt<bool>("help.")
        .desc("Show this message and exit.")
        .parse(helpAction)
        .group(kInternalOptionGroup);
    cmd.helpOpt = &hlp;
    return hlp;
}

//===========================================================================
Cli::Opt<string> & Cli::passwordOpt(bool confirm) {
    return opt<string>("password.")
        .desc("Password required for access.")
        .prompt(kPromptHide | (confirm * kPromptConfirm));
}

//===========================================================================
Cli::Opt<bool> &
Cli::versionOpt(const string & version, const string & progName) {
    auto verAction = [version, progName](
        auto & cli, 
        auto &, // opt 
        auto &  // val
    ) {
        string prog = progName;
        if (prog.empty()) {
            prog = displayName(cli.progName());
        }
        cli.conout() << prog << " version " << version << endl;
        return false;
    };
    return opt<bool>("version.")
        .desc("Show version and exit.")
        .parse(verAction)
        .group(kInternalOptionGroup);
}

//===========================================================================
Cli & Cli::group(const string & name) {
    m_group = name;
    return *this;
}

//===========================================================================
Cli & Cli::title(const string & val) {
    grpCfg().title = val;
    return *this;
}

//===========================================================================
Cli & Cli::sortKey(const string & val) {
    grpCfg().sortKey = val;
    return *this;
}

//===========================================================================
const string & Cli::title() const {
    return grpCfg().title;
}

//===========================================================================
const string & Cli::sortKey() const {
    return grpCfg().sortKey;
}

//===========================================================================
Cli & Cli::command(const string & name, const string & grpName) {
    m_command = name;
    m_group = grpName;
    helpOpt();
    return *this;
}

//===========================================================================
Cli & Cli::action(function<ActionFn> fn) {
    cmdCfg().action = fn;
    return *this;
}

//===========================================================================
Cli & Cli::header(const string & val) {
    cmdCfg().header = val;
    return *this;
}

//===========================================================================
Cli & Cli::desc(const string & val) {
    cmdCfg().desc = val;
    return *this;
}

//===========================================================================
Cli & Cli::footer(const string & val) {
    cmdCfg().footer = val;
    return *this;
}

//===========================================================================
const string & Cli::header() const {
    return cmdCfg().header;
}

//===========================================================================
const string & Cli::desc() const {
    return cmdCfg().desc;
}

//===========================================================================
const string & Cli::footer() const {
    return cmdCfg().footer;
}

//===========================================================================
void Cli::responseFiles(bool enable) {
    m_cfg->responseFiles = enable;
}

#if !defined(DIM_LIB_NO_ENV)
//===========================================================================
void Cli::envOpts(const string & var) {
    m_cfg->envOpts = var;
}
#endif

//===========================================================================
void Cli::iostreams(std::istream * in, std::ostream * out) {
    m_cfg->conin = in ? in : &cin;
    m_cfg->conout = out ? out : &cout;
}

//===========================================================================
istream & Cli::conin() {
    return *m_cfg->conin;
}

//===========================================================================
ostream & Cli::conout() {
    return *m_cfg->conout;
}

//===========================================================================
Cli::GroupConfig & Cli::grpCfg() {
    return findGrpAlways(cmdCfg(), m_group);
}

//===========================================================================
const Cli::GroupConfig & Cli::grpCfg() const {
    return const_cast<Cli *>(this)->grpCfg();
}

//===========================================================================
Cli::CommandConfig & Cli::cmdCfg() {
    return findCmdAlways(*m_cfg, m_command);
}

//===========================================================================
const Cli::CommandConfig & Cli::cmdCfg() const {
    return const_cast<Cli *>(this)->cmdCfg();
}

//===========================================================================
void Cli::addOpt(unique_ptr<OptBase> src) {
    m_cfg->opts.push_back(move(src));
}

//===========================================================================
Cli::OptBase * Cli::findOpt(const void * value) {
    if (value) {
        for (auto && opt : m_cfg->opts) {
            if (opt->sameValue(value))
                return opt.get();
        }
    }
    return nullptr;
}


/****************************************************************************
*
*   Parse argv
*
***/

//===========================================================================
// static
vector<string> Cli::toArgv(const string & cmdline) {
#if defined(_WIN32)
    return toWindowsArgv(cmdline);
#else
    return toGnuArgv(cmdline);
#endif
}

//===========================================================================
// static
std::vector<std::string> Cli::toArgv(size_t argc, char * argv[]) {
    vector<string> out;
    out.reserve(argc);
    for (; *argv; ++argv)
        out.push_back(*argv);
    assert(argc == out.size());
    return out;
}

//===========================================================================
// static
std::vector<std::string> Cli::toArgv(size_t argc, wchar_t * argv[]) {
    vector<string> out;
    out.reserve(argc);
    wstring_convert<CodecvtWchar> wcvt(
        "BAD_ENCODING");
    for (; *argv; ++argv) {
        string tmp = wcvt.to_bytes(*argv);
        out.push_back(move(tmp));
    }
    assert(argc == out.size());
    return out;
}

//===========================================================================
// static
vector<const char *> Cli::toPtrArgv(const vector<string> & args) {
    vector<const char *> argv;
    argv.reserve(args.size() + 1);
    for (auto && arg : args)
        argv.push_back(arg.data());
    argv.push_back(nullptr);
    argv.pop_back();
    return argv;
}

//===========================================================================
// These rules where gleaned by inspecting glib's g_shell_parse_argv which
// takes its rules from the "Shell Command Language" section of the UNIX98
// spec -- ignoring parameter expansion ("$()" and "${}"), command
// substitution (backquote `), operators as separators, etc.
//
// Arguments are split on whitespace (" \t\r\n\f\v") unless the whitespace
// is escaped, quoted, or in a comment.
// - unquoted: any char following a backslash is replaced by that char,
//   except newline, which is removed. An unquoted '#' starts a comment.
// - comment: everything up to, but not including, the next newline is ignored 
// - single quotes: preserve the string exactly, no escape sequences, not
//   even \'
// - double quotes: some chars ($ ' " \ and newline) are escaped when
//   following a backslash, a backslash not followed one of those five chars
//   is preserved. All other chars are preserved.
//
// When escaping it's simplest to not quote and just escape the following:
//   Must: | & ; < > ( ) $ ` \ " ' SP TAB LF
//   Should: * ? [ # ~ = %
//
//===========================================================================
// static
vector<string> Cli::toGlibArgv(const string & cmdline) {
    vector<string> out;
    const char * cur = cmdline.c_str();
    const char * last = cur + cmdline.size();

    string arg;

IN_GAP:
    while (cur < last) {
        char ch = *cur++;
        switch (ch) {
        case '\\':
            if (cur < last) {
                ch = *cur++;
                if (ch == '\n')
                    break;
            }
            arg += ch;
            goto IN_UNQUOTED;
        default: arg += ch; goto IN_UNQUOTED;
        case '"': goto IN_DQUOTE;
        case '\'': goto IN_SQUOTE;
        case '#': goto IN_COMMENT;
        case ' ':
        case '\t':
        case '\r':
        case '\n':
        case '\f':
        case '\v': break;
        }
    }
    return out;

IN_COMMENT:
    while (cur < last) {
        char ch = *cur++;
        switch (ch) {
        case '\r':
        case '\n': goto IN_GAP;
        }
    }
    return out;

IN_UNQUOTED:
    while (cur < last) {
        char ch = *cur++;
        switch (ch) {
        case '\\':
            if (cur < last) {
                ch = *cur++;
                if (ch == '\n')
                    break;
            }
            arg += ch;
            break;
        default: arg += ch; break;
        case '"': goto IN_DQUOTE;
        case '\'': goto IN_SQUOTE;
        case ' ':
        case '\t':
        case '\r':
        case '\n':
        case '\f':
        case '\v':
            out.push_back(move(arg));
            arg.clear();
            goto IN_GAP;
        }
    }
    out.push_back(move(arg));
    return out;

IN_SQUOTE:
    while (cur < last) {
        char ch = *cur++;
        if (ch == '\'')
            goto IN_UNQUOTED;
        arg += ch;
    }
    out.push_back(move(arg));
    return out;

IN_DQUOTE:
    while (cur < last) {
        char ch = *cur++;
        switch (ch) {
        case '"': goto IN_UNQUOTED;
        case '\\':
            if (cur < last) {
                ch = *cur++;
                switch (ch) {
                case '$':
                case '\'':
                case '"':
                case '\\': break;
                case '\n': continue;
                default: arg += '\\';
                }
            }
            arg += ch;
            break;
        default: arg += ch; break;
        }
    }
    out.push_back(move(arg));
    return out;
}

//===========================================================================
// Rules from libiberty's buildargv().
//
// Arguments split on unquoted whitespace (" \t\r\n\f\v")
//  - backslashes: always escapes the following character.
//  - single quotes and double quotes: escape each other and whitespace.
//
//===========================================================================
// static
vector<string> Cli::toGnuArgv(const string & cmdline) {
    vector<string> out;
    const char * cur = cmdline.c_str();
    const char * last = cur + cmdline.size();

    string arg;
    char quote;

IN_GAP:
    while (cur < last) {
        char ch = *cur++;
        switch (ch) {
        case '\\':
            if (cur < last)
                ch = *cur++;
            arg += ch;
            goto IN_UNQUOTED;
        default: arg += ch; goto IN_UNQUOTED;
        case '\'':
        case '"': quote = ch; goto IN_QUOTED;
        case ' ':
        case '\t':
        case '\r':
        case '\n':
        case '\f':
        case '\v': break;
        }
    }
    return out;

IN_UNQUOTED:
    while (cur < last) {
        char ch = *cur++;
        switch (ch) {
        case '\\':
            if (cur < last)
                ch = *cur++;
            arg += ch;
            break;
        default: arg += ch; break;
        case '"':
        case '\'': quote = ch; goto IN_QUOTED;
        case ' ':
        case '\t':
        case '\r':
        case '\n':
        case '\f':
        case '\v':
            out.push_back(move(arg));
            arg.clear();
            goto IN_GAP;
        }
    }
    out.push_back(move(arg));
    return out;


IN_QUOTED:
    while (cur < last) {
        char ch = *cur++;
        if (ch == quote)
            goto IN_UNQUOTED;
        if (ch == '\\' && cur < last)
            ch = *cur++;
        arg += ch;
    }
    out.push_back(move(arg));
    return out;
}

//===========================================================================
// Rules defined in the "Parsing C++ Command-Line Arguments" article on MSDN.
//
// Arguments are split on whitespace (" \t") unless the whitespace is quoted.
// - double quotes: preserves whitespace that would otherwise end the
//   argument, can occur in the midst of an argument.
// - backslashes:
//   - an even number followed by a double quote adds one backslash for each
//     pair and the quote is a delimiter.
//   - an odd number followed by a double quote adds one backslash for each
//     pair, the last one is tossed, and the quote is added to the argument.
//   - any number not followed by a double quote are literals.
//
//===========================================================================
// static
vector<string> Cli::toWindowsArgv(const string & cmdline) {
    vector<string> out;
    const char * cur = cmdline.c_str();
    const char * last = cur + cmdline.size();

    string arg;
    int backslashes = 0;

    auto appendBackslashes = [&arg, &backslashes]() {
        if (backslashes) {
            arg.append(backslashes, '\\');
            backslashes = 0;
        }
    };

IN_GAP:
    while (cur < last) {
        char ch = *cur++;
        switch (ch) {
        case '\\': backslashes += 1; goto IN_UNQUOTED;
        case '"': goto IN_QUOTED;
        case ' ':
        case '\t':
        case '\r':
        case '\n': break;
        default: arg += ch; goto IN_UNQUOTED;
        }
    }
    return out;

IN_UNQUOTED:
    while (cur < last) {
        char ch = *cur++;
        switch (ch) {
        case '\\': backslashes += 1; break;
        case '"':
            if (int num = backslashes) {
                backslashes = 0;
                arg.append(num / 2, '\\');
                if (num % 2 == 1) {
                    arg += ch;
                    break;
                }
            }
            goto IN_QUOTED;
        case ' ':
        case '\t':
        case '\r':
        case '\n':
            appendBackslashes();
            out.push_back(move(arg));
            arg.clear();
            goto IN_GAP;
        default:
            appendBackslashes();
            arg += ch;
            break;
        }
    }
    appendBackslashes();
    out.push_back(move(arg));
    return out;

IN_QUOTED:
    while (cur < last) {
        char ch = *cur++;
        switch (ch) {
        case '\\': backslashes += 1; break;
        case '"':
            if (int num = backslashes) {
                backslashes = 0;
                arg.append(num / 2, '\\');
                if (num % 2 == 1) {
                    arg += ch;
                    break;
                }
            }
            goto IN_UNQUOTED;
        default:
            appendBackslashes();
            arg += ch;
            break;
        }
    }
    appendBackslashes();
    out.push_back(move(arg));
    return out;
}


/****************************************************************************
*
*   Response files
*
***/

// forward declarations
static bool expandResponseFiles(
    Cli & cli,
    vector<string> & args,
    unordered_set<string> & ancestors);

//===========================================================================
// Returns false on error, if there was an error content will either be empty
// or - if there was a transcoding error - contain the original content.
static bool loadFileUtf8(string & content, const fs::path & fn) {
    content.clear();

    error_code err;
    auto bytes = (size_t)fs::file_size(fn, err);
    if (err)
        return false;

    content.resize(bytes);
    ifstream f(fn, ios::binary);
    f.read(const_cast<char *>(content.data()), content.size());
    if (!f) {
        content.clear();
        return false;
    }
    f.close();

    if (content.size() < 2)
        return true;
    if (content[0] == '\xff' && content[1] == '\xfe') {
        wstring_convert<CodecvtWchar> wcvt("");
        const wchar_t * base =
            reinterpret_cast<const wchar_t *>(content.data());
        string tmp = wcvt.to_bytes(base + 1, base + content.size() / 2);
        if (tmp.empty())
            return false;
        content = tmp;
    } else if (
        content.size() >= 3 && content[0] == '\xef' && content[1] == '\xbb'
        && content[2] == '\xbf') {
        content.erase(0, 3);
    }
    return true;
}

//===========================================================================
static bool expandResponseFile(
    Cli & cli,
    vector<string> & args,
    size_t & pos,
    unordered_set<string> & ancestors) {
    string content;
    error_code err;
    fs::path fn = args[pos].substr(1);
    fs::path cfn = fs::canonical(fn, err);
    if (err)
        return cli.badUsage("Invalid response file", fn.string());
    auto ib = ancestors.insert(cfn.string());
    if (!ib.second)
        return cli.badUsage("Recursive response file", fn.string());
    if (!loadFileUtf8(content, fn)) {
        string desc = content.empty() ? "Read error" : "Invalid encoding";
        return cli.badUsage(desc, fn.string());
    }
    auto rargs = cli.toArgv(content);
    if (!expandResponseFiles(cli, rargs, ancestors))
        return false;
    size_t rargLen = rargs.size();
    replace(args, pos, 1, move(rargs));
    pos += rargLen;
    ancestors.erase(ib.first);
    return true;
}

//===========================================================================
// "ancestors" contains the set of response files these args came from,
// directly or indirectly, and is used to detect recursive response files.
static bool expandResponseFiles(
    Cli & cli,
    vector<string> & args,
    unordered_set<string> & ancestors) {
    for (size_t pos = 0; pos < args.size(); ++pos) {
        if (!args[pos].empty() && args[pos][0] == '@') {
            if (!expandResponseFile(cli, args, pos, ancestors))
                return false;
        }
    }
    return true;
}


/****************************************************************************
*
*   Parse command line
*
***/

//===========================================================================
void Cli::resetValues() {
    for (auto && opt : m_cfg->opts) {
        opt->reset();
    }
    m_cfg->exitCode = kExitOk;
    m_cfg->errMsg.clear();
    m_cfg->errDetail.clear();
    m_cfg->progName.clear();
    m_cfg->command.clear();
}

//===========================================================================
void Cli::index(OptIndex & ndx, const string & cmd, bool requireVisible)
    const {
    ndx.argNames.clear();
    ndx.longNames.clear();
    ndx.shortNames.clear();
    ndx.allowCommands = cmd.empty();
    for (auto && opt : m_cfg->opts) {
        if (opt->m_command == cmd && (opt->m_visible || !requireVisible))
            opt->index(ndx);
    }
    for (unsigned i = 0; i < ndx.argNames.size(); ++i) {
        auto & key = ndx.argNames[i];
        if (key.name.empty())
            key.name = "arg" + to_string(i + 1);
    }
}

//===========================================================================
bool Cli::prompt(OptBase & opt, const string & msg, int flags) {
    if (!opt.from().empty())
        return true;
    auto & is = conin();
    auto & os = conout();
    os << msg << ' ';
    if (~flags & kPromptNoDefault) {
        if (opt.m_bool)
            os << "[y/N]: ";
    }
    if (flags & kPromptHide)
        consoleEnableEcho(false); // disable if hide, must be re-enabled
    string val;
    os.flush();
    getline(is, val);
    if (flags & kPromptHide) {
        os << endl;
        if (~flags & kPromptConfirm)
            consoleEnableEcho(true); // re-enable when hide and !confirm
    }
    if (flags & kPromptConfirm) {
        string again;
        os << "Enter again to confirm: " << flush;
        getline(is, again);
        if (flags & kPromptHide) {
            os << endl;
            consoleEnableEcho(true); // re-enable when hide and confirm
        }
        if (val != again)
            return badUsage("Confirm failed, entries not the same.");
    }
    if (opt.m_bool) {
        // Honor the contract that bool parse functions are only presented
        // with either "0" or "1".
        val = val.size() && (val[0] == 'y' || val[0] == 'Y') ? "1" : "0";
    }
    return parseValue(opt, opt.defaultFrom(), 0, val.c_str());
}

//===========================================================================
bool Cli::parseValue(
    OptBase & opt,
    const string & name,
    size_t pos,
    const char ptr[]) {
    opt.set(name, pos);
    string val;
    if (ptr) {
        val = ptr;
        if (!opt.parseAction(*this, val))
            return false;
    } else {
        opt.unspecifiedValue();
    }
    return opt.checkAction(*this, val);
}

//===========================================================================
bool Cli::badUsage(const string & prefix, const string & value) {
    string msg = prefix + ": " + value;
    return badUsage(msg);
}

//===========================================================================
bool Cli::badUsage(const string & msg) {
    string out;
    string & cmd = m_cfg->command;
    if (cmd.size())
        out = "Command '" + cmd + "': ";
    out += msg;
    return fail(kExitUsage, out);
}

//===========================================================================
bool Cli::fail(int code, const string & msg, const string & detail) {
    m_cfg->exitCode = code;
    m_cfg->errMsg = msg;
    m_cfg->errDetail = detail;
    return false;
}

//===========================================================================
bool Cli::parse(vector<string> & args) {
    // the 0th (name of this program) opt should always be present
    assert(!args.empty());

    OptIndex ndx;
    index(ndx, "", false);
    bool needCmd = m_cfg->cmds.size() > 1;

    // Commands can't be added when the top level command has a positional
    // argument, command processing requires that the first positional is
    // available to identify the command.
    assert(ndx.allowCommands || !needCmd);

    resetValues();

#if !defined(DIM_LIB_NO_ENV)
    // insert environment options
    if (m_cfg->envOpts.size()) {
        if (const char * val = getenv(m_cfg->envOpts.c_str()))
            replace(args, 1, 0, toArgv(val));
    }
#endif

    // expand response files
    unordered_set<string> ancestors;
    if (m_cfg->responseFiles && !expandResponseFiles(*this, args, ancestors))
        return false;

    auto arg = args.data();
    auto argc = args.size();

    string name;
    unsigned pos = 0;
    bool moreOpts = true;
    m_cfg->progName = *arg;
    size_t argPos = 1;
    arg += 1;

    for (; argPos < argc; ++argPos, ++arg) {
        OptName argName;
        const char * ptr = arg->c_str();
        if (*ptr == '-' && ptr[1] && moreOpts) {
            ptr += 1;
            for (; *ptr && *ptr != '-'; ++ptr) {
                auto it = ndx.shortNames.find(*ptr);
                name = '-';
                name += *ptr;
                if (it == ndx.shortNames.end())
                    return badUsage("Unknown option", name);
                argName = it->second;
                if (argName.opt->m_bool) {
                    if (!parseValue(
                            *argName.opt,
                            name,
                            argPos,
                            argName.invert ? "0" : "1"))
                        return false;
                    continue;
                }
                ptr += 1;
                goto OPTION_VALUE;
            }
            if (!*ptr)
                continue;

            ptr += 1;
            if (!*ptr) {
                // bare "--" found, all remaining args are positional
                moreOpts = false;
                continue;
            }
            string key;
            const char * equal = strchr(ptr, '=');
            if (equal) {
                key.assign(ptr, equal);
                ptr = equal + 1;
            } else {
                key = ptr;
                ptr = "";
            }
            auto it = ndx.longNames.find(key);
            name = "--";
            name += key;
            if (it == ndx.longNames.end())
                return badUsage("Unknown option", name);
            argName = it->second;
            if (argName.opt->m_bool) {
                if (equal)
                    return badUsage("Unknown option", name + "=");
                if (!parseValue(
                        *argName.opt,
                        name,
                        argPos,
                        argName.invert ? "0" : "1"))
                    return false;
                continue;
            }
            goto OPTION_VALUE;
        }

        // positional value
        if (needCmd) {
            string cmd = ptr;
            auto i = m_cfg->cmds.find(cmd);
            if (i == m_cfg->cmds.end())
                return badUsage("Unknown command", cmd);
            needCmd = false;
            m_cfg->command = cmd;
            index(ndx, cmd, false);
            continue;
        }

        if (pos >= ndx.argNames.size())
            return badUsage("Unexpected argument", ptr);
        argName = ndx.argNames[pos];
        name = argName.name;
        if (!parseValue(*argName.opt, name, argPos, ptr))
            return false;
        if (!argName.opt->m_multiple)
            pos += 1;
        continue;

    OPTION_VALUE:
        if (*ptr) {
            if (!parseValue(*argName.opt, name, argPos, ptr))
                return false;
            continue;
        }
        if (argName.optional) {
            if (!parseValue(*argName.opt, name, argPos, nullptr))
                return false;
            continue;
        }
        argPos += 1;
        arg += 1;
        if (argPos == argc)
            return badUsage("Option requires value", name);
        if (!parseValue(*argName.opt, name, argPos, arg->c_str()))
            return false;
    }

    if (!needCmd && pos < ndx.argNames.size() && !ndx.argNames[pos].optional)
        return badUsage("Missing argument", ndx.argNames[pos].name);

    for (auto && opt : m_cfg->opts) {
        if (!opt->afterActions(*this))
            return false;
    }

    return true;
}

//===========================================================================
bool Cli::parse(ostream & os, vector<string> & args) {
    if (parse(args))
        return true;
    if (exitCode()) {
        os << "Error: " << errMsg() << endl;
        if (m_cfg->errDetail.size())
            os << "Error: " << m_cfg->errDetail << endl;
    }
    return false;
}

//===========================================================================
bool Cli::parse(size_t argc, char * argv[]) {
    auto args = toArgv(argc, argv);
    return parse(args);
}

//===========================================================================
bool Cli::parse(ostream & os, size_t argc, char * argv[]) {
    auto args = toArgv(argc, argv);
    return parse(os, args);
}


/****************************************************************************
*
*   Parse results
*
***/

//===========================================================================
int Cli::exitCode() const {
    return m_cfg->exitCode;
};

//===========================================================================
const string & Cli::errMsg() const {
    return m_cfg->errMsg;
}

//===========================================================================
const string & Cli::errDetail() const {
    return m_cfg->errDetail;
}

//===========================================================================
const string & Cli::progName() const {
    return m_cfg->progName;
}

//===========================================================================
const string & Cli::runCommand() const {
    return m_cfg->command;
}

//===========================================================================
bool Cli::exec() {
    auto & name = runCommand();
    assert(m_cfg->cmds.find(name) != m_cfg->cmds.end());
    auto & cmd = m_cfg->cmds[name];
    if (!cmd.action(*this)) {
        assert(exitCode());
        return false;
    }
    return true;
}


/****************************************************************************
*
*   Help
*
***/

namespace {
struct WrapPos {
    size_t pos{0};
    size_t maxWidth{79};
    string prefix;
};
} // namespace

//===========================================================================
static void writeNewline(ostream & os, WrapPos & wp) {
    os << '\n' << wp.prefix;
    wp.pos = wp.prefix.size();
}

//===========================================================================
// Write token, potentially adding a line break first.
static void writeToken(ostream & os, WrapPos & wp, const string token) {
    if (wp.pos + token.size() + 1 > wp.maxWidth) {
        if (wp.pos > wp.prefix.size()) {
            writeNewline(os, wp);
        }
    }
    if (wp.pos) {
        os << ' ';
        wp.pos += 1;
    }
    os << token;
    wp.pos += token.size();
}

//===========================================================================
// Write series of tokens, collapsing (and potentially breaking on) spaces.
static void writeText(ostream & os, WrapPos & wp, const string & text) {
    const char * base = text.c_str();
    for (;;) {
        while (*base == ' ')
            base += 1;
        if (!*base)
            return;
        const char * nl = strchr(base, '\n');
        const char * ptr = strchr(base, ' ');
        if (!ptr) {
            ptr = text.c_str() + text.size();
        }
        if (nl && nl < ptr) {
            writeToken(os, wp, string(base, nl));
            writeNewline(os, wp);
            base = nl + 1;
        } else {
            writeToken(os, wp, string(base, ptr));
            base = ptr;
        }
    }
}

//===========================================================================
// Like text, except advance to descCol first, and indent any additional
// required lines to descCol.
static void
writeDescCol(ostream & os, WrapPos & wp, const string & text, size_t descCol) {
    if (text.empty())
        return;
    if (wp.pos < descCol) {
        writeToken(os, wp, string(descCol - wp.pos - 1, ' '));
    } else if (wp.pos < descCol + 4) {
        os << ' ';
        wp.pos += 1;
    } else {
        wp.pos = wp.maxWidth;
    }
    wp.prefix.assign(descCol, ' ');
    writeText(os, wp, text);
}

//===========================================================================
string Cli::descStr(const Cli::OptBase & opt) const {
    string desc = opt.m_desc;
    string tmp;
    if (!opt.m_choiceDescs.empty()) {
        // "default" tag is added to individual choices later
    } else if (opt.m_flagValue && opt.m_flagDefault) {
        desc += " (default)";
    } else if (!opt.m_multiple 
        && !opt.m_bool
        && opt.defaultValueStr(tmp, *this)
        && !tmp.empty()
    ) {
        desc += " (default: " + tmp + ")";
    }
    return desc;
}

//===========================================================================
static void writeChoices(
    ostream & os,
    WrapPos & wp,
    const unordered_map<string, Cli::OptBase::ChoiceDesc> & choices) {
    if (choices.empty())
        return;
    size_t colWidth = 0;
    struct ChoiceKey {
        size_t pos;
        const char * key;
        const char * desc;
        const char * sortKey;
        bool def;
    };
    vector<ChoiceKey> keys;
    for (auto && cd : choices) {
        colWidth = max(colWidth, cd.first.size());
        ChoiceKey key;
        key.pos = cd.second.pos;
        key.key = cd.first.c_str();
        key.desc = cd.second.desc.c_str();
        key.sortKey = cd.second.sortKey.c_str();
        key.def = cd.second.def;
        keys.push_back(key);
    }
    const size_t indent = 6;
    colWidth = max(min(colWidth + indent + 1, kMaxDescCol), kMinDescCol);
    sort(keys.begin(), keys.end(), [](auto & a, auto & b) {
        if (int rc = strcmp(a.sortKey, b.sortKey))
            return rc < 0;
        return a.pos < b.pos;
    });

    string desc;
    for (auto && k : keys) {
        wp.prefix.assign(indent + 2, ' ');
        writeToken(os, wp, string(indent, ' ') + k.key);
        desc = k.desc;
        if (k.def)
            desc += " (default)";
        writeDescCol(os, wp, desc, colWidth);
        os << '\n';
        wp.pos = 0;
    }
}

//===========================================================================
int Cli::printHelp(
    ostream & os,
    const string & progName,
    const string & cmdName) const {
    auto & cmd = findCmdAlways(*m_cfg, cmdName);
    if (!cmd.header.empty()) {
        WrapPos wp;
        writeText(os, wp, cmd.header);
        writeNewline(os, wp);
    }
    printUsage(os, progName, cmdName);
    if (!cmd.desc.empty()) {
        WrapPos wp;
        writeText(os, wp, cmd.desc);
        writeNewline(os, wp);
    }
    if (cmdName.empty())
        printCommands(os);
    printPositionals(os, cmdName);
    printOptions(os, cmdName);
    if (!cmd.footer.empty()) {
        WrapPos wp;
        writeNewline(os, wp);
        writeText(os, wp, cmd.footer);
    }
    return exitCode();
}

//===========================================================================
int Cli::writeUsageImpl(
    ostream & os,
    const string & arg0,
    const string & cmdName,
    bool expandedOptions) const {
    OptIndex ndx;
    index(ndx, cmdName, true);
    auto & cmd = findCmdAlways(*m_cfg, cmdName);
    string prog = displayName(arg0.empty() ? progName() : arg0);
    const string usageStr{"usage: "};
    os << usageStr << prog;
    WrapPos wp;
    wp.maxWidth = 79;
    wp.pos = prog.size() + usageStr.size();
    wp.prefix = string(wp.pos, ' ');
    if (cmdName.size())
        writeToken(os, wp, cmdName);
    if (!ndx.shortNames.empty() || !ndx.longNames.empty()) {
        if (!expandedOptions) {
            writeToken(os, wp, "[OPTIONS]");
        } else {
            size_t colWidth = 0;
            vector<ArgKey> namedArgs;
            findNamedArgs(
                namedArgs, colWidth, ndx, cmd, kNameNonDefault, true);
            for (auto && key : namedArgs)
                writeToken(os, wp, "[" + key.list + "]");
        }
    }
    if (cmdName.empty() && m_cfg->cmds.size() > 1) {
        writeToken(os, wp, "command");
        writeToken(os, wp, "[args...]");
    } else {
        for (auto && pa : ndx.argNames) {
            string token = pa.name.find(' ') == string::npos
                ? pa.name
                : "<" + pa.name + ">";
            if (pa.opt->m_multiple)
                token += "...";
            if (pa.optional) {
                writeToken(os, wp, "[" + token + "]");
            } else {
                writeToken(os, wp, token);
            }
        }
    }
    os << '\n';
    return exitCode();
}

//===========================================================================
int Cli::printUsage(ostream & os, const string & arg0, const string & cmd)
    const {
    return writeUsageImpl(os, arg0, cmd, false);
}

//===========================================================================
int Cli::printUsageEx(ostream & os, const string & arg0, const string & cmd)
    const {
    return writeUsageImpl(os, arg0, cmd, true);
}

//===========================================================================
void Cli::printPositionals(ostream & os, const string & cmd) const {
    OptIndex ndx;
    index(ndx, cmd, true);
    size_t colWidth = 0;
    bool hasDesc = false;
    for (auto && pa : ndx.argNames) {
        // find widest positional argument name, with space for <>'s
        colWidth = max(colWidth, pa.name.size() + 2);
        hasDesc = hasDesc || pa.opt->m_desc.size();
    }
    colWidth = max(min(colWidth + 3, kMaxDescCol), kMinDescCol);
    if (!hasDesc)
        return;

    WrapPos wp;
    for (auto && pa : ndx.argNames) {
        wp.prefix.assign(4, ' ');
        writeToken(os, wp, "  " + pa.name);
        writeDescCol(os, wp, descStr(*pa.opt), colWidth);
        os << '\n';
        wp.pos = 0;
        writeChoices(os, wp, pa.opt->m_choiceDescs);
    }
}

//===========================================================================
bool Cli::findNamedArgs(
    vector<ArgKey> & namedArgs,
    size_t & colWidth,
    const OptIndex & ndx,
    CommandConfig & cmd,
    int type,
    bool flatten) const {
    namedArgs.clear();
    for (auto && opt : m_cfg->opts) {
        string list = nameList(ndx, *opt, type);
        if (size_t width = list.size()) {
            colWidth = max(colWidth, width);
            ArgKey key;
            key.opt = opt.get();
            key.list = list;

            // sort by group sort key followed by name list with leading
            // dashes removed
            key.sort = findGrpAlways(cmd, opt->m_group).sortKey;
            if (flatten && key.sort != kInternalOptionGroup)
                key.sort.clear();
            key.sort += '\0';
            key.sort += list.substr(list.find_first_not_of('-'));
            namedArgs.push_back(key);
        }
    }
    sort(namedArgs.begin(), namedArgs.end(), [](auto & a, auto & b) {
        return a.sort < b.sort;
    });
    return !namedArgs.empty();
}

//===========================================================================
void Cli::printOptions(ostream & os, const string & cmdName) const {
    OptIndex ndx;
    index(ndx, cmdName, true);
    auto & cmd = findCmdAlways(*m_cfg, cmdName);

    // find named args and the longest name list
    size_t colWidth = 0;
    vector<ArgKey> namedArgs;
    if (!findNamedArgs(namedArgs, colWidth, ndx, cmd, kNameAll, false))
        return;
    colWidth = max(min(colWidth + 3, kMaxDescCol), kMinDescCol);

    WrapPos wp;
    const char * gname{nullptr};
    for (auto && key : namedArgs) {
        if (!gname || key.opt->m_group != gname) {
            gname = key.opt->m_group.c_str();
            writeNewline(os, wp);
            auto & grp = findGrpAlways(cmd, key.opt->m_group);
            string title = grp.title;
            if (title.empty() && strcmp(gname, kInternalOptionGroup) == 0
                && &key == namedArgs.data()) {
                // First group and it's the internal group, give it a title
                // so it's not just left hanging.
                title = "Options";
            }
            if (!title.empty()) {
                wp.prefix.clear();
                writeText(os, wp, title + ":");
                writeNewline(os, wp);
            }
        }
        wp.prefix.assign(4, ' ');
        os << ' ';
        wp.pos = 1;
        writeText(os, wp, key.list);
        writeDescCol(os, wp, descStr(*key.opt), colWidth);
        wp.prefix.clear();
        writeNewline(os, wp);
        writeChoices(os, wp, key.opt->m_choiceDescs);
        wp.prefix.clear();
    }
}

//===========================================================================
static string trim(const string & val) {
    const char * first = val.data();
    const char * last = first + val.size();
    while (isspace(*first))
        ++first;
    if (!*first)
        return {};
    while (isspace(*--last)) {
        if (last == first)
            break;
    }
    return string(first, last - first + 1);
}

//===========================================================================
void Cli::printCommands(ostream & os) const {
    size_t colWidth = 0;
    struct CmdKey {
        const char * name;
        const CommandConfig * cmd;
    };
    vector<CmdKey> keys;
    for (auto && cmd : m_cfg->cmds) {
        if (auto width = cmd.first.size()) {
            colWidth = max(colWidth, width);
            CmdKey key = {cmd.first.c_str(), &cmd.second};
            keys.push_back(key);
        }
    }
    if (keys.empty())
        return;
    colWidth = max(min(colWidth + 3, kMaxDescCol), kMinDescCol);
    sort(keys.begin(), keys.end(), [](auto & a, auto & b) {
        return strcmp(a.name, b.name) < 0;
    });

    os << "\nCommands:\n";
    WrapPos wp;
    for (auto && key : keys) {
        wp.prefix.assign(4, ' ');
        writeToken(os, wp, "  "s + key.name);
        string desc = key.cmd->desc;
        size_t pos = desc.find_first_of('.');
        if (pos != string::npos)
            desc.resize(pos + 1);
        desc = trim(desc);
        writeDescCol(os, wp, desc, colWidth);
        os << '\n';
        wp.pos = 0;
    }
}

//===========================================================================
static bool includeName(
    const OptName & name,
    int type,
    const Cli::OptBase & opt,
    bool boolean,
    bool inverted) {
    if (name.opt != &opt)
        return false;
    if (boolean) {
        switch (type) {
        case kNameEnable: return !name.invert;
        case kNameDisable: return name.invert;
        case kNameAll: return true;
        case kNameNonDefault: return inverted == name.invert;
        }
        assert(0 && "unknown NameListType");
    }
    return true;
}

//===========================================================================
string Cli::nameList(
    const Cli::OptIndex & ndx,
    const Cli::OptBase & opt,
    int type) const {
    string list;

    if (type == kNameAll) {
        list = nameList(ndx, opt, kNameEnable);
        if (opt.m_bool) {
            string invert = nameList(ndx, opt, kNameDisable);
            if (!invert.empty()) {
                list += list.empty() ? "/ " : " / ";
                list += invert;
            }
        }
        return list;
    }

    bool foundLong = false;
    bool optional = false;

    // names
    for (auto && sn : ndx.shortNames) {
        if (!includeName(sn.second, type, opt, opt.m_bool, opt.inverted()))
            continue;
        optional = sn.second.optional;
        if (!list.empty())
            list += ", ";
        list += '-';
        list += sn.first;
    }
    for (auto && ln : ndx.longNames) {
        if (!includeName(ln.second, type, opt, opt.m_bool, opt.inverted()))
            continue;
        optional = ln.second.optional;
        if (!list.empty())
            list += ", ";
        foundLong = true;
        list += "--";
        list += ln.first;
    }
    if (opt.m_bool || list.empty())
        return list;

    // value
    if (optional) {
        list += foundLong ? "[=" : " [";
        list += opt.m_valueDesc + "]";
    } else {
        list += foundLong ? '=' : ' ';
        list += opt.m_valueDesc;
    }
    return list;
}


/****************************************************************************
*
*   Native console API
*
***/

#if defined(DIM_LIB_NO_CONSOLE)

//===========================================================================
void Cli::consoleEnableEcho(bool enable) {
    assert(enable && "disabling console echo not supported");
}

#elif defined(_WIN32)

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

//===========================================================================
void Cli::consoleEnableEcho(bool enable) {
    HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode = 0;
    GetConsoleMode(hInput, &mode);
    if (enable) {
        mode |= ENABLE_ECHO_INPUT;
    } else {
        mode &= ~ENABLE_ECHO_INPUT;
    }
    SetConsoleMode(hInput, mode);
}

#else

#include <termios.h>
#include <unistd.h>

//===========================================================================
void Cli::consoleEnableEcho(bool enable) {
    termios tty;
    tcgetattr(STDIN_FILENO, &tty);
    if (enable) {
        tty.c_lflag |= ECHO;
    } else {
        tty.c_lflag &= ~ECHO;
    }
    tcsetattr(STDIN_FILENO, TCSANOW, &tty);
}

#endif
