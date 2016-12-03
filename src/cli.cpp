// cli.cpp - dim cli
//
// For documentation and examples:
// https://github.com/gknowles/dimcli

#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;
namespace fs = experimental::filesystem;

// getenv triggers the visual c++ security warning
#if (_MSC_VER >= 1400)
#pragma warning(disable:4996) // this function or variable may be unsafe.
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
const string s_internalOptionGroup = "~";

namespace {
struct OptName {
    Cli::OptBase * opt;
    bool invert;   // set to false instead of true (only for bools)
    bool optional; // value doesn't have to be present? (non-bools only)
    string name;   // name of argument (only for positionals)
};
} // namespace

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

    int exitCode{0};
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
static int cmdAction(Cli & cli);

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

//===========================================================================
static fs::path displayName(const fs::path & file) {
#if defined(_WIN32)
    return file.stem();
#else
    return file.filename();
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
    setNameIfEmpty('-' + string(1, name));
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
    setNameIfEmpty("--" + key);
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
        cli.writeHelp(cli.conout(), {}, cli.runCommand());
        return false;
    }
    return true;
}

//===========================================================================
bool Cli::defaultParse(OptBase & opt, const string & val) {
    if (!opt.parseValue(val)) {
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
    return true;
}

//===========================================================================
static int cmdAction(Cli & cli) {
    if (cli.runCommand().empty()) {
        cerr << "No command given." << endl;
    } else {
        cerr << "Command '" << cli.runCommand()
             << "' has not been implemented." << endl;
    }
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
Cli::Cli(shared_ptr<Config> cfg)
    : m_cfg(cfg) {
    helpOpt();
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
                     .group(s_internalOptionGroup);
    if (!m_command.empty())
        hlp.show(false);
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
    auto verAction = [version, progName](auto & cli, auto & opt, auto & val) {
        fs::path prog = progName;
        if (prog.empty()) {
            prog = displayName(cli.progName());
        }
        cli.conout() << prog << " version " << version << endl;
        return false;
    };
    return opt<bool>("version.")
        .desc("Show version and exit.")
        .parse(verAction)
        .group(s_internalOptionGroup);
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

//===========================================================================
void Cli::envOpts(const string & var) {
    m_cfg->envOpts = var;
}

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
    auto bytes = (size_t) fs::file_size(fn, err);
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
    for (unsigned i = 0; i < size(ndx.argNames); ++i) {
        auto & key = ndx.argNames[i];
        if (key.name.empty())
            key.name = "arg" + to_string(i + 1);
    }
}

//===========================================================================
bool Cli::prompt(OptBase & opt, const string & msg, int flags) {
    if (!opt.from().empty())
        return true;
    struct EnableEcho {
        ~EnableEcho() { consoleEnableEcho(true); }
    } enableEcho;
    auto & is = conin();
    auto & os = conout();
    os << msg << ' ';
    if (~flags & kPromptNoDefault) {
        if (opt.m_bool)
            os << "[y/N]: ";
    }
    if (flags & kPromptHide)
        consoleEnableEcho(false);
    string val;
    os.flush();
    getline(is, val);
    if (flags & kPromptHide)
        os << endl;
    if (flags & kPromptConfirm) {
        string again;
        os << "Enter again to confirm: " << flush;
        getline(is, again);
        if (flags & kPromptHide)
            os << endl;
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
        if (!opt.parseValue(*this, val))
            return false;
    } else {
        opt.unspecifiedValue();
    }
    return opt.checkValue(*this, val);
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
    index(ndx, "", false);
    bool needCmd = m_cfg->cmds.size() > 1;

    // Commands can't be added when the top level command has a positional
    // argument, command processing requires that the first positional is
    // available to identify the command.
    assert(ndx.allowCommands || !needCmd);

    resetValues();

    // insert environment options
    if (m_cfg->envOpts.size()) {
        if (const char * val = getenv(m_cfg->envOpts.c_str()))
            replace(args, 1, 0, toArgv(val));
    }

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
                if (it == ndx.shortNames.end())
                    return badUsage("Unknown option", "-"s + *ptr);
                argName = it->second;
                name = "-"s + *ptr;
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
            if (it == ndx.longNames.end())
                return badUsage("Unknown option", "--"s + key);
            argName = it->second;
            name = "--" + key;
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

        if (pos >= size(ndx.argNames))
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

    if (!needCmd && pos < size(ndx.argNames) && !ndx.argNames[pos].optional)
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
int Cli::run() {
    auto & name = runCommand();
    assert(m_cfg->cmds.find(name) != m_cfg->cmds.end());
    auto & cmd = m_cfg->cmds[name];
    int code = cmd.action(*this);
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
    };
    vector<ChoiceKey> keys;
    for (auto && cd : choices) {
        colWidth = max(colWidth, cd.first.size());
        ChoiceKey key;
        key.pos = cd.second.pos;
        key.key = cd.first.c_str();
        key.desc = cd.second.desc.c_str();
        key.sortKey = cd.second.sortKey.c_str();
        keys.push_back(key);
    }
    colWidth = max(min(colWidth + 5, kMaxDescCol), kMinDescCol);
    sort(keys.begin(), keys.end(), [](auto & a, auto & b) {
        if (int rc = strcmp(a.sortKey, b.sortKey))
            return rc < 0;
        return a.pos < b.pos;
    });

    for (auto && k : keys) {
        wp.prefix.assign(8, ' ');
        writeToken(os, wp, "      "s + k.key);
        writeDescCol(os, wp, k.desc, colWidth);
        os << '\n';
        wp.pos = 0;
    }
}

//===========================================================================
int Cli::writeHelp(
    ostream & os,
    const string & progName,
    const string & cmdName) const {
    auto & cmd = findCmdAlways(*m_cfg, cmdName);
    if (!cmd.header.empty()) {
        WrapPos wp;
        writeText(os, wp, cmd.header);
        writeNewline(os, wp);
    }
    writeUsage(os, progName, cmdName);
    if (!cmd.desc.empty()) {
        WrapPos wp;
        writeText(os, wp, cmd.desc);
    }
    writePositionals(os, cmdName);
    writeOptions(os, cmdName);
    if (cmdName.empty())
        writeCommands(os);
    if (!cmd.footer.empty()) {
        WrapPos wp;
        writeNewline(os, wp);
        writeText(os, wp, cmd.footer);
    }
    return exitCode();
}

//===========================================================================
int Cli::writeUsage(ostream & os, const string & arg0, const string & cmd)
    const {
    OptIndex ndx;
    index(ndx, cmd, true);
    string prog = displayName(arg0.empty() ? progName() : arg0).string();
    const string usageStr{"usage: "};
    os << usageStr << prog;
    WrapPos wp;
    wp.maxWidth = 79;
    wp.pos = prog.size() + size(usageStr);
    wp.prefix = string(wp.pos, ' ');
    if (cmd.size())
        writeToken(os, wp, cmd);
    if (!ndx.shortNames.empty() || !ndx.longNames.empty())
        writeToken(os, wp, "[OPTIONS]");
    if (cmd.empty() && m_cfg->cmds.size() > 1) {
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
void Cli::writePositionals(ostream & os, const string & cmd) const {
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
        writeDescCol(os, wp, pa.opt->m_desc, colWidth);
        os << '\n';
        wp.pos = 0;
        writeChoices(os, wp, pa.opt->m_choiceDescs);
    }
}

//===========================================================================
void Cli::writeOptions(ostream & os, const string & cmdName) const {
    OptIndex ndx;
    index(ndx, cmdName, true);
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
            writeNewline(os, wp);
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
                writeNewline(os, wp);
            }
        }
        wp.prefix.assign(4, ' ');
        os << ' ';
        wp.pos = 1;
        writeText(os, wp, key.list);
        writeDescCol(os, wp, key.opt->m_desc, colWidth);
        wp.prefix.clear();
        writeNewline(os, wp);
        writeChoices(os, wp, key.opt->m_choiceDescs);
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
void Cli::writeCommands(ostream & os) const {
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
string Cli::nameList(const OptBase & opt, const OptIndex & ndx) const {
    string list = nameList(opt, ndx, true);
    if (opt.m_bool) {
        string invert = nameList(opt, ndx, false);
        if (!invert.empty()) {
            list += list.empty() ? "/ " : " / ";
            list += invert;
        }
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
    : Cli(make_shared<Config>()) {}
