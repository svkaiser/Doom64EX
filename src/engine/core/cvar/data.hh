#ifndef __DATA__34737289
#define __DATA__34737289

#include <prelude.hh>
#include <functional>

#include <boost/variant.hpp>
#include <boost/lexical_cast.hpp>

#include "flags.hh"

namespace imp::cvar {
  template <class T>
  using Callback = std::function<void (const T&)>;

  template <class T>
  using OptCallback = Optional<Callback<T>>;

  template <class T>
  using VTuple = std::tuple<T, T, OptCallback<T>>;

  using Variant = boost::variant<
      VTuple<bool>,
      VTuple<int>,
      VTuple<float>,
      VTuple<String>
      >;

  class Data {
      String m_name {};
      String m_desc {};

      bool m_valid { true };

      Variant m_data;

      FlagSet m_flags;

      template <class T>
      VTuple<T>& m_tuple()
      { return boost::get<VTuple<T>>(m_data); }

      template <class T>
      const VTuple<T>& m_tuple() const
      { return boost::get<VTuple<T>>(m_data); }

  public:
      explicit Data(int def):
          m_data(VTuple<int> { def, def, nullopt }) {}

      explicit Data(float def):
          m_data(VTuple<float> { def, def, nullopt }) {}

      explicit Data(const String& def):
          m_data(VTuple<String> { def, def, nullopt }) {}

      explicit Data(bool def):
          m_data(VTuple<bool> { def, def, nullopt }) {}

      template <class T>
      T& get()
      { return std::get<0>(m_tuple<T>()); }

      template <class T>
      const T& get() const
      { return std::get<0>(m_tuple<T>()); }

      template <class T>
      const T& get_default() const
      { return std::get<1>(m_tuple<T>()); }

      bool is_valid() const
      { return m_valid; }

      StringView name() const
      { return m_name; }

      void set_name(StringView name)
      { m_name = name.to_string(); }

      StringView desc() const
      { return m_desc; }

      void set_desc(StringView desc)
      { m_desc = desc.to_string(); }

      void set_valid(bool valid)
      { m_valid = valid; }

      void set_flags(FlagSet flags)
      { m_flags = flags; }

      void set_flag(Flag flag)
      { m_flags.set(flag); }

      bool test_flag(Flag flag)
      { return m_flags.test(flag); }

      template <class T>
      void set_callback(const Callback<T>& cb)
      {
          std::get<2>(m_tuple<T>()) = cb;
      }

      void set_to_default()
      {
          boost::apply_visitor([](auto& var) {
              std::get<0>(var) = std::get<1>(var);
          }, m_data);
      }

      void set_value(StringView new_value)
      {
          boost::apply_visitor([&new_value](auto& var) {
              using type = std::decay_t<decltype(std::get<0>(var))>;
              try {
                  std::get<0>(var) = boost::lexical_cast<type>(new_value);
              } catch (boost::bad_lexical_cast&) {
                  /* Don't change value and ignore */
                  DEBUG("Can't cast '{}' to {}", new_value, boost::typeindex::type_id<type>().pretty_name());
              }
          }, m_data);
      }

      String get_value() const
      {
          return boost::apply_visitor([](const auto& var) {
              return boost::lexical_cast<String>(std::get<0>(var));
          }, m_data);
      }

      String get_default_string() const
      {
          return boost::apply_visitor([](const auto& var) {
              return boost::lexical_cast<String>(std::get<1>(var));
          }, m_data);
      }

      void update() const
      {
          boost::apply_visitor([](const auto &var) {
              auto ocb = std::get<2>(var);
              if (ocb) {
                  (*ocb)(std::get<0>(var));
              }
          }, m_data);
      }

      const FlagSet& flags() const
      { return m_flags; }
  };
}

#endif //__DATA__34737289
