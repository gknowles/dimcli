// cli.h - dim services
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
    class ValueBase;
    template <typename T> class ValueShim;
    template <typename T> class Value;
    template <typename T> class ValueVec;

public:
    void resetValues();
    bool parse(size_t argc, char * argv[]);
    bool parse(std::ostream & os, size_t argc, char * argv[]);

    template <typename T>
    Value<T> & arg(T * value, const std::string & keys, const T & def = {});
    template <typename T>
    ValueVec<T> & argVec(
        std::vector<T> * values, 
        const std::string & keys, 
        int nargs = -1);

    template <typename T>
    Value<T> & arg(const std::string & keys, const T & def = {});
    template <typename T> 
    ValueVec<T> & argVec(const std::string & keys, int nargs = -1);

    int exitCode() const { return m_exitCode; };
    const std::string & errMsg() const { return m_errMsg; }

private:
    bool badUsage(const std::string & msg);
    void addKey(const std::string & name, ValueBase * val);
    void addValue(std::unique_ptr<ValueBase> ptr);
    bool parseValue(ValueBase & out, const char src[]);

    struct ValueKey {
        ValueBase * val;
        bool invert;      // set to false instead of true (only for bools)
        bool optional;    // value doesn't have to be present? (non-bools only)
        std::string name; // name of argument (only for positionals)
    };
    std::list<std::unique_ptr<ValueBase>> m_values;
    std::map<char, ValueKey> m_shortNames;
    std::map<std::string, ValueKey> m_longNames;
    std::vector<ValueKey> m_args;

    int m_exitCode{0};
    std::string m_errMsg;
};

//===========================================================================
template <typename T>
Cli::Value<T> &
Cli::arg(T * value, const std::string & keys, const T & def) {
    auto ptr = std::make_unique<Value<T>>(value, keys, def);
    auto & opt = *ptr;
    addValue(std::move(ptr));
    return opt;
}

//===========================================================================
template <typename T>
Cli::ValueVec<T> &
Cli::argVec(std::vector<T> * values, const std::string & keys, int nargs) {
    auto ptr = std::make_unique<ValueVec<T>>(values, keys, nargs);
    auto & opt = *ptr;
    addValue(std::move(ptr));
    return opt;
}

//===========================================================================
template <typename T>
Cli::Value<T> & Cli::arg(const std::string & keys, const T & def) {
    return arg<T>(nullptr, keys, def);
}

//===========================================================================
template <typename T>
Cli::ValueVec<T> & Cli::argVec(const std::string & keys, int nargs) {
    return argVec<T>(nullptr, keys, nargs);
}


/****************************************************************************
*
*   Cli::ValueBase
*
***/

class Cli::ValueBase {
public:
    ValueBase(const std::string & keys, bool boolean);
    virtual ~ValueBase() {}

    // name of the argument that populated the value, or an empty 
    // string if it wasn't populated.
    const std::string & name() const { return m_refName; }

    explicit operator bool() const { return m_explicit; }

protected:
    virtual bool parseValue(const std::string & value) = 0;
    virtual void resetValue() = 0; // set to passed in default
    virtual void unspecifiedValue() = 0; // set to default constructed
    virtual size_t size() = 0; // number of values, non-vecs are always 1 
    std::string m_desc;
    bool m_multiple{false}; // there can be multiple values
    int m_nargs{1};

private:
    friend class Cli;
    std::string m_names;
    std::string m_refName;
    bool m_explicit{false}; // the value was explicitly set
    bool m_bool{false};     // the value is a bool (no separate value)
};


/****************************************************************************
*
*   Cli::ValueShim
*
***/

template <typename T> class Cli::ValueShim : public ValueBase {
public:
    using ValueBase::ValueBase;
    ValueShim(const ValueShim &) = delete;
    ValueShim & operator=(const ValueShim &) = delete;

    T & desc(const std::string & val) {
        m_desc = val;
        return static_cast<T &>(*this);
    }
};


/****************************************************************************
*
*   Cli::Value
*
***/

template <typename T> class Cli::Value : public ValueShim<Value<T>> {
public:
    Value(T * value, const std::string & keys, const T & def = T{});

    T & operator*() { return *m_value; }
    T * operator->() { return m_value; }

private:
    bool parseValue(const std::string & value) override;
    void resetValue() override;
    void unspecifiedValue() override;
    size_t size() override;

    T * m_value;
    T m_internal;
    T m_defValue;
};

//===========================================================================
template <typename T>
inline Cli::Value<T>::Value(
    T * value, const std::string & keys, const T & def)
    : ValueShim<Value>{keys, std::is_same<T, bool>::value}
    , m_value{value ? value : &m_internal}
    , m_defValue{def} {}

//===========================================================================
template <typename T>
inline bool Cli::Value<T>::parseValue(const std::string & value) {
    return stringTo(*m_value, value);
}

//===========================================================================
template <typename T> inline void Cli::Value<T>::resetValue() {
    *m_value = m_defValue;
}

//===========================================================================
template <typename T> inline void Cli::Value<T>::unspecifiedValue() {
    *m_value = {};
}

//===========================================================================
template <typename T> inline size_t Cli::Value<T>::size() {
    return 1;
}


/****************************************************************************
*
*   Cli::ValueVec
*
***/

template <typename T>
class Cli::ValueVec : public ValueShim<ValueVec<T>> {
public:
    ValueVec(std::vector<T> * values, const std::string & keys, int nargs);

    std::vector<T> & operator*() { return *m_values; }
    std::vector<T> * operator->() { return m_values; }

private:
    bool parseValue(const std::string & value) override;
    void resetValue() override;
    void unspecifiedValue() override;
    size_t size() override;

    std::vector<T> * m_values;
    std::vector<T> m_internal;
};

//===========================================================================
template <typename T>
inline Cli::ValueVec<T>::ValueVec(
    std::vector<T> * values, const std::string & keys, int nargs)
    : ValueShim<ValueVec>{keys, std::is_same<T, bool>::value}
    , m_values(values ? values : &m_internal) 
{
    m_multiple = true;
    m_nargs = nargs;
}

//===========================================================================
template <typename T>
inline bool Cli::ValueVec<T>::parseValue(const std::string & value) {
    T tmp;
    if (!stringTo(tmp, value))
        return false;
    m_values->push_back(std::move(tmp));
    return true;
}

//===========================================================================
template <typename T> inline void Cli::ValueVec<T>::resetValue() {
    m_values->clear();
}

//===========================================================================
template <typename T> inline void Cli::ValueVec<T>::unspecifiedValue() {
    m_values->emplace_back();
}

//===========================================================================
template <typename T> inline size_t Cli::ValueVec<T>::size() {
    return m_values->size();
}


/****************************************************************************
*
*   General
*
***/

std::ostream & operator<<(std::ostream & os, const Cli::ValueBase & val);

} // namespace
