// argv.cpp - dim cli
//
// For documentation and examples follow the links at:
// https://github.com/gknowles/dimcli

#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Public API
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
    wstring_convert<codecvt<wchar_t, char, mbstate_t>, wchar_t> wcvt(
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
