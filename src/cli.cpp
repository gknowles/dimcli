// cli.cpp - dim cli
//
// For documentation and examples:
// https://github.com/gknowles/dimcli

#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;
namespace fs = experimental::filesystem;


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
const string s_internalOptionGroup = "~";

namespace {
struct OptName {
    Cli::OptBase * opt;
    bool invert;      // set to false instead of true (only for bools)
    bool optional;    // value doesn't have to be present? (non-bools only)
    std::string name; // name of argument (only for positionals)
};
} // namespace

struct Cli::OptIndex {
    map<char, OptName> shortNames;
    map<string, OptName> longNames;
    vector<OptName> argNames;
};

struct Cli::GroupConfig {
    string name;
    string title;
    string sortKey;
};

struct Cli::CommandConfig {
    string name;
    string desc;
    string footer;
    std::function<Cli::ActionFn> action;
    Opt<bool> * helpOpt{nullptr};
    map<string, GroupConfig> groups;
};

struct Cli::Config {
    bool constructed{false};

    map<string, CommandConfig> cmds;
    std::list<std::unique_ptr<OptBase>> opts;
    bool responseFiles{true};

    int exitCode{0};
    string errMsg;
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
static int cmdAction(Cli & cli, const string & cmd);

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
    auto & intGrp = findGrpAlways(cmd, s_internalOptionGroup);
    intGrp.title.clear();
    return cmd;
}


/****************************************************************************
*
*   Cli::OptBase
*
***/

//===========================================================================
Cli::OptBase::OptBase(const string & names, bool boolean)
    : m_names{names}
    , m_bool{boolean} {}

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

    switch (name[0]) {
    case '-': assert(name[0] != '-' && "bad argument name"); return;
    case '[':
        ndx.argNames.push_back({this, !invert, optional, name.data() + 1});
        return;
    case '<':
        auto where =
            find_if(ndx.argNames.begin(), ndx.argNames.end(), [](auto && key) {
                return key.optional;
            });
        ndx.argNames.insert(
            where, {this, !invert, !optional, name.data() + 1});
        return;
    }
    if (name.size() == 1) {
        ndx.shortNames[name[0]] = {this, !invert, !optional};
        return;
    }
    switch (name[0]) {
    case '!':
        if (!m_bool) {
            assert(!m_bool && "bad modifier '!' for non-bool argument");
            return;
        }
        if (name.size() == 2) {
            ndx.shortNames[name[1]] = {this, invert, !optional};
        } else {
            indexLongName(ndx, name.substr(1), invert, !optional);
        }
        return;
    case '?':
        if (m_bool) {
            assert(!m_bool && "bad modifier '?' for bool argument");
            return;
        }
        if (name.size() == 2) {
            ndx.shortNames[name[1]] = {this, !invert, optional};
        } else {
            indexLongName(ndx, name.substr(1), !invert, optional);
        }
        return;
    }
    indexLongName(ndx, name, !invert, !optional);
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
    ndx.longNames[key] = {this, invert, optional};
    if (m_bool && allowNo)
        ndx.longNames["no-" + key] = {this, !invert, optional};
}


/****************************************************************************
*
*   Action callbacks
*
***/

//===========================================================================
static bool helpAction(Cli & cli, Cli::Opt<bool> & opt, const string & val) {
    stringTo(*opt, val);
    if (*opt) {
        cli.writeHelp(cout, {}, cli.runCommand());
        return false;
    }
    return true;
}

//===========================================================================
bool Cli::defaultAction(OptBase & opt, const string & val) {
    if (!opt.parseValue(val))
        return badUsage("Invalid '" + opt.from() + "' value: " + val);
    return true;
}

//===========================================================================
static int cmdAction(Cli & cli, const string & cmd) {
    ignore = cli;
    cerr << "Command '" << cmd << "' has not been implemented." << endl;
    return kExitSoftware;
}


/****************************************************************************
*
*   Cli
*
***/

//===========================================================================
// Default constructor creates a handle to the shared configuration, this
// allows options to be statically registered from multiple source files.
Cli::Cli() {
    static auto s_cfg = make_shared<Config>();
    m_cfg = s_cfg;
    helpOpt();
}

//===========================================================================
// protected
Cli::Cli(shared_ptr<Config> cfg, const string & command, const string & group)
    : m_cfg(cfg)
    , m_command(command)
    , m_group(group) {
    helpOpt();
}


/****************************************************************************
*
*   Configuration
*
***/

//===========================================================================
Cli::Opt<bool> &
Cli::versionOpt(const string & version, const string & progName) {
    auto verAction = [version, progName](auto & cli, auto & opt, auto & val) {
        ignore = opt, val;
        fs::path prog = progName;
        if (prog.empty()) {
            prog = cli.progName();
            prog = prog.stem();
        }
        cout << prog << " version " << version << endl;
        return false;
    };
    return opt<bool>("version.")
        .desc("Show version and exit.")
        .action(verAction)
        .group(s_internalOptionGroup);
}

//===========================================================================
Cli::Opt<bool> & Cli::helpOpt() {
    auto & cmd = cmdCfg();
    if (!cmd.helpOpt) {
        cmd.helpOpt = &opt<bool>("help.")
                           .desc("Show this message and exit.")
                           .action(helpAction)
                           .group(s_internalOptionGroup);
        if (!m_command.empty())
            cmd.helpOpt->show(false);
    }
    return *cmd.helpOpt;
}

//===========================================================================
Cli Cli::group(const string & name) {
    return Cli(m_cfg, m_command, name);
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
Cli Cli::command(const string & name, const string & grpName) {
    return Cli(m_cfg, name, grpName);
}

//===========================================================================
Cli & Cli::action(std::function<ActionFn> fn) {
    cmdCfg().action = fn;
    return *this;
}

//===========================================================================
Cli & Cli::desc(const std::string & val) {
    cmdCfg().desc = val;
    return *this;
}

//===========================================================================
Cli & Cli::footer(const std::string & val) {
    cmdCfg().footer = val;
    return *this;
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
*   Response files
*
***/

// forward declarations
static bool expandResponseFiles(
    Cli & cli,
    vector<string> & args,
    set<fs::path> & ancestors);

//===========================================================================
// Returns false on error, if there was an error content will either be empty
// or - if there was a transcoding error - contain the original content.
static bool loadFileUtf8(string & content, const fs::path & fn) {
    content.clear();

    error_code err;
    auto bytes = fs::file_size(fn, err);
    if (err)
        return false;

    content.resize(bytes);
    ifstream f(fn, ios::binary);
    f.read(content.data(), content.size());
    if (!f) {
        content.clear();
        return false;
    }
    f.close();

    if (content.size() < 2)
        return true;
    if (content[0] == '\xff' && content[1] == '\xfe') {
        wstring_convert<codecvt<wchar_t, char, mbstate_t>, wchar_t> wcvt("");
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
    set<fs::path> & ancestors) {
    string content;
    error_code err;
    fs::path fn = args[pos].substr(1);
    fs::path cfn = fs::canonical(fn, err);
    if (err)
        return cli.badUsage("Invalid response file: " + fn.string());
    auto ib = ancestors.insert(cfn);
    if (!ib.second)
        return cli.badUsage("Recursive response file: " + fn.string());
    if (!loadFileUtf8(content, fn)) {
        string desc = content.empty() ? "Read error" : "Invalid encoding";
        return cli.badUsage(desc + ": " + fn.string());
    }
    auto rargs = cli.toArgv(content);
    if (!expandResponseFiles(cli, rargs, ancestors))
        return false;
    if (rargs.empty()) {
        args.erase(args.begin() + pos);
    } else {
        args.insert(args.begin() + pos + 1, rargs.size() - 1, {});
        auto i = args.begin() + pos;
        for (auto && arg : rargs)
            *i++ = move(arg);
        pos += rargs.size();
    }
    ancestors.erase(ib.first);
    return true;
}

//===========================================================================
// "ancestors" contains the set of response files these args came from,
// directly or indirectly, and is used to detect recursive response files.
static bool expandResponseFiles(
    Cli & cli,
    vector<string> & args,
    set<fs::path> & ancestors) {
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
*   Parse
*
***/

//===========================================================================
void Cli::resetValues() {
    for (auto && opt : m_cfg->opts) {
        opt->reset();
    }
    m_cfg->exitCode = kExitOk;
    m_cfg->errMsg.clear();
    m_cfg->progName.clear();
}

//===========================================================================
void Cli::index(OptIndex & ndx, const std::string & cmd) const {
    for (auto && opt : m_cfg->opts) {
        if (opt->m_command == cmd && opt->m_visible)
            opt->index(ndx);
    }
    for (unsigned i = 0; i < size(ndx.argNames); ++i) {
        auto & key = ndx.argNames[i];
        if (key.name.empty())
            key.name = "arg" + to_string(i + 1);
    }
}

//===========================================================================
bool Cli::parseAction(
    OptBase & opt,
    const string & name,
    int pos,
    const char ptr[]) {
    opt.set(name, pos);
    if (ptr) {
        return opt.parseAction(*this, ptr);
    } else {
        opt.unspecifiedValue();
        return true;
    }
}

//===========================================================================
bool Cli::fail(int code, const string & msg) {
    m_cfg->errMsg = msg;
    m_cfg->exitCode = code;
    return false;
}

//===========================================================================
bool Cli::parse(vector<string> & args) {
    // the 0th (name of this program) opt should always be present
    assert(!args.empty());

    OptIndex ndx;
    index(ndx, "");

    resetValues();
    set<fs::path> ancestors;
    if (m_cfg->responseFiles && !expandResponseFiles(*this, args, ancestors))
        return false;

    auto arg = args.data();
    auto argc = args.size();

    string name;
    unsigned pos = 0;
    bool moreOpts = true;
    m_cfg->progName = *arg;
    int argPos = 1;
    arg += 1;

    for (; argPos < argc; ++argPos, ++arg) {
        OptName argName;
        const char * ptr = arg->c_str();
        if (*ptr == '-' && ptr[1] && moreOpts) {
            ptr += 1;
            for (; *ptr && *ptr != '-'; ++ptr) {
                auto it = ndx.shortNames.find(*ptr);
                if (it == ndx.shortNames.end()) {
                    return badUsage("Unknown option: -"s + *ptr);
                }
                argName = it->second;
                name = "-"s + *ptr;
                if (argName.opt->m_bool) {
                    if (!parseAction(
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
            if (it == ndx.longNames.end()) {
                return badUsage("Unknown option: --"s + key);
            }
            argName = it->second;
            name = "--" + key;
            if (argName.opt->m_bool) {
                if (equal) {
                    return badUsage("Unknown option: " + name + "=");
                }
                if (!parseAction(
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
        if (pos >= size(ndx.argNames)) {
            return badUsage("Unexpected argument: "s + ptr);
        }
        argName = ndx.argNames[pos];
        name = argName.name;
        if (!parseAction(*argName.opt, name, argPos, ptr))
            return false;
        if (!argName.opt->m_multiple)
            pos += 1;
        continue;

    OPTION_VALUE:
        if (*ptr) {
            if (!parseAction(*argName.opt, name, argPos, ptr))
                return false;
            continue;
        }
        if (argName.optional) {
            if (!parseAction(*argName.opt, name, argPos, nullptr))
                return false;
            continue;
        }
        argPos += 1;
        arg += 1;
        if (argPos == argc) {
            return badUsage("No value given for " + name);
        }
        if (!parseAction(*argName.opt, name, argPos, arg->c_str()))
            return false;
    }

    if (pos < size(ndx.argNames) && !ndx.argNames[pos].optional) {
        return badUsage("No value given for " + ndx.argNames[pos].name);
    }
    return true;
}

//===========================================================================
bool Cli::parse(ostream & os, vector<string> & args) {
    if (parse(args))
        return true;
    if (exitCode())
        os << args[0] << ": " << errMsg() << endl;
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
const string & Cli::progName() const {
    return m_cfg->progName;
}

//===========================================================================
const string & Cli::runCommand() const {
    return m_cfg->command;
}

//===========================================================================
int Cli::run() {
    auto & name = runCommand();
    assert(m_cfg->cmds.find(name) != m_cfg->cmds.end());
    auto & cmd = m_cfg->cmds[name];
    int code = cmd.action(*this, name);
    fail(code, "");
    return code;
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
// Write token, potentially adding a line break first.
static void writeToken(ostream & os, WrapPos & wp, const string token) {
    if (wp.pos + token.size() + 1 > wp.maxWidth) {
        if (wp.pos > wp.prefix.size()) {
            os << '\n' << wp.prefix;
            wp.pos = wp.prefix.size();
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
            os << '\n';
            wp.pos = 0;
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
int Cli::writeHelp(
    ostream & os,
    const string & progName,
    const string & cmdName) const {
    auto & cmd = findCmdAlways(*m_cfg, cmdName);
    writeUsage(os, progName, cmdName);
    if (!cmd.desc.empty()) {
        WrapPos wp;
        writeText(os, wp, cmd.desc);
    }
    writePositionals(os, cmdName);
    writeOptions(os, cmdName);
    if (!cmd.footer.empty()) {
        WrapPos wp;
        writeText(os, wp, cmd.footer);
    }
    return exitCode();
}

//===========================================================================
int Cli::writeUsage(ostream & os, const string & arg0, const string & cmd)
    const {
    OptIndex ndx;
    index(ndx, cmd);
    streampos base = os.tellp();
    os << "usage: " << (arg0.empty() ? progName() : arg0);
    WrapPos wp;
    wp.maxWidth = 79;
    wp.pos = os.tellp() - base;
    wp.prefix = string(wp.pos, ' ');
    if (!ndx.shortNames.empty() || !ndx.longNames.empty())
        writeToken(os, wp, "[OPTIONS]");
    for (auto && pa : ndx.argNames) {
        string token =
            pa.name.find(' ') == string::npos ? pa.name : "<" + pa.name + ">";
        if (pa.opt->m_multiple)
            token += "...";
        if (pa.optional) {
            writeToken(os, wp, "[" + token + "]");
        } else {
            writeToken(os, wp, token);
        }
    }
    os << '\n';
    return exitCode();
}

//===========================================================================
void Cli::writePositionals(ostream & os, const string & cmd) const {
    OptIndex ndx;
    index(ndx, cmd);
    size_t colWidth = 0;
    for (auto && pa : ndx.argNames) {
        // find widest positional argument name, with space for <>'s
        colWidth = max(colWidth, pa.name.size() + 2);
    }
    colWidth = max(min(colWidth + 3, kMaxDescCol), kMinDescCol);

    WrapPos wp;
    for (auto && pa : ndx.argNames) {
        wp.prefix.assign(4, ' ');
        writeToken(os, wp, "  " + pa.name);
        writeDescCol(os, wp, pa.opt->m_desc, colWidth);
        os << '\n';
        wp.pos = 0;
    }
}

//===========================================================================
void Cli::writeOptions(ostream & os, const string & cmdName) const {
    OptIndex ndx;
    index(ndx, cmdName);
    auto & cmd = findCmdAlways(*m_cfg, cmdName);

    // find named args and the longest name list
    size_t colWidth = 0;

    struct ArgKey {
        string sort; // sort key
        string list;
        OptBase * opt;
    };
    vector<ArgKey> namedArgs;
    for (auto && opt : m_cfg->opts) {
        string list = nameList(*opt, ndx);
        if (size_t width = list.size()) {
            colWidth = max(colWidth, width);
            ArgKey key;
            key.opt = opt.get();
            key.list = list;

            // sort by group sort key followed by name list with leading
            // dashes removed
            key.sort = findGrpAlways(cmd, opt->m_group).sortKey;
            key.sort += '\0';
            key.sort += list.substr(list.find_first_not_of('-'));
            namedArgs.push_back(key);
        }
    }
    colWidth = max(min(colWidth + 3, kMaxDescCol), kMinDescCol);

    if (namedArgs.empty())
        return;

    sort(namedArgs.begin(), namedArgs.end(), [](auto & a, auto & b) {
        return tie(a.opt->m_group, a.sort) < tie(b.opt->m_group, b.sort);
    });

    WrapPos wp;
    const char * gname{nullptr};
    for (auto && key : namedArgs) {
        if (!gname || key.opt->m_group != gname) {
            gname = key.opt->m_group.c_str();
            os << '\n';
            wp.pos = 0;
            auto & grp = findGrpAlways(cmd, key.opt->m_group);
            string title = grp.title;
            if (title.empty() && gname == s_internalOptionGroup
                && &key == namedArgs.data()) {
                // First group and it's the internal group, give it a title
                // so it's not just left hanging.
                title = "Options";
            }
            if (!title.empty()) {
                wp.prefix.clear();
                writeText(os, wp, title + ":");
                os << '\n';
                wp.pos = 0;
            }
        }
        wp.prefix.assign(4, ' ');
        os << ' ';
        wp.pos = 1;
        writeText(os, wp, key.list);
        writeDescCol(os, wp, key.opt->m_desc, colWidth);
        os << '\n';
        wp.pos = 0;
    }
}

//===========================================================================
string Cli::nameList(const OptBase & opt, const OptIndex & ndx) const {
    string list = nameList(opt, ndx, true);
    if (opt.m_bool) {
        string invert = nameList(opt, ndx, false);
        if (!invert.empty())
            list += " / " + invert;
    }
    return list;
}

//===========================================================================
string Cli::nameList(
    const OptBase & opt,
    const OptIndex & ndx,
    bool enableOptions) const {
    string list;
    bool foundLong = false;
    bool optional = false;

    // names
    for (auto && sn : ndx.shortNames) {
        if (sn.second.opt != &opt
            || opt.m_bool && sn.second.invert == enableOptions) {
            continue;
        }
        optional = sn.second.optional;
        if (!list.empty())
            list += ", ";
        list += '-';
        list += sn.first;
    }
    for (auto && ln : ndx.longNames) {
        if (ln.second.opt != &opt
            || opt.m_bool && ln.second.invert == enableOptions) {
            continue;
        }
        optional = ln.second.optional;
        if (!list.empty())
            list += ", ";
        foundLong = true;
        list += "--" + ln.first;
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
*   CliLocal
*
***/

//===========================================================================
CliLocal::CliLocal()
    : Cli(make_shared<Config>(), "", "") {}
