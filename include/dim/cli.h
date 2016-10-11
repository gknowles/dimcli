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
    template <typename T> class ArgShim;
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
Cli::Arg<T> & Cli::arg(T * value, const std::string & keys, const T & def) {
    auto proxy = getProxy<Arg<T>, Value<T>>(value);
    auto ptr = std::make_unique<Arg<T>>(proxy, keys, def);
    auto & opt = *ptr;
    addValue(std::move(ptr));
    return opt;
}

//===========================================================================
template <typename T>
Cli::ArgVec<T> &
Cli::argVec(std::vector<T> * values, const std::string & keys, int nargs) {
    auto proxy = getProxy<ArgVec<T>, ValueVec<T>>(values);
    auto ptr = std::make_unique<ArgVec<T>>(proxy, keys, nargs);
    auto & opt = *ptr;
    addValue(std::move(ptr));
    return opt;
}

//===========================================================================
template <typename T>
Cli::Arg<T> & Cli::arg(const std::string & keys, const T & def) {
    return arg<T>(nullptr, keys, def);
}

//===========================================================================
template <typename T>
Cli::ArgVec<T> & Cli::argVec(const std::string & keys, int nargs) {
    return argVec<T>(nullptr, keys, nargs);
}

//===========================================================================
template <typename Arg, typename Value, typename Ptr>
std::shared_ptr<Value> Cli::getProxy(Ptr * ptr) {
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
*   Cli::Value
*
***/

template <typename T> struct Cli::Value {
    // name of the argument that populated the value, or an empty
    // string if it wasn't populated.
    std::string m_from;
    bool m_explicit{false};

    T * m_value;
    T m_internal;

    Value(T * value) : m_value(value ? value : &m_internal) {}
};


/****************************************************************************
*
*   Cli::ValueVec
*
***/

template <typename T> struct Cli::ValueVec {
    // name of the argument that populated the value, or an empty
    // string if it wasn't populated.
    std::string m_from;
    bool m_explicit{false}; // the value was explicitly set

    std::vector<T> * m_values;
    std::vector<T> m_internal;

    ValueVec(std::vector<T> * value) : m_values(value ? value : &m_internal) {}
};


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
    virtual void resetValue() = 0;       // set to passed in default
    virtual void unspecifiedValue() = 0; // set to default constructed
    virtual size_t size() = 0; // number of values, non-vecs are always 1
    std::string m_desc;
    bool m_multiple{false}; // there can be multiple values
    int m_nargs{1};

private:
    friend class Cli;
    std::string m_names;
    bool m_bool{false};     // the value is a bool (no separate value)
};


/****************************************************************************
*
*   Cli::ArgShim
*
***/

template <typename T> class Cli::ArgShim : public ArgBase {
public:
    using ArgBase::ArgBase;
    ArgShim(const ArgShim &) = delete;
    ArgShim & operator=(const ArgShim &) = delete;

    T & desc(const std::string & val) {
        m_desc = val;
        return static_cast<T &>(*this);
    }
};


/****************************************************************************
*
*   Cli::Arg
*
***/

template <typename T> class Cli::Arg : public ArgShim<Arg<T>> {
public:
    Arg(
        std::shared_ptr<Value<T>> value, 
        const std::string & keys, 
        const T & def = T{});

    Arg & optional(const T & implicit = T{});

    T & operator*() { return *m_proxy->m_value; }
    T * operator->() { return m_proxy->m_value; }

    // name of the argument that populated the value, or an empty
    // string if it wasn't populated.
    const std::string & from() const override { return m_proxy->m_from; }
    explicit operator bool() const { return m_proxy->m_explicit; }

private:
    friend class Cli;
    bool parseValue(
        const std::string & name, 
        const std::string & value) override;
    void resetValue() override;
    void unspecifiedValue() override;
    size_t size() override;

    std::shared_ptr<Value<T>> m_proxy;
    T m_defValue;
};

//===========================================================================
template <typename T>
inline Cli::Arg<T>::Arg(
    std::shared_ptr<Value<T>> value, const std::string & keys, const T & def)
    : ArgShim<Arg>{keys, std::is_same<T, bool>::value}
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
    *m_proxy->m_value = m_defValue;
    m_proxy->m_explicit = false;
    m_proxy->m_from.clear();
}

//===========================================================================
template <typename T> inline void Cli::Arg<T>::unspecifiedValue() {
    *m_proxy->m_value = {};
}

//===========================================================================
template <typename T> inline size_t Cli::Arg<T>::size() {
    return 1;
}


/****************************************************************************
*
*   Cli::ArgVec
*
***/

template <typename T> class Cli::ArgVec : public ArgShim<ArgVec<T>> {
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
    explicit operator bool() const { return m_proxy->m_explicit; }

private:
    friend class Cli;
    bool parseValue(
        const std::string & name, 
        const std::string & value) override;
    void resetValue() override;
    void unspecifiedValue() override;
    size_t size() override;

    std::shared_ptr<ValueVec<T>> m_proxy;
};

//===========================================================================
template <typename T>
inline Cli::ArgVec<T>::ArgVec(
    std::shared_ptr<ValueVec<T>> values, const std::string & keys, int nargs)
    : ArgShim<ArgVec>{keys, std::is_same<T, bool>::value}
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
    m_proxy->m_explicit = true;
    T tmp;
    if (!stringTo(tmp, value))
        return false;
    m_proxy->m_values->push_back(std::move(tmp));
    return true;
}

//===========================================================================
template <typename T> inline void Cli::ArgVec<T>::resetValue() {
    m_proxy->m_values->clear();
    m_proxy->m_explicit = false;
    m_proxy->m_from.clear();
}

//===========================================================================
template <typename T> inline void Cli::ArgVec<T>::unspecifiedValue() {
    m_proxy->m_values->emplace_back();
}

//===========================================================================
template <typename T> inline size_t Cli::ArgVec<T>::size() {
    return m_proxy->m_values->size();
}


/****************************************************************************
*
*   General
*
***/

std::ostream & operator<<(std::ostream & os, const Cli::ArgBase & val);

} // namespace
