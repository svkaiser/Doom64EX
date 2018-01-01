// -*- mode: c++ -*-
#ifndef __IMP_APP__27387470
#define __IMP_APP__27387470

#include "prelude.hh"
#include "utility/convert.hh"
#include "core/cvar.hh"

namespace imp {
  class Cvar;
  template <class T>
  class BasicCvar;

  namespace app {
    class Param;

    /** Program entry point */
    [[noreturn]]
    void main(int argc, char** argv);

    bool file_exists(StringView name);

    Optional<String> find_data_file(StringView name, StringView dir_hint = "");

    StringView program();

    void register_param(StringView, Param*);

    bool have_param(StringView name);

    const Param* param(StringView name);

    class Param {
    public:
        enum struct Arity {
            nullary, // 0 arguments
            unary,   // 1 argument
            nary,    // n arguments
        };

    private:
        bool have_ {};
        Arity arity_;

    public:
        Param(StringView name, Arity arity);

        Arity arity() const
        { return arity_; }

        void set_have()
        { have_ = true; }

        bool have() const
        { return have_; }

        bool operator!() const
        { return !have_; }

        operator bool() const
        { return have_; }

        virtual void add(StringView) = 0;
    };

    template <class T>
    class TypedParam : public Param {
        T value_ {};

        using return_type = std::conditional_t<std::is_fundamental<T>::value, T, const T&>;

    public:
        TypedParam(StringView name):
            Param(name, Arity::unary) {}

        return_type get() const
        { return value_; }

        void add(StringView val) override
        { value_ = from_string<T>(val); }
    };

    template <>
    class TypedParam<bool> : public Param {
    public:
        TypedParam(StringView name):
            Param(name, Arity::nullary) {}

        bool get() const
        { return have(); }

        void add(StringView val) override {}
    };

    class ListParam : public Param {
        Vector<String> list_;

    public:
        ListParam(StringView name):
            Param(name, Arity::nary) {}

        const Vector<String>& get() const
        { return list_; }

        void add(StringView val) override
        { list_.emplace_back(val); }
    };

    template <class T>
    class BasicCvarParam : public BasicCvar<T>, private Param {
    public:
        using Arity = Param::Arity;
        using setter_func = typename BasicCvar<T>::setter_func;

        BasicCvarParam(StringView name, StringView description):
            BasicCvar<T>(name, description),
            Param(name, Arity::unary) {}

        BasicCvarParam(StringView name, StringView description, T def, int flags = 0, setter_func setter = nullptr):
            BasicCvar<T>(name, description, def, flags, setter),
            Param(name, Arity::unary) {}

        BasicCvarParam(std::pair<StringView, StringView> name, StringView description):
            BasicCvar<T>(name.first, description),
            Param(name.second, Arity::unary) {}

        BasicCvarParam(std::pair<StringView, StringView> name, StringView description, T def, int flags = 0, setter_func setter = nullptr):
            BasicCvar<T>(name.first, description, def, flags, setter),
            Param(name.second, Arity::unary) {}

    private:
        void add(StringView val) override
        {
            *static_cast<BasicCvar<T>*>(this) = from_string<T>(val);
            this->set_flag(Cvar::from_param);
        }
    };

    using IntParam = TypedParam<int>;
    using FloatParam = TypedParam<float>;
    using BoolParam = TypedParam<bool>;
    using StringParam = TypedParam<String>;

    using IntCvarParam = BasicCvarParam<int>;
    using FloatCvarParam = BasicCvarParam<float>;
    using BoolCvarParam = BasicCvarParam<bool>;
    using StringCvarParam = BasicCvarParam<String>;
  }
}

#endif //__IMP_APP__27387470
