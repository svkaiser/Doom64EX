// -*- mode: c++ -*-
#ifndef __BINARYREADER__49644297
#define __BINARYREADER__49644297

#include <istream>
#include <type_traits>
#include <imp/util/Endian>
#include <imp/util/StringView>

namespace imp {
  struct magic_error : std::runtime_error {
      using std::runtime_error::runtime_error;
  };

  // namespace detail {
  //   template <class T, class... Args>
  //   struct read_big_endian {
  //       void operator(std::istream& s, T& out, Args&... )
  //       {
            
  //       }
  //   }
  // }

  class BinaryReader {
      std::istream& s_;
      size_t size_ {};

      template <class T>
      void unsafe_read(T& out)
      {
          static_assert(std::is_standard_layout<T>::value, "T must have standard layout");
          static_assert(!std::is_pointer<T>::value, "T must not be a pointer");
          static_assert(!std::is_const<T>::value, "T must not be const");
          s_.read(reinterpret_cast<char*>(&out), sizeof(T));
          size_ += sizeof(T);
      }

      using traits_type = std::istream::traits_type;

  public:
      BinaryReader(std::istream& s):
          s_(s)
      {}

      /*! Magic */
      BinaryReader& magic(StringView cmp)
      {
          char str[cmp.length() + 1];
          s_.read(str, cmp.length());
          str[cmp.length()] = 0;

          if (cmp != str)
              throw magic_error("Magic string doesn't match");

          size_ += cmp.length();
          return *this;
      }

      /*! Ignore some number of bytes */
      BinaryReader& ignore(std::streamsize count = 1, int delim = std::istream::traits_type::eof())
      {
          s_.ignore(count, delim);

          size_ += count;
          return *this;
      }

      template <class T>
      BinaryReader& ignore_t(std::streamsize count = 1)
      {
          s_.ignore(count * sizeof(T));

          size_ += count * sizeof(T);
          return *this;
      }

      /*! Read a big endian */
      template <class T>
      BinaryReader& big_endian(T& out)
      {
          static_assert(std::is_integral<T>::value, "T must be an integral type");
          unsafe_read(out);
          set_big_endian(out);
          return *this;
      }

      template <class T, size_t N>
      BinaryReader& big_endian(T (&out)[N])
      {
          static_assert(std::is_integral<T>::value, "T must be an integral type");
          for (auto &x : out) {
              unsafe_read(x);
              set_big_endian(x);
          }
          return *this;
      }

      template <class T, class... Args>
      BinaryReader& big_endian(T& out, Args&... args)
      {
          return big_endian(out).big_endian(args...);
      }

      /*! Read a little endian */
      template <class T>
      BinaryReader& little_endian(T& out)
      {
          static_assert(std::is_integral<T>::value, "T must be an integral type");
          unsafe_read(out);
          set_little_endian(out);
          return *this;
      }

      /*! Read little endian ints */
      template <class T, class... Args>
      BinaryReader& little_endian(T& out, Args&... args)
      {
          return little_endian(out).little_endian(args...);
      }

      /*! Read a zero-terminated C string */
      BinaryReader& c_string(std::string& out)
      {
          int c;
          while ((c = s_.get()) != traits_type::eof()) {
              if (c == 0)
                  break;

              out.push_back(c);
          }

          return *this;
      }

      /*! Assert that the size is correct */
      BinaryReader& assert_size(std::size_t size)
      {
          assert(size_ == size);
          return *this;
      }
  };

  template <class T>
  T from_istream(std::istream& s)
  {
      return T::from_istream(s);
  }

  template <class T>
  std::unique_ptr<T[]> array_from_istream(std::istream& s, size_t count)
  {
      auto array = std::make_unique<T[]>(count);
      for (size_t i {}; i < count; ++i)
          array[i] = from_istream<T>(s);

      return array;
  }
}

#endif //__BINARYREADER__49644297
