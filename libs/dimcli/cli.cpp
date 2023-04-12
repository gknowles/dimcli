// Copyright Glen Knowles 2016 - 2023.
// Distributed under the Boost Software License, Version 1.0.
//
// cli.cpp - dimcli
//
// For documentation and examples:
// https://github.com/gknowles/dimcli

#ifndef DIMCLI_LIB_SOURCE
#define DIMCLI_LIB_SOURCE
#endif
#include "cli.h"

#include <algorithm>
#include <atomic>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <locale>
#include <sstream>

using namespace std;
using namespace Dim;
#ifdef DIMCLI_LIB_FILESYSTEM
namespace fs = DIMCLI_LIB_FILESYSTEM;
#endif

// wstring_convert triggers the visual c++ security warning
#if (_MSC_VER >= 1400)
#pragma warning(disable : 4996) // this function or variable may be unsafe.
#elif __clang_major__ >= 15
#pragma clang diagnostic ignored "-Wunqualified-std-cast-call"
#endif


/****************************************************************************
*
*   Tuning parameters
*
***/

const size_t kDefaultConsoleWidth = 80;
const size_t kMinConsoleWidth = 50;
const size_t kMaxConsoleWidth = 80;

// column where description text starts
const auto kDefaultMinKeyWidth = 15.0f;
const auto kDefaultMaxKeyWidth = 38.0f;

// maximum help text line length
const size_t kDefaultMaxLineWidth = kDefaultConsoleWidth - 1;


/****************************************************************************
*
*   Declarations
*
***/

// Name of group containing --help, --version, etc
const char kInternalOptionGroup[] = "~";

namespace {

struct GroupConfig {
    string name;
    string title;
    string sortKey;
};

struct CommandConfig {
    string name;
    string header;
    string desc;
    string footer;
    function<Cli::ActionFn> action;
    string cmdGroup;
    Cli::Opt<bool> * helpOpt{};
    unordered_map<string, GroupConfig> groups;
};

struct OptName {
    Cli::OptBase * opt;
    bool invert;    // set to false instead of true (only for bools)
    bool optional;  // value need not be present? (non-bools only)
    string name;    // name of argument (only for operands)
    int pos;        // used to sort an option's names in declaration order
};

struct OptKey {
    string sort;    // sort key
    string list;    // name list
    Cli::OptBase * opt{};
};

// Option name filters for opts that are externally bool
enum NameListType {
    kNameEnable,     // include names that enable the opt
    kNameDisable,    // include names that disable
    kNameAll,        // include all names
    kNameNonDefault, // include names that change from the default
};

struct CodecvtWchar : codecvt<wchar_t, char, mbstate_t> {
    // public destructor required for use with wstring_convert
    ~CodecvtWchar() {}
};

struct RawValue {
    enum Type { kOperand, kOption, kCommand } type;
    Cli::OptBase * opt;
    string name;
    size_t pos;
    const char * ptr;

    RawValue(
        Type type,
        Cli::OptBase * opt,
        string name,
        size_t pos = 0,
        const char * ptr = nullptr
    )
        : type(type)
        , opt(opt)
        , name(name)
        , pos(pos)
        , ptr(ptr)
    {}
};

} // namespace

struct Cli::OptIndex {
    unordered_map<char, OptName> m_shortNames;
    unordered_map<string, OptName> m_longNames;
    vector<OptName> m_argNames;
    bool m_allowCommands {};

    enum class Final {
        // In order of priority
        kUnset,     // nothing interesting found
        kNonOpt,    // found an optional operand
        kNonVar,    // found variable size operand
        kOpt,       // found final on an optional operand
        kReq,       // found final on required operand
    } m_final {};
    int m_minOprs {0};
    int m_finalOpr {0};

    void index(
        const Cli & cli,
        const string & cmd,
        bool requireVisible
    );
    void index(OptBase & opt);
    vector<OptKey> findNamedOpts(
        const Cli & cli,
        CommandConfig & cmd,
        NameListType type,
        bool flatten
    ) const;
    string nameList(
        const Cli & cli,
        const OptBase & opt,
        NameListType type
    ) const;

    static string desc(const OptBase & opt, bool withMarkup = true);
    static const unordered_map<string, OptBase::ChoiceDesc> & choiceDescs(
        const OptBase & opt
    );

private:
    void indexName(OptBase & opt, const string & name, int pos);
    void indexShortName(
        OptBase & opt,
        char name,
        bool invert,
        bool optional,
        int pos
    );
    void indexLongName(
        OptBase & opt,
        const string & name,
        bool invert,
        bool optional,
        int pos
    );
};

struct Cli::Config {
    vector<function<BeforeFn>> befores;
    bool allowUnknown {false};
    function<ActionFn> unknownCmd;
    unordered_map<string, CommandConfig> cmds;
    unordered_map<string, GroupConfig> cmdGroups;
    list<unique_ptr<OptBase>> opts;
    bool responseFiles {true};
    string envOpts;
    istream * conin {&cin};
    ostream * conout {&cout};
    shared_ptr<locale> defLoc = make_shared<locale>();
    shared_ptr<locale> numLoc = make_shared<locale>("");

    bool parseExit {false};
    int exitCode {kExitOk};
    string errMsg;
    string errDetail;
    string progName;
    string command;
    vector<string> unknownArgs;

    size_t maxWidth {kDefaultConsoleWidth};
    float minKeyWidth {kDefaultMinKeyWidth};
    float maxKeyWidth {kDefaultMaxKeyWidth};
    size_t maxLineWidth {kDefaultMaxLineWidth};

    static void touchAllCmds(Cli & cli);
    static Config & get(Cli & cli);
    static CommandConfig & findCmdAlways(Cli & cli);
    static CommandConfig & findCmdAlways(Cli & cli, const string & name);
    static const CommandConfig & findCmdOrDie(const Cli & cli);

    static GroupConfig & findCmdGrpAlways(Cli & cli);
    static GroupConfig & findCmdGrpAlways(Cli & cli, const string & name);
    static GroupConfig & findCmdGrpOrDie(const Cli & cli);

    static GroupConfig & findGrpAlways(Cli & cli);
    static GroupConfig & findGrpAlways(
        CommandConfig & cmd,
        const string & name
    );
    static const GroupConfig & findGrpOrDie(const Cli & cli);

    Config();
    void updateWidth(size_t width);
};


#ifdef DIMCLI_LIB_BUILD_COVERAGE
/****************************************************************************
*
*   Hook for testing asserts
*
***/

static atomic<AssertHandlerFn> s_assertFn;

//===========================================================================
AssertHandlerFn Dim::setAssertHandler(AssertHandlerFn fn) {
    return s_assertFn.exchange(fn);
}

//===========================================================================
void Dim::doAssert(const char expr[], unsigned line) {
    if (auto fn = s_assertFn.load())
        (*fn)(expr, line);
}
#endif


/****************************************************************************
*
*   Helpers
*
***/

// forward declarations
static void helpOptAction(Cli & cli, Cli::Opt<bool> & opt, const string & val);
static void defCmdAction(Cli & cli);
static void writeChoicesDetail(
    string * outPtr,
    const unordered_map<string, Cli::OptBase::ChoiceDesc> & choices
);
static string format(const Cli::Config & cfg, const string & text);

//===========================================================================
#if defined(_WIN32)
static string displayName(const string & file) {
    char fname[_MAX_FNAME];
    char ext[_MAX_EXT];
    if (!_splitpath_s(
        file.c_str(),
        nullptr, 0,
        nullptr, 0,
        fname, sizeof fname,
        ext, sizeof ext
    )) {
        auto & f = use_facet<ctype<char>>(locale());
        string out = fname;
        auto flen = out.size();
        out += ext;
        f.tolower(ext, ext + out.size() - flen);
        if (strcmp(ext, ".exe") == 0 || strcmp(ext, ".com") == 0)
            out.resize(flen);
        return out;
    }

    // Split failed, return full source path.
    return file;
}
#else
#include <libgen.h>
static string displayName(string file) {
    return basename((char *) file.data());
}
#endif

//===========================================================================
// Replaces a set of contiguous values in one vector with the entire contents
// of another, growing or shrinking it as needed.
template <typename T>
static void replace(
    vector<T> & out,
    size_t pos,
    size_t count,
    vector<T> && src
) {
    auto srcLen = src.size();
    if (count > srcLen) {
        out.erase(out.begin() + pos + srcLen, out.begin() + pos + count);
    } else if (count < srcLen) {
        out.insert(out.begin() + pos + count, srcLen - count, {});
    }
    auto i = out.begin() + pos;
    for (auto && val : src)
        *i++ = move(val);
}

//===========================================================================
static string trim(const string & val) {
    auto first = val.c_str();
    auto last = first + val.size();
    while (isspace(*first))
        ++first;
    if (!*first)
        return {};
    while (isspace(*--last))
        ;
    return string(first, last - first + 1);
}

//===========================================================================
static string intToString(const Cli::Convert & cvt, int val) {
    string tmp;
    (void) cvt.toString(tmp, val);
    return tmp;
}


/****************************************************************************
*
*   CliLocal
*
***/

//===========================================================================
CliLocal::CliLocal()
    : Cli(make_shared<Config>())
{}


/****************************************************************************
*
*   Cli::Config
*
***/

//===========================================================================
// static
void Cli::Config::touchAllCmds(Cli & cli) {
    // Make sure all opts have a backing command config.
    for (auto && opt : cli.m_cfg->opts)
        Config::findCmdAlways(cli, opt->m_command);
    // Make sure all commands have a backing command group.
    for (auto && cmd : cli.m_cfg->cmds)
        Config::findCmdGrpAlways(cli, cmd.second.cmdGroup);
}

//===========================================================================
// static
Cli::Config & Cli::Config::get(Cli & cli) {
    return *cli.m_cfg;
}

//===========================================================================
// static
CommandConfig & Cli::Config::findCmdAlways(Cli & cli) {
    return findCmdAlways(cli, cli.command());
}

//===========================================================================
// static
CommandConfig & Cli::Config::findCmdAlways(
    Cli & cli,
    const string & name
) {
    auto & cmds = cli.m_cfg->cmds;
    auto i = cmds.find(name);
    if (i != cmds.end())
        return i->second;

    auto & cmd = cmds[name];
    cmd.name = name;
    cmd.action = defCmdAction;
    cmd.cmdGroup = cli.cmdGroup();
    auto & defGrp = findGrpAlways(cmd, "");
    defGrp.title = "Options";
    auto & intGrp = findGrpAlways(cmd, kInternalOptionGroup);
    intGrp.title.clear();
    auto & hlp = cli.opt<bool>("help.")
        .desc("Show this message and exit.")
        .check(helpOptAction)
        .command(name)
        .group(kInternalOptionGroup);
    cmd.helpOpt = &hlp;
    return cmd;
}

//===========================================================================
// static
const CommandConfig & Cli::Config::findCmdOrDie(const Cli & cli) {
    auto & cmds = cli.m_cfg->cmds;
    auto i = cmds.find(cli.command());
    assert(i != cmds.end() // LCOV_EXCL_LINE
        && "Internal dimcli error: uninitialized command context.");
    return i->second;
}

//===========================================================================
// static
GroupConfig & Cli::Config::findCmdGrpAlways(Cli & cli) {
    return findCmdGrpAlways(cli, cli.cmdGroup());
}

//===========================================================================
// static
GroupConfig & Cli::Config::findCmdGrpAlways(Cli & cli, const string & name) {
    auto & grps = cli.m_cfg->cmdGroups;
    auto i = grps.find(name);
    if (i != grps.end())
        return i->second;

    auto & grp = grps[name];
    grp.name = grp.sortKey = name;
    if (name.empty()) {
        grp.title = "Commands";
    } else if (name == kInternalOptionGroup) {
        grp.title = "";
    } else {
        grp.title = name;
    }
    return grp;
}

//===========================================================================
// static
GroupConfig & Cli::Config::findCmdGrpOrDie(const Cli & cli) {
    auto & name = cli.cmdGroup();
    auto & grps = cli.m_cfg->cmdGroups;
    auto i = grps.find(name);
    assert(i != grps.end() // LCOV_EXCL_LINE
        && "Internal dimcli error: uninitialized command group context.");
    return i->second;
}

//===========================================================================
// static
GroupConfig & Cli::Config::findGrpAlways(Cli & cli) {
    return findGrpAlways(findCmdAlways(cli), cli.group());
}

//===========================================================================
// static
GroupConfig & Cli::Config::findGrpAlways(
    CommandConfig & cmd,
    const string & name
) {
    auto i = cmd.groups.find(name);
    if (i != cmd.groups.end())
        return i->second;
    auto & grp = cmd.groups[name];
    grp.name = grp.sortKey = grp.title = name;
    return grp;
}

//===========================================================================
// static
const GroupConfig & Cli::Config::findGrpOrDie(const Cli & cli) {
    auto & grps = Config::findCmdOrDie(cli).groups;
    auto i = grps.find(cli.group());
    assert(i != grps.end() // LCOV_EXCL_LINE
        && "Internal dimcli error: uninitialized group context.");
    return i->second;
}

//===========================================================================
Cli::Config::Config() {
    static size_t width = clamp<size_t>(
        Cli::consoleWidth(),
        kMinConsoleWidth,
        kMaxConsoleWidth
    );
    updateWidth(width);
}

//===========================================================================
void Cli::Config::updateWidth(size_t width) {
    this->maxWidth = width;
    this->maxLineWidth = width - 1;
    this->minKeyWidth = kDefaultMinKeyWidth
        * (kDefaultConsoleWidth + width) / 2
        / width;
    this->maxKeyWidth = kDefaultMaxKeyWidth;
}


/****************************************************************************
*
*   Cli::OptBase
*
***/

//===========================================================================
Cli::OptBase::OptBase(const string & names, bool flag)
    : m_bool{flag}
    , m_names{names}
{
    // Set m_fromName and assert if names is malformed.
    OptIndex ndx;
    ndx.index(*this);
}

//===========================================================================
locale Cli::OptBase::imbue(const locale & loc) {
    return m_interpreter.imbue(loc);
}

//===========================================================================
string Cli::OptBase::defaultPrompt() const {
    auto name = (string) m_fromName;
    while (name.size() && name[0] == '-')
        name.erase(0, 1);
    if (name.size())
        name[0] = (char)toupper(name[0]);
    return name;
}

//===========================================================================
void Cli::OptBase::setNameIfEmpty(const string & name) {
    if (m_fromName.empty())
        m_fromName = name;
}

//===========================================================================
bool Cli::OptBase::withUnits(
    long double & out,
    Cli & cli,
    const string & val,
    const unordered_map<string, long double> & units,
    int flags
) const {
    auto & f = use_facet<ctype<char>>(m_interpreter.getloc());

    auto pos = val.size();
    for (;;) {
        if (!pos--) {
            cli.badUsage(*this, val);
            return false;
        }
        if (f.is(f.digit, val[pos]) || val[pos] == '.') {
            pos += 1;
            break;
        }
    }
    auto num = val.substr(0, pos);
    auto unit = val.substr(pos);

    if (!fromString(out, num)) {
        cli.badUsage(*this, val);
        return false;
    }
    if (unit.empty()) {
        if (~flags & fUnitRequire)
            return true;
        cli.badUsage(
            *this,
            val,
            "Value requires suffix specifying the units."
        );
        return false;
    }

    if (flags & fUnitInsensitive)
        f.tolower(unit.data(), unit.data() + unit.size());
    auto i = units.find(unit);
    if (i != units.end()) {
        out *= i->second;
        return true;
    } else {
        cli.badUsage(
            *this,
            val,
            "Units symbol '" + unit + "' not recognized."
        );
        return false;
    }
}


/****************************************************************************
*
*   Cli::OptIndex
*
***/

//===========================================================================
void Cli::OptIndex::index(
    const Cli & cli,
    const string & cmd,
    bool requireVisible
) {
    *this = {};
    m_allowCommands = cmd.empty();

    for (auto && opt : cli.m_cfg->opts) {
        if (opt->m_command == cmd && (opt->m_visible || !requireVisible))
            index(*opt);
    }

    if (m_final < Final::kOpt)
        m_finalOpr = -1;

    for (unsigned i = 0; i < m_argNames.size(); ++i) {
        auto & key = m_argNames[i];
        if (key.name.empty())
            key.name = "ARG" + to_string(i + 1);
    }
}

//===========================================================================
vector<OptKey> Cli::OptIndex::findNamedOpts(
    const Cli & cli,
    CommandConfig & cmd,
    NameListType type,
    bool flatten
) const {
    vector<OptKey> out;
    for (auto && opt : cli.m_cfg->opts) {
        auto list = nameList(cli, *opt, type);
        if (list.size()) {
            OptKey key;
            key.opt = opt.get();
            key.list = list;

            // Sort by group sort key followed by name list with leading
            // dashes removed.
            key.sort = Config::findGrpAlways(cmd, opt->m_group).sortKey;
            if (flatten && key.sort != kInternalOptionGroup)
                key.sort.clear();
            key.sort += '\0';
            key.sort += list.substr(list.find_first_not_of('-'));
            out.push_back(key);
        }
    }
    sort(out.begin(), out.end(), [](auto & a, auto & b) {
        return a.sort < b.sort;
    });
    return out;
}

//===========================================================================
static bool includeName(
    const OptName & name,
    NameListType type,
    const Cli::OptBase & opt,
    bool flag,
    bool inverted
) {
    if (name.opt != &opt)
        return false;
    if (flag) {
        if (type == kNameEnable)
            return !name.invert;
        if (type == kNameDisable)
            return name.invert;

        // includeName is always called with a filter (i.e. not kNameAll).
        assert(type == kNameNonDefault // LCOV_EXCL_LINE
            && "Internal dimcli error: unknown NameListType.");
        return inverted == name.invert;
    }
    return true;
}

//===========================================================================
string Cli::OptIndex::nameList(
    const Cli & cli,
    const Cli::OptBase & opt,
    NameListType type
) const {
    string list;

    if (type == kNameAll) {
        list = nameList(cli, opt, kNameEnable);
        if (opt.m_bool) {
            auto invert = nameList(cli, opt, kNameDisable);
            if (!invert.empty()) {
                list += list.empty() ? "/ " : " / ";
                list += invert;
            }
        }
        return list;
    }

    bool foundLong = false;
    bool optional = false;

    // Names
    vector<const decltype(m_shortNames)::value_type *> snames;
    for (auto & sn : m_shortNames)
        snames.push_back(&sn);
    sort(snames.begin(), snames.end(), [](auto & a, auto & b) {
        return a->second.pos < b->second.pos;
    });
    for (auto && sn : snames) {
        if (!includeName(sn->second, type, opt, opt.m_bool, opt.inverted()))
            continue;
        optional = sn->second.optional;
        if (!list.empty())
            list += ", ";
        list += '-';
        list += sn->first;
    }
    vector<const decltype(m_longNames)::value_type *> lnames;
    for (auto & ln : m_longNames)
        lnames.push_back(&ln);
    sort(lnames.begin(), lnames.end(), [](auto & a, auto & b) {
        return a->second.pos < b->second.pos;
    });
    for (auto && ln : lnames) {
        if (!includeName(ln->second, type, opt, opt.m_bool, opt.inverted()))
            continue;
        optional = ln->second.optional;
        if (!list.empty())
            list += ", ";
        foundLong = true;
        list += "--";
        list += ln->first;
    }
    if (opt.m_bool || list.empty())
        return list;

    // Value
    auto valDesc = opt.m_valueDesc.empty()
        ? opt.defaultValueDesc()
        : opt.m_valueDesc;
    if (optional) {
        list += foundLong ? "[=" : " [";
        list += valDesc + "]";
    } else {
        list += foundLong ? '=' : ' ';
        list += valDesc;
    }
    return list;
}

//===========================================================================
void Cli::OptIndex::index(OptBase & opt) {
    auto ptr = opt.m_names.c_str();
    string name;
    char close;
    bool hasPos = false;
    int pos = 0;
    for (;; ++ptr) {
        switch (*ptr) {
        case 0: return;
        case ' ': continue;
        case '[': close = ']'; break;
        case '<': close = '>'; break;
        default: close = ' '; break;
        }
        auto b = ptr;
        bool hasEqual = false;
        while (*ptr && *ptr != close) {
            if (*ptr == '=')
                hasEqual = true;
            ptr += 1;
        }
        if (hasEqual && close == ' ') {
            assert(!"Bad argument name, contains '='.");
        } else if (hasPos && close != ' ') {
            assert(!"Argument with multiple operand names.");
        } else {
            if (close == ' ') {
                name = string(b, ptr - b);
            } else {
                hasPos = true;
                name = string(b, 1);
                name += trim(string(b + 1, ptr - b - 1));
            }
            indexName(opt, name, pos);
            pos += 2;
        }
        if (!*ptr)
            return;
    }
}

//===========================================================================
void Cli::OptIndex::indexName(OptBase & opt, const string & name, int pos) {
    bool invert = false;
    bool optional = false;

    switch (name[0]) {
    case '-':
        assert(!"Bad argument name, starts with '-'.");
        return;
    case '[':
        optional = true;
        // fallthrough
    case '<':
        if (!opt.maxSize())
            return;
        if (!optional)
            m_minOprs += opt.minSize();
        if (opt.m_command.empty()
            && (optional || opt.minSize() != opt.maxSize())
        ) {
            // Operands that cause potential subcommand names to be ambiguous
            // are not allowed to be combined with them.
            m_allowCommands = false;
        }

        if (m_final == Final::kOpt && !optional) {
            assert(!"Required operand after optional operand w/finalOpt.");
            return;
        }
        if (!opt.m_finalOpt) {
            if (m_final < Final::kNonVar && opt.minSize() != opt.maxSize())
                m_final = Final::kNonVar;
            if (m_final < Final::kNonOpt && optional)
                m_final = Final::kNonOpt;
        } else if (m_final == Final::kNonVar) {
            assert(!"Operand w/finalOpt after variable size operand.");
            return;
        } else if (m_final == Final::kNonOpt) {
            if (optional) {
                m_final = Final::kOpt;
            } else {
                assert(!"Required operand w/finalOpt after optional operand.");
                return;
            }
        } else if (m_final == Final::kUnset) {
            if (optional) {
                m_final = Final::kOpt;
            } else {
                m_final = Final::kReq;
            }
        }
        if (m_final < Final::kOpt)
            m_finalOpr += opt.minSize();

        OptName oname = {&opt, invert, optional, name.data() + 1, pos};
        m_argNames.push_back(oname);
        opt.setNameIfEmpty(m_argNames.back().name);
        return;
    }

    auto prefix = 0;
    if (name.size() > 1) {
        switch (name[0]) {
        case '!':
            if (!opt.m_bool) {
                // Inversion doesn't make any sense for non-bool options and
                // will be ignored for them. But it is allowed to be specified
                // because they might be turned into flagValues, and flagValues
                // act like bools syntacticly.
                //
                // And the case of an inverted bool converted to a flagValue
                // has to be handled anyway.
            }
            prefix = 1;
            invert = true;
            break;
        case '?':
            if (opt.m_bool) {
                // Bool options don't have values, only their presences or
                // absence, therefore they can't have optional values.
                assert(!"Bad modifier '?' for bool argument.");
                return;
            }
            prefix = 1;
            optional = true;
            break;
        }
    }
    if (name.size() - prefix == 1) {
        indexShortName(opt, name[prefix], invert, optional, pos);
    } else {
        indexLongName(opt, name.substr(prefix), invert, optional, pos);
    }
}

//===========================================================================
void Cli::OptIndex::indexShortName(
    OptBase & opt,
    char name,
    bool invert,
    bool optional,
    int pos
) {
    m_shortNames[name] = {&opt, invert, optional, {}, pos};
    opt.setNameIfEmpty("-"s + name);
}

//===========================================================================
void Cli::OptIndex::indexLongName(
    OptBase & opt,
    const string & name,
    bool invert,
    bool optional,
    int pos
) {
    bool allowNo = true;
    auto key = string{name};
    if (key.back() == '.') {
        allowNo = false;
        if (key.size() == 2) {
            assert(!"Bad modifier '.' for short name.");
            return;
        }
        key.pop_back();
    }
    opt.setNameIfEmpty("--" + key);
    m_longNames[key] = {&opt, invert, optional, {}, pos};
    if (allowNo && opt.m_bool && !opt.m_flagValue)
        m_longNames["no-" + key] = {&opt, !invert, optional, {}, pos + 1};
}


/****************************************************************************
*
*   Action callbacks
*
***/

//===========================================================================
// static
void Cli::defParseAction(Cli & cli, OptBase & opt, const string & val) {
    if (opt.parseValue(val))
        return;

    string desc;
    writeChoicesDetail(&desc, opt.m_choiceDescs);
    cli.badUsage(opt, val, desc);
}

//===========================================================================
// static
void Cli::requireAction(Cli & cli, OptBase & opt, const string &) {
    if (opt)
        return;

    const string & name = !opt.defaultFrom().empty()
        ? opt.defaultFrom()
        : "UNKNOWN";
    cli.badUsage("No value given for " + name);
}

//===========================================================================
static void helpBeforeAction(Cli &, vector<string> & args) {
    if (args.size() == 1)
        args.push_back("--help");
}

//===========================================================================
static void helpOptAction(
    Cli & cli,
    Cli::Opt<bool> & opt,
    const string & // val
) {
    if (*opt) {
        cli.printHelp(cli.conout(), {}, cli.commandMatched());
        cli.parseExit();
    }
}

//===========================================================================
static void defCmdAction(Cli & cli) {
    if (cli.commandMatched().empty()) {
        cli.badUsage("No command given.");
    } else {
        cli.fail(
            kExitSoftware,
            "Command '" + cli.commandMatched() + "' has not been implemented."
        );
    }
}

//===========================================================================
static void helpCmdAction(Cli & cli) {
    Cli::OptIndex ndx;
    ndx.index(cli, cli.commandMatched(), false);
    auto cmd = *static_cast<Cli::Opt<string> &>(*ndx.m_argNames[0].opt);
    auto usage = *static_cast<Cli::Opt<bool> &>(*ndx.m_shortNames['u'].opt);
    if (!cli.commandExists(cmd))
        return cli.badUsage("Help requested for unknown command", cmd);

    if (usage) {
        cli.printUsageEx(cli.conout(), {}, cmd);
    } else {
        cli.printHelp(cli.conout(), {}, cmd);
    }
    cli.parseExit();
}


/****************************************************************************
*
*   Cli
*
***/

//===========================================================================
static shared_ptr<Cli::Config> globalConfig() {
    static auto s_cfg = make_shared<Cli::Config>();
    return s_cfg;
}

//===========================================================================
// Creates a handle to the shared configuration
Cli::Cli()
    : m_cfg(globalConfig())
{
    helpOpt();
}

//===========================================================================
Cli::Cli(const Cli & from)
    : m_cfg(from.m_cfg)
    , m_group(from.m_group)
    , m_command(from.m_command)
{}

//===========================================================================
// protected
Cli::Cli(shared_ptr<Config> cfg)
    : m_cfg(cfg)
{
    helpOpt();
}

//===========================================================================
Cli & Cli::operator=(const Cli & from) {
    m_cfg = from.m_cfg;
    m_group = from.m_group;
    m_command = from.m_command;
    return *this;
}

//===========================================================================
Cli & Cli::operator=(Cli && from) noexcept {
    m_cfg = move(from.m_cfg);
    m_group = move(from.m_group);
    m_command = move(from.m_command);
    return *this;
}


/****************************************************************************
*
*   Configuration
*
***/

//===========================================================================
Cli::Opt<bool> & Cli::confirmOpt(const string & prompt) {
    auto & ask = opt<bool>("y yes.")
        .desc("Suppress prompting to allow execution.")
        .check([](auto & cli, auto & opt, auto &) {
            if (!*opt)
                cli.parseExit();
        })
        .prompt(prompt.empty() ? "Are you sure?" : prompt);
    return ask;
}

//===========================================================================
Cli::Opt<bool> & Cli::helpOpt() {
    return *Config::findCmdAlways(*this).helpOpt;
}

//===========================================================================
Cli::Opt<string> & Cli::passwordOpt(bool confirm) {
    return opt<string>("password.")
        .desc("Password required for access.")
        .prompt(fPromptHide | fPromptNoDefault | (confirm * fPromptConfirm));
}

//===========================================================================
Cli::Opt<bool> & Cli::versionOpt(
    const string & version,
    const string & progName
) {
    auto act = [version, progName](auto & cli, auto &/*opt*/, auto &/*val*/) {
        auto prog = string{progName};
        if (prog.empty())
            prog = displayName(cli.progName());
        cli.conout() << prog << " version " << version << endl;
        cli.parseExit();
    };
    return opt<bool>("version.")
        .desc("Show version and exit.")
        .check(act)
        .group(kInternalOptionGroup);
}

//===========================================================================
Cli & Cli::group(const string & name) & {
    m_group = name;
    Config::findGrpAlways(*this);
    return *this;
}

//===========================================================================
Cli && Cli::group(const string & name) && {
    return move(group(name));
}

//===========================================================================
Cli & Cli::title(const string & val) & {
    Config::findGrpAlways(*this).title = val;
    return *this;
}

//===========================================================================
Cli && Cli::title(const string & val) && {
    return move(title(val));
}

//===========================================================================
Cli & Cli::sortKey(const string & val) & {
    Config::findGrpAlways(*this).sortKey = val;
    return *this;
}

//===========================================================================
Cli && Cli::sortKey(const string & val) && {
    return move(sortKey(val));
}

//===========================================================================
const string & Cli::title() const {
    return Config::findGrpOrDie(*this).title;
}

//===========================================================================
const string & Cli::sortKey() const {
    return Config::findGrpOrDie(*this).sortKey;
}

//===========================================================================
Cli & Cli::command(const string & name, const string & grpName) & {
    Config::findCmdAlways(*this, name);
    m_command = name;
    m_group = grpName;
    return *this;
}

//===========================================================================
Cli && Cli::command(const string & name, const string & grpName) && {
    return move(command(name, grpName));
}

//===========================================================================
Cli & Cli::action(function<ActionFn> fn) & {
    Config::findCmdAlways(*this).action = move(fn);
    return *this;
}

//===========================================================================
Cli && Cli::action(function<ActionFn> fn) && {
    return move(action(fn));
}

//===========================================================================
Cli & Cli::header(const string & val) & {
    auto & hdr = Config::findCmdAlways(*this).header;
    hdr = val;
    if (hdr.empty())
        hdr.push_back('\0');
    return *this;
}

//===========================================================================
Cli && Cli::header(const string & val) && {
    return move(header(val));
}

//===========================================================================
Cli & Cli::desc(const string & val) & {
    Config::findCmdAlways(*this).desc = val;
    return *this;
}

//===========================================================================
Cli && Cli::desc(const string & val) && {
    return move(desc(val));
}

//===========================================================================
Cli & Cli::footer(const string & val) & {
    auto & ftr = Config::findCmdAlways(*this).footer;
    ftr = val;
    if (ftr.empty())
        ftr.push_back('\0');
    return *this;
}

//===========================================================================
Cli && Cli::footer(const string & val) && {
    return move(footer(val));
}

//===========================================================================
const string & Cli::header() const {
    return Config::findCmdOrDie(*this).header;
}

//===========================================================================
const string & Cli::desc() const {
    return Config::findCmdOrDie(*this).desc;
}

//===========================================================================
const string & Cli::footer() const {
    return Config::findCmdOrDie(*this).footer;
}

//===========================================================================
Cli & Cli::helpCmd() & {
    // Use new instance so the current context (command and option group) is
    // preserved.
    Cli cli{*this};
    cli.command("help")
        .cmdGroup(kInternalOptionGroup)
        .desc("Show help for individual commands and exit. If no command is "
            "given the list of commands and general options are shown.")
        .action(helpCmdAction);
    cli.opt<string>("[COMMAND]")
        .desc("Command to show help information about.");
    cli.opt<bool>("u usage")
        .desc("Only show condensed usage.");
    return *this;
}

//===========================================================================
Cli && Cli::helpCmd() && {
    return move(helpCmd());
}

//===========================================================================
Cli & Cli::unknownCmd(function<ActionFn> fn) & {
    m_cfg->allowUnknown = true;
    m_cfg->unknownCmd = move(fn);
    return *this;
}

//===========================================================================
Cli && Cli::unknownCmd(function<ActionFn> fn) && {
    return move(unknownCmd(fn));
}

//===========================================================================
Cli & Cli::helpNoArgs() & {
    return before(helpBeforeAction);
}

//===========================================================================
Cli && Cli::helpNoArgs() && {
    return move(helpNoArgs());
}

//===========================================================================
Cli & Cli::cmdGroup(const string & name) & {
    Config::findCmdAlways(*this).cmdGroup = name;
    Config::findCmdGrpAlways(*this);
    return *this;
}

//===========================================================================
Cli && Cli::cmdGroup(const string & name) && {
    return move(cmdGroup(name));
}

//===========================================================================
Cli & Cli::cmdTitle(const string & val) & {
    Config::findCmdGrpAlways(*this).title = val;
    return *this;
}

//===========================================================================
Cli && Cli::cmdTitle(const string & val) && {
    return move(cmdTitle(val));
}

//===========================================================================
Cli & Cli::cmdSortKey(const string & key) & {
    Config::findCmdGrpAlways(*this).sortKey = key;
    return *this;
}

//===========================================================================
Cli && Cli::cmdSortKey(const string & key) && {
    return move(cmdSortKey(key));
}

//===========================================================================
const string & Cli::cmdGroup() const {
    return Config::findCmdOrDie(*this).cmdGroup;
}

//===========================================================================
const string & Cli::cmdTitle() const {
    return Config::findCmdGrpOrDie(*this).title;
}

//===========================================================================
const string & Cli::cmdSortKey() const {
    return Config::findCmdGrpOrDie(*this).sortKey;
}

//===========================================================================
Cli & Cli::before(function<BeforeFn> fn) & {
    m_cfg->befores.push_back(move(fn));
    return *this;
}

//===========================================================================
Cli && Cli::before(function<BeforeFn> fn) && {
    return move(before(fn));
}

#if !defined(DIMCLI_LIB_NO_ENV)
//===========================================================================
Cli & Cli::envOpts(const string & var) & {
    m_cfg->envOpts = var;
    return *this;
}

//===========================================================================
Cli && Cli::envOpts(const string & var) && {
    return move(envOpts(var));
}
#endif

//===========================================================================
Cli & Cli::responseFiles(bool enable) & {
    m_cfg->responseFiles = enable;
    return *this;
}

//===========================================================================
Cli && Cli::responseFiles(bool enable) && {
    return move(responseFiles(enable));
}

//===========================================================================
Cli & Cli::iostreams(istream * in, ostream * out) & {
    m_cfg->conin = in ? in : &cin;
    m_cfg->conout = out ? out : &cout;
    return *this;
}

//===========================================================================
Cli && Cli::iostreams(istream * in, ostream * out) && {
    return move(iostreams(in, out));
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
Cli & Cli::maxWidth(int maxWidth, int minDescCol, int maxDescCol) & {
    m_cfg->updateWidth(maxWidth);
    if (minDescCol)
        m_cfg->minKeyWidth = 100.0f * minDescCol / maxWidth;
    if (maxDescCol)
        m_cfg->maxKeyWidth = 100.0f * maxDescCol / maxWidth;
    return *this;
}

//===========================================================================
Cli && Cli::maxWidth(int maxW, int minDescCol, int maxDescCol) && {
    return move(maxWidth(maxW, minDescCol, maxDescCol));
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

//===========================================================================
bool Cli::parseExited() const {
    return m_cfg->parseExit;
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
vector<string> Cli::toArgv(size_t argc, char * argv[]) {
    vector<string> out;
    out.reserve(argc);
    for (unsigned i = 0; i < argc && argv[i]; ++i)
        out.push_back(argv[i]);
    if (argc != out.size() || argv[argc]) {
        assert(!"Bad arguments, "
            "argc and null terminator don't agree.");
    }
    return out;
}

//===========================================================================
// static
vector<string> Cli::toArgv(size_t argc, const char * argv[]) {
    return toArgv(argc, (char **) argv);
}

//===========================================================================
// static
vector<string> Cli::toArgv(size_t argc, wchar_t * argv[]) {
    vector<string> out;
    out.reserve(argc);
    wstring_convert<CodecvtWchar> wcvt("BAD_ENCODING");
    for (unsigned i = 0; i < argc && argv[i]; ++i) {
        auto tmp = (string) wcvt.to_bytes(argv[i]);
        out.push_back(move(tmp));
    }
    if (argc != out.size() || argv[argc]) {
        assert(!"Bad arguments, "
            "argc and null terminator don't agree.");
    }
    return out;
}

//===========================================================================
// static
vector<string> Cli::toArgv(size_t argc, const wchar_t * argv[]) {
    return toArgv(argc, (wchar_t **) argv);
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
// spec -- but ignoring parameter expansion ("$()" and "${}"), command
// substitution (back quote `), operators as separators, etc.
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
//   Must: | & ; < > ( ) $ ` \ " ' SP TAB CR LF FF VTAB
//   Should: * ? [ # ~ = %
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
// Arguments are split on whitespace (" \t\r\n\f\v") unless quoted or escaped
//  - backslashes: always escapes the following character.
//  - single quotes and double quotes: escape each other and whitespace.
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
*   Join argv to form a command line
*
***/

//===========================================================================
// static
string Cli::toCmdline(const vector<string> & args) {
    auto ptrs = toPtrArgv(args);
    return toCmdline(ptrs.size(), ptrs.data());
}

//===========================================================================
// static
string Cli::toCmdline(size_t argc, char * argv[]) {
#if defined(_WIN32)
    return toWindowsCmdline(argc, argv);
#else
    return toGnuCmdline(argc, argv);
#endif
}

//===========================================================================
// static
string Cli::toCmdline(size_t argc, const char * argv[]) {
    return toCmdline(argc, (char **) argv);
}

//===========================================================================
// static
string Cli::toCmdline(size_t argc, wchar_t * argv[]) {
    return toCmdline(toArgv(argc, argv));
}

//===========================================================================
// static
string Cli::toCmdline(size_t argc, const wchar_t * argv[]) {
    return toCmdline(argc, (wchar_t **) argv);
}

//===========================================================================
// static
string Cli::toGlibCmdline(size_t, char * argv[]) {
    string out;
    if (!*argv)
        return out;
    for (;;) {
        for (auto ptr = *argv; *ptr; ++ptr) {
            switch (*ptr) {
            // Must escape
            case '|': case '&': case ';': case '<': case '>': case '(':
            case ')': case '$': case '`': case '\\': case '"': case '\'':
            case ' ': case '\t': case '\r': case '\n': case '\f': case '\v':
            // Should escape
            case '*': case '?': case '[': case '#': case '~': case '=':
            case '%':
                out += '\\';
            }
            out += *ptr;
        }
        if (!*++argv)
            return out;
        out += ' ';
    }
}

//===========================================================================
// static
string Cli::toGnuCmdline(size_t, char * argv[]) {
    string out;
    if (!*argv)
        return out;
    for (;;) {
        for (auto ptr = *argv; *ptr; ++ptr) {
            switch (*ptr) {
            case ' ': case '\t': case '\r': case '\n': case '\f': case '\v':
            case '\\': case '\'': case '"':
                out += '\\';
            }
            out += *ptr;
        }
        if (!*++argv)
            return out;
        out += ' ';
    }
}

//===========================================================================
// static
string Cli::toWindowsCmdline(size_t, char * argv[]) {
    string out;
    if (!*argv)
        return out;

    for (;;) {
        auto base = out.size();
        size_t backslashes = 0;
        auto ptr = *argv;
        for (; *ptr; ++ptr) {
            switch (*ptr) {
            case '\\': backslashes += 1; break;
            case ' ':
            case '\t': goto QUOTE;
            case '"':
                out.append(backslashes + 1, '\\');
                backslashes = 0;
                break;
            default: backslashes = 0; break;
            }
            out += *ptr;
        }
        goto NEXT;

    QUOTE:
        backslashes = 0;
        out.insert(base, 1, '"');
        out += *ptr++;
        for (; *ptr; ++ptr) {
            switch (*ptr) {
            case '\\': backslashes += 1; break;
            case '"':
                out.append(backslashes + 1, '\\');
                backslashes = 0;
                break;
            default: backslashes = 0; break;
            }
            out += *ptr;
        }
        out += '"';

    NEXT:
        if (!*++argv)
            return out;
        out += ' ';
    }
}


/****************************************************************************
*
*   Response files
*
***/

#ifdef DIMCLI_LIB_FILESYSTEM

// forward declarations
static bool expandResponseFiles(
    Cli & cli,
    vector<string> & args,
    vector<string> & ancestors
);

//===========================================================================
// Returns false on error, if there was an error content will either be empty
// or - if there was a transcoding error - contain the original content.
static bool loadFileUtf8(string & content, const fs::path & fn) {
    content.clear();

    error_code ec;
    auto bytes = (size_t) fs::file_size(fn, ec);
    if (ec) {
        // A file system race is required for this to fail (fs::exist success
        // immediately followed by fs::file_size failure) and there's no
        // practical way to cause it in a test. So give up and exclude this
        // line from the test coverage report.
        return false; // LCOV_EXCL_LINE
    }

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
        auto base = reinterpret_cast<const wchar_t *>(content.data());
        auto tmp = (string) wcvt.to_bytes(
            base + 1,
            base + content.size() / sizeof *base
        );
        if (tmp.empty())
            return false;
        content = tmp;
    } else if (content.size() >= 3
        && content[0] == '\xef'
        && content[1] == '\xbb'
        && content[2] == '\xbf'
    ) {
        content.erase(0, 3);
    }
    return true;
}

//===========================================================================
static bool expandResponseFile(
    Cli & cli,
    vector<string> & args,
    size_t & pos,
    vector<string> & ancestors
) {
    string content;
    error_code ec;
    auto fn = args[pos].substr(1);
    auto cfn = ancestors.empty()
        ? (fs::path) fn
        : fs::path(ancestors.back()).parent_path() / fn;
    cfn = fs::canonical(cfn, ec);
    if (ec || !fs::exists(cfn)) {
        cli.badUsage("Invalid response file", fn);
        return false;
    }
    for (auto && a : ancestors) {
        if (a == cfn.string()) {
            cli.badUsage("Recursive response file", fn);
            return false;
        }
    }
    ancestors.push_back(cfn.string());
    if (!loadFileUtf8(content, cfn)) {
        string desc = content.empty() ? "Read error" : "Invalid encoding";
        cli.badUsage(desc, fn);
        return false;
    }
    auto rargs = cli.toArgv(content);
    if (!expandResponseFiles(cli, rargs, ancestors))
        return false;
    auto rargLen = rargs.size();
    replace(args, pos, 1, move(rargs));
    pos += rargLen - 1;
    ancestors.pop_back();
    return true;
}

//===========================================================================
// "ancestors" contains the set of response files these args came from,
// directly or indirectly, and is used to detect recursive response files.
static bool expandResponseFiles(
    Cli & cli,
    vector<string> & args,
    vector<string> & ancestors
) {
    for (size_t pos = 0; pos < args.size(); ++pos) {
        if (!args[pos].empty() && args[pos][0] == '@') {
            if (!expandResponseFile(cli, args, pos, ancestors))
                return false;
        }
    }
    return true;
}

#endif


/****************************************************************************
*
*   Parse command line
*
***/

//===========================================================================
// static
vector<pair<string, double>> Cli::siUnitMapping(
    const string & symbol,
    int flags
) {
    vector<pair<string, double>> units({
        {"ki", double(1ull << 10)},
        {"Mi", double(1ull << 20)},
        {"Gi", double(1ull << 30)},
        {"Ti", double(1ull << 40)},
        {"Pi", double(1ull << 50)},
    });
    if (flags & fUnitBinaryPrefix) {
        units.insert(units.end(), {
            {"k", double(1ull << 10)},
            {"M", double(1ull << 20)},
            {"G", double(1ull << 30)},
            {"T", double(1ull << 40)},
            {"P", double(1ull << 50)},
        });
    } else {
        units.insert(units.end(), {
            {"k", 1e+3},
            {"M", 1e+6},
            {"G", 1e+9},
            {"T", 1e+12},
            {"P", 1e+15},
        });
        if (~flags & fUnitInsensitive) {
            units.insert(units.end(), {
                {"m", 1e-3},
                {"u", 1e-6},
                {"n", 1e-9},
                {"p", 1e-12},
                {"f", 1e-15},
            });
        }
    }
    if (!symbol.empty()) {
        if (flags & fUnitRequire) {
            for (auto && kv : units)
                kv.first += symbol;
        } else {
            units.reserve(2 * units.size());
            for (auto i = units.size(); i-- > 0;) {
                auto & kv = units[i];
                units.push_back({kv.first + symbol, kv.second});
            }
        }
        units.push_back({symbol, 1});
    }
    return units;
}

//===========================================================================
Cli & Cli::resetValues() & {
    for (auto && opt : m_cfg->opts)
        opt->reset();
    m_cfg->parseExit = false;
    m_cfg->exitCode = kExitOk;
    m_cfg->errMsg.clear();
    m_cfg->errDetail.clear();
    m_cfg->progName.clear();
    m_cfg->command.clear();
    m_cfg->unknownArgs.clear();
    return *this;
}

//===========================================================================
Cli && Cli::resetValues() && {
    return move(resetValues());
}

//===========================================================================
void Cli::prompt(OptBase & opt, const string & msg, int flags) {
    if (!opt.from().empty())
        return;
    auto & is = conin();
    auto & os = conout();
    if (msg.empty()) {
        os << opt.defaultPrompt();
    } else {
        os << msg;
    }
    bool defAdded = false;
    if (~flags & fPromptNoDefault) {
        if (opt.m_bool) {
            defAdded = true;
            bool def = false;
            if (!opt.m_flagValue) {
                auto & bopt = static_cast<Opt<bool>&>(opt);
                def = bopt.defaultValue();
            }
            os << (def ? " [Y/n]:" : " [y/N]:");
        } else {
            string tmp;
            if (opt.defaultValueToString(tmp) && !tmp.empty()) {
                defAdded = true;
                os << " [" << tmp << "]:";
            }
        }
    }
    if (!defAdded && msg.empty())
        os << ':';
    os << ' ';
    if (flags & fPromptHide)
        consoleEnableEcho(false); // Disable if hide, must be re-enabled.
    string val;
    os.flush();
    getline(is, val);
    if (flags & fPromptHide) {
        os << endl;
        if (~flags & fPromptConfirm)
            consoleEnableEcho(true); // Re-enable when hide and !confirm.
    }
    if (flags & fPromptConfirm) {
        string again;
        os << "Enter again to confirm: " << flush;
        getline(is, again);
        if (flags & fPromptHide) {
            os << endl;
            consoleEnableEcho(true); // Re-enable when hide and confirm.
        }
        if (val != again) {
            badUsage("Confirm failed, entries not the same.");
            return;
        }
    }
    if (opt.m_bool) {
        // Honor the contract that bool parse functions are only presented
        // with either "0" or "1".
        val = val.size() && (val[0] == 'y' || val[0] == 'Y') ? "1" : "0";
    }
    (void) parseValue(opt, opt.defaultFrom(), 0, val.c_str());
}

//===========================================================================
bool Cli::parseValue(
    OptBase & opt,
    const string & name,
    size_t pos,
    const char ptr[]
) {
    if (!opt.assign(name, pos)) {
        string prefix = "Too many '" + name + "' values";
        string detail = "The maximum number of values is "
            + intToString(opt, opt.maxSize()) + ".";
        badUsage(prefix, ptr, detail);
        return false;
    }
    string val;
    if (ptr) {
        val = ptr;
        opt.doParseAction(*this, val);
        if (parseExited())
            return false;
    } else {
        opt.assignImplicit();
    }
    opt.doCheckActions(*this, val);
    return !parseExited();
}

//===========================================================================
void Cli::badUsage(
    const string & prefix,
    const string & value,
    const string & detail
) {
    string out;
    auto & cmd = commandMatched();
    if (cmd.size())
        out = "Command '" + cmd + "': ";
    out += prefix;
    if (!value.empty()) {
        out += ": ";
        out += value;
    }
    fail(kExitUsage, out, detail);
    m_cfg->parseExit = true;
}

//===========================================================================
void Cli::badUsage(
    const OptBase & opt,
    const string & value,
    const string & detail
) {
    string prefix = "Invalid '" + opt.from() + "' value";
    return badUsage(prefix, value, detail);
}

//===========================================================================
void Cli::parseExit() {
    m_cfg->parseExit = true;
    m_cfg->exitCode = kExitOk;
    m_cfg->errMsg.clear();
    m_cfg->errDetail.clear();
}

//===========================================================================
void Cli::fail(int code, const string & msg, const string & detail) {
    m_cfg->parseExit = false;
    m_cfg->exitCode = code;
    m_cfg->errMsg = format(*m_cfg, msg);
    m_cfg->errDetail = format(*m_cfg, detail);
}

//===========================================================================
static bool parseBool(bool & out, const string & val) {
    static const unordered_map<string, bool> allowed = {
        { "1", true },
        { "t", true },
        { "y", true },
        { "+", true },
        { "true", true },
        { "yes", true },
        { "on", true },
        { "enable", true },

        { "0", false },
        { "f", false },
        { "n", false },
        { "-", false },
        { "false", false },
        { "no", false },
        { "off", false },
        { "disable", false },
    };

    string tmp = val;
    auto & f = use_facet<ctype<char>>(locale());
    f.tolower(tmp.data(), tmp.data() + tmp.size());
    auto i = allowed.find(tmp);
    if (i == allowed.end()) {
        out = false;
        return false;
    }
    out = i->second;
    return true;
}

enum class OprCat {
    kMinReq,    // Minimum expected for all required opts (vectors may be >1)
    kReq,       // Max expected for all required opts
    kOpt,       // All optionals
};

//===========================================================================
static int numMatches(
    OprCat cat,
    int avail,
    const OptName & optn
) {
    bool op = optn.optional;
    auto minVec = optn.opt->minSize();
    auto maxVec = optn.opt->maxSize();
    bool vec = minVec != 1 || maxVec != 1;

    if (cat == OprCat::kMinReq && !op && vec && avail >= minVec) {
        // Min required with required vector not requiring too many arguments.
        return minVec;
    }
    if (cat == OprCat::kReq && !op && vec) {
        // Max required with required vector.
        return maxVec == -1 ? avail : min(avail, maxVec - minVec);
    }
    if (cat == OprCat::kOpt && op && vec && avail >= minVec) {
        // All optionals with optional vector not requiring too many arguments.
        return maxVec == -1 ? avail : min(avail, maxVec);
    }
    if (cat == OprCat::kMinReq && !op && !vec
        || cat == OprCat::kOpt && op && !vec
    ) {
        // Min required with required non-vector; or
        // all optional with optional non-vector.
        return min(avail, 1);
    }

    // Fall through for unmatched combinations.
    return 0;
}

//===========================================================================
static bool assignOperands(
    RawValue * rawValues,
    size_t numRawValues,
    Cli & cli,
    const Cli::OptIndex & ndx,
    int numPos
) {
    // Assign positional values to operands. There must be enough values for
    // all opts of a category for any of the next category to be eligible.
    vector<int> matched(ndx.m_argNames.size());
    int usedPos = 0;

    for (auto&& cat : { OprCat::kMinReq, OprCat::kReq, OprCat::kOpt }) {
        for (unsigned i = 0; i < matched.size() && usedPos < numPos; ++i) {
            auto & argName = ndx.m_argNames[i];
            int num = numMatches(cat, numPos - usedPos, argName);
            matched[i] += num;
            usedPos += num;
        }
    }

    if (usedPos < numPos) {
        cli.badUsage("Unexpected argument", rawValues[usedPos].ptr);
        return false;
    }
    assert(usedPos == numPos // LCOV_EXCL_LINE
        && "Internal dimcli error: not all operands mapped to variables.");

    int ipos = 0;       // Operand being matched.
    int imatch = 0;     // Values already been matched to this opt.
    for (auto val = rawValues; val < rawValues + numRawValues; ++val) {
        if (val->opt || val->type != RawValue::kOperand)
            continue;
        if (matched[ipos] <= imatch) {
            imatch = 0;
            for (;;) {
                ipos += 1;
                if (matched[ipos])
                    break;
            }
        }
        auto & argName = ndx.m_argNames[ipos];
        val->opt = argName.opt;
        val->name = argName.name;
        imatch += 1;
    }
    return true;
}

//===========================================================================
static bool badMinMatched(
    Cli & cli,
    const Cli::OptBase & opt,
    const string & name = {}
) {
    int min = opt.minSize();
    int max = opt.maxSize();
    string detail;
    if (min != 1 && min == max) {
        detail = "Must have " + intToString(opt, min)
            + " values.";
    } else if (max == -1) {
        detail = "Must have " + intToString(opt, min)
            + " or more values.";
    } else if (min != max) {
        detail = "Must have " + intToString(opt, min)
            + " to " + intToString(opt, max) + " values.";
    }
    cli.badUsage(
        "Option '" + (name.empty() ? opt.from() : name) + "' missing value.",
        {},
        detail
    );
    return false;
}

//===========================================================================
bool Cli::parse(vector<string> & args) {
    // The 0th (name of this program) opt must always be present.
    assert(!args.empty()
        && "At least one argument (the program name) required.");

    Config::touchAllCmds(*this);
    OptIndex ndx;
    ndx.index(*this, "", false);
    enum {
        kNone,
        kPending,
        kFound,
        kUnknown
    } cmdMode = m_cfg->allowUnknown || m_cfg->cmds.size() > 1
        ? kPending
        : kNone;

    // Commands can't be added when the top level has an operand that requires
    // look ahead to match, command processing requires that the command be
    // unambiguously identifiable.
    assert((ndx.m_allowCommands || !cmdMode)
        && "Mixing top level operands with commands.");

    resetValues();

#if !defined(DIMCLI_LIB_NO_ENV)
    // Insert environment options
    if (m_cfg->envOpts.size()) {
        if (auto val = getenv(m_cfg->envOpts.c_str()))
            replace(args, 1, 0, toArgv(val));
    }
#endif

    // Expand response files
#ifdef DIMCLI_LIB_FILESYSTEM
    if (m_cfg->responseFiles) {
        vector<string> ancestors;
        if (!expandResponseFiles(*this, args, ancestors))
            return false;
    }
#endif

    // Before actions
    for (auto && fn : m_cfg->befores) {
        fn(*this, args);
        if (parseExited())
            return false;
    }

    // Extract raw values and assign non-positional values to opts.
    vector<RawValue> rawValues;

    auto arg = args.data();
    auto argc = args.size();

    string name;
    bool moreOpts = true;
    int numPos = 0;
    size_t precmdValues = 0;
    m_cfg->progName = *arg;
    size_t argPos = 1;
    arg += 1;

    for (; argPos < argc; ++argPos, ++arg) {
        OptName argName;
        const char * equal = nullptr;
        auto ptr = arg->c_str();
        if (*ptr == '-' && ptr[1] && moreOpts) {
            ptr += 1;
            for (; *ptr && *ptr != '-'; ++ptr) {
                auto it = ndx.m_shortNames.find(*ptr);
                name = '-';
                name += *ptr;
                if (it == ndx.m_shortNames.end()) {
                    badUsage("Unknown option", name);
                    return false;
                }
                argName = it->second;
                if (argName.opt->m_finalOpt)
                    moreOpts = false;
                if (argName.opt->m_bool) {
                    rawValues.emplace_back(
                        RawValue::kOption,
                        argName.opt,
                        name,
                        argPos,
                        argName.invert ? "0" : "1"
                    );
                    continue;
                }
                ptr += 1;
                goto OPTION_VALUE;
            }
            if (!*ptr)
                continue;

            ptr += 1;
            if (!*ptr) {
                // Bare "--" found, all remaining args are operands.
                moreOpts = false;
                continue;
            }
            string key;
            equal = strchr(ptr, '=');
            if (equal) {
                key.assign(ptr, equal);
                ptr = equal + 1;
            } else {
                key = ptr;
                ptr = "";
            }
            auto it = ndx.m_longNames.find(key);
            name = "--";
            name += key;
            if (it == ndx.m_longNames.end()) {
                badUsage("Unknown option", name);
                return false;
            }
            argName = it->second;
            if (argName.opt->m_finalOpt)
                moreOpts = false;
            if (argName.opt->m_bool) {
                auto val = true;
                if (equal
                    && (argName.opt->m_flagValue || !parseBool(val, ptr))
                ) {
                    badUsage("Invalid '" + name + "' value", ptr);
                    return false;
                }
                rawValues.emplace_back(
                    RawValue::kOption,
                    argName.opt,
                    name,
                    argPos,
                    argName.invert == val ? "0" : "1"
                );
                continue;
            }
            goto OPTION_VALUE;
        }

        // Positional value
        if (cmdMode == kPending && numPos == ndx.m_minOprs) {
            auto cmd = (string) ptr;
            bool noExtras = assignOperands(
                rawValues.data(),
                rawValues.size(),
                *this,
                ndx,
                numPos
            );
            // Number of assigned operands should always exactly match the
            // count, since it's equal to the calculated minimum.
            assert(noExtras // LCOV_EXCL_LINE
                && "Internal dimcli error: operand count mismatch.");
            noExtras = true;

            rawValues.emplace_back(RawValue::kCommand, nullptr, cmd);
            precmdValues = rawValues.size();
            numPos = 0;

            if (commandExists(cmd)) {
                cmdMode = kFound;
                ndx.index(*this, cmd, false);
            } else if (m_cfg->allowUnknown) {
                cmdMode = kUnknown;
                moreOpts = false;
            } else {
                badUsage("Unknown command", cmd);
                return false;
            }
            m_cfg->command = cmd;
            continue;
        }
        if (cmdMode == kUnknown) {
            m_cfg->unknownArgs.push_back(ptr);
            continue;
        }

        if (numPos == ndx.m_finalOpr)
            moreOpts = false;

        rawValues.emplace_back(
            RawValue::kOperand,
            nullptr,
            string{},
            argPos,
            ptr
        );

        numPos += 1;
        continue;

    OPTION_VALUE:
        if (*ptr || equal) {
            rawValues.emplace_back(
                RawValue::kOption,
                argName.opt,
                name,
                argPos,
                ptr
            );
            continue;
        }
        if (argName.optional) {
            rawValues.emplace_back(
                RawValue::kOption,
                argName.opt,
                name,
                argPos,
                nullptr
            );
            continue;
        }
        argPos += 1;
        arg += 1;
        if (argPos == argc) {
            badUsage("No value given for " + name);
            return false;
        }
        rawValues.emplace_back(
            RawValue::kOption,
            argName.opt,
            name,
            argPos,
            arg->c_str()
        );
    }

    if (cmdMode != kUnknown) {
        if (!assignOperands(
            rawValues.data() + precmdValues,
            rawValues.size() - precmdValues,
            *this,
            ndx,
            numPos
        )) {
            return false;
        }
    }

    // Parse values and assign them to arguments.
    m_cfg->command = "";
    for (auto&& val : rawValues) {
        switch (val.type) {
        case RawValue::kCommand:
            m_cfg->command = val.name;
            continue;
        default:
            break;
        }
        if (!parseValue(*val.opt, val.name, val.pos, val.ptr))
            return false;
    }

    // Report options with too few values.
    for (auto&& argName : ndx.m_argNames) {
        auto & opt = *argName.opt;
        if (!argName.optional) {
            // Report required operands that are missing.
            if (!opt || opt.size() < (size_t) opt.minSize())
                return badMinMatched(*this, opt, argName.name);
        }
    }
    for (auto&& nv : ndx.m_shortNames) {
        auto & opt = *nv.second.opt;
        if (!nv.second.pos && opt && opt.size() < (size_t) opt.minSize())
            return badMinMatched(*this, opt);
    }
    for (auto && nv : ndx.m_longNames) {
        auto & opt = *nv.second.opt;
        if (!nv.second.pos && opt && opt.size() < (size_t) opt.minSize())
            return badMinMatched(*this, opt);
    }

    // After actions
    for (auto && opt : m_cfg->opts) {
        if (!opt->m_command.empty() && opt->m_command != commandMatched())
            continue;
        opt->doAfterActions(*this);
        if (parseExited())
            return false;
    }

    return true;
}

//===========================================================================
bool Cli::parse(vector<string> && args) {
    return parse(args);
}

//===========================================================================
bool Cli::parse(size_t argc, char * argv[]) {
    auto args = toArgv(argc, argv);
    return parse(move(args));
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
const string & Cli::commandMatched() const {
    return m_cfg->command;
}

//===========================================================================
const vector<string> & Cli::unknownArgs() const {
    return m_cfg->unknownArgs;
}

//===========================================================================
bool Cli::exec() {
    auto & name = commandMatched();
    auto cmdFn = commandExists(name)
        ? m_cfg->cmds[name].action
        : m_cfg->unknownCmd;

    if (cmdFn) {
        fail(kExitOk, {});
        cmdFn(*this);
        return !parseExited();
    } else {
        // Most likely parse failed, was never run, or "this" was reset.
        assert(!"Command found by parse not defined.");
        fail(
            kExitSoftware,
            "Command '" + name + "' found by parse not defined."
        );
        return false;
    }
}

//===========================================================================
bool Cli::exec(size_t argc, char * argv[]) {
    return parse(argc, argv) && exec();
}

//===========================================================================
bool Cli::exec(vector<string> & args) {
    return parse(args) && exec();
}

//===========================================================================
bool Cli::commandExists(const string & name) const {
    auto & cmds = m_cfg->cmds;
    return cmds.find(name) != cmds.end();
}


/****************************************************************************
*
*   Help Text
*
***/

namespace {

struct ChoiceKey {
    size_t pos;
    const char * key;
    const char * desc;
    const char * sortKey;
    bool def;
};

struct RawCol {
    int indent {};
    int childIndent {};
    const char * text {};
    int textLen {};
    int width {-1};         // chars
    float minWidth {-1};    // percentage
    float maxWidth {-1};    // percentage
};

struct RawLine {
    bool newTable {};
    vector<RawCol> cols;
};

} // namespace

//===========================================================================
static size_t parseLine(RawLine * out, const char line[]) {
    *out = {};
    auto ptr = line;
    for (;;) {
        out->cols.emplace_back();
        auto & col = out->cols.back();
        for (;; ++ptr) {
            if (*ptr == ' ') {
                col.indent += 1;
            } else if (*ptr == '\a') {
                char * eptr;
                col.minWidth = strtof(ptr + 1, &eptr);
                col.maxWidth = strtof(eptr, &eptr);
                if (*eptr == '\a'
                    && col.minWidth <= col.maxWidth
                    && col.minWidth >= 0 && col.minWidth <= 100
                ) {
                    col.maxWidth = min(col.maxWidth, 100.0f);
                    ptr = eptr;
                } else {
                    col.minWidth = -1;
                    col.maxWidth = -1;
                    col.text = ptr;
                    break;
                }
            } else if (*ptr == '\v') {
                col.childIndent += 1;
            } else if (*ptr == '\r') {
                col.childIndent -= 1;
            } else if (*ptr == '\f') {
                out->newTable = true;
            } else {
                col.text = ptr;
                break;
            }
        }

        // Don't allow unindent to bleed into previous column.
        col.childIndent = max(col.childIndent, -col.indent);

        // Width limits can only be set when a table is started, this prevents
        // multiply defined column rules.
        if (!out->newTable) {
            col.minWidth = -1;
            col.maxWidth = -1;
        }

        col.textLen = 0;
        for (;;) {
            char ch = *ptr++;
            if (!ch) {
                return ptr - line - 1;
            } else if (ch == '\n') {
                return ptr - line;
            } else if (ch == '\t') {
                break;
            } else if (ch == ' ' || ch == '\r') {
                // skip
            } else {
                col.textLen = int(ptr - col.text);
            }
        }
    }
}

//===========================================================================
static int wrapIndent(int indent, size_t width) {
    if (indent >= (int) width) {
        return (indent - 2) % ((int) width - 2) + 2;
    } else {
        return indent;
    }
}

//===========================================================================
// Appends formatted text to *outPtr, adds the number of formatted lines to
// *lines, and returns the new output column position.
static size_t formatCol(
    string * outPtr,
    int * lines,
    const RawCol & col,
    size_t startPos,
    size_t pos,
    size_t lineWidth
) {
    auto & out = *outPtr;
    auto width = (col.width == -1) ? lineWidth : col.width;
    assert(width // LCOV_EXCL_LINE
        && "Internal dimcli error: unknown column width.");

    if (startPos && col.textLen) {
        if (pos + 1 < startPos) {
            out.append(startPos - pos, ' ');
            pos = startPos;
        } else if (pos < startPos + 3) {
            out += "  ";
            pos += 2;
        } else {
            pos = lineWidth;
        }
    }

    bool firstWord = true;
    auto indent = wrapIndent(col.indent, width);
    auto childIndent = startPos
        + wrapIndent(col.indent + col.childIndent, width);
    out.append(indent, ' ');
    pos += indent;
    for (auto ptr = col.text, eptr = ptr + col.textLen; ptr != eptr;) {
        if (*ptr == ' ') {
            ptr += 1;
            continue;
        }
        auto eword = (const char *) memchr(ptr, ' ', eptr - ptr);
        if (!eword)
            eword = eptr;
        size_t wordLen = eword - ptr;
        if (pos + wordLen + 1 > lineWidth
            && pos > wordLen
        ) {
            firstWord = true;
            *lines += 1;
            out += '\n';
            out.append(childIndent, ' ');
            pos = childIndent;
        }
        if (firstWord) {
            firstWord = false;
        } else {
            out += ' ';
            pos += 1;
        }
        for (; ptr != eword; ++ptr) {
            if (*ptr == '\b') {
                out += ' ';
            } else {
                out += *ptr;
            }
        }
        pos += wordLen;
        ptr = eword;
    }
    return pos;
}

//===========================================================================
// Returns number of output lines after formatting.
//
// Special characters:
//  \s  soft word break
//  \b  non-breaking space
//  \t  transitions from key to description column
//  \v  increase indentation after line wrap by one
//  \r  reduced indentation after line wrap by one
//  \f  line starts a new table, not extending current table at this indent
static int formatLine(
    string * outPtr,
    const RawLine & raw,
    size_t lineWidth
) {
    auto & out = *outPtr;
    if (raw.cols.size() == 1 && !raw.cols[0].textLen)
        return 0;

    auto lines = 0;
    size_t pos = 0;
    size_t startPos = 0;
    for (auto&& col : raw.cols) {
        pos = formatCol(&out, &lines, col, startPos, pos, lineWidth);
        startPos += col.width;
    }
    return lines;
}

//===========================================================================
// Returns formatted text and adds lines generated to *lines.
static string format(const Cli::Config & cfg, const string & text) {
    vector<RawLine> raws;
    for (auto ptr = text.c_str(); *ptr;) {
        raws.emplace_back();
        auto & raw = raws.back();
        ptr += parseLine(&raw, ptr);
    }

    struct TableInfo {
        vector<int> width;
        vector<int> rows;

        void apply(vector<RawLine> * raws) {
            for (auto&& line : this->rows) {
                auto & cols = (*raws)[line].cols;
                for (unsigned i = 0; i < cols.size(); ++i) {
                    if (this->width[i])
                        cols[i].width = this->width[i];
                }
            }
            this->width.clear();
            this->rows.clear();
        }
    };
    unordered_map<int, TableInfo> tables;
    for (unsigned i = 0; i < raws.size(); ++i) {
        auto & raw = raws[i];
        auto & cols = raw.cols;
        if (cols.size() == 1)
            continue;
        auto & tab = tables[cols[0].indent];
        if (raw.newTable)
            tab.apply(&raws);
        tab.rows.push_back(i);
        if (cols.size() > tab.width.size())
            tab.width.resize(cols.size());
        auto & tcols = raws[tab.rows[0]].cols;
        for (unsigned icol = 0; icol < cols.size(); ++icol) {
            auto & tcol = tcols[icol];
            if (tcol.maxWidth == -1) {
                tcol.minWidth = cfg.minKeyWidth;
                tcol.maxWidth = icol ? cfg.minKeyWidth : cfg.maxKeyWidth;
            }
            auto & col = cols[icol];
            int width = col.indent + col.textLen + 2;
            int minWidth = (int) round(tcol.minWidth * cfg.maxLineWidth / 100);
            int maxWidth = (int) round(tcol.maxWidth * cfg.maxLineWidth / 100);
            if (width < minWidth || width > maxWidth + 2)
                width = minWidth;
            if (width > tab.width[icol])
                tab.width[icol] = min(width, maxWidth);
        }
    }
    for (auto&& kv : tables)
        kv.second.apply(&raws);

    int cnt = 0;
    string out;
    if (auto num = (int) raws.size()) {
        cnt += num - 1;
        cnt += formatLine(&out, raws[0], cfg.maxLineWidth);
        for (auto i = 1; i < num; ++i) {
            out += '\n';
            cnt += formatLine(&out, raws[i], cfg.maxLineWidth);
        }
    }
    return out;
}

//===========================================================================
// static
string Cli::OptIndex::desc(
    const Cli::OptBase & opt,
    bool withMarkup
) {
    string suffix;
    if (!withMarkup) {
        // Raw description without any markup.
    } else if (!opt.m_choiceDescs.empty()) {
        // "default" tag is added to individual choices later.
    } else if (opt.m_flagValue && opt.m_flagDefault) {
        suffix += "(default)";
    } else if (opt.m_vector) {
        auto minVec = opt.minSize();
        auto maxVec = opt.maxSize();
        if (minVec != 1 || maxVec != -1) {
            suffix += "(limit: " + intToString(opt, minVec);
            if (maxVec == -1) {
                suffix += "+";
            } else if (minVec != maxVec) {
                suffix += " to " + intToString(opt, maxVec);
            }
            suffix += ")";
        }
    } else if (!opt.m_bool) {
        string tmp;
        if (opt.m_defaultDesc.empty()) {
            if (!opt.defaultValueToString(tmp))
                tmp.clear();
        } else {
            tmp = opt.m_defaultDesc;
            if (!tmp[0])
                tmp.clear();
        }
        if (!tmp.empty())
            suffix += "(default: " + tmp + ")";
    }
    if (suffix.empty()) {
        return opt.m_desc;
    } else if (opt.m_desc.empty()) {
        return suffix;
    } else {
        return opt.m_desc + ' ' + suffix;
    }
}

//===========================================================================
// static
const unordered_map<string, Cli::OptBase::ChoiceDesc> &
Cli::OptIndex::choiceDescs(const OptBase & opt) {
    return opt.m_choiceDescs;
}

//===========================================================================
static vector<ChoiceKey> getChoiceKeys(
    const unordered_map<string, Cli::OptBase::ChoiceDesc> & choices
) {
    vector<ChoiceKey> out;
    for (auto && cd : choices) {
        ChoiceKey key;
        key.pos = cd.second.pos;
        key.key = cd.first.c_str();
        key.desc = cd.second.desc.c_str();
        key.sortKey = cd.second.sortKey.c_str();
        key.def = cd.second.def;
        out.push_back(key);
    }
    sort(out.begin(), out.end(), [](auto & a, auto & b) {
        if (int rc = strcmp(a.sortKey, b.sortKey))
            return rc < 0;
        return a.pos < b.pos;
    });
    return out;
}

//===========================================================================
static void writeNbsp(string * out, const string & str) {
    for (auto ch : str)
        out->append(1, (ch == ' ') ? '\b' : ch);
}

//===========================================================================
static void writeChoices(
    string * outPtr,
    const unordered_map<string, Cli::OptBase::ChoiceDesc> & choices
) {
    auto & out = *outPtr;
    if (choices.empty())
        return;
    auto keys = getChoiceKeys(choices);
    const size_t indent = 6;

    string desc;
    string prefix(indent, ' ');
    for (auto && k : keys) {
        out += prefix;
        writeNbsp(&out, k.key);
        out += '\t';
        out += k.desc;
        if (k.def)
            out += " (default)";
        out += '\n';
    }
}

//===========================================================================
static void writeChoicesDetail(
    string * outPtr,
    const unordered_map<string, Cli::OptBase::ChoiceDesc> & choices
) {
    auto & out = *outPtr;
    if (choices.empty())
        return;
    out += "Must be";
    auto keys = getChoiceKeys(choices);

    string val;
    size_t pos = 0;
    auto num = keys.size();
    for (auto i = keys.begin(); pos < num; ++pos, ++i) {
        out += " '";
        writeNbsp(&out, i->key);
        out += "'";
        if (pos == 0 && num == 2) {
            out += " or";
        } else if (pos + 1 == num) {
            out += '.';
        } else {
            out += ',';
            if (pos + 2 == num)
                out += " or";
        }
    }
}

//===========================================================================
static void writeUsage(
    string * outPtr,
    Cli & cli,
    const string & arg0,
    const string & cmdName,
    bool expandedOptions
) {
    auto & out = *outPtr;
    auto & cfg = Cli::Config::get(cli);
    Cli::OptIndex ndx;
    ndx.index(cli, cmdName, true);
    auto prog = displayName(arg0.empty() ? cli.progName() : arg0);
    auto prefix = "Usage: " + prog;
    out.append(prefix.size() + 1, '\v');
    out += prefix;
    if (cmdName.size()) {
        out += ' ';
        out += cmdName;
    }
    if (!ndx.m_shortNames.empty() || !ndx.m_longNames.empty()) {
        if (!expandedOptions) {
            out += " [OPTIONS]";
        } else {
            auto & cmd = Cli::Config::findCmdAlways(cli, cmdName);
            auto namedOpts = ndx.findNamedOpts(
                cli,
                cmd,
                kNameNonDefault,
                true
            );
            for (auto && key : namedOpts) {
                out += ' ';
                writeNbsp(&out, "[" + key.list + "]");
            }
        }
    }
    if (cmdName.empty() && cfg.cmds.size() > 1) {
        out += " COMMAND [ARGS...]";
    } else if (cmdName.empty() && cfg.allowUnknown) {
        out += " [COMMAND] [ARGS...]";
    } else if (!cli.commandExists(cmdName)) {
        out += " [ARGS...]";
    } else {
        for (auto && pa : ndx.m_argNames) {
            out += ' ';
            string token = pa.name.find(' ') == string::npos
                ? pa.name
                : "<" + pa.name + ">";
            if (pa.opt->maxSize() < 0 || pa.opt->maxSize() > 1)
                token += "...";
            if (pa.optional) {
                writeNbsp(&out, "[" + token + "]");
            } else {
                writeNbsp(&out, token);
            }
        }
    }
    out += '\n';
}

//===========================================================================
static void writeCommands(string * outPtr, Cli & cli) {
    auto & out = *outPtr;
    auto & cfg = Cli::Config::get(cli);
    Cli::Config::touchAllCmds(cli);

    struct CmdKey {
        const char * name;
        const CommandConfig * cmd;
        const GroupConfig * grp;
    };
    vector<CmdKey> keys;
    for (auto && cmd : cfg.cmds) {
        if (cmd.first.size()) {
            CmdKey key = {
                cmd.first.c_str(),
                &cmd.second,
                &cfg.cmdGroups[cmd.second.cmdGroup]
            };
            keys.push_back(key);
        }
    }
    if (keys.empty())
        return;
    sort(keys.begin(), keys.end(), [](auto & a, auto & b) {
        if (int rc = a.grp->sortKey.compare(b.grp->sortKey))
            return rc < 0;
        return strcmp(a.name, b.name) < 0;
    });

    const char * gname = nullptr;
    for (auto && key : keys) {
        string indent = "  \v\v";
        if (!gname || key.grp->name != gname) {
            if (!gname)
                indent += '\f';
            gname = key.grp->name.c_str();
            out += '\n';
            auto title = key.grp->title;
            if (title.empty()
                && strcmp(gname, kInternalOptionGroup) == 0
                && &key == keys.data()
            ) {
                // First group and it's the internal group, give it a title.
                title = "Commands";
            }
            if (!title.empty()) {
                out += title;
                out += ":\n";
            }
        }

        out += indent;
        writeNbsp(&out, key.name);
        auto desc = key.cmd->desc;
        size_t pos = 0;
        for (;;) {
            pos = desc.find_first_of(".!?", pos);
            if (pos == string::npos)
                break;
            pos += 1;
            if (desc[pos] == ' ') {
                desc.resize(pos);
                break;
            }
        }
        desc = trim(desc);
        if (!desc.empty()) {
            out += '\t';
            out += desc;
        }
        out += '\n';
    }
}

//===========================================================================
static void writeOperands(string * outPtr, Cli & cli, const string & cmd) {
    auto & out = *outPtr;
    Cli::OptIndex ndx;
    ndx.index(cli, cmd, true);
    bool hasDesc = false;
    for (auto && pa : ndx.m_argNames) {
        if (!ndx.desc(*pa.opt, false).empty()) {
            hasDesc = true;
            break;
        }
    }
    if (!hasDesc)
        return;

    out += '\f';
    for (auto && pa : ndx.m_argNames) {
        out += "  \v\v";
        writeNbsp(&out, pa.name);
        out += '\t';
        out += ndx.desc(*pa.opt);
        out += '\n';
        writeChoices(&out, ndx.choiceDescs(*pa.opt));
    }
}

//===========================================================================
static void writeOptions(string * outPtr, Cli & cli, const string & cmdName) {
    auto & out = *outPtr;
    Cli::OptIndex ndx;
    ndx.index(cli, cmdName, true);
    auto & cmd = Cli::Config::findCmdAlways(cli, cmdName);

    // Find named args and the longest name list.
    auto namedOpts = ndx.findNamedOpts(cli, cmd, kNameAll, false);
    if (namedOpts.empty())
        return;

    const char * gname = nullptr;
    for (auto && key : namedOpts) {
        string indent = "  \v\v";
        if (!gname || key.opt->group() != gname) {
            if (!gname)
                indent += '\f';
            gname = key.opt->group().c_str();
            out += '\n';
            auto & grp = Cli::Config::findGrpAlways(cmd, key.opt->group());
            auto title = grp.title;
            if (title.empty()
                && strcmp(gname, kInternalOptionGroup) == 0
                && &key == namedOpts.data()
            ) {
                // First group and it's the internal group, give it a title
                // so it's not just left hanging.
                title = "Options";
            }
            if (!title.empty()) {
                out += title;
                out += ":\n";
            }
        }
        out += indent;
        out += key.list;
        out += '\t';
        out += ndx.desc(*key.opt);
        out += '\n';
        writeChoices(&out, ndx.choiceDescs(*key.opt));
    }
}

//===========================================================================
static void writeHelp(
    string * outPtr,
    Cli & cli,
    const string & progName,
    const string & cmdName
) {
    auto & out = *outPtr;
    if (!cli.commandExists(cmdName))
        return writeUsage(&out, cli, progName, cmdName, false);

    auto & cmd = Cli::Config::findCmdAlways(cli, cmdName);
    auto & top = Cli::Config::findCmdAlways(cli, "");
    auto & hdr = cmd.header.empty() ? top.header : cmd.header;
    if (*hdr.data()) {
        out += hdr;
        out += '\n';
    }
    writeUsage(&out, cli, progName, cmdName, false);
    if (!cmd.desc.empty()) {
        out.append(1, '\n').append(cmd.desc).append(1, '\n');
    }
    if (cmdName.empty())
        writeCommands(&out, cli);
    writeOperands(&out, cli, cmdName);
    writeOptions(&out, cli, cmdName);
    auto & ftr = cmd.footer.empty() ? top.footer : cmd.footer;
    if (*ftr.data()) {
        out += '\n';
        out += ftr;
    }
}

//===========================================================================
int Cli::printHelp(
    ostream & os,
    const string & progName,
    const string & cmdName
) {
    string raw;
    writeHelp(&raw, *this, progName, cmdName);
    os << format(*m_cfg, raw) << '\n';
    return exitCode();
}

//===========================================================================
int Cli::printUsage(
    ostream & os,
    const string & arg0,
    const string & cmd
) {
    string raw;
    writeUsage(&raw, *this, arg0, cmd, false);
    os << format(*m_cfg, raw) << '\n';
    return exitCode();
}

//===========================================================================
int Cli::printUsageEx(
    ostream & os,
    const string & arg0,
    const string & cmd
) {
    string raw;
    writeUsage(&raw, *this, arg0, cmd, true);
    os << format(*m_cfg, raw) << '\n';
    return exitCode();
}

//===========================================================================
void Cli::printOperands(ostream & os, const string & cmd) {
    string raw;
    writeOperands(&raw, *this, cmd);
    os << format(*m_cfg, raw);
}

//===========================================================================
void Cli::printOptions(ostream & os, const string & cmd) {
    string raw;
    writeOptions(&raw, *this, cmd);
    os << format(*m_cfg, raw);
}

//===========================================================================
void Cli::printCommands(ostream & os) {
    string raw;
    writeCommands(&raw, *this);
    os << format(*m_cfg, raw);
}

//===========================================================================
void Cli::printText(ostream & os, const string & text) {
    os << format(*m_cfg, text);
}

//===========================================================================
int Cli::printError(ostream & os) {
    auto code = exitCode();
    if (code) {
        os << "Error: " << errMsg() << endl;
        auto & detail = errDetail();
        if (detail.size())
            os << detail << endl;
    }
    return code;
}


/****************************************************************************
*
*   Native console API
*
***/

#if defined(DIMCLI_LIB_NO_CONSOLE)

//===========================================================================
void Cli::consoleEnableEcho(bool enable) {
    assert(enable && "Disabling echo requires console support enabled.");
}

//===========================================================================
unsigned Cli::consoleWidth(bool /* queryWidth */) {
    return kDefaultConsoleWidth;
}

#elif defined(_WIN32)

#pragma pack(push)
#pragma pack()
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define UNICODE
#include <Windows.h>
#pragma pack(pop)

//===========================================================================
bool Cli::consoleEnableEcho(bool enable) {
    auto hInput = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode = 0;
    GetConsoleMode(hInput, &mode);
    if (enable) {
        mode |= ENABLE_ECHO_INPUT;
    } else {
        mode &= ~ENABLE_ECHO_INPUT;
    }
    return SetConsoleMode(hInput, mode) == TRUE;
}

//===========================================================================
unsigned Cli::consoleWidth(bool queryWidth) {
    if (queryWidth) {
        HANDLE hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
        CONSOLE_SCREEN_BUFFER_INFO info;
        if (GetConsoleScreenBufferInfo(hOutput, &info))
            return info.dwSize.X;
    }
    return kDefaultConsoleWidth;
}

#else

#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

//===========================================================================
bool Cli::consoleEnableEcho(bool enable) {
    termios tty = {};
    bool got = !tcgetattr(STDIN_FILENO, &tty);
    if (enable) {
        tty.c_lflag |= ECHO;
    } else {
        tty.c_lflag &= ~ECHO;
    }
    return got && !tcsetattr(STDIN_FILENO, TCSANOW, &tty);
}

//===========================================================================
unsigned Cli::consoleWidth(bool queryWidth) {
    winsize w;
    if (queryWidth) {
        if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) != -1) {
            // Some CI platforms (Github Actions) run unix test scripts without
            // a console attached, making it impossible to get coverage for
            // this line in those environments.
            if (w.ws_col) return w.ws_col;  // LCOV_EXCL_LINE
        }
#if !defined(DIMCLI_LIB_NO_ENV)
        if (auto val = getenv("COLUMNS")) {
            auto width = atoi(val);
            if (width > 0)
                return width;
        }
#endif
    }
    return kDefaultConsoleWidth;
}

#endif
