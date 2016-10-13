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
*   Cli::ArgBase
*
***/

//===========================================================================
Cli::ArgBase::ArgBase(const std::string & names, bool boolean)
    : m_names{names}
    , m_bool{boolean} {}


/****************************************************************************
*
*   Cli
*
***/

//===========================================================================
void Cli::resetValues() {
    for (auto && kv : m_shortNames) {
        auto val = kv.second.val;
        val->resetValue();
    }
    for (auto && kv : m_longNames) {
        auto val = kv.second.val;
        val->resetValue();
    }
    for (unsigned i = 0; i < size(m_argNames); ++i) {
        auto & key = m_argNames[i];
        if (key.name.empty())
            key.name = "arg" + to_string(i + 1);
        key.val->resetValue();
    }
    m_exitCode = EX_OK;
    m_errMsg.clear();
}

//===========================================================================
void Cli::addValue(std::unique_ptr<ArgBase> src) {
    ArgBase * val = src.get();
    m_args.push_back(std::move(src));
    const char * ptr = val->m_names.data();
    string name;
    char close;
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
            assert(hasEqual && "bad argument name");
        } else {
            name = string(b, ptr - b);
            addKey(name, val);
        }
        if (!*ptr)
            return;
    }
}

//===========================================================================
void Cli::addKey(const string & name, ArgBase * val) {
    const bool invert = true;
    const bool optional = true;

    switch (name[0]) {
    case '-': assert(name[0] != '-' && "bad argument name"); return;
    case '[':
        m_argNames.push_back({val, !invert, optional, name.data() + 1});
        return;
    case '<':
        auto where = find_if(m_argNames.begin(), m_argNames.end(), [](auto && key) {
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
            m_longNames[name.data() + 1] = {val, invert, !optional};
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
            m_longNames[name.data() + 1] = {val, !invert, optional};
        }
        return;
    }
    m_longNames[name] = {val, !invert, !optional};
}

//===========================================================================
bool Cli::parseValue(ArgBase & val, const std::string & name, const char ptr[]) {
    return val.parseValue(name, ptr);
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
                    return badUsage("Unknown option: - "s + *ptr);
                }
                vkey = it->second;
                name = "-"s + *ptr;
                if (vkey.val->m_bool) {
                    parseValue(*vkey.val, name, vkey.invert ? "0" : "1");
                    continue;
                }
                ptr += 1;
                goto OPTION_VALUE;
            }
            if (!*ptr) {
                ptr -= 1;
                goto POSITIONAL_VALUE;
            }
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
            bool hasNo = false;
            while (it == m_longNames.end()) {
                if (key.size() > 3 && key.compare(0, 3, "no-") == 0) {
                    hasNo = true;
                    it = m_longNames.find(key.data() + 3);
                    continue;
                }
                return badUsage("Unknown option: --"s + key);
            }
            vkey = it->second;
            name = "--" + key;
            if (vkey.val->m_bool) {
                if (equal) {
                    return badUsage("Unknown option: " + name + "=");
                }
                parseValue(*vkey.val, name, (hasNo ^ vkey.invert) ? "0" : "1");
                continue;
            } else if (hasNo) {
                return badUsage("Unknown option: " + name);
            }
            goto OPTION_VALUE;
        }

    POSITIONAL_VALUE:
        if (pos >= size(m_argNames)) {
            return badUsage("Unexpected argument: "s + ptr);
        }
        vkey = m_argNames[pos];
        name = vkey.name;
        if (!parseValue(*vkey.val, name, ptr)) {
            return badUsage("Invalid " + name + ": " + ptr);
        }
        if (!vkey.val->m_multiple)
            pos += 1;
        continue;

    OPTION_VALUE:
        if (*ptr) {
            if (!parseValue(*vkey.val, name, ptr)) {
                return badUsage("Invalid option value: " + name + "=" + ptr);
            }
            continue;
        }
        if (vkey.optional) {
            vkey.val->unspecifiedValue(name);
            continue;
        }
        argc -= 1;
        argv += 1;
        if (!argc) {
            return badUsage("No value given for " + name);
        }
        if (!parseValue(*vkey.val, name, *argv)) {
            return badUsage("Invalid option value: " + name + "=" + *argv);
        }
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
*   Public Interface
*
***/

//===========================================================================
ostream & Dim::operator<<(ostream & os, const Cli::ArgBase & val) {
    return os << val.from();
}
