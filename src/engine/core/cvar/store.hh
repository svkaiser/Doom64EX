// -*- mode: c++ -*-
#ifndef __CVAR_STORE__88662872
#define __CVAR_STORE__88662872

#include <unordered_map>
#include "data.hh"
#include "store_iterator.hh"
#include <iostream>

namespace imp {
  namespace cvar {
    struct var_already_exists : std::logic_error {
        using std::logic_error::logic_error;
    };

    template <class>
    class VarBase;

    template <class>
    class RefBase;

    template <class Predicate>
    class StoreRange {
        using hash_iterator = typename std::unordered_map<String, std::weak_ptr<Data>>::iterator;
        using iterator = StoreIterator<Predicate>;

        hash_iterator m_begin;
        hash_iterator m_end;
        Predicate m_pred;

    public:
        StoreRange(hash_iterator begin, hash_iterator end, Predicate pred):
            m_begin(begin), m_end(end), m_pred(pred)
        {
            // Find first element that the predicate will accept.
            for (; m_begin != m_end && !m_pred(m_begin->first, m_begin->second); ++m_begin);
        }

        iterator begin()
        { return { m_begin, m_end, m_pred }; }

        iterator end()
        { return { m_end, m_end, m_pred }; }
    };

    class Store {
        std::unordered_map<String, std::weak_ptr<Data>> m_vars;
        std::unordered_map<String, String> m_user_values;

        void p_add(std::shared_ptr<Data> data, StringView name, StringView description, const FlagSet& flags);

    public:
        using iterator = StoreIterator<>;
        using prefix_range = StoreRange<PrefixPred>;
        using prefix_iterator = StoreIterator<PrefixPred>;

        template <class T>
        void add(VarBase<T>& var, StringView name, StringView description, const FlagSet& flags = {})
        { p_add(var.m_data, name, description, flags); }

        void user_value(StringView name, StringView value);

        template <class T>
        Optional<RefBase<T>> find_type(StringView name);

        Optional<Ref> find(StringView name);

        StoreRange<FlagPred> iter_flags(const FlagSet& flags)
        { return { m_vars.begin(), m_vars.end(), flags }; }

        prefix_range iter_prefix(StringView prefix)
        { return { m_vars.begin(), m_vars.end(), prefix }; }

        iterator begin()
        { return { m_vars.begin(), m_vars.end() }; }

        iterator end()
        { return { m_vars.end(), m_vars.end() }; }
    };

    extern UniquePtr<Store> g_store;

  } // cvar namespace
} // imp namespace

#endif //__CVAR_STORE__88662872
