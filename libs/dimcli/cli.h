// cli.h - dim cli
//
// Command line parser toolkit
//
// Instead of trying to figure it all out from just this header please take 
// a moment and look at the documentation and examples:
// https://github.com/gknowles/dimcli

#pragma once


/****************************************************************************
*
*   Configuration
*
***/

//---------------------------------------------------------------------------
// Configuration of this installation, these are options that must be the
// same when building the app as when building the library.

// DIM_LIB_STANDALONE: Defines this as standalone library that is not
// being built as part of the DIM framework.
#define DIM_LIB_STANDALONE

// DIM_LIB_DYN_LINK: Forces all libraries that have separate source, to be
// linked as dll's rather than static libraries on Microsoft Windows (this
// macro is used to turn on __declspec(dllimport) modifiers, so that the
// compiler knows which symbols to look for in a dll rather than in a static
// library). Note that there may be some libraries that can only be linked in
// one way (statically or dynamically), in these cases this macro has no
// effect.
//#define DIM_LIB_DYN_LINK

// DIM_LIB_WINAPI_FAMILY_APP: Removes all functions that rely on windows
// WINAPI_FAMILY_DESKTOP mode, such as the console and environment
// variables.
//#define DIM_LIB_WINAPI_FAMILY_APP


//---------------------------------------------------------------------------
// Configuration of the application. These options, if desired, are set by the
// application before including the library headers.

// DIM_LIB_KEEP_MACROS: By default the DIM_LIB_* macros defined internally
// (including in this file) are undef'd so they don't leak out to application
// code. Setting this macro leaves them available for the application to use.
// Also included are other platform specific adjustments, such as suppression
// of specific compiler warnings.


//===========================================================================
// Internal
//===========================================================================

#if defined(DIM_LIB_SOURCE) && !defined(DIM_LIB_KEEP_MACROS)
#define DIM_LIB_KEEP_MACROS
#endif

#ifdef DIM_LIB_WINAPI_FAMILY_APP
#define DIM_LIB_NO_ENV
#define DIM_LIB_NO_CONSOLE
#endif

#ifdef _MSC_VER
#ifndef DIM_LIB_KEEP_MACROS
#pragma warning(push)
#endif
#endif

#ifdef DIM_LIB_DYN_LINK
#if defined(_MSC_VER)
// 'identifier': class 'type' needs to have dll-interface to be used
// by clients of class 'type2'
#pragma warning(disable : 4251)
#endif

#if defined(_WIN32)
#ifdef DIM_LIB_SOURCE
#define DIM_LIB_DECL __declspec(dllexport)
#else
#define DIM_LIB_DECL __declspec(dllimport)
#endif
#endif
#else
#define DIM_LIB_DECL
#endif

#ifndef _CPPUNWIND
#define _HAS_EXCEPTIONS 0
#endif


/****************************************************************************
*
*   Includes
*
***/

#include <cassert>
#include <experimental/filesystem>
#include <functional>
#include <list>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>


namespace Dim {

// forward declarations
class Cli;


/****************************************************************************
*
*   Exit codes
*
***/

// These mirror the program exit codes defined in <sysexits.h>
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

class DIM_LIB_DECL Cli {
public:
    struct Config;
    struct CommandConfig;
    struct GroupConfig;

    class OptBase;
    template <typename A, typename T> class OptShim;
    template <typename T> class Opt;
    template <typename T> class OptVec;
    struct OptIndex;

    struct ArgKey;
    struct ArgMatch;
    template <typename T> struct Value;
    template <typename T> struct ValueVec;

public:
    // Creates a handle to the shared command line configuration, this
    // indirection allows options to be statically registered from multiple
    // source files.
    Cli();

    //-----------------------------------------------------------------------
    // Configuration

    template <
        typename T,
        typename U,
        typename =
            typename std::enable_if<std::is_convertible<U, T>::value>::type>
    Opt<T> & opt(T * value, const std::string & keys, const U & def);

    template <typename T> Opt<T> & opt(T * value, const std::string & keys);

    template <typename T>
    OptVec<T> &
    optVec(std::vector<T> * values, const std::string & keys, int nargs = -1);

    template <typename T>
    Opt<T> & opt(const std::string & keys, const T & def = {});

    template <typename T>
    OptVec<T> & optVec(const std::string & keys, int nargs = -1);

    template <
        typename T,
        typename U,
        typename =
            typename std::enable_if<std::is_convertible<U, T>::value>::type>
    Opt<T> & opt(Opt<T> & value, const std::string & keys, const U & def);

    template <typename T>
    Opt<T> & opt(Opt<T> & value, const std::string & keys);

    template <typename T>
    OptVec<T> &
    optVec(OptVec<T> & values, const std::string & keys, int nargs = -1);

    // Add -y, --yes option that exits early when false and has an "are you
    // sure?" style prompt when it's not present.
    Opt<bool> & confirmOpt(const std::string & prompt = {});

    // Get reference to internal help option, can be used to change the
    // desciption, option group, etc.
    Opt<bool> & helpOpt();

    // Add --password option and prompts for a password if it's not given
    // on the command line. If confirm is true and it's not on the command
    // line you have to enter it twice.
    Opt<std::string> & passwordOpt(bool confirm = false);

    // Add --version option that shows "${progName.filename()} version ${ver}"
    // and exits. An empty progName defaults to argv[0].
    Opt<bool> &
    versionOpt(const std::string & ver, const std::string & progName = {});

    //-----------------------------------------------------------------------
    // A group collects options into sections in the help text. Options are
    // always added to a group, either the default group of the cli (or of the
    // selected command), or an explicitly created one.

    // Changes config context to point at the selected option group of the
    // current command, that you can then start stuffing args into.
    Cli & group(const std::string & name);

    // Heading title to display, defaults to group name. If empty there will
    // be a single blank line separating this group from the previous one.
    Cli & title(const std::string & val);

    // Option groups are sorted by key, defaults to group name
    Cli & sortKey(const std::string & key);

    const std::string & group() const { return m_group; }
    const std::string & title() const;
    const std::string & sortKey() const;

    //-----------------------------------------------------------------------
    // Changes config context to point at the default option group of the
    // selected command.
    Cli & command(const std::string & name, const std::string & group = {});

    // Action that should be taken when the currently selected command is run.
    // Actions are executed when cli.run() is called by the application. The
    // parse function should:
    //  - do something useful
    //  - return an exitCode.
    using ActionFn = int(Cli & cli);
    Cli & action(std::function<ActionFn> fn);

    // Arbitrary text can be added to the generated help for each command,
    // this text can come before the usage (header), between the usage and
    // the arguments / options (desc), or after the options (footer). Use
    // line breaks for semantics, let the automatic line wrapping take care
    // of the rest.
    Cli & header(const std::string & val);
    Cli & desc(const std::string & val);
    Cli & footer(const std::string & val);

    const std::string & command() const { return m_command; }
    const std::string & header() const;
    const std::string & desc() const;
    const std::string & footer() const;

    //-----------------------------------------------------------------------
    // Enabled by default, reponse file expansion replaces arguments of the
    // form "@file" with the contents of the file.
    void responseFiles(bool enable = true);

#if !defined(DIM_LIB_NO_ENV)
    // Environment variable to get initial options from. Defaults to the empty
    // string, but when set the content of the named variable is parsed into
    // args which are then inserted into the argument list right after arg0.
    void envOpts(const std::string & envVar);
#endif

    // Changes the streams used for prompting, outputing help messages, etc.
    // Mainly intended for testing. Setting to null restores the defaults
    // which are cin and cout respectively.
    void iostreams(std::istream * in, std::ostream * out);
    std::istream & conin();
    std::ostream & conout();

    //-----------------------------------------------------------------------
    // Help

    // printHelp & printUsage return the current exitCode()
    int printHelp(
        std::ostream & os,
        const std::string & progName = {},
        const std::string & cmd = {}) const;
    int printUsage(
        std::ostream & os,
        const std::string & progName = {},
        const std::string & cmd = {}) const;
    // Same as printUsage(), except individually lists all non-default options 
    // instead of the [OPTIONS] catchall.
    int printUsageEx(
        std::ostream & os,
        const std::string & progName = {},
        const std::string & cmd = {}) const;

    void
    printPositionals(std::ostream & os, const std::string & cmd = {}) const;
    void printOptions(std::ostream & os, const std::string & cmd = {}) const;
    void printCommands(std::ostream & os) const;

    //-----------------------------------------------------------------------
    // Parsing

    bool parse(size_t argc, char * argv[]);
    bool parse(std::ostream & os, size_t argc, char * argv[]);

    // "args" is non-const so response files can be expanded in place
    bool parse(std::vector<std::string> & args);
    bool parse(std::ostream & os, std::vector<std::string> & args);

    // Sets all options to their defaults, called internally when parsing
    // starts.
    void resetValues();

    // Parse cmdline into vector of args, using the default conventions
    // (Gnu or Windows) of the platform.
    static std::vector<std::string> toArgv(const std::string & cmdline);
    // Copy array of pointers into vector of args
    static std::vector<std::string> toArgv(size_t argc, char * argv[]);
    // Copy array of wchar_t pointers into vector of utf-8 encoded args
    static std::vector<std::string> toArgv(size_t argc, wchar_t * argv[]);
    // Create vector of pointers suitable for use with argc/argv APIs,
    // includes trailing null, so use "size() - 1" for argc. The return
    // values point into the source string vector and are only valid until
    // that vector is resized or destroyed.
    static std::vector<const char *>
    toPtrArgv(const std::vector<std::string> & args);

    // Parse according to glib conventions, based on the UNIX98 shell spec
    static std::vector<std::string> toGlibArgv(const std::string & cmdline);
    // Parse using GNU conventions, same rules as buildargv()
    static std::vector<std::string> toGnuArgv(const std::string & cmdline);
    // Parse using Windows rules
    static std::vector<std::string> toWindowsArgv(const std::string & cmdline);

    template <typename T>
    bool fromString(T & out, const std::string & src) const;
    template <typename T>
    bool toString(std::string & out, const T & src) const;

    //-----------------------------------------------------------------------
    // Support for parsing callbacks

    // Intended for use from return statements in action callbacks. Sets
    // exit code (to EX_USAGE) and error msg, then returns false.
    bool badUsage(const std::string & msg);
    // Calls badUsage(msg) with msg set to, depending on if its a subcommand,
    // one of the following:
    //  "<prefix>: <value>"
    //  "Command: '<command>': <prefix>: <value>"
    bool badUsage(const std::string & prefix, const std::string & value);

    // Used to populate an option with an arbitrary input string through the
    // standard parsing logic. Since it causes the parse and check actions to
    // be called care must be taken to avoid infinite recursion if used from
    // those actions.
    bool parseValue(
        OptBase & out,
        const std::string & name,
        size_t pos,
        const char src[]);

    // Prompt sends a prompt message to cout and read a response from cin
    // (unless cli.iostreams() changed the streams to use), the response is
    // then passed to cli.parseValue() to set the value and run any actions.
    enum {
        kPromptHide = 1,      // hide user input as they type
        kPromptConfirm = 2,   // make the user enter it twice
        kPromptNoDefault = 4, // don't include default value in prompt
    };
    bool prompt(OptBase & opt, const std::string & msg, int flags);

    //-----------------------------------------------------------------------
    // After parsing

    int exitCode() const;
    const std::string & errMsg() const;

    // Additional information that may help the user correct their mistake,
    // may be empty.
    const std::string & errDetail() const;

    // Program name received in argv[0]
    const std::string & progName() const;

    // Command to run, as selected by argv, empty string if there are no
    // commands defined or none were selected.
    const std::string & runCommand() const;

    // Runs the action of the selected command and returns its exit code;
    // which is also used to set cli.exitCode(). If no command was selected
    // it runs the action of the empty "" command, which can be set via
    // cli.action() just like any other command.
    int run();

protected:
    Cli(std::shared_ptr<Config> cfg);

private:
    static void consoleEnableEcho(bool enable = true);

    bool defaultParse(OptBase & opt, const std::string & val);

    void addOpt(std::unique_ptr<OptBase> opt);

    template <typename A> A & addOpt(std::unique_ptr<A> ptr);

    template <typename Opt, typename Value, typename T>
    std::shared_ptr<Value> getProxy(T * ptr);

    // find an option (from any subcommand) that updates the value
    OptBase * findOpt(const void * value);

    CommandConfig & cmdCfg();
    const CommandConfig & cmdCfg() const;
    GroupConfig & grpCfg();
    const GroupConfig & grpCfg() const;

    std::string descStr(const Cli::OptBase & opt) const;
    int writeUsageImpl(
        std::ostream & os,
        const std::string & arg0,
        const std::string & cmd,
        bool expandedOptions) const;
    void
    index(OptIndex & ndx, const std::string & cmd, bool requireVisible) const;
    bool findNamedArgs(
        std::vector<ArgKey> & namedArgs,
        size_t & colWidth,
        const OptIndex & ndx,
        CommandConfig & cmd,
        int type,
        bool flatten) const;
    std::string nameList(
        const OptIndex & ndx,
        const OptBase & opt,
        int type) const;

    bool fail(int code, const std::string & msg);

    template <typename T>
    auto fromString_impl(T & out, const std::string & src, int, int) const
        -> decltype(out = src, bool());
    template <typename T>
    auto fromString_impl(T & out, const std::string & src, int, long) const
        -> decltype(std::declval<std::istream &>() >> out, bool());
    template <typename T>
    bool fromString_impl(T & out, const std::string & src, long, long) const;

    template <typename T>
    auto toString_impl(std::string & out, const T & src, int) const
        -> decltype(std::declval<std::ostream &>() << src, bool());
    template <typename T>
    bool toString_impl(std::string & out, const T & src, long) const;

    std::shared_ptr<Config> m_cfg;
    std::string m_group;
    std::string m_command;
    mutable std::stringstream m_interpreter;
};

//===========================================================================
template <typename T, typename U, typename>
inline Cli::Opt<T> &
Cli::opt(T * value, const std::string & keys, const U & def) {
    auto proxy = getProxy<Opt<T>, Value<T>>(value);
    auto ptr = std::make_unique<Opt<T>>(proxy, keys);
    ptr->defaultValue(def);
    return addOpt(std::move(ptr));
}

//===========================================================================
template <typename T>
inline Cli::Opt<T> & Cli::opt(T * value, const std::string & keys) {
    return opt(value, keys, T{});
}

//===========================================================================
template <typename T>
inline Cli::OptVec<T> &
Cli::optVec(std::vector<T> * values, const std::string & keys, int nargs) {
    auto proxy = getProxy<OptVec<T>, ValueVec<T>>(values);
    auto ptr = std::make_unique<OptVec<T>>(proxy, keys, nargs);
    return addOpt(std::move(ptr));
}

//===========================================================================
template <typename T, typename U, typename>
inline Cli::Opt<T> &
Cli::opt(Opt<T> & alias, const std::string & keys, const U & def) {
    return opt(&*alias, keys, def);
}

//===========================================================================
template <typename T>
inline Cli::Opt<T> & Cli::opt(Opt<T> & alias, const std::string & keys) {
    return opt(&*alias, keys, T{});
}

//===========================================================================
template <typename T>
inline Cli::OptVec<T> &
Cli::optVec(OptVec<T> & alias, const std::string & keys, int nargs) {
    return optVec(&*alias, keys, nargs);
}

//===========================================================================
template <typename T>
inline Cli::Opt<T> & Cli::opt(const std::string & keys, const T & def) {
    return opt<T>(nullptr, keys, def);
}

//===========================================================================
template <typename T>
inline Cli::OptVec<T> & Cli::optVec(const std::string & keys, int nargs) {
    return optVec<T>(nullptr, keys, nargs);
}

//===========================================================================
template <typename A> inline A & Cli::addOpt(std::unique_ptr<A> ptr) {
    auto & opt = *ptr;
    opt.parse(&Cli::defaultParse).command(command()).group(group());
    addOpt(std::unique_ptr<OptBase>(ptr.release()));
    return opt;
}

//===========================================================================
template <typename A, typename V, typename T>
inline std::shared_ptr<V> Cli::getProxy(T * ptr) {
    if (OptBase * opt = findOpt(ptr)) {
        A * dopt = static_cast<A *>(opt);
        return dopt->m_proxy;
    }

    // Since there was no pre-existing proxy to raw value, create new proxy.
    return std::make_shared<V>(ptr);
}

//===========================================================================
// fromString - converts from string to T
//===========================================================================
template <typename T> 
bool Cli::fromString(T & out, const std::string & src) const {
    // versions of fromString_impl taking ints as extra parameters are
    // preferred (better conversion from 0), if they don't exist for T 
    // (because no out=src assignment operator exists) then the versions 
    // taking longs are considered.
    return fromString_impl(out, src, 0, 0);
}

//===========================================================================
template <typename T>
auto Cli::fromString_impl(T & out, const std::string & src, int, int) const
    -> decltype(out = src, bool()) 
{
    out = src;
    return true;
}

//===========================================================================
template <typename T>
auto Cli::fromString_impl(T & out, const std::string & src, int, long) const
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
bool Cli::fromString_impl(
    T &, // out 
    const std::string &, // src
    long, 
    long
) const {
    // In order to parse an argument there must be one of:
    //  - assignment operator for std::string to T
    //  - istream extraction operator for T
    //  - parse action attached to the Opt<T> instance that doesn't call
    //    opt.parseValue(), such as opt.choice().
    assert(false && "no assignment from string or stream extraction operator");
    return false;
}

//===========================================================================
// toString - converts to string from T, returns empty string and returns
// false if conversion fails or no conversion available.
//===========================================================================
template <typename T>
bool Cli::toString(std::string & out, const T & src) const {
    return toString_impl(out, src, 0);
}

//===========================================================================
template <typename T>
auto Cli::toString_impl(std::string & out, const T & src, int) const 
    -> decltype(std::declval<std::ostream &>() << src, bool())
{
    m_interpreter.clear();
    m_interpreter.str("");
    if (!(m_interpreter << src)) {
        out.clear();
        return false;
    }
    out = m_interpreter.str();
    return true;
}

//===========================================================================
template <typename T>
bool Cli::toString_impl(std::string & out, const T &, long) const {
    out.clear();
    return false;
}


/****************************************************************************
*
*   CliLocal
*
*   Stand-alone cli instance independent of the shared configuration. Mainly
*   for testing.
*
***/

class DIM_LIB_DECL CliLocal : public Cli {
public:
    CliLocal();
};


/****************************************************************************
*
*   Cli::OptBase
*
*   Common base class for all options, has no information about the derived
*   options value type, and handles all services required by the parser.
*
***/

class DIM_LIB_DECL Cli::OptBase {
public:
    struct ChoiceDesc {
        std::string desc;
        std::string sortKey;
        size_t pos{0};
        bool def{false};
    };

public:
    OptBase(const std::string & keys, bool boolean);
    virtual ~OptBase() {}

    // Name of the last argument to populated the value, or an empty string if
    // it wasn't populated. For vectors, it's what populated the last value.
    virtual const std::string & from() const = 0;

    // Absolute position in argv[] of last the argument that populated the
    // value. For vectors, it refers to where the value on the back came from.
    // If pos() is 0 the value wasn't populated from the command line or
    // wasn't populated at all, check from() to tell the difference.
    //
    // It's possible for a value to come from prompt() or some other action
    // (which should set the position to 0) instead of the command.
    virtual int pos() const = 0;

    // set to passed in default value
    virtual void reset() = 0;

    // parses the string into the value, returns false on error
    virtual bool parseValue(const std::string & value);

    // set to (or add to vec) value for missing optional
    virtual void unspecifiedValue() = 0;

    // number of values, non-vecs are always 1
    virtual size_t size() const = 0;

    // defaults to use when populating the option from an action that's not
    // tied to a command line argument.
    const std::string & defaultFrom() const { return m_fromName; }
    std::string defaultPrompt() const;

protected:
    virtual bool fromString(Cli & cli, const std::string & value) = 0;
    virtual bool defaultValueStr(
        std::string & out, 
        const Cli & cli
    ) const = 0;

    virtual bool parseAction(Cli & cli, const std::string & value) = 0;
    virtual bool checkAction(Cli & cli, const std::string & value) = 0;
    virtual bool afterActions(Cli & cli) = 0;
    virtual void set(const std::string & name, size_t pos) = 0;

    // True for flags (bool on command line) that default to true.
    virtual bool inverted() const = 0;

    // Allows the type unaware layer to determine if a new option is pointing
    // at the same value as an existing option -- with RTTI disabled
    virtual bool sameValue(const void * value) const = 0;

    template <typename T> void setValueName();

    void setNameIfEmpty(const std::string & name);

    void index(OptIndex & ndx);
    void indexName(OptIndex & ndx, const std::string & name);
    void indexShortName(OptIndex & ndx, char name, bool invert, bool optional);
    void indexLongName(
        OptIndex & ndx,
        const std::string & name,
        bool invert,
        bool optional);

    std::string m_command;
    std::string m_group;

    bool m_visible{true};
    std::string m_desc;
    std::string m_valueDesc;
    std::unordered_map<std::string, ChoiceDesc> m_choiceDescs;

    // Are multiple values are allowed, and how many there can be (-1 for
    // unlimited).
    bool m_multiple{false};
    int m_nargs{1};

    // the value is a bool on the command line (no separate value)?
    bool m_bool{false};

    bool m_flagValue{false};
    bool m_flagDefault{false};

private:
    friend class Cli;
    std::string m_names;
    std::string m_fromName;
};

//===========================================================================
template <typename T> inline void Cli::OptBase::setValueName() {
    if (std::is_integral<T>::value) {
        m_valueDesc = "NUM";
    } else if (std::is_convertible<T, std::string>::value) {
        m_valueDesc = "STRING";
    } else {
        m_valueDesc = "VALUE";
    }
}

//===========================================================================
template <>
inline void Cli::OptBase::setValueName<std::experimental::filesystem::path>() {
    m_valueDesc = "FILE";
}


/****************************************************************************
*
*   Cli::OptShim
*
*   Common base for the more derived simple and vector option types. Has
*   knowledge of the value type but no knowledge about it.
*
***/

template <typename A, typename T> class Cli::OptShim : public OptBase {
public:
    OptShim(const std::string & keys, bool boolean);
    OptShim(const OptShim &) = delete;
    OptShim & operator=(const OptShim &) = delete;

    //-----------------------------------------------------------------------
    // Configuration

    // Controls whether or not the option appears in help pages.
    A & show(bool visible = true);

    // Set subcommand for which this is an option.
    A & command(const std::string & val);

    // Set group under which this argument will show up in the help text.
    A & group(const std::string & val);

    // Set desciption to associate with the argument in printHelp()
    A & desc(const std::string & val);

    // Set name of meta-variable in printHelp. For example, in "--count NUM"
    // this is used to change "NUM" to something else.
    A & valueDesc(const std::string & val);

    // Allows the default to be changed after the opt has been created.
    A & defaultValue(const T & val);

    // The implicit value is used for arguments with optional values when
    // the argument was specified in the command line without a value.
    A & implicitValue(const T & val);

    // Turns the argument into a feature switch, there are normally multiple
    // switches pointed at a single external value, one of which should be
    // flagged as the default. If none (or many) are set marked as the default
    // one will be choosen for you.
    A & flagValue(bool isDefault = false);

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
        const std::string & sortKey = {});

    // Fail if the value given for this option is not in within the range
    // (inclusive) of low to high.
    A & range(const T & low, const T & high);

    // Forces the value to be within the range, if it's less than the low
    // it's set to the low, if higher than high it's made merely high.
    A & clamp(const T & low, const T & high);

    // Enables prompting. When the option hasn't been provided on the command
    // line the user will be prompted for it. Use Cli::kPrompt* flags to
    // adjust behavior.
    A & prompt(int flags = 0);
    A & prompt(
        const std::string & msg, // custom prompt message
        int flags = 0            // Cli::kPrompt* flags
        );

    // Change the action to take when parsing this argument. The function
    // should:
    //  - Parse the src string and use the result to set the value (or
    //    push_back the new value for vectors).
    //  - Call cli.badUsage() with an error message if there's a problem.
    //  - Return false if the program should stop, otherwise true. This
    //    could be due to error or just to early out like "--version" and
    //    "--help".
    //
    // You can use opt.from() and opt.pos() to get the argument name that the
    // value was attached to on the command line and its position in argv[].
    // For bool arguments the source value string will always be either "0"
    // or "1".
    //
    // If you just need support for a new type you can provide a std::istream
    // extraction (>>) or assignment from std::string operator and the
    // default parse action will pick it up.
    using ActionFn = bool(Cli & cli, A & opt, const std::string & val);
    A & parse(std::function<ActionFn> fn);

    // Action to take after each value is parsed, unlike parsing where there
    // can only be one action, any number of check actions can be added. They
    // will be called in the order they were added and if any of them return
    // false it stops processing. As an example, opt.clamp() and opt.range()
    // both do their job by adding check actions.
    //
    // The function should:
    //  - Check the options new value, possibly in relation to other options.
    //  - Call cli.badUsage() with an error message if there's a problem.
    //  - Return false if the program should stop, otherwise true to let
    //    processing continue.
    //
    // The opt is fully populated so *opt, opt.from(), etc are all available.
    A & check(std::function<ActionFn> fn);

    // Action to run after all arguments have been parsed, any number of
    // after actions can be added and will, for each option, be called in the
    // order they're added. The function should:
    //  - Do something interesting.
    //  - Call cli.badUsage() and return false on error.
    //  - Return true if processing should continue.
    A & after(std::function<ActionFn> fn);

    //-----------------------------------------------------------------------
    // Queries
    const T & implicitValue() const { return m_implicitValue; }
    const T & defaultValue() const { return m_defValue; }

protected:
    bool inverted() const final;
    bool parseAction(Cli & cli, const std::string & value) final;
    bool checkAction(Cli & cli, const std::string & value) final;
    bool afterActions(Cli & cli) final;
    bool exec(
        Cli & cli,
        const std::string & value,
        std::vector<std::function<ActionFn>> actions);

    std::function<ActionFn> m_parse;
    std::vector<std::function<ActionFn>> m_checks;
    std::vector<std::function<ActionFn>> m_afters;

    T m_implicitValue{};
    T m_defValue{};
    std::vector<T> m_choices;
};

//===========================================================================
template <typename A, typename T>
inline Cli::OptShim<A, T>::OptShim(const std::string & keys, bool boolean)
    : OptBase(keys, boolean) {
    setValueName<T>();
}

//===========================================================================
template <typename A, typename T>
inline bool Cli::OptShim<A, T>::inverted() const {
    return this->m_bool && this->m_flagValue && this->m_flagDefault;
}

//===========================================================================
template <> inline bool Cli::OptShim<Cli::Opt<bool>, bool>::inverted() const {
    assert(this->m_bool);
    if (this->m_flagValue)
        return this->m_flagDefault;
    return this->defaultValue();
}

//===========================================================================
template <typename A, typename T>
inline bool
Cli::OptShim<A, T>::parseAction(Cli & cli, const std::string & val) {
    auto self = static_cast<A *>(this);
    return m_parse(cli, *self, val);
}

//===========================================================================
template <typename A, typename T>
inline bool
Cli::OptShim<A, T>::checkAction(Cli & cli, const std::string & val) {
    return exec(cli, val, m_checks);
}

//===========================================================================
template <typename A, typename T>
inline bool Cli::OptShim<A, T>::afterActions(Cli & cli) {
    return exec(cli, {}, m_afters);
}

//===========================================================================
template <typename A, typename T>
inline bool Cli::OptShim<A, T>::exec(
    Cli & cli,
    const std::string & val,
    std::vector<std::function<ActionFn>> actions) {
    auto self = static_cast<A *>(this);
    for (auto && fn : actions) {
        if (!fn(cli, *self, val))
            return false;
    }
    return true;
}

//===========================================================================
template <typename A, typename T>
inline A & Cli::OptShim<A, T>::show(bool visible) {
    m_visible = visible;
    return static_cast<A &>(*this);
}

//===========================================================================
template <typename A, typename T>
inline A & Cli::OptShim<A, T>::command(const std::string & val) {
    m_command = val;
    return static_cast<A &>(*this);
}

//===========================================================================
template <typename A, typename T>
inline A & Cli::OptShim<A, T>::group(const std::string & val) {
    m_group = val;
    return static_cast<A &>(*this);
}

//===========================================================================
template <typename A, typename T>
inline A & Cli::OptShim<A, T>::desc(const std::string & val) {
    m_desc = val;
    return static_cast<A &>(*this);
}

//===========================================================================
template <typename A, typename T>
inline A & Cli::OptShim<A, T>::valueDesc(const std::string & val) {
    m_valueDesc = val;
    return static_cast<A &>(*this);
}

//===========================================================================
template <typename A, typename T>
inline A & Cli::OptShim<A, T>::defaultValue(const T & val) {
    m_defValue = val;
    for (auto && cd : m_choiceDescs)
        cd.second.def = (val == m_choices[cd.second.pos]);
    return static_cast<A &>(*this);
}

//===========================================================================
template <typename A, typename T>
inline A & Cli::OptShim<A, T>::implicitValue(const T & val) {
    if (m_bool) {
        // they don't have separate values, just their presence/absence
        assert(!m_bool && "bool argument values are never implicit");
    } else {
        m_implicitValue = val;
    }
    return static_cast<A &>(*this);
}

//===========================================================================
template <typename A, typename T>
inline A & Cli::OptShim<A, T>::flagValue(bool isDefault) {
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
inline A & Cli::OptShim<A, T>::choice(
    const T & val,
    const std::string & key,
    const std::string & desc,
    const std::string & sortKey) {
    assert(!key.empty());
    ChoiceDesc & cd = m_choiceDescs[key];
    cd.pos = m_choices.size();
    cd.desc = desc;
    cd.sortKey = sortKey;
    cd.def = (val == this->defaultValue());
    m_choices.push_back(val);
    return static_cast<A &>(*this);
}

//===========================================================================
template <typename A, typename T>
inline A & Cli::OptShim<A, T>::parse(std::function<ActionFn> fn) {
    this->m_parse = fn;
    return static_cast<A &>(*this);
}

//===========================================================================
template <typename A, typename T>
inline A & Cli::OptShim<A, T>::check(std::function<ActionFn> fn) {
    this->m_checks.push_back(fn);
    return static_cast<A &>(*this);
}

//===========================================================================
template <typename A, typename T>
inline A & Cli::OptShim<A, T>::after(std::function<ActionFn> fn) {
    this->m_afters.push_back(fn);
    return static_cast<A &>(*this);
}

//===========================================================================
template <typename A, typename T>
inline A & Cli::OptShim<A, T>::range(const T & low, const T & high) {
    assert(low < high && !(high < low));
    check([low, high](auto & cli, auto & opt, auto & val) {
        if (*opt < low || high < *opt) {
            std::ostringstream os;
            os << "Out of range '" << opt.from() << "' value [" << low << " - "
               << high << "]";
            return cli.badUsage(os.str(), val);
        }
        return true;
    });
    return static_cast<A &>(*this);
}

//===========================================================================
template <typename A, typename T>
inline A & Cli::OptShim<A, T>::clamp(const T & low, const T & high) {
    assert(low < high && !(high < low));
    check([low, high](auto & /* cli */, auto & opt, auto & /* val */) {
        if (*opt < low) {
            *opt = low;
        } else if (high < *opt) {
            *opt = high;
        }
        return true;
    });
    return static_cast<A &>(*this);
}

//===========================================================================
template <typename A, typename T> A & Cli::OptShim<A, T>::prompt(int flags) {
    return prompt(this->defaultPrompt() + ":", flags);
}

//===========================================================================
template <typename A, typename T>
A & Cli::OptShim<A, T>::prompt(const std::string & msg, int flags) {
    after([=](auto & cli, auto & opt, auto & /* val */) {
        return cli.prompt(opt, msg, flags);
    });
    return static_cast<A &>(*this);
}


/****************************************************************************
*
*   Cli::ArgMatch
*
*   Reference to the commandline argument that was used to populate a value
*
***/

struct Cli::ArgMatch {
    // name of the argument that populated the value, or an empty
    // string if it wasn't populated.
    std::string name;

    // member of argv[] that populated the value or 0 if it wasn't.
    int pos{0};
};


/****************************************************************************
*
*   Cli::Value
*
***/

template <typename T> struct Cli::Value {
    // where the value came from
    ArgMatch m_match;

    // the value was explicitly set
    bool m_explicit{false};

    // points to the opt with the default flag value
    Opt<T> * m_defFlagOpt{nullptr};

    T * m_value{nullptr};
    T m_internal;

    Value(T * value)
        : m_value(value ? value : &m_internal) {}
};


/****************************************************************************
*
*   Cli::Opt
*
***/

template <typename T> class Cli::Opt : public OptShim<Opt<T>, T> {
public:
    Opt(std::shared_ptr<Value<T>> value, const std::string & keys);

    T & operator*() { return *m_proxy->m_value; }
    T * operator->() { return m_proxy->m_value; }

    // True if the value was populated from the command line and while the
    // value may be the same as the default it wasn't simply left that way.
    explicit operator bool() const { return m_proxy->m_explicit; }

    // OptBase
    const std::string & from() const final { return m_proxy->m_match.name; }
    int pos() const final { return m_proxy->m_match.pos; }
    void reset() final;
    void unspecifiedValue() final;
    size_t size() const final;

private:
    friend class Cli;
    bool fromString(Cli & cli, const std::string & value) final;
    bool defaultValueStr(std::string & out, const Cli & cli) const final;
    void set(const std::string & name, size_t pos) final;
    bool sameValue(const void * value) const final {
        return value == m_proxy->m_value;
    }

    std::shared_ptr<Value<T>> m_proxy;
};

//===========================================================================
template <typename T>
inline Cli::Opt<T>::Opt(
    std::shared_ptr<Value<T>> value,
    const std::string & keys)
    : OptShim<Opt, T>{keys, std::is_same<T, bool>::value}
    , m_proxy{value} {}

//===========================================================================
template <typename T>
inline void Cli::Opt<T>::set(const std::string & name, size_t pos) {
    m_proxy->m_match.name = name;
    m_proxy->m_match.pos = (int)pos;
    m_proxy->m_explicit = true;
}

//===========================================================================
template <typename T> inline void Cli::Opt<T>::reset() {
    if (!this->m_flagValue || this->m_flagDefault)
        *m_proxy->m_value = this->defaultValue();
    m_proxy->m_match.name.clear();
    m_proxy->m_match.pos = 0;
    m_proxy->m_explicit = false;
}

//===========================================================================
template <typename T>
inline bool Cli::Opt<T>::fromString(Cli & cli, const std::string & value) {
    if (this->m_flagValue) {
        bool flagged;
        if (!cli.fromString(flagged, value))
            return false;
        if (flagged)
            *m_proxy->m_value = this->defaultValue();
        return true;
    }
    if (!this->m_choices.empty()) {
        auto i = this->m_choiceDescs.find(value);
        if (i == this->m_choiceDescs.end())
            return false;
        *m_proxy->m_value = this->m_choices[i->second.pos];
        return true;
    }
    return cli.fromString(*m_proxy->m_value, value);
}

//===========================================================================
template <typename T> inline bool 
Cli::Opt<T>::defaultValueStr(std::string & out, const Cli & cli) const {
    return cli.toString(out, this->defaultValue());
}

//===========================================================================
template <typename T> inline void Cli::Opt<T>::unspecifiedValue() {
    *m_proxy->m_value = this->implicitValue();
}

//===========================================================================
template <typename T> inline size_t Cli::Opt<T>::size() const {
    return 1;
}


/****************************************************************************
*
*   Cli::ValueVec
*
***/

template <typename T> struct Cli::ValueVec {
    // where the values came from
    std::vector<ArgMatch> m_matches;

    // points to the opt with the default flag value
    OptVec<T> * m_defFlagOpt{nullptr};

    std::vector<T> * m_values{nullptr};
    std::vector<T> m_internal;

    ValueVec(std::vector<T> * value)
        : m_values(value ? value : &m_internal) {}
};


/****************************************************************************
*
*   Cli::OptVec
*
***/

template <typename T> class Cli::OptVec : public OptShim<OptVec<T>, T> {
public:
    OptVec(
        std::shared_ptr<ValueVec<T>> values,
        const std::string & keys,
        int nargs);

    std::vector<T> & operator*() { return *m_proxy->m_values; }
    std::vector<T> * operator->() { return m_proxy->m_values; }

    T & operator[](size_t index) { return (*m_proxy->m_values)[index]; }

    // True if values where added from the command line
    explicit operator bool() const { return !m_proxy->m_values->empty(); }

    // OptBase
    const std::string & from() const final { return from(size() - 1); }
    int pos() const final { return pos(size() - 1); }
    void reset() final;
    void unspecifiedValue() final;
    size_t size() const final;

    // Information about a specific member of the vector of values at the
    // time it was parsed. If the value vector has been changed (sort, erase,
    // insert, etc) by the app these will no longer correspond.
    const std::string & from(size_t index) const;
    int pos(size_t index) const;

private:
    friend class Cli;
    bool fromString(Cli & cli, const std::string & value) final;
    bool defaultValueStr(std::string & out, const Cli & cli) const final;
    void set(const std::string & name, size_t pos) final;
    bool sameValue(const void * value) const final {
        return value == m_proxy->m_values;
    }

    std::shared_ptr<ValueVec<T>> m_proxy;
    std::string m_empty;
};

//===========================================================================
template <typename T>
inline Cli::OptVec<T>::OptVec(
    std::shared_ptr<ValueVec<T>> values,
    const std::string & keys,
    int nargs)
    : OptShim<OptVec, T>{keys, std::is_same<T, bool>::value}
    , m_proxy(values) {
    this->m_multiple = true;
    this->m_nargs = nargs;
}

//===========================================================================
template <typename T>
inline void Cli::OptVec<T>::set(const std::string & name, size_t pos) {
    ArgMatch match;
    match.name = name;
    match.pos = (int)pos;
    m_proxy->m_matches.push_back(match);
}

//===========================================================================
template <typename T>
inline const std::string & Cli::OptVec<T>::from(size_t index) const {
    return index >= size() ? m_empty : m_proxy->m_matches[index].name;
}

//===========================================================================
template <typename T> inline int Cli::OptVec<T>::pos(size_t index) const {
    return index >= size() ? 0 : m_proxy->m_matches[index].pos;
}

//===========================================================================
template <typename T> inline void Cli::OptVec<T>::reset() {
    m_proxy->m_values->clear();
    m_proxy->m_matches.clear();
}

//===========================================================================
template <typename T>
inline bool Cli::OptVec<T>::fromString(Cli & cli, const std::string & value) {
    if (this->m_flagValue) {
        bool flagged;
        if (!cli.fromString(flagged, value))
            return false;
        if (flagged)
            m_proxy->m_values->push_back(this->defaultValue());
        return true;
    }
    if (!this->m_choices.empty()) {
        auto i = this->m_choiceDescs.find(value);
        if (i == this->m_choiceDescs.end())
            return false;
        m_proxy->m_values->push_back(this->m_choices[i->second.pos]);
        return true;
    }

    T tmp;
    if (!cli.fromString(tmp, value))
        return false;
    m_proxy->m_values->push_back(std::move(tmp));
    return true;
}

//===========================================================================
template <typename T> inline bool 
Cli::OptVec<T>::defaultValueStr(std::string & out, const Cli &) const {
    out.clear();
    return false;
}

//===========================================================================
template <typename T> inline void Cli::OptVec<T>::unspecifiedValue() {
    m_proxy->m_values->push_back(this->implicitValue());
}

//===========================================================================
template <typename T> inline size_t Cli::OptVec<T>::size() const {
    return m_proxy->m_values->size();
}

} // namespace


/****************************************************************************
*
*   Restore settings
*
***/

// Restore as many compiler settings as we can so they don't leak into
// the applications
#ifndef DIM_LIB_KEEP_MACROS

// clear all dim header macros so they don't leak into the application
#ifdef DIM_LIB_STANDALONE
#undef DIM_LIB_DYN_LINK
#undef DIM_LIB_KEEP_MACROS
#undef DIM_LIB_STANDALONE
#undef DIM_LIB_WINAPI_FAMILY_APP

#undef DIM_LIB_DECL
#undef DIM_LIB_NO_ENV
#undef DIM_LIB_NO_CONSOLE
#undef DIM_LIB_SOURCE
#endif

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif
