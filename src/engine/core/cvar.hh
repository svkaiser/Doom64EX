// -*- mode: c++ -*-
#ifndef __CVAR__90140360
#define __CVAR__90140360

#include <utility/convert.hh>
#include <utility/clamp.hh>
#include <prelude.hh>
#include <utility/optional.hh>

namespace std {
  inline String to_string(String x)
  { return x; }
}

namespace imp {
  template <class T>
  class BasicCvar;

  class Cvar {
      String name_;
      String description_;
      int flags_;

  public:
      /*! Don't save the property in config.cfg */
      static constexpr int noconfig  = 0x1;
      /*!  */
      static constexpr int from_param = 0x2;
      /*! Readonly if sv_cheats == 0 */
      static constexpr int cheat   = 0x4;
      /*! Don't display in console */
      static constexpr int hidden  = 0x8;
      /*! Synchronise the value with the server */
      static constexpr int network = 0x16;

      static std::vector<Cvar *> all();

      /*!
       * \brief Look for a property
       * \param name Cvar name
       * \return A Cvar instance or nullptr if not found
       */
      static Cvar* find(StringView name);

      /*!
       * \brief Look for a typed property
       * \tparam T Cvar type
       * \param name Cvar name
       * \return A TypeCvar instance or nullptr if not found
       */
      template <class T>
      static BasicCvar<T>* find(StringView name)
      {
          if (auto p = find(name)) {
              return dynamic_cast<BasicCvar<T>*>(p);
          }
          return nullptr;
      }

      /*!
       * Get a list of all properties that whose name starts with a prefix
       * @param prefix Prefix to search for
       * @return a list of non-hidden properties
       */
      static Vector<Cvar *> partial(StringView prefix);

      Cvar(StringView name, StringView description, int flags = 0);

      virtual ~Cvar();

      StringView name() const
      {
          return name_;
      }

      StringView description() const
      {
          return description_;
      }

      void set_flag(int flag)
      { flags_ |= flag; }

      int flags() const
      { return flags_; }

      bool is_noconfig() const
      { return (flags_ & noconfig) != 0; }

      bool is_from_param() const
      { return (flags_ & from_param) != 0; }

      bool is_cheat() const
      { return (flags_ & cheat) != 0; }

      bool is_hidden() const
      { return (flags_ & hidden) != 0; }

      bool is_network() const
      { return (flags_ & network) != 0; }

      virtual String string() const = 0;

      virtual void set_string(StringView str) = 0;

      virtual String default_string() const = 0;

      virtual void reset_default() = 0;

      void update();
  };

  template <class T>
  class BasicCvar : public Cvar {
  public:
      using setter_func = void (*)(const BasicCvar &property, T oldValue, T& value);

  protected:
      setter_func mSetter {};

      const T mDefault;

      T mValue;

  public:
      BasicCvar(StringView name, StringView description):
          Cvar(name, description, 0),
          mDefault(T()),
          mValue(T()) {}

      BasicCvar(StringView name, StringView description, T def, int flags = 0, setter_func setter = nullptr):
          Cvar(name, description, flags),
          mSetter(setter),
          mDefault(def),
          mValue(def) {}

      const T& operator*() const
      { return mValue; }

      const T* operator->() const
      { return &mValue; }

      const T& value() const
      { return mValue; }

      template <class U>
      explicit operator U() const
      { return static_cast<U>(mValue); }

      template <class U>
      BasicCvar& operator=(U value)
      {
          auto old = mValue;
          mValue = value;
          mSetter ? mSetter(*this, old, mValue) : (void) 0;
          update();
          return *this;
      }

      String string() const override
      {
          return std::to_string(mValue);
      }

      String default_string() const override
      {
          return std::to_string(mDefault);
      }

      void set_string(StringView strValue) override
      {
          try {
              *this = from_string<T>(strValue);
          } catch (boost::bad_lexical_cast& e) {
              log::error("Bad lexical cast when converting '{}' to cvar of type '{}'", strValue, type_to_string<T>());
              *this = mDefault;
          }
      }

      void reset_default() override
      {
          mValue = mDefault;
          update();
      }
  };

  using IntCvar    = BasicCvar<int>;
  using FloatCvar  = BasicCvar<float>;
  using BoolCvar   = BasicCvar<bool>;
  using StringCvar = BasicCvar<String>;
}

#define __BASIC_CVAR_ARITHMETIC_OPERATORS(_Op) \
template <class T, class U> constexpr T operator _Op(const imp::BasicCvar<T> &l, const U &r) { return *l _Op r; } \
template <class T, class U> constexpr T operator _Op(const T &l, const imp::BasicCvar<U> &r) { return l _Op *r; }

__BASIC_CVAR_ARITHMETIC_OPERATORS(+)
__BASIC_CVAR_ARITHMETIC_OPERATORS(-)
__BASIC_CVAR_ARITHMETIC_OPERATORS(*)
__BASIC_CVAR_ARITHMETIC_OPERATORS(/)
__BASIC_CVAR_ARITHMETIC_OPERATORS(%)

#undef __BASIC_CVAR_ARITHMETIC_OPERATORS

#define __BASIC_CVAR_COMPARISON_OPERATORS(_Op) \
template <class T, class U> constexpr bool operator _Op(const imp::BasicCvar<T> &l, const imp::BasicCvar<U> &r) { return *l _Op *r; } \
template <class T, class U> constexpr bool operator _Op(const imp::BasicCvar<T> &l, const U &r) { return *l _Op r; } \
template <class T, class U> constexpr bool operator _Op(const T &l, const imp::BasicCvar<U> &r) { return l _Op *r; }

__BASIC_CVAR_COMPARISON_OPERATORS(==)
__BASIC_CVAR_COMPARISON_OPERATORS(!=)
__BASIC_CVAR_COMPARISON_OPERATORS(<)
__BASIC_CVAR_COMPARISON_OPERATORS(>)
__BASIC_CVAR_COMPARISON_OPERATORS(<=)
__BASIC_CVAR_COMPARISON_OPERATORS(>=)

#undef __BASIC_CVAR_COMPARISON_OPERATORS

#endif //__CVAR__90140360
