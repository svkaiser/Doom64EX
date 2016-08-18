
#include <gtest/gtest.h>
#include <kex/util/StringView>

namespace {
  constexpr const char *_char_strings[] = {
      "Hello",
      "hello",
      "HELLO"
  };

  constexpr const char16_t *_char16_strings[] = {
      u"Hello",
      u"hello",
      u"HELLO"
  };

  constexpr const char32_t *_char32_strings[] = {
      U"Hello",
      U"hello",
      U"HELLO"
  };

  constexpr const wchar_t *_wchar_strings[] = {
      L"Hello",
      L"hello",
      L"HELLO"
  };

  constexpr const char * const *strings(char)
  { return _char_strings; }

  constexpr const char16_t * const *strings(char16_t)
  { return _char16_strings; }

  constexpr const char32_t * const *strings(char32_t)
  { return _char32_strings; }

  constexpr const wchar_t * const *strings(wchar_t)
  { return _wchar_strings; }

  template <class CharT, class Traits = std::char_traits<CharT>>
  struct StringView_static_tests {
      using StringView = kex::BasicStringView<CharT, Traits>;
      constexpr static const CharT * const *strings = ::strings(CharT());

      static constexpr StringView remove_prefix_copy(const StringView &s, size_t n)
      {
          StringView r = s;
          r.remove_prefix(n);
          return r;
      }

      static constexpr StringView remove_suffix_copy(const StringView &s, size_t n)
      {
          StringView r = s;
          r.remove_suffix(n);
          return r;
      }

      static constexpr StringView sv_1 = strings[0]; // "Hello"
      static_assert(sv_1.length() == 5, "'Hello' has length 5");
      static_assert(sv_1 == sv_1, "'Hello' should equal 'Hello'");
      static_assert(sv_1 == strings[0], "'Hello' should equal 'Hello'");
      static_assert(strings[0] == sv_1, "'Hello' should equal 'Hello'");

      static constexpr StringView sv_2 = sv_1;
      static_assert(sv_2.length() == 5, "Copy should have the same length as original");
      static_assert(sv_1 == sv_2, "Copy should equal original");

      static constexpr StringView sv_3 = remove_prefix_copy(sv_2, 1);
      static_assert(sv_3.length() == 4, "");
      static_assert(sv_3 != sv_2, "");
      static_assert(sv_3 < sv_2, "");
      static_assert(!(sv_3 > sv_2), "");

      static constexpr StringView sv_4 = remove_suffix_copy(sv_3, 2);
      static_assert(sv_4.length() == 2, "");
      static_assert(sv_4 != sv_3, "");
      static_assert(sv_4 < sv_2, "");
      static_assert(!(sv_4 > sv_2), "");

      static constexpr StringView sv_5 = strings[1]; // "hello"
      static constexpr StringView sv_6 = strings[2]; // "HELLO"
      static_assert(sv_5 != sv_6, "");
      static_assert(sv_5 > sv_6, "");
  };

  StringView_static_tests<char> _char_test;
  StringView_static_tests<wchar_t> _wchar_test;

  template <class CharT>
  struct StringView : public ::testing::Test {
      using type = kex::BasicStringView<CharT>;
      using string = std::basic_string<CharT>;
      using cstr = const CharT *;

      static constexpr auto strings = ::strings(CharT());
  };

  using CharTypes = ::testing::Types<char, char16_t, char32_t, wchar_t>;

  TYPED_TEST_CASE(StringView, CharTypes);

  TYPED_TEST(StringView, ctor_and_assign)
  {
      using Sv = ::StringView<TypeParam>;

      // ctors
      typename Sv::string string = Sv::strings[0];
      typename Sv::type sv_from_cstr = Sv::strings[0];
      typename Sv::type sv_from_string = string;

      ASSERT_EQ(string.length(), sv_from_cstr.length());
      ASSERT_EQ(string.length(), sv_from_string.length());
      ASSERT_EQ(string, sv_from_cstr);
      ASSERT_EQ(string, sv_from_string);

      // assignment operators
      string = Sv::strings[1];
      sv_from_cstr = Sv::strings[1];
      sv_from_string = string;

      ASSERT_EQ(string.length(), sv_from_cstr.length());
      ASSERT_EQ(string.length(), sv_from_string.length());
      ASSERT_EQ(string, sv_from_cstr);
      ASSERT_EQ(string, sv_from_string);
  }

  TYPED_TEST(StringView, equality_operators)
  {
      using Sv = ::StringView<TypeParam>;

      typename Sv::cstr cstr = Sv::strings[0];
      typename Sv::string string = Sv::strings[0];
      typename Sv::type string_view = Sv::strings[0];

      // string_view is equal to others
      ASSERT_TRUE(string_view == string_view);
      ASSERT_TRUE(string_view == string);
      ASSERT_TRUE(string_view == cstr);
      ASSERT_TRUE(string == string_view);
      ASSERT_TRUE(cstr == string_view);
      ASSERT_FALSE(string_view != string_view);
      ASSERT_FALSE(string_view != string);
      ASSERT_FALSE(string_view != cstr);
      ASSERT_FALSE(string != string_view);
      ASSERT_FALSE(cstr != string_view);

      string_view = Sv::strings[1];

      // string_view is inequal to others
      ASSERT_TRUE(string_view != string);
      ASSERT_TRUE(string_view != cstr);
      ASSERT_TRUE(string != string_view);
      ASSERT_TRUE(cstr != string_view);
      ASSERT_FALSE(string_view == string);
      ASSERT_FALSE(string_view == cstr);
      ASSERT_FALSE(string == string_view);
      ASSERT_FALSE(cstr == string_view);
  }

  TYPED_TEST(StringView, swap)
  {
      using Sv = ::StringView<TypeParam>;

      typename Sv::type sv_1 = Sv::strings[0];
      typename Sv::type sv_2 = Sv::strings[1];

      ASSERT_TRUE(sv_1 != sv_2);
      ASSERT_TRUE(sv_1 < sv_2);

      sv_1.swap(sv_2);

      ASSERT_TRUE(sv_2 != sv_1);
      ASSERT_TRUE(sv_2 < sv_1);
  }

  TEST(StringView, literals)
  {
      auto sv = "Hello"_sv;
      auto u16sv = u"Hello"_sv;
      auto u32sv = U"Hello"_sv;
      auto wsv = L"Hello"_sv;

      ASSERT_EQ(5, sv.length());
      ASSERT_EQ(5, u16sv.length());
      ASSERT_EQ(5, u32sv.length());
      ASSERT_EQ(5, wsv.length());
  }
}
