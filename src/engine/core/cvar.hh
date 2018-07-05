// -*- mode: c++ -*-
#ifndef __CVAR__90140360
#define __CVAR__90140360

#include <utility/convert.hh>
#include <utility/clamp.hh>
#include <prelude.hh>
#include <utility/optional.hh>

#include <boost/variant.hpp>

namespace imp {
  template <class T>
  class BasicCvar;

  class Cvar {
      String m_name;
      String m_description;
      uint32 m_flags;

  public:
      /*! Save the cvar in config.cfg */
      static constexpr int user_config = 0x1;
      /*! Save the cvar in system.cfg */
      static constexpr int sys_config = 0x2;
      /*! Default if sv_cheats == 0 */
      static constexpr int cheat = 0x4;
      /*! Don't list this cvar */
      static constexpr int hidden  = 0x8;
      /*! Synchronise the value with the client */
      static constexpr int net_server = 0x16;
      /*! Synchronise the value with the server */
      static constexpr int net_client = 0x32;

      static void write_config(FILE*);

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
          return m_name;
      }

      StringView description() const
      {
          return m_description;
      }

      void set_flag(int flag)
      { m_flags |= flag; }

      int flags() const
      { return m_flags; }

      bool is_config() const
      { return (m_flags & (user_config | sys_config)) != 0; }

      bool is_cheat() const
      { return (m_flags & cheat) != 0; }

      bool is_hidden() const
      { return (m_flags & hidden) != 0; }

      bool is_network() const
      { return (m_flags & (net_server | net_client)) != 0; }

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
      setter_func m_setter {};

      const T m_default;

      T m_value;

  public:
      BasicCvar(StringView name, StringView description):
          Cvar(name, description, 0),
          m_default(T()),
          m_value(T()) {}

      BasicCvar(StringView name, StringView description, T def, int flags = 0, setter_func setter = nullptr):
          Cvar(name, description, flags),
          m_setter(setter),
          m_default(def),
          m_value(def) {}

      const T& operator*() const
      { return m_value; }

      const T* operator->() const
      { return &m_value; }

      const T& value() const
      { return m_value; }

      explicit operator const T&() const
      { return m_value; }

      template <class U>
      BasicCvar& operator=(U value)
      {
          auto old = m_value;
          m_value = value;
          m_setter ? m_setter(*this, old, m_value) : (void) 0;
          update();
          return *this;
      }

      String string() const override
      {
          return boost::lexical_cast<String>(m_value);
      }

      String default_string() const override
      {
          return boost::lexical_cast<String>(m_default);
      }

      void set_string(StringView strValue) override
      {
          try {
              *this = from_string<T>(strValue);
          } catch (boost::bad_lexical_cast& e) {
              log::error("Bad lexical cast when converting '{}' to cvar of type '{}'", strValue, type_to_string<T>());
              *this = m_default;
          }
      }

      void reset_default() override
      {
          m_value = m_default;
          update();
      }
  };

  using IntCvar    = BasicCvar<int>;
  using FloatCvar  = BasicCvar<float>;
  using BoolCvar   = BasicCvar<bool>;
  using StringCvar = BasicCvar<String>;

#define __BASIC_CVAR_ARITHMETIC_OPERATORS(_Op) \
template <class T, class U> \
constexpr T operator _Op(const imp::BasicCvar<T> &l, const U &r) { return *l _Op r; } \
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
}

#endif //__CVAR__90140360
