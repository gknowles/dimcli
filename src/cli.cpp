// cli.cpp - dim cli
//
// For documentation and examples follow the links at:
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
const std::string s_internalOptionGroup = "~";


/****************************************************************************
*
*   CliBase::Group
*
***/

//===========================================================================
CliBase::Group::Group(Cli & cli, const std::string & name)
    : m_cli(cli)
    , m_name(name)
    , m_title(name)
    , m_sortKey(name) {}

//===========================================================================
CliBase::Group & CliBase::Group::operator=(const Group & from) {
    m_name = from.m_name;
    m_title = from.m_title;
    m_sortKey = from.m_sortKey;
    return *this;
}

//===========================================================================
CliBase::Group & CliBase::Group::title(const std::string & val) {
    m_title = val;
    return *this;
}

//===========================================================================
CliBase::Group & CliBase::Group::sortKey(const std::string & val) {
    m_sortKey = val;
    return *this;
}


/****************************************************************************
*
*   CliBase::ArgBase
*
***/

//===========================================================================
CliBase::ArgBase::ArgBase(const std::string & names, bool boolean)
    : m_names{names}
    , m_bool{boolean} {}


/****************************************************************************
*
*   Configure
*
***/

// forward declarations
static bool
helpAction(Cli & cli, Cli::Arg<bool> & arg, const std::string & val);

//===========================================================================
Cli::Cli()
    : Group{*this, ""} {
    title("Options");
    group(s_internalOptionGroup).title("");
    arg<bool>("help.")
        .desc("Show this message and exit.")
        .action(helpAction)
        .group(s_internalOptionGroup);
}

//===========================================================================
CliBase::Group & Cli::group(const std::string & name) {
    if (name.empty()) {
        return *this;
    } else {
        auto i = m_groups.find(name);
        if (i == m_groups.end()) {
            i = m_groups.emplace(make_pair(name, Group(*this, name))).first;
        }
        return i->second;
    }
}

//===========================================================================
const CliBase::Group & Cli::group(const std::string & name) const {
    return const_cast<Cli *>(this)->group(name);
}

//===========================================================================
void Cli::responseFiles(bool enable) {
    m_responseFiles = enable;
}

//===========================================================================
Cli::Arg<bool> &
Cli::versionArg(const std::string & version, const std::string & progName) {
    auto verAction = [version, progName](auto & cli, auto & arg, auto & val) {
        ignore = arg, val;
        fs::path prog = progName;
        if (prog.empty()) {
            prog = cli.progName();
            prog = prog.stem();
        }
        cout << prog << " version " << version << endl;
        return false;
    };
    return group(s_internalOptionGroup)
        .arg<bool>("version.")
        .desc("Show version and exit.")
        .action(verAction);
}

//===========================================================================
void Cli::addArg(std::unique_ptr<ArgBase> src) {
    ArgBase * arg = src.get();
    m_args.push_back(std::move(src));
    const char * ptr = arg->m_names.data();
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
            addArgName(name, arg);
        }
        if (!*ptr)
            return;
    }
}

//===========================================================================
void Cli::addLongName(
    const string & src,
    ArgBase * arg,
    bool invert,
    bool optional) {
    bool allowNo = true;
    string key{src};
    if (key.back() == '.') {
        allowNo = false;
        if (key.size() == 2) {
            assert(key.size() > 2 && "bad modifier '.' for short name");
            return;
        }
        key.pop_back();
    }
    m_longNames[key] = {arg, invert, optional};
    if (arg->m_bool && allowNo)
        m_longNames["no-" + key] = {arg, !invert, optional};
}

//===========================================================================
void Cli::addArgName(const string & name, ArgBase * arg) {
    const bool invert = true;
    const bool optional = true;

    switch (name[0]) {
    case '-': assert(name[0] != '-' && "bad argument name"); return;
    case '[':
        m_argNames.push_back({arg, !invert, optional, name.data() + 1});
        return;
    case '<':
        auto where =
            find_if(m_argNames.begin(), m_argNames.end(), [](auto && key) {
                return key.optional;
            });
        m_argNames.insert(where, {arg, !invert, !optional, name.data() + 1});
        return;
    }
    if (name.size() == 1) {
        m_shortNames[name[0]] = {arg, !invert, !optional};
        return;
    }
    switch (name[0]) {
    case '!':
        if (!arg->m_bool) {
            assert(!arg->m_bool && "bad modifier '!' for non-bool argument");
            return;
        }
        if (name.size() == 2) {
            m_shortNames[name[1]] = {arg, invert, !optional};
        } else {
            addLongName(name.substr(1), arg, invert, !optional);
        }
        return;
    case '?':
        if (arg->m_bool) {
            assert(!arg->m_bool && "bad modifier '?' for bool argument");
            return;
        }
        if (name.size() == 2) {
            m_shortNames[name[1]] = {arg, !invert, optional};
        } else {
            addLongName(name.substr(1), arg, !invert, optional);
        }
        return;
    }
    addLongName(name, arg, !invert, !optional);
}


/****************************************************************************
*
*   Action callbacks
*
***/

//===========================================================================
static bool
helpAction(Cli & cli, Cli::Arg<bool> & arg, const std::string & val) {
    stringTo(*arg, val);
    if (*arg) {
        cli.writeHelp(cout);
        return false;
    }
    return true;
}

//===========================================================================
bool Cli::defaultAction(ArgBase & arg, const std::string & val) {
    if (!arg.parseValue(val))
        return badUsage("Invalid '" + arg.from() + "' value: " + val);
    return true;
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
    for (auto && arg : m_args) {
        arg->reset();
    }
    for (unsigned i = 0; i < size(m_argNames); ++i) {
        auto & key = m_argNames[i];
        if (key.name.empty())
            key.name = "arg" + to_string(i + 1);
    }
    m_exitCode = kExitOk;
    m_errMsg.clear();
    m_progName.clear();
}

//===========================================================================
bool Cli::parseAction(
    ArgBase & arg,
    const std::string & name,
    int pos,
    const char ptr[]) {
    arg.set(name, pos);
    if (ptr) {
        return arg.parseAction(*this, ptr);
    } else {
        arg.unspecifiedValue();
        return true;
    }
}

//===========================================================================
bool Cli::fail(int code, const string & msg) {
    m_errMsg = msg;
    m_exitCode = code;
    return false;
}

//===========================================================================
bool Cli::parse(vector<string> & args) {
    // the 0th (name of this program) arg should always be present
    assert(!args.empty());

    resetValues();
    set<fs::path> ancestors;
    if (m_responseFiles && !expandResponseFiles(*this, args, ancestors))
        return false;

    auto arg = args.data();
    auto argc = args.size();

    string name;
    unsigned pos = 0;
    bool moreOpts = true;
    m_progName = *arg;
    int argPos = 1;
    arg += 1;

    for (; argPos < argc; ++argPos, ++arg) {
        ArgName argName;
        const char * ptr = arg->c_str();
        if (*ptr == '-' && ptr[1] && moreOpts) {
            ptr += 1;
            for (; *ptr && *ptr != '-'; ++ptr) {
                auto it = m_shortNames.find(*ptr);
                if (it == m_shortNames.end()) {
                    return badUsage("Unknown option: -"s + *ptr);
                }
                argName = it->second;
                name = "-"s + *ptr;
                if (argName.arg->m_bool) {
                    if (!parseAction(
                            *argName.arg,
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
            auto it = m_longNames.find(key);
            if (it == m_longNames.end()) {
                return badUsage("Unknown option: --"s + key);
            }
            argName = it->second;
            name = "--" + key;
            if (argName.arg->m_bool) {
                if (equal) {
                    return badUsage("Unknown option: " + name + "=");
                }
                if (!parseAction(
                        *argName.arg,
                        name,
                        argPos,
                        argName.invert ? "0" : "1"))
                    return false;
                continue;
            }
            goto OPTION_VALUE;
        }

        // positional value
        if (pos >= size(m_argNames)) {
            return badUsage("Unexpected argument: "s + ptr);
        }
        argName = m_argNames[pos];
        name = argName.name;
        if (!parseAction(*argName.arg, name, argPos, ptr))
            return false;
        if (!argName.arg->m_multiple)
            pos += 1;
        continue;

    OPTION_VALUE:
        if (*ptr) {
            if (!parseAction(*argName.arg, name, argPos, ptr))
                return false;
            continue;
        }
        if (argName.optional) {
            if (!parseAction(*argName.arg, name, argPos, nullptr))
                return false;
            continue;
        }
        argPos += 1;
        arg += 1;
        if (argPos == argc) {
            return badUsage("No value given for " + name);
        }
        if (!parseAction(*argName.arg, name, argPos, arg->c_str()))
            return false;
    }

    if (pos < size(m_argNames) && !m_argNames[pos].optional) {
        return badUsage("No value given for " + m_argNames[pos].name);
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
*   Help
*
***/

namespace {
struct WrapPos {
    size_t pos{0};
    size_t maxWidth{79};
    std::string prefix;
};
} // namespace

//===========================================================================
// Write token, potentially adding a line break first.
static void writeToken(ostream & os, WrapPos & wp, const std::string token) {
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
        const char * ptr = strchr(base, ' ');
        if (!ptr) {
            ptr = text.c_str() + text.size();
        }
        writeToken(os, wp, string(base, ptr));
        base = ptr;
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
int Cli::writeHelp(ostream & os, const string & progName) const {
    writeUsage(os, progName);
    writePositionals(os);
    writeOptions(os);
    return exitCode();
}

//===========================================================================
void Cli::writePositionals(ostream & os) const {
    size_t colWidth = 0;
    for (auto && pa : m_argNames) {
        // find widest positional argument name, with space for <>'s
        colWidth = max(colWidth, pa.name.size() + 2);
    }
    colWidth = max(min(colWidth + 3, kMaxDescCol), kMinDescCol);

    WrapPos wp;
    for (auto && pa : m_argNames) {
        wp.prefix.assign(4, ' ');
        writeToken(os, wp, "  " + pa.name);
        writeDescCol(os, wp, pa.arg->m_desc, colWidth);
        os << '\n';
        wp.pos = 0;
    }
}

//===========================================================================
void Cli::writeOptions(ostream & os) const {
    // find named args and the longest name list
    size_t colWidth = 0;

    struct ArgKey {
        string sort; // sort key
        string list;
        ArgBase * arg;
    };
    vector<ArgKey> namedArgs;
    for (auto && arg : m_args) {
        string list = optionList(*arg);
        if (size_t width = list.size()) {
            colWidth = max(colWidth, width);
            ArgKey key;
            key.arg = arg.get();
            key.list = list;

            // sort by group sort key followed by name list with leading 
            // dashes removed
            key.sort = group(arg->m_group).sortKey();
            key.sort += '\0';
            key.sort += list.substr(list.find_first_not_of('-'));
            namedArgs.push_back(key);
        }
    }
    colWidth = max(min(colWidth + 3, kMaxDescCol), kMinDescCol);

    if (namedArgs.empty())
        return;

    sort(namedArgs.begin(), namedArgs.end(), [](auto & a, auto & b) {
        return tie(a.arg->m_group, a.sort) < tie(b.arg->m_group, b.sort);
    });

    WrapPos wp;
    const char * gname{nullptr};
    for (auto && key : namedArgs) {
        if (!gname || key.arg->m_group != gname) {
            gname = key.arg->m_group.c_str();
            os << '\n';
            wp.pos = 0;
            auto grp = group(key.arg->m_group);
            string title = grp.title();
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
        writeDescCol(os, wp, key.arg->m_desc, colWidth);
        os << '\n';
        wp.pos = 0;
    }
}

//===========================================================================
int Cli::writeUsage(ostream & os, const string & progName) const {
    streampos base = os.tellp();
    os << "usage: " << (progName.empty() ? m_progName : progName) << ' ';
    WrapPos wp;
    wp.maxWidth = 79;
    wp.pos = os.tellp() - base;
    wp.prefix = string(wp.pos, ' ');
    if (!m_shortNames.empty() || !m_longNames.empty())
        writeToken(os, wp, "[OPTIONS]");
    for (auto && pa : m_argNames) {
        string token = pa.name.find(' ') == string::npos 
            ? pa.name : "<" + pa.name + ">";
        if (pa.arg->m_multiple)
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
string Cli::optionList(ArgBase & arg) const {
    string list = optionList(arg, true);
    if (arg.m_bool) {
        string invert = optionList(arg, false);
        if (!invert.empty())
            list += " / " + invert;
    }
    return list;
}

//===========================================================================
string Cli::optionList(ArgBase & arg, bool enableOptions) const {
    string list;
    bool foundLong = false;
    bool optional = false;

    // names
    for (auto && sn : m_shortNames) {
        if (sn.second.arg != &arg
            || arg.m_bool && sn.second.invert == enableOptions) {
            continue;
        }
        optional = sn.second.optional;
        if (!list.empty())
            list += ", ";
        list += '-';
        list += sn.first;
    }
    for (auto && ln : m_longNames) {
        if (ln.second.arg != &arg
            || arg.m_bool && ln.second.invert == enableOptions) {
            continue;
        }
        optional = ln.second.optional;
        if (!list.empty())
            list += ", ";
        foundLong = true;
        list += "--" + ln.first;
    }
    if (arg.m_bool || list.empty())
        return list;

    // value
    if (optional) {
        list += foundLong ? "[=" : " [";
        list += arg.m_valueDesc + "]";
    } else {
        list += foundLong ? '=' : ' ';
        list += arg.m_valueDesc;
    }
    return list;
}
