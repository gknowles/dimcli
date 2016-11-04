// cli.h - dim cli
//
// For documentation and examples follow the links at:
// https://github.com/gknowles/dimcli

#pragma once

#include "config.h"

#include "util.h"

#include <cassert>
#include <experimental/filesystem>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace Dim {


/****************************************************************************
*
*   Cli
*
***/

class Cli {
public:
    class ArgBase;
    template <typename A, typename T> class ArgShim;
    template <typename T> class Arg;
    template <typename T> class ArgVec;

    template <typename T> struct Value;
    template <typename T> struct ValueVec;

public:
    Cli();

    //-----------------------------------------------------------------------
    // Configuration
    template <typename T,
              typename U,
              typename = enable_if<is_convertible<U, T>::value>::type>
    Arg<T> & arg(T * value, const std::string & keys, const U & def);

    template <typename T> Arg<T> & arg(T * value, const std::string & keys);

    template <typename T>
    ArgVec<T> &
    argVec(std::vector<T> * values, const std::string & keys, int nargs = -1);

    template <typename T>
    Arg<T> & arg(const std::string & keys, const T & def = {});

    template <typename T>
    ArgVec<T> & argVec(const std::string & keys, int nargs = -1);

    // Add --version argument that shows "${progName.stem()} version ${ver}"
    // and exits.
    Arg<bool> &
    versionArg(const std::string & ver, const std::string & progName = {});

    //-----------------------------------------------------------------------
    // Parsing
    bool parse(size_t argc, char * argv[]);
    bool parse(std::ostream & os, size_t argc, char * argv[]);

    void resetValues();

    // Intended for use from return statements in action callbacks. Sets
    // exit code (to EX_USAGE) and error msg, then returns false.
    bool badUsage(const std::string & msg);

    //-----------------------------------------------------------------------
    // Inspection after parsing
    int exitCode() const { return m_exitCode; };
    const std::string & errMsg() const { return m_errMsg; }

    // Program name received in argv[0]
    const std::string & progName() const { return m_progName; }

    // writeHelp & writeUsage return the current exitCode()
    int writeHelp(std::ostream & os, const std::string & progName = {}) const;
    int writeUsage(std::ostream & os, const std::string & progName = {}) const;

private:
    std::string Cli::optionList(ArgBase & arg) const;
    std::string Cli::optionList(ArgBase & arg, bool disableOptions) const;

    bool defaultAction(ArgBase & arg, const std::string & val);

    void addLongName(
        const std::string & name, ArgBase * val, bool invert, bool optional);
    void addArgName(const std::string & name, ArgBase * val);
    void addArg(std::unique_ptr<ArgBase> ptr);
    bool parseAction(ArgBase & out, const std::string & name, const char src[]);

    template <typename Arg, typename Value, typename Ptr>
    std::shared_ptr<Value> getProxy(Ptr * ptr);
    template <typename A> A & addArg(std::unique_ptr<A> ptr);

    struct ArgName {
        ArgBase * val;
        bool invert;      // set to false instead of true (only for bools)
        bool optional;    // value doesn't have to be present? (non-bools only)
        std::string name; // name of argument (only for positionals)
    };
    std::list<std::unique_ptr<ArgBase>> m_args;
    std::map<char, ArgName> m_shortNames;
    std::map<std::string, ArgName> m_longNames;
    std::vector<ArgName> m_argNames;

    int m_exitCode{0};
    std::string m_errMsg;
    std::string m_progName;
};

//===========================================================================
template <typename T, typename U, typename>
inline Cli::Arg<T> &
Cli::arg(T * value, const std::string & keys, const U & def) {
    auto proxy = getProxy<Arg<T>, Value<T>>(value);
    auto ptr = std::make_unique<Arg<T>>(proxy, keys, def);
    return addArg(std::move(ptr));
}

//===========================================================================
template <typename T>
inline Cli::Arg<T> & Cli::arg(T * value, const std::string & keys) {
    return arg<T>(value, keys, T{});
}

//===========================================================================
template <typename T>
inline Cli::ArgVec<T> &
Cli::argVec(std::vector<T> * values, const std::string & keys, int nargs) {
    auto proxy = getProxy<ArgVec<T>, ValueVec<T>>(values);
    auto ptr = std::make_unique<ArgVec<T>>(proxy, keys, nargs);
    return addArg(std::move(ptr));
}

//===========================================================================
template <typename T>
inline Cli::Arg<T> & Cli::arg(const std::string & keys, const T & def) {
    return arg<T>(nullptr, keys, def);
}

//===========================================================================
template <typename T>
inline Cli::ArgVec<T> & Cli::argVec(const std::string & keys, int nargs) {
    return argVec<T>(nullptr, keys, nargs);
}

//===========================================================================
template <typename Arg, typename Value, typename Ptr>
inline std::shared_ptr<Value> Cli::getProxy(Ptr * ptr) {
    if (ptr) {
        for (auto && a : m_args) {
            auto ap = dynamic_cast<Arg *>(a.get());
            if (ap && &**ap == ptr)
                return ap->m_proxy;
        }
    }

    // Since there was no pre-existing proxy to raw value, create new proxy.
    return std::make_shared<Value>(ptr);
}

//===========================================================================
template <typename A> inline A & Cli::addArg(std::unique_ptr<A> ptr) {
    auto & opt = *ptr;
    opt.action(&Cli::defaultAction);
    addArg(std::unique_ptr<ArgBase>(ptr.release()));
    return opt;
}


/****************************************************************************
*
*   Cli::ArgBase
*
***/

class Cli::ArgBase {
public:
    ArgBase(const std::string & keys, bool boolean);
    virtual ~ArgBase() {}

    // Name of the argument that populated the value, or an empty
    // string if it wasn't populated.
    virtual const std::string & from() const = 0;

    // set to passed in default
    virtual void reset() = 0;

    // parses the string into the value, returns false on error
    virtual bool parseValue(const std::string & value) = 0;

    // set to (or add to vec) value for missing optional
    virtual void unspecifiedValue() = 0;

    // number of values, non-vecs are always 1
    virtual size_t size() const = 0;

protected:
    virtual bool parseAction(Cli & cli, const std::string & value) = 0;
    virtual void set(const std::string & name) = 0;

    template <typename T> void setValueName();

    std::string m_desc;
    std::string m_valueName;

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
};

//===========================================================================
template <typename T> inline void Cli::ArgBase::setValueName() {
    if (is_integral<T>::value) {
        m_valueName = "NUM";
    } else if (is_convertible<T, std::string>::value) {
        m_valueName = "STRING";
    } else {
        m_valueName = "VALUE";
    }
}

//===========================================================================
template <>
inline void Cli::ArgBase::setValueName<std::experimental::filesystem::path>() {
    m_valueName = "FILE";
}


/****************************************************************************
*
*   Cli::ArgShim
*
***/

template <typename A, typename T> class Cli::ArgShim : public ArgBase {
public:
    ArgShim(const std::string & keys, bool boolean);
    ArgShim(const ArgShim &) = delete;
    ArgShim & operator=(const ArgShim &) = delete;

    // Set desciption to associate with the argument in writeHelp()
    A & desc(const std::string & val);

    // Set name of meta-variable in writeHelp. For example, in "--count NUM"
    // this is used to change "NUM" to something else.
    A & descValue(const std::string & val);

    // Allows the default to be changed after the arg has been created.
    A & defaultValue(const T & val);

    // The implicit value is used for arguments with optional values when
    // the argument was specified in the command line without a value.
    A & implicitValue(const T & val);

    // Turns the argument into a feature switch, there are normally multiple
    // switches pointed at a single external value, one of which should be
    // flagged as the default. If none (or many) are set marked as the default
    // a default will be choosen for you.
    A & flagValue(bool isDefault = false);

    // Change the action to take when parsing this argument. The function
    // should:
    //  - parse the src string and use the result to set the value (or
    //    push_back the new value for vectors).
    //  - call cli.badUsage() with an error message if there's a problem
    //  - return false if the program should stop, otherwise true. This
    //    could be due to error or just to early out like "--version" and
    //    "--help".
    //
    // You can use arg.from() to get the argument name that the value was
    // attached to on the command line. For bool arguments the source value
    // string will always be either "0" or "1".
    //
    // If you just need support for a new type you can provide a std::istream
    // extraction (>>) or assignment from std::string operator and the
    // default action will pick it up.
    using ActionFn = bool(Cli & cli, A & arg, const std::string & src);
    A & action(std::function<ActionFn> fn);

protected:
    bool parseAction(Cli & cli, const std::string & value) override;

    std::function<ActionFn> m_action;
    T m_implicitValue{};
    T m_defValue{};
};

//===========================================================================
template <typename A, typename T>
inline Cli::ArgShim<A, T>::ArgShim(const std::string & keys, bool boolean)
    : ArgBase(keys, boolean) {
    setValueName<T>();
}

//===========================================================================
template <typename A, typename T>
inline bool
Cli::ArgShim<A, T>::parseAction(Cli & cli, const std::string & val) {
    auto self = static_cast<A *>(this);
    return m_action(cli, *self, val);
}

//===========================================================================
template <typename A, typename T>
inline A & Cli::ArgShim<A, T>::desc(const std::string & val) {
    m_desc = val;
    return static_cast<A &>(*this);
}

//===========================================================================
template <typename A, typename T>
inline A & Cli::ArgShim<A, T>::descValue(const std::string & val) {
    m_valueName = val;
    return static_cast<A &>(*this);
}

//===========================================================================
template <typename A, typename T>
inline A & Cli::ArgShim<A, T>::defaultValue(const T & val) {
    m_defValue = val;
    return static_cast<A &>(*this);
}

//===========================================================================
template <typename A, typename T>
inline A & Cli::ArgShim<A, T>::implicitValue(const T & val) {
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
inline A & Cli::ArgShim<A, T>::flagValue(bool isDefault) {
    auto self = static_cast<A *>(this);
    m_flagValue = true;
    if (!self->m_proxy->m_defFlagArg) {
        self->m_proxy->m_defFlagArg = self;
        m_flagDefault = true;
    } else if (isDefault) {
        self->m_proxy->m_defFlagArg->m_flagDefault = false;
        self->m_proxy->m_defFlagArg = self;
        m_flagDefault = true;
    } else {
        m_flagDefault = false;
    }
    m_bool = true;
    return *self;
}

//===========================================================================
template <typename A, typename T>
inline A & Cli::ArgShim<A, T>::action(std::function<ActionFn> fn) {
    m_action = fn;
    return static_cast<A &>(*this);
}


/****************************************************************************
*
*   Cli::Value
*
***/

template <typename T> struct Cli::Value {
    // name of the argument that populated the value, or an empty
    // string if it wasn't populated.
    std::string m_from;

    // the value was explicitly set
    bool m_explicit{false};

    // points to the arg with the default flag value
    Cli::Arg<T> * m_defFlagArg{nullptr};

    T * m_value{nullptr};
    T m_internal;

    Value(T * value)
        : m_value(value ? value : &m_internal) {}
};


/****************************************************************************
*
*   Cli::Arg
*
***/

template <typename T> class Cli::Arg : public ArgShim<Arg<T>, T> {
public:
    typedef T value_type;

public:
    Arg(std::shared_ptr<Value<T>> value,
        const std::string & keys,
        const T & def = {});

    T & operator*() { return *m_proxy->m_value; }
    T * operator->() { return m_proxy->m_value; }

    // True if the value was populated from the command line and while the
    // value may be the same as the default it wasn't simply left that way.
    explicit operator bool() const { return m_proxy->m_explicit; }

    // ArgBase
    const std::string & from() const override { return m_proxy->m_from; }
    void reset() override;
    bool parseValue(const std::string & value) override;
    void unspecifiedValue() override;
    size_t size() const override;

private:
    friend class Cli;
    void set(const std::string & name) override;

    std::shared_ptr<Value<T>> m_proxy;
};

//===========================================================================
template <typename T>
inline Cli::Arg<T>::Arg(
    std::shared_ptr<Value<T>> value, const std::string & keys, const T & def)
    : ArgShim<Arg, T>{keys, std::is_same<T, bool>::value}
    , m_proxy{value} {
    m_defValue = def;
}

//===========================================================================
template <typename T> inline void Cli::Arg<T>::set(const std::string & name) {
    m_proxy->m_from = name;
    m_proxy->m_explicit = true;
}

//===========================================================================
template <typename T> inline void Cli::Arg<T>::reset() {
    if (!m_flagValue || m_flagDefault)
        *m_proxy->m_value = m_defValue;
    m_proxy->m_from.clear();
    m_proxy->m_explicit = false;
}

//===========================================================================
template <typename T>
inline bool Cli::Arg<T>::parseValue(const std::string & value) {
    if (m_flagValue) {
        bool flagged;
        if (!stringTo(flagged, value))
            return false;
        if (flagged)
            *m_proxy->m_value = m_defValue;
        return true;
    }

    return stringTo(*m_proxy->m_value, value);
}

//===========================================================================
template <typename T> inline void Cli::Arg<T>::unspecifiedValue() {
    *m_proxy->m_value = m_implicitValue;
}

//===========================================================================
template <typename T> inline size_t Cli::Arg<T>::size() const {
    return 1;
}


/****************************************************************************
*
*   Cli::ValueVec
*
***/

template <typename T> struct Cli::ValueVec {
    // name of the last argument to append to the value, or an empty
    // string if it wasn't populated.
    std::string m_from;

    // points to the arg with the default flag value
    Cli::ArgVec<T> * m_defFlagArg{nullptr};

    std::vector<T> * m_values{nullptr};
    std::vector<T> m_internal;

    ValueVec(std::vector<T> * value)
        : m_values(value ? value : &m_internal) {}
};


/****************************************************************************
*
*   Cli::ArgVec
*
***/

template <typename T> class Cli::ArgVec : public ArgShim<ArgVec<T>, T> {
public:
    using value_type = T;

public:
    ArgVec(
        std::shared_ptr<ValueVec<T>> values,
        const std::string & keys,
        int nargs);

    std::vector<T> & operator*() { return *m_proxy->m_values; }
    std::vector<T> * operator->() { return m_proxy->m_values; }

    // True if values where added from the command line.
    explicit operator bool() const { return !m_proxy->m_values->empty(); }

    // ArgBase
    const std::string & from() const override { return m_proxy->m_from; }
    void reset() override;
    bool parseValue(const std::string & value) override;
    void unspecifiedValue() override;
    size_t size() const override;

private:
    friend class Cli;
    void set(const std::string & name) override;

    std::shared_ptr<ValueVec<T>> m_proxy;
};

//===========================================================================
template <typename T>
inline Cli::ArgVec<T>::ArgVec(
    std::shared_ptr<ValueVec<T>> values, const std::string & keys, int nargs)
    : ArgShim<ArgVec, T>{keys, std::is_same<T, bool>::value}
    , m_proxy(values) {
    m_multiple = true;
    m_nargs = nargs;
}

//===========================================================================
template <typename T>
inline void Cli::ArgVec<T>::set(const std::string & name) {
    m_proxy->m_from = name;
}

//===========================================================================
template <typename T> inline void Cli::ArgVec<T>::reset() {
    m_proxy->m_values->clear();
    m_proxy->m_from.clear();
}

//===========================================================================
template <typename T>
inline bool Cli::ArgVec<T>::parseValue(const std::string & value) {
    if (m_flagValue) {
        bool flagged;
        if (!stringTo(flagged, value))
            return false;
        if (flagged)
            m_proxy->m_values->push_back(m_defValue);
        return true;
    }

    T tmp;
    if (!stringTo(tmp, value))
        return false;
    m_proxy->m_values->push_back(std::move(tmp));
    return true;
}

//===========================================================================
template <typename T> inline void Cli::ArgVec<T>::unspecifiedValue() {
    m_proxy->m_values->push_back(m_implicitValue);
}

//===========================================================================
template <typename T> inline size_t Cli::ArgVec<T>::size() const {
    return m_proxy->m_values->size();
}

} // namespace
