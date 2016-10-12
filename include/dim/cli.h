// cli.h - dim cli
//
// For documentation and examples follow the links at:
// https://github.com/gknowles/dimcli

#pragma once

#include "config.h"
#include "util.h"

#include <cassert>
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
    void resetValues();
    bool parse(size_t argc, char * argv[]);
    bool parse(std::ostream & os, size_t argc, char * argv[]);

    template <typename T>
    Arg<T> & arg(T * value, const std::string & keys, const T & def = {});
    template <typename T>
    ArgVec<T> &
    argVec(std::vector<T> * values, const std::string & keys, int nargs = -1);

    template <typename T>
    Arg<T> & arg(const std::string & keys, const T & def = {});
    template <typename T>
    ArgVec<T> & argVec(const std::string & keys, int nargs = -1);

    int exitCode() const { return m_exitCode; };
    const std::string & errMsg() const { return m_errMsg; }

private:
    bool badUsage(const std::string & msg);
    void addKey(const std::string & name, ArgBase * val);
    void addValue(std::unique_ptr<ArgBase> ptr);
    bool parseValue(ArgBase & out, const std::string & name, const char src[]);

    template <typename Arg, typename Value, typename Ptr>
    std::shared_ptr<Value> getProxy(Ptr * ptr);

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
};

//===========================================================================
template <typename T>
inline Cli::Arg<T> & Cli::arg(T * value, const std::string & keys, const T & def) {
    auto proxy = getProxy<Arg<T>, Value<T>>(value);
    auto ptr = std::make_unique<Arg<T>>(proxy, keys, def);
    auto & opt = *ptr;
    addValue(std::move(ptr));
    return opt;
}

//===========================================================================
template <typename T>
inline Cli::ArgVec<T> &
Cli::argVec(std::vector<T> * values, const std::string & keys, int nargs) {
    auto proxy = getProxy<ArgVec<T>, ValueVec<T>>(values);
    auto ptr = std::make_unique<ArgVec<T>>(proxy, keys, nargs);
    auto & opt = *ptr;
    addValue(std::move(ptr));
    return opt;
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


/****************************************************************************
*
*   Cli::ArgBase
*
***/

class Cli::ArgBase {
public:
    ArgBase(const std::string & keys, bool boolean);
    virtual ~ArgBase() {}

    virtual const std::string & from() const = 0;

protected:
    virtual bool parseValue(
        const std::string & name, const std::string & value) = 0;

    // set to default constructed
    virtual void unspecifiedValue(const std::string & name) = 0; 

    // set to passed in default
    virtual void resetValue() = 0; 
    
    // number of values, non-vecs are always 1
    virtual size_t size() const = 0; 
    
    std::string m_desc;

    // Are multiple values are allowed, and how many there can be (-1 for 
    // unlimited).
    bool m_multiple{false}; 
    int m_nargs{1};

    // the value is a bool (no separate value)
    bool m_bool{false}; 

private:
    friend class Cli;
    std::string m_names;
};


/****************************************************************************
*
*   Cli::ArgShim
*
***/

template <typename A, typename T> class Cli::ArgShim : public ArgBase {
public:
    using ArgBase::ArgBase;
    ArgShim(const ArgShim &) = delete;
    ArgShim & operator=(const ArgShim &) = delete;

    A & desc(const std::string & val);
    A & implicitValue(const T & val);

protected:
    T m_implicitValue;
};

//===========================================================================
template <typename A, typename T>
inline A & Cli::ArgShim<A, T>::desc(const std::string & val) {
    m_desc = val;
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

    T * m_value;
    T m_internal;

    Value(T * value) : m_value(value ? value : &m_internal) {}
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
    Arg(
        std::shared_ptr<Value<T>> value, 
        const std::string & keys, 
        const T & def = {});

    T & operator*() { return *m_proxy->m_value; }
    T * operator->() { return m_proxy->m_value; }

    // Name of the argument that populated the value, or an empty
    // string if it wasn't populated.
    const std::string & from() const override { return m_proxy->m_from; }

    // True if the value was populated from the command line and while the
    // value may be the same as the default it wasn't simply left that way.
    explicit operator bool() const { return m_proxy->m_explicit; }

private:
    friend class Cli;
    bool parseValue(
        const std::string & name, 
        const std::string & value) override;
    void unspecifiedValue(const std::string & name) override;
    void resetValue() override;
    size_t size() const override;

    std::shared_ptr<Value<T>> m_proxy;
    T m_defValue;
};

//===========================================================================
template <typename T>
inline Cli::Arg<T>::Arg(
    std::shared_ptr<Value<T>> value, const std::string & keys, const T & def)
    : ArgShim<Arg, T>{keys, std::is_same<T, bool>::value}
    , m_proxy{value}
    , m_defValue{def} {}

//===========================================================================
template <typename T>
inline bool Cli::Arg<T>::parseValue(
    const std::string & name, 
    const std::string & value) {
    m_proxy->m_from = name;
    m_proxy->m_explicit = true;
    return stringTo(*m_proxy->m_value, value);
}

//===========================================================================
template <typename T> inline void Cli::Arg<T>::resetValue() {
    m_proxy->m_from.clear();
    m_proxy->m_explicit = false;
    *m_proxy->m_value = m_defValue;
}

//===========================================================================
template <typename T> 
inline void Cli::Arg<T>::unspecifiedValue(const std::string & name) {
    m_proxy->m_from = name;
    m_proxy->m_explicit = true;
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

    std::vector<T> * m_values;
    std::vector<T> m_internal;

    ValueVec(std::vector<T> * value) : m_values(value ? value : &m_internal) {}
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

    // name of the argument that populated the value, or an empty
    // string if it wasn't populated.
    const std::string & from() const override { return m_proxy->m_from; }

    // True if values where added from the command line.
    explicit operator bool() const { return !m_proxy->m_values->empty(); }

private:
    friend class Cli;
    bool parseValue(
        const std::string & name, 
        const std::string & value) override;
    void unspecifiedValue(const std::string & name) override;
    void resetValue() override;
    size_t size() const override;

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
inline bool Cli::ArgVec<T>::parseValue(
    const std::string & name, 
    const std::string & value) {
    m_proxy->m_from = name;
    T tmp;
    if (!stringTo(tmp, value))
        return false;
    m_proxy->m_values->push_back(std::move(tmp));
    return true;
}

//===========================================================================
template <typename T> inline void Cli::ArgVec<T>::resetValue() {
    m_proxy->m_from.clear();
    m_proxy->m_values->clear();
}

//===========================================================================
template <typename T> 
inline void Cli::ArgVec<T>::unspecifiedValue(const std::string & name) {
    m_proxy->m_from = name;
    m_proxy->m_values->push_back(m_implicitValue);
}

//===========================================================================
template <typename T> inline size_t Cli::ArgVec<T>::size() const {
    return m_proxy->m_values->size();
}


/****************************************************************************
*
*   General
*
***/

std::ostream & operator<<(std::ostream & os, const Cli::ArgBase & val);

} // namespace
