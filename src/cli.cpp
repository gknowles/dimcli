// cli.cpp - dim cli
//
// For documentation and examples follow the links at:
// https://github.com/gknowles/dimcli

#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


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
*   Cli::ArgBase
*
***/

//===========================================================================
Cli::ArgBase::ArgBase(const std::string & names, bool boolean)
    : m_names{names}
    , m_bool{boolean} {}


/****************************************************************************
*
*   Configure
*
***/

static bool
helpAction(Cli & cli, Cli::Arg<bool> & arg, const std::string & val);

//===========================================================================
Cli::Cli() {
    arg<bool>("help.").desc("Show this message and exit.").action(helpAction);
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
    const string & src, ArgBase * arg, bool invert, bool optional) {
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

//===========================================================================
Cli::Arg<bool> &
Cli::versionArg(const std::string & version, const std::string & progName) {
    auto verAction = [version, progName](auto & cli, auto & arg, auto & val) {
        ignore = arg, val;
        experimental::filesystem::path prog = progName;
        if (prog.empty()) {
            prog = cli.progName();
            prog = prog.stem();
        }
        cout << prog << " version " << version << endl;
        return false;
    };
    auto & ver = arg<bool>("version.").desc("Show version and exit.");
    ver.action(verAction);
    return ver;
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
    ArgBase & arg, const std::string & name, int pos, const char ptr[]) {
    arg.set(name, pos);
    if (ptr) {
        return arg.parseAction(*this, ptr);
    } else {
        arg.unspecifiedValue();
        return true;
    }
}

//===========================================================================
bool Cli::badUsage(const string & msg) {
    m_errMsg = msg;
    m_exitCode = kExitUsage;
    return false;
}

//===========================================================================
bool Cli::parse(size_t argc, char * argv[]) {
    ignore = argc;

    resetValues();

    // the 0th (name of this program) arg should always be present
    assert(argc && *argv);

    string name;
    unsigned pos = 0;
    bool moreOpts = true;
    m_progName = *argv;
    int argPos = 1;
    argv += 1;

    for (; *argv; ++argPos, ++argv) {
        ArgName argName;
        const char * ptr = *argv;
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
                        *argName.arg, name, argPos, argName.invert ? "0" : "1"))
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
        argv += 1;
        if (!*argv) {
            return badUsage("No value given for " + name);
        }
        if (!parseAction(*argName.arg, name, argPos, *argv))
            return false;
    }
    assert(argPos == argc);

    if (pos < size(m_argNames) && !m_argNames[pos].optional) {
        return badUsage("No value given for " + m_argNames[pos].name);
    }
    return true;
}

//===========================================================================
bool Cli::parse(ostream & os, size_t argc, char * argv[]) {
    if (parse(argc, argv))
        return true;
    if (exitCode())
        os << argv[0] << ": " << errMsg() << endl;
    return false;
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
static void writeToken(ostream & os, WrapPos & wp, const std::string token) {
    if (wp.pos + token.size() + 1 > wp.maxWidth) {
        if (wp.pos > wp.prefix.size()) {
            os << '\n' << wp.prefix;
            wp.pos = wp.prefix.size();
        }
    }
    os << ' ' << token;
    wp.pos += token.size() + 1;
}

//===========================================================================
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
static void
writeDesc(ostream & os, WrapPos & wp, const string & text, size_t descCol) {
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

    // size arg column
    size_t colWidth = 0;
    for (auto && pa : m_argNames)
        colWidth = max(colWidth, pa.name.size() + 2);
    for (auto && arg : m_args) {
        string list = optionList(*arg);
        colWidth = max(colWidth, list.size());
    }
    colWidth = max(min(colWidth + 3, kMaxDescCol), kMinDescCol);

    // positional args
    WrapPos wp;
    for (auto && pa : m_argNames) {
        wp.prefix.assign(4, ' ');
        writeToken(os, wp, " <" + pa.name + ">");
        writeDesc(os, wp, pa.arg->m_desc, colWidth);
        os << '\n';
        wp.pos = 0;
    }

    // named args
    if (!m_shortNames.empty() || !m_longNames.empty()) {
        os << "\nOptions:\n";
        for (auto && arg : m_args) {
            wp.prefix.assign(4, ' ');
            string list = optionList(*arg, true);
            if (arg->m_bool) {
                string invert = optionList(*arg, false);
                if (!invert.empty())
                    list += " / " + invert;
            }
            if (list.empty())
                continue;
            os << ' ';
            wp.pos = 1;
            writeText(os, wp, list);
            writeDesc(os, wp, arg->m_desc, colWidth);
            os << '\n';
            wp.pos = 0;
        }
    }

    return exitCode();
}

//===========================================================================
int Cli::writeUsage(ostream & os, const string & progName) const {
    streampos base = os.tellp();
    os << "usage: " << (progName.empty() ? m_progName : progName);
    WrapPos wp;
    wp.maxWidth = 79;
    wp.pos = os.tellp() - base;
    wp.prefix = string(wp.pos, ' ');
    if (!m_shortNames.empty() || !m_longNames.empty())
        writeToken(os, wp, "[OPTIONS]");
    for (auto && pa : m_argNames) {
        string token = pa.name;
        if (pa.arg->m_multiple)
            token += "...";
        if (pa.optional) {
            writeToken(os, wp, "[<" + token + ">]");
        } else {
            writeToken(os, wp, "<" + token + ">");
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
        if (sn.second.arg != &arg ||
            arg.m_bool && sn.second.invert == enableOptions) {
            continue;
        }
        optional = sn.second.optional;
        if (!list.empty())
            list += ", ";
        list += '-';
        list += sn.first;
    }
    for (auto && ln : m_longNames) {
        if (ln.second.arg != &arg ||
            arg.m_bool && ln.second.invert == enableOptions) {
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
    if (foundLong) {
        list += '=';
    } else {
        list += ' ';
    }
    if (optional) {
        list += "[" + arg.m_valueDesc + "]";
    } else {
        list += arg.m_valueDesc;
    }
    return list;
}
