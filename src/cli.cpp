// cli.cpp - dim cli
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Cli::ValueBase
*
***/

//===========================================================================
Cli::ValueBase::ValueBase(
    const std::string & names, bool boolean)
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
        val->m_explicit = false;
        val->m_refName.clear();
        val->resetValue();
    }
    for (auto && kv : m_longNames) {
        auto val = kv.second.val;
        val->m_explicit = false;
        val->m_refName.clear();
        val->resetValue();
    }
    for (unsigned i = 0; i < size(m_args); ++i) {
        auto & key = m_args[i];
        if (key.name.empty())
            key.name = "arg" + to_string(i + 1);
        key.val->m_explicit = false;
        key.val->m_refName = key.name;
        key.val->resetValue();
    }
    m_exitCode = EX_OK;
    m_errMsg.clear();
}

//===========================================================================
void Cli::addValue(std::unique_ptr<ValueBase> src) {
    ValueBase * val = src.get();
    m_values.push_back(std::move(src));
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
void Cli::addKey(const string & name, ValueBase * val) {
    const bool invert = true;
    const bool optional = true;

    switch (name[0]) {
    case '-': assert(name[0] != '-' && "bad argument name"); return;
    case '[':
        m_args.push_back({val, !invert, optional, name.data() + 1});
        return;
    case '<':
        auto where = find_if(m_args.begin(), m_args.end(), [](auto && key) {
            return key.optional;
        });
        m_args.insert(where, {val, !invert, !optional, name.data() + 1});
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
bool Cli::parseValue(ValueBase & val, const char ptr[]) {
    val.m_explicit = true;
    return val.parseValue(ptr);
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

    unsigned pos = 0;
    bool moreOpts = true;
    argc -= 1;
    argv += 1;

    for (; argc; --argc, ++argv) {
        ValueKey vkey;
        const char * ptr = *argv;
        if (*ptr == '-' && ptr[1] && moreOpts) {
            ptr += 1;
            for (; *ptr && *ptr != '-'; ++ptr) {
                auto it = m_shortNames.find(*ptr);
                if (it == m_shortNames.end()) {
                    return badUsage("Unknown option: - "s + *ptr);
                }
                vkey = it->second;
                vkey.val->m_refName = "-"s + *ptr;
                if (vkey.val->m_bool) {
                    parseValue(*vkey.val, vkey.invert ? "0" : "1");
                    continue;
                }
                ptr += 1;
                goto option_value;
            }
            if (!*ptr) {
                continue;
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
            vkey.val->m_refName = "--" + key;
            if (vkey.val->m_bool) {
                if (equal) {
                    return badUsage("Unknown option: --" + key + "=");
                }
                parseValue(*vkey.val, (hasNo ^ vkey.invert) ? "0" : "1");
                continue;
            } else if (hasNo) {
                return badUsage("Unknown option: --" + key);
            }
            goto option_value;
        }

        // positional
        if (pos >= size(m_args)) {
            return badUsage("Unexpected argument: "s + ptr);
        }
        vkey = m_args[pos];
        vkey.val->m_refName = vkey.name;
        if (!parseValue(*vkey.val, ptr)) {
            return badUsage("Invalid " + vkey.val->m_refName + ": " + ptr);
        }
        if (!vkey.val->m_multiple)
            pos += 1;
        continue;

    option_value:
        if (*ptr) {
            if (!parseValue(*vkey.val, ptr)) {
                return badUsage("Invalid option value: "s + ptr);
            }
            continue;
        }
        if (vkey.optional) {
            continue;
        }
        argc -= 1;
        argv += 1;
        if (!argc) {
            return badUsage("No value given for " + vkey.val->m_refName);
        }
        if (!parseValue(*vkey.val, *argv)) {
            return badUsage("Invalid option value: "s + *argv);
        }
    }

    if (pos < size(m_args) && !m_args[pos].optional) {
        return badUsage("No value given for " + m_args[pos].name);
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
ostream & Dim::operator<<(ostream & os, const Cli::ValueBase & val) {
    return os << val.name();
}
