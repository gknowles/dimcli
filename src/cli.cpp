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

//===========================================================================
// free function
ostream & Dim::operator<<(ostream & os, const Cli::ArgBase & val) {
    return os << val.from();
}


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
    ArgBase * val = src.get();
    m_args.push_back(std::move(src));
    const char * ptr = val->m_names.data();
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
            addArgName(name, val);
        }
        if (!*ptr)
            return;
    }
}

//===========================================================================
void Cli::addLongName(
    const string & src, ArgBase * val, bool invert, bool optional) {
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
    m_longNames[key] = {val, invert, optional};
    if (val->m_bool && allowNo)
        m_longNames["no-" + key] = {val, !invert, optional};
}

//===========================================================================
void Cli::addArgName(const string & name, ArgBase * val) {
    const bool invert = true;
    const bool optional = true;

    switch (name[0]) {
    case '-': assert(name[0] != '-' && "bad argument name"); return;
    case '[':
        m_argNames.push_back({val, !invert, optional, name.data() + 1});
        return;
    case '<':
        auto where =
            find_if(m_argNames.begin(), m_argNames.end(), [](auto && key) {
                return key.optional;
            });
        m_argNames.insert(where, {val, !invert, !optional, name.data() + 1});
        return;
    }
    if (name.size() == 1) {
        m_shortNames[name[0]] = {val, !invert, !optional};
        return;
    }
    switch (name[0]) {
    case '!':
        if (!val->m_bool) {
            assert(!val->m_bool && "bad modifier '!' for non-bool argument");
            return;
        }
        if (name.size() == 2) {
            m_shortNames[name[1]] = {val, invert, !optional};
        } else {
            addLongName(name.substr(1), val, invert, !optional);
        }
        return;
    case '?':
        if (val->m_bool) {
            assert(!val->m_bool && "bad modifier '?' for bool argument");
            return;
        }
        if (name.size() == 2) {
            m_shortNames[name[1]] = {val, !invert, optional};
        } else {
            addLongName(name.substr(1), val, !invert, optional);
        }
        return;
    }
    addLongName(name, val, !invert, !optional);
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
    m_exitCode = EX_OK;
    m_errMsg.clear();
}

//===========================================================================
bool Cli::parseAction(
    ArgBase & val, const std::string & name, const char ptr[]) {
    val.set(name);
    if (ptr) {
        return val.parseAction(*this, ptr);
    } else {
        val.unspecifiedValue();
        return true;
    }
}

//===========================================================================
bool Cli::badUsage(const string & msg) {
    m_errMsg = msg;
    m_exitCode = EX_USAGE;
    return false;
}

//===========================================================================
bool Cli::parse(size_t argc, char * argv[]) {
    resetValues();

    // the 0th (name of this program) arg should always be present
    assert(argc && *argv);

    string name;
    unsigned pos = 0;
    bool moreOpts = true;
    m_progName = *argv;
    argc -= 1;
    argv += 1;

    for (; argc; --argc, ++argv) {
        ArgName vkey;
        const char * ptr = *argv;
        if (*ptr == '-' && ptr[1] && moreOpts) {
            ptr += 1;
            for (; *ptr && *ptr != '-'; ++ptr) {
                auto it = m_shortNames.find(*ptr);
                if (it == m_shortNames.end()) {
                    return badUsage("Unknown option: -"s + *ptr);
                }
                vkey = it->second;
                name = "-"s + *ptr;
                if (vkey.val->m_bool) {
                    if (!parseAction(*vkey.val, name, vkey.invert ? "0" : "1"))
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
            vkey = it->second;
            name = "--" + key;
            if (vkey.val->m_bool) {
                if (equal) {
                    return badUsage("Unknown option: " + name + "=");
                }
                if (!parseAction(*vkey.val, name, vkey.invert ? "0" : "1"))
                    return false;
                continue;
            }
            goto OPTION_VALUE;
        }

        // positional value
        if (pos >= size(m_argNames)) {
            return badUsage("Unexpected argument: "s + ptr);
        }
        vkey = m_argNames[pos];
        name = vkey.name;
        if (!parseAction(*vkey.val, name, ptr))
            return false;
        if (!vkey.val->m_multiple)
            pos += 1;
        continue;

    OPTION_VALUE:
        if (*ptr) {
            if (!parseAction(*vkey.val, name, ptr))
                return false;
            continue;
        }
        if (vkey.optional) {
            if (!parseAction(*vkey.val, name, nullptr))
                return false;
            continue;
        }
        argc -= 1;
        argv += 1;
        if (!argc) {
            return badUsage("No value given for " + name);
        }
        if (!parseAction(*vkey.val, name, *argv))
            return false;
    }

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
        writeDesc(os, wp, pa.val->m_desc, colWidth);
        os << '\n';
        wp.pos = 0;
    }

    // named args
    if (!m_shortNames.empty() || !m_longNames.empty()) {
        os << "Options:\n";
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
        if (pa.val->m_multiple)
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
        if (sn.second.val != &arg ||
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
        if (ln.second.val != &arg ||
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
        list += "[" + arg.m_valueName + "]";
    } else {
        list += arg.m_valueName;
    }
    return list;
}
