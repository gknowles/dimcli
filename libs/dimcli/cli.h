// Copyright Glen Knowles 2016 - 2024.
// Distributed under the Boost Software License, Version 1.0.
//
// cli.h - dimcli
//
// Command line parser toolkit
//
// Instead of trying to figure it out from just this header please, PLEASE,
// take a moment and look at the documentation and examples:
// https://github.com/gknowles/dimcli

#pragma once


/****************************************************************************
*
*   Configuration
*
***/

// The dimcli_userconfig.h include is defined by the application and may be
// used to configure the standard library before the headers get included.
// This includes macros such as _ITERATOR_DEBUG_LEVEL (Microsoft),
// _LIBCPP_DEBUG (libc++), etc.
#ifdef __has_include
#if __has_include("dimcli_userconfig.h")
#include "dimcli_userconfig.h"
#elif __has_include("cppconf/dimcli_userconfig.h")
#include "cppconf/dimcli_userconfig.h"
#endif
#endif

//---------------------------------------------------------------------------
// Configuration of this installation, these are options that must be the
// same when building the app as when building the library.

// DIMCLI_LIB_DYN_LINK: Forces all libraries that have separate source, to be
// linked as dll's rather than static libraries on Microsoft Windows (this
// macro is used to turn on __declspec(dllimport) modifiers, so that the
// compiler knows which symbols to look for in a dll rather than in a static
// library). Note that there may be some libraries that can only be linked in
// one way (statically or dynamically), in these cases this macro has no
// effect.
//#define DIMCLI_LIB_DYN_LINK

// DIMCLI_LIB_WINAPI_FAMILY_APP: Removes all functions that rely on windows
// WINAPI_FAMILY_DESKTOP mode, such as the console and environment
// variables.
//#define DIMCLI_LIB_WINAPI_FAMILY_APP


//---------------------------------------------------------------------------
// Configuration of the application. These options, if desired, are set by the
// application before including the library headers.

// DIMCLI_LIB_KEEP_MACROS: By default the DIMCLI_LIB_* macros defined
// internally (including in this file) are undef'd so they don't leak out to
// application code. Setting this macro leaves them available for the
// application to use. Also included are other platform specific adjustments,
// such as suppression of specific compiler warnings.

// DIMCLI_LIB_NO_FILESYSTEM: Prevents the <filesystem> header from being
// included and as a side-effect disables support for response files. You can
// try this if you are working with an older compiler with incompatible or
// broken filesystem support.


//===========================================================================
// Internal
//===========================================================================

#if defined(DIMCLI_LIB_SOURCE) && !defined(DIMCLI_LIB_KEEP_MACROS)
#define DIMCLI_LIB_KEEP_MACROS
#endif

#ifdef DIMCLI_LIB_WINAPI_FAMILY_APP
#define DIMCLI_LIB_NO_ENV
#define DIMCLI_LIB_NO_CONSOLE
#endif

#if defined(_MSC_VER)
#ifndef DIMCLI_LIB_KEEP_MACROS
#pragma warning(push)
#endif
// attribute 'identifier' is not recognized
#pragma warning(disable : 5030)
#elif defined(__clang__)
#ifndef DIMCLI_LIB_KEEP_MACROS
#pragma clang diagnostic push
#endif
#pragma clang diagnostic ignored "-Wlogical-op-parentheses"
#elif defined(__GNUC__)
#ifndef DIMCLI_LIB_KEEP_MACROS
#pragma GCC diagnostic push
#endif
#endif

#ifndef DIMCLI_LIB_DYN_LINK
#define DIMCLI_LIB_DECL
#else
#if defined(_MSC_VER)
// 'identifier': class 'type' needs to have dll-interface to be used
// by clients of class 'type2'
#pragma warning(disable : 4251)
#endif

#if defined(_WIN32)
#ifdef DIMCLI_LIB_SOURCE
#define DIMCLI_LIB_DECL __declspec(dllexport)
#else
#define DIMCLI_LIB_DECL __declspec(dllimport)
#endif
#endif
#endif

#if !defined(_CPPUNWIND) && !defined(_HAS_EXCEPTIONS)
#define _HAS_EXCEPTIONS 0
#endif


/****************************************************************************
*
*   Includes
*
***/

#include <cassert>
#include <cmath>
#include <cstddef>
#include <functional>
#include <limits>
#include <list>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#ifndef DIMCLI_LIB_NO_FILESYSTEM
#if defined(_MSC_VER) && _MSC_VER < 1914
#include <experimental/filesystem>
#define DIMCLI_LIB_FILESYSTEM std::experimental::filesystem
#define DIMCLI_LIB_FILESYSTEM_PATH std::experimental::filesystem::path
#elif defined(__has_include)
#if __has_include(<filesystem>)
#include <filesystem>
#define DIMCLI_LIB_FILESYSTEM std::filesystem
#define DIMCLI_LIB_FILESYSTEM_PATH std::filesystem::path
#elif __has_include(<experimental/filesystem>)
#include <experimental/filesystem>
#define DIMCLI_LIB_FILESYSTEM std::experimental::filesystem
#define DIMCLI_LIB_FILESYSTEM_PATH std::experimental::filesystem::path
#endif
#endif
#endif


namespace Dim {

#ifdef DIMCLI_LIB_BUILD_COVERAGE
using AssertHandlerFn = void(*)(const char expr[], unsigned line);
AssertHandlerFn setAssertHandler(AssertHandlerFn fn);
void doAssert(const char expr[], unsigned line);

#undef assert
#define assert(expr) \
    (void) ( (!!(expr)) || (Dim::doAssert(#expr, unsigned(__LINE__)), 0) )
#endif


/****************************************************************************
*
*   Exit codes
*
***/

// These mirror the program exit codes defined in <sysexits.h> on Unix.
enum {
    kExitOk = 0,        // EX_OK
    kExitUsage = 64,    // EX_USAGE     - bad command line
    kExitSoftware = 70, // EX_SOFTWARE  - internal software error
};


/****************************************************************************
*
*   Cli
*
***/

class DIMCLI_LIB_DECL Cli {
public:
    struct Config;
    class Convert;

    class OptBase;
    template <typename A, typename T> class OptShim;
    template <typename T> class Opt;
    template <typename T> class OptVec;
    struct OptIndex;

    struct ArgMatch;
    template <typename T> struct Value;
    template <typename T> struct ValueVec;

public:
    // Creates a handle to the shared command line configuration, this
    // indirection allows options to be statically registered from multiple
    // source files.
    Cli();
    Cli(const Cli & from);
    Cli & operator=(const Cli & from);
    Cli & operator=(Cli && from) noexcept;

    //-----------------------------------------------------------------------
    // CONFIGURATION

    // The 'names' parameter is a whitespace seperated list of option and
    // operand names, names take one of four forms and may have prefix/suffix
    // modifiers.
    //
    //      Type of name                            Example
    // short name (single character)                f or (f)
    // long name (more than one character)          file or (file)
    // optional operand (within square brackets)    [file name]
    // required operand (within angled brackets)    <file>
    //
    // Prefix modifiers
    //  !   For boolean options, when setting the value it is first inverted.
    //  ?   For non-boolean options, makes the value optional.
    //  *   For non-boolean options, makes it multivalued. If no attached value
    //      the following arguments, up to the next option, are included as
    //      values.
    //
    // Suffix modifiers
    //  .   For boolean options with long names, suppresses the addition of the
    //      "no-" version.
    //  !   For options and operands, final option, all following arguments
    //      will be positional operands.
    //
    // Additional rules
    // - Names of operands (inside angled or square brackets) may contain
    //   whitespace.
    // - Option names must start and end with an alpha numeric character, be
    //   enclosed in parentheses, or be a single character without modifiers
    //   other than '(', '[', and '<'.
    // - Within parentheses a ')' pair is treated as a literal ')' and doesn't
    //   close the parenthetical. Within angled and square brackets the closing
    //   char (']' or '>') can be escaped in the same way.
    // - Long names for boolean options get a second "no-" version implicitly
    //   created for them.
    //
    // Examples
    //  "f file"    Short name 'f' and long name "file"
    //  "f [file]"  Short name 'f' and optional operand
    //  "!"         Short name '!'
    //  "?!", "!!." Error - no name, only modifiers
    //  "?(!)"      Short name '!' with optional value
    //  "(!!)."     Long name "!!", without "no-!!" version
    //  "?a.b.c."   Long name "a.b.c" with option value and without "no-"
    //  "())) ([)"  Short names ')' and '['
    template <typename T>
    Opt<T> & opt(const std::string & names, const T & def = {});

    template <typename T,
        typename = typename std::enable_if<!std::is_const<T>::value>::type>
    Opt<T> & opt(T * value, const std::string & names);

    template <typename T, typename U,
        typename = typename
            std::enable_if<std::is_convertible<U, T>::value>::type,
        typename = typename std::enable_if<!std::is_const<T>::value>::type>
    Opt<T> & opt(T * value, const std::string & names, const U & def);

    template <typename T>
    Opt<T> & opt(Opt<T> & value, const std::string & names);

    template <typename T, typename U,
        typename = typename
            std::enable_if<std::is_convertible<U, T>::value>::type>
    Opt<T> & opt(Opt<T> & value, const std::string & names, const U & def);

    template <typename T>
    OptVec<T> & optVec(const std::string & names);

    template <typename T>
    OptVec<T> & optVec(std::vector<T> * values, const std::string & names);

    template <typename T>
    OptVec<T> & optVec(OptVec<T> & values, const std::string & names);

    // Add -y, --yes option that exits early when false and has an "are you
    // sure?" style prompt when it's not present.
    Opt<bool> & confirmOpt(const std::string & prompt = {});

    // Get reference to internal help option, can be used to change the
    // description, option group, etc. To completely replace it, add another
    // option that responds to --help.
    Opt<bool> & helpOpt();

    // Add --password option and prompts for a password if it's not given
    // on the command line. If confirm is true and it's not on the command
    // line you have to enter it twice.
    Opt<std::string> & passwordOpt(bool confirm = false);

    // Add --version option that shows "${progName.filename()} version ${ver}"
    // and exits. An empty progName defaults to argv[0].
    Opt<bool> & versionOpt(
        const std::string & ver,
        const std::string & progName = {}
    );

    //-----------------------------------------------------------------------
    // A group collects options into sections in the help text. Options are
    // always added to a group, either the default group of the cli (or of the
    // selected command), or an explicitly created one.

    // Changes config context to point at the selected option group of the
    // current command, that you can then start stuffing things into.
    Cli & group(const std::string & name) &;
    Cli && group(const std::string & name) &&;

    // Heading title to display, defaults to group name. If empty there will
    // be a single blank line separating this group from the previous one.
    Cli & title(const std::string & val) &;
    Cli && title(const std::string & val) &&;

    // Option groups are sorted by key, defaults to group name.
    Cli & sortKey(const std::string & key) &;
    Cli && sortKey(const std::string & key) &&;

    const std::string & group() const { return m_group; }
    const std::string & title() const;
    const std::string & sortKey() const;

    //-----------------------------------------------------------------------
    // Changes config context to reference an option group of the selected
    // command. Use an empty string to specify the top level context. If a new
    // command is selected it is created in the command group of the current
    // context.
    Cli & command(const std::string & name, const std::string & group = {}) &;
    Cli && command(
        const std::string & name,
        const std::string & group = {}
    ) &&;

    // Function signature of actions that are tied to commands.
    using ActionFn = void(Cli & cli);

    // Action that should be taken when the currently selected command is run,
    // which happens when cli.exec() is called by the application. The
    // command's action function should:
    //  - For parsing errors not caught by parse(), such as complex
    //    interactions between arguments; call cli.badUsage() and return. Also
    //    consider an after action instead.
    //  - If no action was really attempted, as when only printing help text
    //    or a version string; call cli.parseExit() and return.
    //  - Do something useful
    //  - Call cli.fail() to set exit code and error message for other errors.
    // Generally, only use badUsage() or parseExit() when responding like an
    // extension of parse time processing.
    //
    // If the process should exit but there may still be asynchronous work
    // going on, consider a custom "exit pending" exit code with special
    // handling in main to wait for it to complete.
    Cli & action(std::function<ActionFn> fn) &;
    Cli && action(std::function<ActionFn> fn) &&;

    // Arbitrary text can be added to the help text for each command, this text
    // can come before the usage (header), immediately after the usage (desc),
    // or after the options (footer).
    //
    // Unless individually overridden, commands default to using the header and
    // footer (but not the desc) specified at the top level.
    //
    // The text is run through cli.printText(), so use line breaks only for
    // paragraph breaks, and let the automatic line wrapping take care of the
    // rest.
    Cli & header(const std::string & val) &;
    Cli && header(const std::string & val) &&;
    Cli & desc(const std::string & val) &;
    Cli && desc(const std::string & val) &&;
    Cli & footer(const std::string & val) &;
    Cli && footer(const std::string & val) &&;

    const std::string & command() const { return m_command; }
    const std::string & header() const;
    const std::string & desc() const;
    const std::string & footer() const;

    // Makes all arguments following the command appear in cli.unknownArgs()
    // instead of populating any defined options/operands. At the top level it
    // also supercedes subcommands.
    Cli & unknownArgs(bool enable) &;
    Cli && unknownArgs(bool enable) &&;

    // Add "help" command that shows the help text for other commands. Allows
    // users to run "prog help command" instead of the slightly more awkward
    // "prog command --help".
    Cli & helpCmd() &;
    Cli && helpCmd() &&;

    // Allows unknown subcommands, and sets either a default action, which
    // errors out, or a custom action to run when there is an unknown command.
    // Use cli.commandMatched() and cli.unknownArgs() to determine the command
    // and it's arguments.
    Cli & unknownCmd(std::function<ActionFn> fn = {}) &;
    Cli && unknownCmd(std::function<ActionFn> fn = {}) &&;

    // Adds before action that replaces the command line with "--help" when
    // it's empty.
    Cli & helpNoArgs() &;
    Cli && helpNoArgs() &&;

    //-----------------------------------------------------------------------
    // A command group collects commands into sections in the help text, in the
    // same way that option groups do with options.

    // Changes the command group of the current command. Because new commands
    // start out in the same group as the current command, it can be convenient
    // to create all the commands of one group before moving to the next.
    //
    // Setting the command group at the top level (the "" command) only serves
    // to set the initial command group for new commands created while in the
    // top level context.
    Cli & cmdGroup(const std::string & name) &;
    Cli && cmdGroup(const std::string & name) &&;

    // Heading title to display, defaults to group name. If empty there will be
    // a single blank line separating this group from the previous one.
    Cli & cmdTitle(const std::string & val) &;
    Cli && cmdTitle(const std::string & val) &&;

    // Command groups are sorted by key, defaults to group name.
    Cli & cmdSortKey(const std::string & key) &;
    Cli && cmdSortKey(const std::string & key) &&;

    const std::string & cmdGroup() const;
    const std::string & cmdTitle() const;
    const std::string & cmdSortKey() const;

    //-----------------------------------------------------------------------
    // Function signature of actions that run before options are populated.
    using BeforeFn = void(Cli & cli, std::vector<std::string> & args);

    // Actions taken after environment variable and response file expansion
    // but before any individual arguments are parsed. The before action
    // function should:
    //  - inspect and possibly modify the raw arguments coming in
    //  - call badUsage() for errors
    //  - call parseExit() if parsing should stop, but there was no error
    Cli & before(std::function<BeforeFn> fn) &;
    Cli && before(std::function<BeforeFn> fn) &&;

#if !defined(DIMCLI_LIB_NO_ENV)
    // Environment variable to get initial options from. Defaults to the empty
    // string, but when set the content of the named variable is parsed into
    // args which are then inserted into the argument list right after arg0.
    Cli & envOpts(const std::string & envVar) &;
    Cli && envOpts(const std::string & envVar) &&;
#endif

    // Change the column at which errors and help text wraps. When there is a
    // second column for descriptions (the first being argument, command, or
    // option names) it's position is equal to the length needed for the
    // longest name clamped to within the given description column min/max.
    //
    // The width cannot be set to less than 20, and out of range values of
    // min(max)DescCol are ignored.
    //
    // By default min(max)DescCol are derived from width, and width defaults to
    // the width of the console clamped to be within 50 to 80 columns.
    Cli & maxWidth(int width, int minDescCol = 0, int maxDescCol = 0) &;
    Cli && maxWidth(int width, int minDescCol = 0, int maxDescCol = 0) &&;

    // Enabled by default, response file expansion replaces arguments of the
    // form "@file" with the contents of the named file.
    Cli & responseFiles(bool enable = true) &;
    Cli && responseFiles(bool enable = true) &&;

    // Changes the streams used for prompting, printing help messages, etc.
    // Mainly intended for testing. Setting to null restores the defaults
    // which are cin and cout respectively.
    Cli & iostreams(std::istream * in, std::ostream * out) &;
    Cli && iostreams(std::istream * in, std::ostream * out) &&;
    std::istream & conin();
    std::ostream & conout();

    //-----------------------------------------------------------------------
    // RENDERING HELP TEXT
    //
    // NOTE: The print*() family of methods return incomplete or meaningless
    //       results when used before parse() has been called to supply the
    //       program name and finalize the configuration. The exception is
    //       printText(), which only uses console width, and is safe to use
    //       without first calling parse(). To learn about printText(), see
    //       the section on rendering arbitrary text.

    // If exitCode() is not EX_OK, prints errMsg and errDetail (if present),
    // otherwise prints nothing. Returns exitCode(). Only makes sense after
    // parsing has completed.
    int printError(std::ostream & os);

    // printHelp() & printUsage() return the current exitCode().
    int printHelp(
        std::ostream & os,
        const std::string & progName = {},
        const std::string & cmd = {}
    );
    int printUsage(
        std::ostream & os,
        const std::string & progName = {},
        const std::string & cmd = {}
    );
    // Same as printUsage(), except always lists all non-default options
    // individually instead of the [OPTIONS] catchall.
    int printUsageEx(
        std::ostream & os,
        const std::string & progName = {},
        const std::string & cmd = {}
    );

    void printCommands(std::ostream & os);
    void printOperands(
        std::ostream & os,
        const std::string & cmd = {}
    );
    void printOptions(std::ostream & os, const std::string & cmd = {});

    // Friendly name for type used in help text, such as NUM, VALUE, or FILE.
    template <typename T>
    static std::string valueDesc();

    //-----------------------------------------------------------------------
    // PARSING

    // Parse the command line, populate the options, and set the error and
    // other miscellaneous state. Returns true if processing should continue.
    //
    // Error information can also be extracted after parse() completes, see
    // errMsg() and friends.
    [[nodiscard]] bool parse(size_t argc, char * argv[]);

    // "args" is non-const so that response files can be expanded in place and
    // before actions can modify it.
    [[nodiscard]] bool parse(std::vector<std::string> & args);
    [[nodiscard]] bool parse(std::vector<std::string> && args);

    // Sets all options to their defaults, called internally when parsing
    // starts.
    Cli & resetValues() &;
    Cli && resetValues() &&;

    // Parse cmdline into vector of args, using the default conventions
    // (Gnu or Windows) of the platform.
    static std::vector<std::string> toArgv(const std::string & cmdline);
    // Copy array of pointers into vector of args.
    static std::vector<std::string> toArgv(size_t argc, char * argv[]);
    static std::vector<std::string> toArgv(size_t argc, const char * argv[]);
    // Copy array of wchar_t pointers into vector of UTF-8 encoded args.
    static std::vector<std::string> toArgv(size_t argc, wchar_t * argv[]);
    static std::vector<std::string> toArgv(
        size_t argc,
        const wchar_t * argv[]
    );
    // Copy args into vector of args. Arguments must be convertible to string
    // via Convert::toString().
    template <typename ...Args>
    static std::vector<std::string> toArgvL(Args &&... args);

    // Create vector of pointers suitable for use with argc/argv APIs, has a
    // trailing null that is not included in size(). The return values point
    // into the source string vector and are only valid until that vector is
    // resized or destroyed.
    static std::vector<const char *> toPtrArgv(
        const std::vector<std::string> & args
    );

    // Parse according to glib conventions, based on the UNIX98 shell spec.
    static std::vector<std::string> toGlibArgv(const std::string & cmdline);
    // Parse using GNU conventions, same rules as buildargv().
    static std::vector<std::string> toGnuArgv(const std::string & cmdline);
    // Parse using Windows conventions.
    static std::vector<std::string> toWindowsArgv(const std::string & cmdline);

    // Join arguments into a single command line, escaping as needed, that
    // parses back into those same arguments. Uses the default conventions (Gnu
    // or Windows) of the platform.
    static std::string toCmdline(const std::vector<std::string> & args);
    // Join array of pointers into command line, escaping as needed.
    static std::string toCmdline(size_t argc, char * argv[]);
    static std::string toCmdline(size_t argc, const char * argv[]);
    static std::string toCmdline(size_t argc, wchar_t * argv[]);
    static std::string toCmdline(size_t argc, const wchar_t * argv[]);
    // Join arguments into command line, escaping as needed. Arguments must be
    // convertible to string via Convert::toString().
    template <typename ...Args>
    static std::string toCmdlineL(Args &&... args);

    // Join according to glib conventions, based on the UNIX98 shell spec.
    static std::string toGlibCmdline(const std::vector<std::string> & args);
    static std::string toGlibCmdline(size_t argc, char * argv[]);
    template <typename ...Args>
    static std::string toGlibCmdlineL(Args &&... args);
    // Join using GNU conventions, same rules as buildargv().
    static std::string toGnuCmdline(const std::vector<std::string> & args);
    static std::string toGnuCmdline(size_t argc, char * argv[]);
    template <typename ...Args>
    static std::string toGnuCmdlineL(Args &&... args);
    // Join using Windows conventions.
    static std::string toWindowsCmdline(const std::vector<std::string> & args);
    static std::string toWindowsCmdline(size_t argc, char * argv[]);
    template <typename ...Args>
    static std::string toWindowsCmdlineL(Args &&... args);

    //-----------------------------------------------------------------------
    // Support functions for use from parsing actions

    // Intended for use in action callbacks. Sets exitCode (to EX_USAGE),
    // errMsg, and errDetail. errMsg is set to "<prefix>: <value>" or
    // "<prefix>" if value.empty(), with an additional leading prefix of
    // "Command: '<command>'" for subcommands.
    void badUsage(
        const std::string & prefix,
        const std::string & value = {},
        const std::string & detail = {}
    );

    // Calls badUsage(prefix, value, detail) with prefix set to:
    //  "Invalid '" + opt.from() + "' value"
    void badUsage(
        const OptBase & opt,
        const std::string & value,
        const std::string & detail = {}
    );

    // Calls badUsage with "Out of range" message and the low and high in the
    // detail.
    template <typename A, typename T>
    void badRange(
        A & opt,
        const std::string & val,
        const T & low,
        const T & high
    );

    // Intended for use in action callbacks. Stops parsing, sets exitCode to
    // EX_OK, and causes an in progress cli.parse() and cli.exec() to return
    // false.
    void parseExit();

    // Used to populate an option with an arbitrary input string through the
    // standard parsing logic. Since it causes the parse and check actions to
    // be called care must be taken to avoid infinite recursion if used from
    // those actions.
    [[nodiscard]] bool parseValue(
        OptBase & out,
        const std::string & name,
        size_t pos,
        const char src[]
    );

    // fUnit* flags modify how unit suffixes are interpreted by opt.siUnits(),
    // opt.timeUnits(), and opt.anyUnits().
    enum {
        // When this flag is set the symbol must be specified and it's
        // absence in option values makes them invalid arguments.
        fUnitRequire = 1,

        // Makes units case insensitive. For siUnits() unit prefixes are
        // also case insensitive, but fractional prefixes (milli, micro, etc)
        // are prohibited because they would clash with mega and peta.
        fUnitInsensitive = 2,

        // Makes k,M,G,T,P factors of 1024, same as ki,Mi,Gi,Ti,Pi. Factional
        // unit prefixes (milli, micro, etc) are prohibited.
        fUnitBinaryPrefix = 4,
    };

    // Prompt sends a prompt message to cout and read a response from cin
    // (unless cli.iostreams() changed the streams to use), the response is
    // then passed to cli.parseValue() to set the value and run any actions.
    enum {
        fPromptHide = 1,      // Hide user input as they type
        fPromptConfirm = 2,   // Make the user enter it twice
        fPromptNoDefault = 4, // Don't include default value in prompt
    };
    void prompt(
        OptBase & opt,
        const std::string & msg,
        int flags // fPrompt* flags
    );

    //-----------------------------------------------------------------------
    // AFTER PARSING

    int exitCode() const;
    const std::string & errMsg() const;

    // Additional information to help the user correct their mistake, may be
    // empty.
    const std::string & errDetail() const;

    // Program name received in argv[0]
    const std::string & progName() const;

    // Command to run, as determined by the arguments, empty string if there
    // are no commands defined or none were matched.
    const std::string & commandMatched() const;

    // If commands are defined, even just unknownCmd(), and the matched command
    // is unknown, the unknownArgs vector is populated with the all arguments
    // that follow the command. Including any that started with "-", as if "--"
    // had been given.
    const std::vector<std::string> & unknownArgs() const;

    // Executes the action of the matched command and propagates it's return
    // value back to he caller. On failure the action is expected to have set
    // exitCode, errMsg, and optionally errDetail by calling fail(). If no
    // command was matched the action of the empty "" command is run, which
    // defaults to failing with "No command given." but can be set using
    // cli.action() like any other command.
    //
    // To be consistent with cli.parse() the action should return false if it
    // ends immediately, such as a usage error, printing help text, or a version
    // string. Otherwise, if the action was really attempted, return true.
    //
    // It is assumed that a prior call to parse() has already been made to set
    // the matched command.
    bool exec();

    // Helpers to parse and, if successful, execute.
    bool exec(size_t argc, char * argv[]);
    bool exec(std::vector<std::string> & args);

    // Sets exitCode(), errMsg(), errDetail(). Intended to be called from
    // command actions, parsing related failures should use badUsage() instead.
    void fail(
        int code,
        const std::string & msg,
        const std::string & detail = {}
    );

    // Returns true if the named command has been defined, used by the help
    // command implementation. Not reliable before cli.parse() has been called
    // and had a chance to update the internal data structures.
    bool commandExists(const std::string & name) const;

    //-----------------------------------------------------------------------
    // RENDERING ARBITRARY TEXT
    //
    // NOTE: This text rendering method is not needed for applications to
    //       process command lines but may be useful to command line programs
    //       that output text messages.
    //
    // Write text and simple tables, wrapping as needed. The text is split on
    // \n into lines, and each line is processed as either a paragraph (if
    // there are no \t chars) or table row (if there are \t chars). Formatting
    // is modified by embedding special characters in the text.
    //
    // PARAGRAPHS
    // A paragraph consists of a preamble followed by the body. The preamble
    // contains any number of the following and ends at the first character
    // that is something else:
    //  - \r    Decrease indent of wrapped text.
    //  - \v    Increase indent of wrapped text.
    //  - SP    Increase indent of first line of paragraph or column text.
    //
    // cli.maxWidth(50); // These examples assume console width of 50.
    // cli.printText(cout,
    //   "Default paragraph wrapped at column 50 with default indentation.\n"
    //   "  \r\rIndented paragraph with all following lines unindented.\n"
    //   "\v\vParagraph with all lines but the very first indented.\n");
    // ----
    // Default paragraph wrapped at column 50 with
    // default indentation.
    //   Indented paragraph with all following lines
    // unindented.
    // Paragraph with all lines but the very first
    //   indented.
    // ----
    //
    // The body of a paragraph consists of space separated tokens (consecutive
    // spaces are treated as one). Line breaks are added between tokens as
    // needed. The following characters have special meaning in the body:
    //  - \b    Non-breaking space.
    //
    // "The quick brown fox jumped underneath the lazy dog.\n"
    // "The quick brown fox jumped underneath the lazy\bdog.\n"
    // ----
    // The quick brown fox jumped underneath the lazy
    // dog.
    // The quick brown fox jumped underneath the
    // lazy dog.
    // ----
    //
    // TABLES
    // All lines containing one or more \t characters are table rows. Tables
    // are made up of rows grouped by the first column indent and then split by
    // those with the \f (new table) flag. Columns are just additional
    // paragraphs with larger indentation. In other words, column width is only
    // used to find the starting position of the next column.
    //
    // Additional special phrases in column preamble:
    //  - \a<MIN> <MAX>\a
    //          Set min and max widths of a table column, where MIN and MAX
    //          are percentages of console width encoded as floats. Used in
    //          columns of a row that is marked with \f (new table).
    //  - \f    Start of new table, allowed in preamble of any or all columns.
    //
    // Tables can be interleaved.
    //
    // "Table A, Row I\tOnly 0 indent table\n"
    // "  Table B, Row I\tFirst 2 indent table\n"
    // "Table A, Row II\tOnly 0 indent table\n"
    // "  \fTable C, Row I\tNew 2 indent table (because\bof\b\\f)\n"
    // ----
    // Table A, Row I   Only 0 indent table
    //   Table B, Row I  First 2 indent table
    // Table A, Row II  Only 0 indent table
    //   Table C, Row I  New 2 indent table
    //                   (because of \f)
    // ----
    //
    // Text never wraps until the end of the console window.
    //
    // "This is first column text that extends to the following line.\t"
    // "Second column, also with enough text to wrap all the way around.\t"
    // "Third and final column, also wrapping.\n"
    // ----
    // This is first column text that extends to the
    // following line.
    //           Second column, also with enough text
    //           to wrap all the way around.
    //                     Third and final column, also
    //                     wrapping.
    // ----
    //
    // Column width is calculated by finding the longest text of any cell in
    // that column of the table that doesn't exceed the column's max width with
    // a minimum of the min width. Default min/max column width is 15%/38% for
    // the first and 15%/15% for the rest. The default for the first column can
    // be changed with cli.maxWidth().
    //
    // "\f\a10 10\aone\tSet column width to 5 (10% of 50).\n"
    // "two\tSecond row.\n"
    // "fifteen\tToo long for column width, pushed down.\n"
    // ----
    // one    Set column width to 5 (10% of 50).
    // two    There is always an extra two space gap
    //        between columns.
    // seventeen
    //        Too long for column width, pushed down.
    // ----
    void printText(std::ostream & os, const std::string & text);

    //-----------------------------------------------------------------------
    // HELPERS

    // Returns false if echo was unable to be set.
    static bool consoleEnableEcho(bool enable = true);
    // Returns the default when queryWidth is false.
    static unsigned consoleWidth(bool queryWidth = true);

protected:
    Cli(std::shared_ptr<Config> cfg);

private:
    static void defParseAction(
        Cli & cli,
        OptBase & opt,
        const std::string & val
    );
    static void requireAction(
        Cli & cli,
        OptBase & opt,
        const std::string & val
    );

    static std::vector<std::pair<std::string, double>> siUnitMapping(
        const std::string & symbol,
        int flags
    );

    void addOpt(std::unique_ptr<OptBase> opt);
    template <typename A> A & addOpt(std::unique_ptr<A> ptr);

    template <typename A, typename V, typename T>
    std::shared_ptr<V> getProxy(T * ptr);

    // Find an option (from any subcommand) that targets the value.
    OptBase * findOpt(const void * value);

    bool parseExited() const;

    std::shared_ptr<Config> m_cfg;
    std::string m_group;
    std::string m_command;
};

//===========================================================================
template <typename T, typename U, typename, typename>
Cli::Opt<T> & Cli::opt(
    T * value,
    const std::string & names,
    const U & def
) {
    auto proxy = getProxy<Opt<T>, Value<T>>(value);
    auto ptr = std::make_unique<Opt<T>>(proxy, names);
    ptr->defaultValue(def);
    return addOpt(std::move(ptr));
}

//===========================================================================
template <typename T, typename>
Cli::Opt<T> & Cli::opt(T * value, const std::string & names) {
    return opt(value, names, T{});
}

//===========================================================================
template <typename T>
Cli::OptVec<T> & Cli::optVec(
    std::vector<T> * values,
    const std::string & names
) {
    auto proxy = getProxy<OptVec<T>, ValueVec<T>>(values);
    auto ptr = std::make_unique<OptVec<T>>(proxy, names);
    return addOpt(std::move(ptr));
}

//===========================================================================
template <typename T, typename U, typename>
Cli::Opt<T> & Cli::opt(
    Opt<T> & alias,
    const std::string & names,
    const U & def
) {
    return opt(&*alias, names, def);
}

//===========================================================================
template <typename T>
Cli::Opt<T> & Cli::opt(Opt<T> & alias, const std::string & names) {
    return opt(&*alias, names, T{});
}

//===========================================================================
template <typename T>
Cli::OptVec<T> & Cli::optVec(OptVec<T> & alias, const std::string & names) {
    return optVec(&*alias, names);
}

//===========================================================================
template <typename T>
Cli::Opt<T> & Cli::opt(const std::string & names, const T & def) {
    return opt<T>(nullptr, names, def);
}

//===========================================================================
template <typename T>
Cli::OptVec<T> & Cli::optVec(const std::string & names) {
    return optVec<T>(nullptr, names);
}

//===========================================================================
template <typename A, typename T>
void Cli::badRange(
    A & opt,
    const std::string & val,
    const T & low,
    const T & high
) {
    auto prefix = "Out of range '" + opt.from() + "' value";
    std::string detail, lstr, hstr;
    if (opt.toString(lstr, low)) {
        if (opt.toString(hstr, high))
            detail = "Must be between '" + lstr + "' and '" + hstr + "'.";
    }
    badUsage(prefix, val, detail);
}

//===========================================================================
template <typename A>
A & Cli::addOpt(std::unique_ptr<A> ptr) {
    auto & opt = *ptr;
    opt.parse(&Cli::defParseAction).command(command()).group(group());
    addOpt(std::unique_ptr<OptBase>(ptr.release()));
    return opt;
}

//===========================================================================
template <typename A, typename V, typename T>
std::shared_ptr<V> Cli::getProxy(T * ptr) {
    if (OptBase * opt = findOpt(ptr)) {
        auto dopt = static_cast<A *>(opt);
        return dopt->m_proxy;
    }

    // Since there was no existing proxy to the raw value, create one.
    return std::make_shared<V>(ptr);
}

//===========================================================================
// valueDesc - descriptive name for values of type T
//===========================================================================
template <typename T>
// static
std::string Cli::valueDesc() {
    if (std::is_integral<T>::value) {
        return "NUM";
    } else if (std::is_floating_point<T>::value) {
        return "FLOAT";
    } else if (std::is_convertible<T, std::string>::value) {
        return "STRING";
    } else {
        return "VALUE";
    }
}

#ifdef DIMCLI_LIB_FILESYSTEM
//===========================================================================
template <>
inline // static
std::string Cli::valueDesc<DIMCLI_LIB_FILESYSTEM_PATH>() {
    return "FILE";
}
#endif


/****************************************************************************
*
*   CliLocal
*
*   Constructs new cli instance independent of the shared configuration.
*   Mainly for testing.
*
***/

class DIMCLI_LIB_DECL CliLocal : public Cli {
public:
    CliLocal();
};


/****************************************************************************
*
*   Cli::Convert
*
*   Class for converting between values and strings
*
***/

class DIMCLI_LIB_DECL Cli::Convert {
public:
    // Converts from string to T.
    template <typename T>
    [[nodiscard]] bool fromString(T & out, const std::string & value) const;

    // Converts to string from T. Sets to empty string and returns false if
    // conversion fails or no conversion available.
    template <typename T>
    [[nodiscard]] bool toString(std::string & out, const T & src) const;

protected:
    mutable std::stringstream m_interpreter;

private:
    template <typename T>
    auto fromString_impl(T & out, const std::string & src, int, int, int) const
        -> decltype(out = src, bool());

    template <typename T, typename = typename
        std::enable_if<std::is_constructible<T, std::string>::value>::type>
    bool fromString_impl(
        T & out,
        const std::string & src,
        int, int, long
    ) const;

    template <typename T>
    auto fromString_impl(
        T & out,
        const std::string & src,
        int, long, long
    ) const -> decltype(std::declval<std::istream &>() >> out, bool());

    template <typename T>
    bool fromString_impl(
        T & out,
        const std::string & src,
        long, long, long
    ) const;

    template <typename T>
    auto toString_impl(std::string & out, const T & src, int) const
        -> decltype(std::declval<std::ostream &>() << src, bool());

    template <typename T>
    bool toString_impl(std::string & out, const T & src, long) const;
};

//===========================================================================
template <typename T>
[[nodiscard]] bool Cli::Convert::fromString(
    T & out,
    const std::string & src
) const {
    // Versions of fromString_impl taking ints as extra parameters are
    // preferred by the compiler (better conversion from 0), if they don't
    // exist for T (because, for example, no out=src assignment operator
    // exists) then the versions taking longs are considered.
    return fromString_impl(out, src, 0, 0, 0);
}

//===========================================================================
template <typename T>
auto Cli::Convert::fromString_impl(
    T & out,
    const std::string & src,
    int, int, int
) const
    -> decltype(out = src, bool())
{
    out = src;
    return true;
}

//===========================================================================
template <typename T, typename>
bool Cli::Convert::fromString_impl(
    T & out,
    const std::string & src,
    int, int, long
) const {
    out = T{src};
    return true;
}

//===========================================================================
template <typename T>
auto Cli::Convert::fromString_impl(
    T & out,
    const std::string & src,
    int, long, long
) const
    -> decltype(std::declval<std::istream &>() >> out, bool())
{
    m_interpreter.clear();
    m_interpreter.str(src);
    if (!(m_interpreter >> out) || !(m_interpreter >> std::ws).eof()) {
        out = {};
        return false;
    }
    return true;
}

//===========================================================================
template <typename T>
bool Cli::Convert::fromString_impl(
    T &, // out
    const std::string &, // src
    long, long, long
) const {
    // In order to parse an argument one of the following must exist:
    //  - assignment operator to T accepting const std::string&
    //  - constructor of T accepting const std::string&
    //  - std::istream extraction operator for T
    //  - specialization of Cli::Convert::fromString template for T, such as:
    //      template<> bool Cli::Convert::fromString<MyType>(
    //          MyType & out, const std::string & src) const { ... }
    //  - parse action attached to the Opt<T> instance that does NOT call
    //    opt.parseValue(), such as opt.choice().
    assert(!"Unusable type, no conversion from string exists.");
    return false;
}

//===========================================================================
template <typename T>
[[nodiscard]] bool Cli::Convert::toString(
    std::string & out,
    const T & src
) const {
    return toString_impl(out, src, 0);
}

//===========================================================================
template <typename T>
auto Cli::Convert::toString_impl(
    std::string & out,
    const T & src,
    int
) const
    -> decltype(std::declval<std::ostream &>() << src, bool())
{
    m_interpreter.clear();
    m_interpreter.str({});
    if (!(m_interpreter << src)) {
        out.clear();
        return false;
    }
    out = m_interpreter.str();
    return true;
}

//===========================================================================
template <typename T>
bool Cli::Convert::toString_impl(
    std::string & out,
    const T &,
    long
) const {
    out.clear();
    return false;
}


/****************************************************************************
*
*   Cli
*
*   Additional members that must come after Cli::Convert is fully declared.
*
***/

//===========================================================================
template <typename ...Args>
std::vector<std::string> Cli::toArgvL(Args &&... args) {
    std::string tmp;
    Convert cvt;
    std::vector<std::string> out = {
        ((void) cvt.toString(tmp, std::forward<Args>(args)), tmp)...
    };
    return out;
}

//===========================================================================
template <typename ...Args>
std::string Cli::toCmdlineL(Args &&... args) {
    return toCmdline(toArgvL(std::forward<Args>(args)...));
}

//===========================================================================
template <typename ...Args>
std::string Cli::toGlibCmdlineL(Args &&... args) {
    return toGlibCmdline(toArgvL(std::forward<Args>(args)...));
}

//===========================================================================
template <typename ...Args>
std::string Cli::toGnuCmdlineL(Args &&... args) {
    return toGnuCmdline(toArgvL(std::forward<Args>(args)...));
}

//===========================================================================
template <typename ...Args>
std::string Cli::toWindowsCmdlineL(Args &&... args) {
    return toWindowsCmdline(toArgvL(std::forward<Args>(args)...));
}


/****************************************************************************
*
*   Cli::OptBase
*
*   Common base class for all options, has no information about the derived
*   options value type, and handles all services required by the parser.
*
*   Hierarchy:
*                     OptBase
*                    /       \
*     OptShim<Opt<T>, T>    OptShim<OptVec<T>, T>
*            |                        |
*          Opt<T>                  OptVec<T>
*
***/

class DIMCLI_LIB_DECL Cli::OptBase : public Cli::Convert {
public:
    struct ChoiceDesc {
        std::string desc;
        std::string sortKey;
        size_t pos = {};
        bool def = {};
    };

public:
    OptBase(const std::string & names, bool flag);
    virtual ~OptBase() {}

    //-----------------------------------------------------------------------
    // CONFIGURATION

    // Change the locale used when parsing values via iostream. Defaults to
    // the user's preferred locale (aka locale("")) for arithmetic types and
    // the "C" locale for everything else.
    std::locale imbue(const std::locale & loc);

    //-----------------------------------------------------------------------
    // QUERIES

    // True if the value was populated from the command line, whether the
    // resulting value is the same as the default is immaterial.
    explicit operator bool() const { return matched(); }

    // Name of the last argument to populated the value, or an empty string if
    // it wasn't populated. For vectors, it's what populated the last value.
    virtual const std::string & from() const = 0;

    // Absolute position in argv[] of last the argument that populated the
    // value. For vectors, it refers to where the value on the back came from.
    // If pos() is 0 the value wasn't populated from the command line or
    // wasn't populated at all, check from() to tell the difference.
    //
    // It's possible for a value to come from prompt() or some other action
    // (which should set the position to 0) instead of the command args.
    virtual int pos() const = 0;

    // Number of values, non-vectors are always 1.
    virtual size_t size() const { return 1; }

    // Number of allowed values, maxSize of -1 for unlimited. Both minSize and
    // maxSize are always 1 for non-vectors.
    virtual int minSize() const { return 1; }
    virtual int maxSize() const { return 1; }

    // Defaults to use when populating the option from an action that's not
    // tied to a command line argument.
    const std::string & defaultFrom() const { return m_fromName; }
    std::string defaultPrompt() const;

    // Command and group this option belongs to.
    const std::string & command() const { return m_command; }
    const std::string & group() const { return m_group; }

    //-----------------------------------------------------------------------
    // UPDATE VALUE

    // Clears options argument reference (name and pos) and sets to its default
    // value.
    virtual void reset() = 0;

    // Parse the string into the value, return false on error.
    [[nodiscard]] virtual bool parseValue(const std::string & value) = 0;

protected:
    virtual bool defaultValueToString(std::string & out) const = 0;
    virtual std::string defaultValueDesc() const = 0;

    virtual void doParseAction(Cli & cli, const std::string & value) = 0;
    virtual void doCheckActions(Cli & cli, const std::string & value) = 0;
    virtual void doAfterActions(Cli & cli) = 0;

    // Record the command line argument that this opt matched with.
    virtual bool match(const std::string & name, size_t pos) = 0;
    virtual bool matched() const = 0;

    // Assign the implicit value to the value. Used when an option, with an
    // optional value, is specified without one. The default implicit value is
    // T{}, but can be changed with implicitValue().
    virtual void assignImplicit() = 0;

    // True for flags (bool on command line) that default to true.
    virtual bool inverted() const = 0;

    // Allows the type unaware layer to determine if a new option is pointing
    // at the same value as an existing option -- with RTTI disabled
    virtual bool sameValue(const void * value) const = 0;

    void setNameIfEmpty(const std::string & name);

    bool withUnits(
        long double & out,
        Cli & cli,
        const std::string & val,
        const std::unordered_map<std::string, long double> & units,
        int flags
    ) const;

    std::string m_command;
    std::string m_group;

    bool m_visible = true;
    std::string m_desc;
    std::string m_valueDesc;

    // empty() to use default, size 1 and *data == '\0' to suppress
    std::string m_defaultDesc;

    std::unordered_map<std::string, ChoiceDesc> m_choiceDescs;

    // Whether this option has one value or a vector of values.
    bool m_vector = {};

    // Whether only operands appear after this value, or if more options are
    // still allowed.
    bool m_finalOpt = {};

    // Whether the value is a bool on the command line (no separate value). Set
    // for flag values and true bools.
    bool m_bool = {};

    bool m_flagValue = {};
    bool m_flagDefault = {};

private:
    friend class Cli;

    std::string m_names;
    std::string m_fromName;
};


/****************************************************************************
*
*   Cli::OptShim
*
*   Common base for the more derived simple and vector option types. Has
*   knowledge of the value type but no knowledge about it.
*
***/

template <typename A, typename T>
class Cli::OptShim : public OptBase {
public:
    OptShim(const std::string & names, bool flag);
    OptShim(const OptShim &) = delete;
    OptShim & operator=(const OptShim &) = delete;

    //-----------------------------------------------------------------------
    // CONFIGURATION

    // Set subcommand for which this is an option.
    A & command(const std::string & val);

    // Set group under which this opt will show up in the help text.
    A & group(const std::string & val);

    // Controls whether or not the opt appears in help text.
    A & show(bool visible = true);

    // Set description to associate with the opt in help text.
    A & desc(const std::string & val);

    // Set name of meta-variable in help text. For example, would change the
    // "NUM" in "--count NUM" to something else.
    A & valueDesc(const std::string & val);

    // Set text to appear in the default clause of this options help text. Can
    // change the "0" in "(default: 0)" to something else, or use an empty
    // string to suppress the entire clause.
    A & defaultDesc(const std::string & val);

    // Allows the default to be changed after the opt has been created.
    A & defaultValue(const T & val);

    // The implicit value is used for arguments with optional values when
    // the argument was specified in the command line without an attached
    // value (e.g. -fFILE or --file=FILE).
    A & implicitValue(const T & val);

    // Turns the argument into a feature switch. If there are multiple switches
    // pointed at a single external value one of them should be flagged as the
    // default. If none (or many) are marked as the default one will be chosen
    // for you.
    A & flagValue(bool isDefault = false);

    // All following arguments are treated as operands (positional) rather
    // than options (positionless), as if "--" had been used.
    //
    // There are restrictions when used on an operand:
    //  - If operand is required, must not be preceded by optional operands.
    //  - If operand is optional, must not be followed by required operands.
    //  - Must not be preceded by vector operands with variable arity.
    A & finalOpt();

    // Adds a choice, when choices have been added only values that match one
    // of the choices are allowed. Useful for things like enums where there is
    // a controlled set of possible values.
    //
    // In help text choices are sorted first by sortKey and then by the order
    // they were added.
    A & choice(
        const T & val,
        const std::string & key,
        const std::string & desc = {},
        const std::string & sortKey = {}
    );

    // Normalizes the value by removing the symbol and if SI unit prefixes
    // (such as u, m, k, M, G, ki, Mi, and Gi) are present, multiplying the
    // value by the corresponding factor and removing it as well. The
    // Cli::fUnit* flags can be used to: base k and friends on 1024, require
    // the symbol, and/or be case-insensitive. Fractional prefixes such as
    // m (milli) work best with floating point options.
    A & siUnits(
        const std::string & symbol = {},
        int flags = 0 // Cli::fUnit* flags
    );

    // Adjusts the value to seconds when time units are present: removing the
    // units and multiplying by the required factor. Recognized units are:
    // y (365d), w (7d), d, h, min, m (minute), s, ms, us, ns. Some behavior
    // can be adjusted with Cli::fUnit* flags.
    A & timeUnits(int flags = 0);

    // Given a series of unit names and factors, incoming values have trailing
    // unit names removed and are multiplied by the factor. For example,
    // given {{"in",1},{"ft",12},{"yd",36}} the inputs '36in', '3ft', and '1yd'
    // would all be converted to 36. Some behavior can be adjusted with
    // Cli::fUnit* flags.
    A & anyUnits(
        std::initializer_list<std::pair<const char *, double>> units,
        int flags = 0 // Cli::fUnit * flags
    );
    template <typename InputIt>
    A & anyUnits(InputIt first, InputIt last, int flags = 0);

    // Forces the value to be within the range, if it's less than the low
    // it's set to the low, if higher than high it's made merely high.
    A & clamp(const T & low, const T & high);

    // Fail if the value given for this option is not in within the range
    // (inclusive) of low to high.
    A & range(const T & low, const T & high);

    // Causes a check whether the option value was set during parsing, and
    // reports badUsage() if it wasn't.
    A & require();

    // Enables prompting. When the option hasn't been provided on the command
    // line the user will be prompted for it. Use Cli::fPrompt* flags to
    // adjust behavior.
    A & prompt(int flags = 0);
    A & prompt(
        const std::string & msg, // custom prompt message
        int flags = 0            // Cli::fPrompt* flags
    );

    // Function signature of actions that are tied to options.
    using ActionFn = void(Cli & cli, A & opt, const std::string & val);

    // Change the action to take when parsing this argument. The function
    // should:
    //  - Parse the val string and use the result to set the value (or, for
    //    vectors, push_back the new value).
    //  - Call cli.badUsage() with an error message if there's a problem.
    //  - Call cli.parseExit() if the program should stop without an error.
    //    This could be due to an early out like "--version" and "--help".
    //
    // You can use opt.from() and opt.pos() to get the option name that the
    // value was matched with on the command line and its position in argv[].
    // For bool arguments the source value string will always be either "0"
    // or "1".
    //
    // If you just need support for a new type you can provide an istream
    // extraction (>>) or assignment from string operator and the default
    // parse action will pick it up.
    A & parse(std::function<ActionFn> fn);

    // Action to take immediately after each value is parsed, unlike parsing
    // itself where there can only be one action, any number of check actions
    // can be added. They will be called in the order they were added and if
    // any of them return false it stops processing. As an example,
    // opt.clamp() and opt.range() both do their job by adding check actions.
    //
    // The function should:
    //  - Check the options new value, possibly in relation to other options.
    //  - Call cli.badUsage() with an error message if there's a problem.
    //  - Call cli.parseExit() if the program should stop without an error.
    //
    // The opt is fully populated so *opt, opt.from(), etc are all available.
    A & check(std::function<ActionFn> fn);

    // Action to run after all arguments have been parsed, any number of
    // after actions can be added and will, for each option, be called in the
    // order they're added. When using subcommands, the after actions bound
    // to unmatched subcommands are not executed. The function should:
    //  - Do something interesting.
    //  - Call cli.badUsage() and return on error.
    //  - Call cli.parseExit() if processing should stop without error.
    //
    // Because after actions are not tied to a specific argument, the val
    // parameter passed to the function is always empty.
    A & after(std::function<ActionFn> fn);

    //-----------------------------------------------------------------------
    // QUERIES

    const T & implicitValue() const { return m_implicitValue; }
    const T & defaultValue() const { return m_defValue; }

protected:
    std::string defaultValueDesc() const final;
    void doParseAction(Cli & cli, const std::string & value) final;
    void doCheckActions(Cli & cli, const std::string & value) final;
    void doAfterActions(Cli & cli) final;
    void act(
        Cli & cli,
        const std::string & value,
        const std::vector<std::function<ActionFn>> & actions
    );
    bool inverted() const final;

    // If numeric_limits<T>::min & max are defined and 'x' is outside of
    // those limits badRange() is called, otherwise returns true.
    template <typename U>
    auto checkLimits(Cli & cli, const std::string & val, const U & x, int)
        -> decltype((U) std::declval<T&>() > x);
    template <typename U>
    bool checkLimits(Cli & cli, const std::string & val, const U & x, long);

    std::function<ActionFn> m_parse;
    std::vector<std::function<ActionFn>> m_checks;
    std::vector<std::function<ActionFn>> m_afters;

    T m_implicitValue = {};
    T m_defValue = {};
    std::vector<T> m_choices;
};

//===========================================================================
template <typename A, typename T>
Cli::OptShim<A, T>::OptShim(const std::string & names, bool flag)
    : OptBase(names, flag)
{
    if (std::is_arithmetic<T>::value)
        this->imbue(std::locale(""));
}

//===========================================================================
template <typename A, typename T>
inline std::string Cli::OptShim<A, T>::defaultValueDesc() const {
    return Cli::valueDesc<T>();
}

//===========================================================================
template <typename A, typename T>
inline void Cli::OptShim<A, T>::doParseAction(
    Cli & cli,
    const std::string & val
) {
    auto self = static_cast<A *>(this);
    return m_parse(cli, *self, val);
}

//===========================================================================
template <typename A, typename T>
inline void Cli::OptShim<A, T>::doCheckActions(
    Cli & cli,
    const std::string & val
) {
    return act(cli, val, m_checks);
}

//===========================================================================
template <typename A, typename T>
inline void Cli::OptShim<A, T>::doAfterActions(Cli & cli) {
    return act(cli, {}, m_afters);
}

//===========================================================================
template <typename A, typename T>
inline void Cli::OptShim<A, T>::act(
    Cli & cli,
    const std::string & val,
    const std::vector<std::function<ActionFn>> & actions
) {
    auto self = static_cast<A *>(this);
    for (auto && fn : actions) {
        fn(cli, *self, val);
        if (cli.parseExited())
            break;
    }
}

//===========================================================================
template <typename A, typename T>
inline bool Cli::OptShim<A, T>::inverted() const {
    return this->m_bool && this->m_flagValue && this->m_flagDefault;
}

//===========================================================================
template <>
inline bool Cli::OptShim<Cli::Opt<bool>, bool>::inverted() const {
    // bool options are always marked as bool
    assert(this->m_bool // LCOV_EXCL_LINE
        && "Internal dimcli error: bool option not marked bool.");
    if (this->m_flagValue)
        return this->m_flagDefault;
    return this->defaultValue();
}

//===========================================================================
template <typename A, typename T>
A & Cli::OptShim<A, T>::command(const std::string & val) {
    m_command = val;
    return static_cast<A &>(*this);
}

//===========================================================================
template <typename A, typename T>
A & Cli::OptShim<A, T>::group(const std::string & val) {
    m_group = val;
    return static_cast<A &>(*this);
}

//===========================================================================
template <typename A, typename T>
A & Cli::OptShim<A, T>::show(bool visible) {
    m_visible = visible;
    return static_cast<A &>(*this);
}

//===========================================================================
template <typename A, typename T>
A & Cli::OptShim<A, T>::desc(const std::string & val) {
    m_desc = val;
    return static_cast<A &>(*this);
}

//===========================================================================
template <typename A, typename T>
A & Cli::OptShim<A, T>::valueDesc(const std::string & val) {
    m_valueDesc = val;
    return static_cast<A &>(*this);
}

//===========================================================================
template <typename A, typename T>
A & Cli::OptShim<A, T>::defaultDesc(const std::string & val) {
    m_defaultDesc = val;
    if (val.empty())
        m_defaultDesc.push_back('\0');
    return static_cast<A &>(*this);
}

//===========================================================================
template <typename A, typename T>
A & Cli::OptShim<A, T>::defaultValue(const T & val) {
    m_defValue = val;
    for (auto && cd : m_choiceDescs)
        cd.second.def = !this->m_vector && val == m_choices[cd.second.pos];
    return static_cast<A &>(*this);
}

//===========================================================================
template <typename A, typename T>
A & Cli::OptShim<A, T>::implicitValue(const T & val) {
    if (m_bool) {
        // bools don't have separate values, just their presence/absence
        assert(!"Bad modifier (implicit) for bool argument.");
    } else {
        m_implicitValue = val;
    }
    return static_cast<A &>(*this);
}

//===========================================================================
template <typename A, typename T>
A & Cli::OptShim<A, T>::flagValue(bool isDefault) {
    auto self = static_cast<A *>(this);
    m_flagValue = true;
    if (!self->m_proxy->m_defFlagOpt) {
        self->m_proxy->m_defFlagOpt = self;
        m_flagDefault = true;
    } else if (isDefault) {
        self->m_proxy->m_defFlagOpt->m_flagDefault = false;
        self->m_proxy->m_defFlagOpt = self;
        m_flagDefault = true;
    } else {
        m_flagDefault = false;
    }
    m_bool = true;
    return *self;
}

//===========================================================================
template <typename A, typename T>
A & Cli::OptShim<A, T>::finalOpt() {
    this->m_finalOpt = true;
    return static_cast<A &>(*this);
}

//===========================================================================
template <typename A, typename T>
A & Cli::OptShim<A, T>::choice(
    const T & val,
    const std::string & key,
    const std::string & desc,
    const std::string & sortKey
) {
    // The empty string isn't a valid choice because it can't be specified on
    // the command line, where unspecified picks the default instead.
    if (key.empty())
        assert(!"An empty string can't be a choice.");
    auto & cd = m_choiceDescs[key];
    cd.pos = m_choices.size();
    cd.desc = desc;
    cd.sortKey = sortKey;
    cd.def = (!this->m_vector && val == this->defaultValue());
    m_choices.push_back(val);
    return static_cast<A &>(*this);
}

//===========================================================================
template <typename A, typename T>
A & Cli::OptShim<A, T>::parse(std::function<ActionFn> fn) {
    this->m_parse = std::move(fn);
    return static_cast<A &>(*this);
}

//===========================================================================
template <typename A, typename T>
A & Cli::OptShim<A, T>::check(std::function<ActionFn> fn) {
    this->m_checks.push_back(std::move(fn));
    return static_cast<A &>(*this);
}

//===========================================================================
template <typename A, typename T>
A & Cli::OptShim<A, T>::after(std::function<ActionFn> fn) {
    this->m_afters.push_back(std::move(fn));
    return static_cast<A &>(*this);
}

//===========================================================================
template <typename A, typename T>
A & Cli::OptShim<A, T>::require() {
    return after(&Cli::requireAction);
}

//===========================================================================
template <typename A, typename T>
A & Cli::OptShim<A, T>::siUnits(const std::string & symbol, int flags) {
    auto units = siUnitMapping(symbol, flags);
    return anyUnits(units.begin(), units.end(), flags);
}

//===========================================================================
template <typename A, typename T>
A & Cli::OptShim<A, T>::timeUnits(int flags) {
    return anyUnits({
            {"y", 365 * 24 * 60 * 60},
            {"w", 7 * 24 * 60 * 60},
            {"d", 24 * 60 * 60},
            {"h", 60 * 60},
            {"m", 60},
            {"min", 60},
            {"s", 1},
            {"ms", 1e-3},
            {"us", 1e-6},
            {"ns", 1e-9},
        },
        flags
    );
}

//===========================================================================
template <typename A, typename T>
A & Cli::OptShim<A, T>::anyUnits(
    std::initializer_list<std::pair<const char *, double>> units,
    int flags
) {
    return anyUnits(units.begin(), units.end(), flags);
}

//===========================================================================
template <typename A, typename T>
template <typename U>
auto Cli::OptShim<A, T>::checkLimits(
    Cli & cli,
    const std::string & val,
    const U & x,
    int
)
    -> decltype((U) std::declval<T&>() > x)
{
    constexpr auto low = std::numeric_limits<T>::min();
    constexpr auto high = std::numeric_limits<T>::max();
    if (!std::is_arithmetic<T>::value)
        return true;
    if ((long double) x >= (long double) low
        && (long double) x <= (long double) high
    ) {
        return true;
    }
    cli.badRange(*this, val, low, high);
    return false;
}

//===========================================================================
template <typename A, typename T>
template <typename U>
bool Cli::OptShim<A, T>::checkLimits(
    Cli &,
    const std::string &,
    const U &,
    long
) {
    return true;
}

//===========================================================================
template <typename A, typename T>
template <typename InputIt>
A & Cli::OptShim<A, T>::anyUnits(InputIt first, InputIt last, int flags) {
    if (m_valueDesc.empty()) {
        m_valueDesc = defaultValueDesc()
            + ((flags & fUnitRequire) ? "<units>" : "[<units>]");
    }
    std::unordered_map<std::string, long double> units;
    if (flags & fUnitInsensitive) {
        auto & f = std::use_facet<std::ctype<char>>(std::locale());
        std::string name;
        for (; first != last; ++first) {
            name = first->first;
            f.tolower((char *) name.data(), name.data() + name.size());
            units[name] = first->second;
        }
    } else {
        units.insert(first, last);
    }
    return parse([units, flags](auto & cli, auto & opt, auto & val) {
        long double dval;
        bool success = true;
        if (!opt.withUnits(dval, cli, val, units, flags))
            return;
        if (!opt.checkLimits(cli, val, dval, 0))
            return;
        std::string sval;
        if (std::is_integral<T>::value)
            dval = std::round(dval);
        auto ival = (int64_t) dval;
        if (ival == dval) {
            success = opt.toString(sval, ival);
            assert(success // LCOV_EXCL_LINE
                && "Internal dimcli error: convert int64_t to string failed.");
        } else {
            success = opt.toString(sval, dval);
            assert(success // LCOV_EXCL_LINE
                && "Internal dimcli error: convert double to string failed.");
        }
        if (!opt.parseValue(sval)) {
            success = false;
            cli.badUsage(opt, val);
        }
    });
}

//===========================================================================
template <typename A, typename T>
A & Cli::OptShim<A, T>::clamp(const T & low, const T & high) {
    if (high < low)
        assert(!"Bad clamp, low greater than high.");
    return check([low, high](auto & /* cli */, auto & opt, auto & /* val */) {
        if (*opt < low) {
            *opt = low;
        } else if (*opt > high) {
            *opt = high;
        }
        return true;
    });
}

//===========================================================================
template <typename A, typename T>
A & Cli::OptShim<A, T>::range(const T & low, const T & high) {
    if (high < low)
        assert(!"Bad range, low greater than high.");
    return check([low, high](auto & cli, auto & opt, auto & val) {
        if (*opt >= low && *opt <= high)
            return;
        cli.badRange(opt, val, low, high);
    });
}

//===========================================================================
template <typename A, typename T>
A & Cli::OptShim<A, T>::prompt(int flags) {
    return prompt({}, flags);
}

//===========================================================================
template <typename A, typename T>
A & Cli::OptShim<A, T>::prompt(const std::string & msg, int flags) {
    return after([=](auto & cli, auto & opt, auto & /* val */) {
        cli.prompt(opt, msg, flags);
    });
}


/****************************************************************************
*
*   Cli::ArgMatch
*
*   Reference to the command line argument that was used to populate a value
*
***/

struct Cli::ArgMatch {
    // Name of the argument that populated the value, or an empty
    // string if it wasn't populated.
    std::string name;

    // Member of argv[] that populated the value or 0 if it wasn't.
    int pos = {};
};


/****************************************************************************
*
*   Cli::Value
*
***/

template <typename T>
struct Cli::Value {
    // Where the value came from.
    ArgMatch m_match;

    // Whether the value was explicitly set.
    bool m_explicit = {};

    // Points to the opt with the default flag value.
    Opt<T> * m_defFlagOpt = {};

    T * m_value = {};
    T m_internal = {};

    Value(T * value) : m_value(value ? value : &m_internal) {}
};


/****************************************************************************
*
*   Cli::Opt
*
***/

template <typename T>
class Cli::Opt : public OptShim<Opt<T>, T> {
public:
    Opt(std::shared_ptr<Value<T>> value, const std::string & names);

    //-----------------------------------------------------------------------
    // QUERIES

    T & operator*() { return *m_proxy->m_value; }
    T * operator->() { return m_proxy->m_value; }

    // Inherited via OptBase
    const std::string & from() const final { return m_proxy->m_match.name; }
    int pos() const final { return m_proxy->m_match.pos; }

    //-----------------------------------------------------------------------
    // UPDATE VALUE

    // Inherited via OptBase
    void reset() final;
    bool parseValue(const std::string & value) final;

private:
    friend class Cli;
    bool defaultValueToString(std::string & out) const final;
    bool match(const std::string & name, size_t pos) final;
    bool matched() const final { return m_proxy->m_explicit; }
    void assignImplicit() final;
    bool sameValue(const void * value) const final {
        return value == m_proxy->m_value;
    }

    std::shared_ptr<Value<T>> m_proxy;
};

//===========================================================================
template <typename T>
Cli::Opt<T>::Opt(
    std::shared_ptr<Value<T>> value,
    const std::string & names
)
    : OptShim<Opt, T>(names, std::is_same<T, bool>::value)
    , m_proxy(value)
{}

//===========================================================================
template <typename T>
inline void Cli::Opt<T>::reset() {
    if (!this->m_flagValue || this->m_flagDefault)
        *m_proxy->m_value = this->defaultValue();
    m_proxy->m_match.name.clear();
    m_proxy->m_match.pos = 0;
    m_proxy->m_explicit = false;
}

//===========================================================================
template <typename T>
inline bool Cli::Opt<T>::parseValue(const std::string & value) {
    auto & tmp = *m_proxy->m_value;
    if (this->m_flagValue) {
        // Value passed for flagValue (just like bools) is generated
        // internally and will be 0 or 1.
        if (value == "1") {
            tmp = this->defaultValue();
        } else {
            assert(value == "0" // LCOV_EXCL_LINE
                && "Internal dimcli error: flagValue not parsed from 0 or 1.");
        }
        return true;
    }
    if (!this->m_choices.empty()) {
        auto i = this->m_choiceDescs.find(value);
        if (i == this->m_choiceDescs.end())
            return false;
        tmp = this->m_choices[i->second.pos];
        return true;
    }
    return this->fromString(tmp, value);
}

//===========================================================================
template <typename T>
inline bool Cli::Opt<T>::defaultValueToString(std::string & out) const {
    return this->toString(out, this->defaultValue());
}

//===========================================================================
template <typename T>
inline bool Cli::Opt<T>::match(const std::string & name, size_t pos) {
    m_proxy->m_match.name = name;
    m_proxy->m_match.pos = (int)pos;
    m_proxy->m_explicit = true;
    return true;
}

//===========================================================================
template <typename T>
inline void Cli::Opt<T>::assignImplicit() {
    *m_proxy->m_value = this->implicitValue();
}


/****************************************************************************
*
*   Cli::ValueVec
*
***/

template <typename T>
struct Cli::ValueVec {
    // Where the values came from.
    std::vector<ArgMatch> m_matches;

    // Points to the opt with the default flag value.
    OptVec<T> * m_defFlagOpt = {};

    std::vector<T> * m_values = {};
    std::vector<T> m_internal;

    ValueVec(std::vector<T> * value)
        : m_values(value ? value : &m_internal)
    {}
};


/****************************************************************************
*
*   Cli::OptVec
*
***/

template <typename T>
class Cli::OptVec : public OptShim<OptVec<T>, T> {
public:
    OptVec(std::shared_ptr<ValueVec<T>> values, const std::string & names);

    //-----------------------------------------------------------------------
    // CONFIGURATION

    // Set the number of values that can be assigned to a vector option.
    // Defaults to a min/max of 1/-1 where -1 means unlimited.
    OptVec & size(int exact);
    OptVec & size(int min, int max);

    //-----------------------------------------------------------------------
    // QUERIES

    std::vector<T> & operator*() { return *m_proxy->m_values; }
    std::vector<T> * operator->() { return m_proxy->m_values; }

    T & operator[](size_t index) { return (*m_proxy->m_values)[index]; }
    const T & operator[](size_t index) const {
        return const_cast<T *>(this)[index];
    }

    // Information about a specific member of the vector of values at the
    // time it was parsed. If the value vector has been changed (sort, erase,
    // insert, etc) by the application these will no longer correspond.
    const std::string & from(size_t index) const;
    int pos(size_t index) const;

    // Inherited via OptBase
    const std::string & from() const final { return from(size() - 1); }
    int pos() const final { return pos(size() - 1); }
    size_t size() const final { return m_proxy->m_values->size(); }
    int minSize() const final { return m_minVec; }
    int maxSize() const final { return m_maxVec; }

    //-----------------------------------------------------------------------
    // UPDATE VALUE

    // Inherited via OptBase
    void reset() final;
    bool parseValue(const std::string & value) final;

private:
    friend class Cli;
    bool defaultValueToString(std::string & out) const final;
    bool match(const std::string & name, size_t pos) final;
    bool matched() const final { return !m_proxy->m_values->empty(); }
    void assignImplicit() final;
    bool sameValue(const void * value) const final {
        return value == m_proxy->m_values;
    }

    std::shared_ptr<ValueVec<T>> m_proxy;
    std::string m_empty;

    // Minimum and maximum number of values allowed in vector.
    int m_minVec = 1;
    int m_maxVec = 1;
};

//===========================================================================
template <typename T>
Cli::OptVec<T>::OptVec(
    std::shared_ptr<ValueVec<T>> values,
    const std::string & names
)
    : OptShim<OptVec, T>(names, std::is_same<T, bool>::value)
    , m_proxy(values)
{
    this->m_vector = true;
    this->m_minVec = 1;
    this->m_maxVec = -1;
}

//===========================================================================
template <typename T>
inline Cli::OptVec<T> & Cli::OptVec<T>::size(int exact) {
    if (exact < 0) {
        assert(!"Bad optVec size, minimum must be >= 0.");
    } else {
        this->m_minVec = this->m_maxVec = exact;
    }
    return *this;
}

//===========================================================================
template <typename T>
inline Cli::OptVec<T> & Cli::OptVec<T>::size(int min, int max) {
    if (min < 0) {
        assert(!"Bad optVec size, minimum must be >= 0.");
    } else if (max < -1) {
        assert(!"Bad optVec size, maximum must be >= 0 or -1 (unlimited).");
    } else if (max != -1 && min > max) {
        assert(!"Bad optVec size, min greater than max.");
    } else {
        this->m_minVec = min;
        this->m_maxVec = max;
    }
    return *this;
}

//===========================================================================
template <typename T>
inline bool Cli::OptVec<T>::parseValue(const std::string & value) {
    auto back = std::prev(m_proxy->m_values->end());
    if (this->m_flagValue) {
        // Value passed for flagValue (just like bools) is generated
        // internally and will be 0 or 1.
        if (value == "1") {
            *back = this->defaultValue();
        } else {
            assert(value == "0" // LCOV_EXCL_LINE
                && "Internal dimcli error: flagValue not parsed from 0 or 1.");
            m_proxy->m_values->pop_back();
            m_proxy->m_matches.pop_back();
        }
        return true;
    }
    if (!this->m_choices.empty()) {
        auto i = this->m_choiceDescs.find(value);
        if (i == this->m_choiceDescs.end())
            return false;
        *back = this->m_choices[i->second.pos];
        return true;
    }

    // Parsed indirectly through temporary for cases like vector<bool> where
    // *back returns a proxy object instead of a reference to T.
    T tmp{};
    bool result = this->fromString(tmp, value);
    *back = std::move(tmp);
    return result;
}

//===========================================================================
template <typename T>
inline bool Cli::OptVec<T>::defaultValueToString(std::string & out) const {
    out.clear();
    return false;
}

//===========================================================================
template <typename T>
inline void Cli::OptVec<T>::reset() {
    m_proxy->m_values->clear();
    m_proxy->m_matches.clear();
}

//===========================================================================
template <typename T>
inline bool Cli::OptVec<T>::match(const std::string & name, size_t pos) {
    if (this->m_maxVec != -1
        && (size_t) this->m_maxVec == m_proxy->m_matches.size()
    ) {
        return false;
    }

    ArgMatch match;
    match.name = name;
    match.pos = (int)pos;
    m_proxy->m_matches.push_back(match);
    m_proxy->m_values->resize(m_proxy->m_matches.size());
    return true;
}

//===========================================================================
template <typename T>
inline void Cli::OptVec<T>::assignImplicit() {
    m_proxy->m_values->back() = this->implicitValue();
}

//===========================================================================
template <typename T>
inline const std::string & Cli::OptVec<T>::from(size_t index) const {
    if (index >= size()) {
        return m_empty;
    } else {
        return m_proxy->m_matches[index].name;
    }
}

//===========================================================================
template <typename T>
inline int Cli::OptVec<T>::pos(size_t index) const {
    return index >= size() ? 0 : m_proxy->m_matches[index].pos;
}

} // namespace


/****************************************************************************
*
*   Restore settings
*
***/

// Restore as many compiler settings as possible so they don't leak into the
// application.
#ifndef DIMCLI_LIB_KEEP_MACROS

// clear all dim header macros so they don't leak into the application
#undef DIMCLI_LIB_DYN_LINK
#undef DIMCLI_LIB_KEEP_MACROS
#undef DIMCLI_LIB_STANDALONE
#undef DIMCLI_LIB_WINAPI_FAMILY_APP

#undef DIMCLI_LIB_DECL
#undef DIMCLI_LIB_NO_ENV
#undef DIMCLI_LIB_NO_CONSOLE
#undef DIMCLI_LIB_SOURCE

#if defined(_MSC_VER)
#pragma warning(pop)
#elif defined(__clang__) || defined(__GNUC__) || defined(__GNUG__)
#pragma GCC diagnostic pop
#endif

#undef DIMCLI_LIB_FILESYSTEM
#undef DIMCLI_LIB_FILESYSTEM_PATH

#endif
