// -*- mode: c++ -*-
#ifndef __IMP_ENDIAN__41348837
#define __IMP_ENDIAN__41348837

#include "types.hh"

#if defined(_WIN32)
// We assume that Windows only runs on little endian CPUs.
// (Intel and ARM)
# define BIG_ENDIAN 0x1234
# define LITTLE_ENDIAN 0x4321
# define BYTE_ORDER LITTLE_ENDIAN
#elif defined(__APPLE__)
# include <machine/endian.h>
#else
# include <endian.h>
#endif

#if defined(_WIN32)
# include <stdlib.h>
namespace imp {
  inline int16 swap_bytes(int16 x) noexcept
  { return _byteswap_ushort(x); }

  inline uint16 swap_bytes(uint16 x) noexcept
  { return _byteswap_ushort(x); }

  inline int32 swap_bytes(int32 x) noexcept
  { return _byteswap_ulong(x); }

  inline uint32 swap_bytes(uint32 x) noexcept
  { return _byteswap_ulong(x); }

  inline int64 swap_bytes(int64 x) noexcept
  { return _byteswap_uint64(x); }

  inline uint64 swap_bytes(uint64 x) noexcept
  { return _byteswap_uint64(x); }
}
#elif defined(__APPLE__)
# include <libkern/OSByteOrder.h>
namespace imp {
    inline int16 swap_bytes(int16 x) noexcept
    { return OSSwapInt16(x); }

    inline uint16 swap_bytes(uint16 x) noexcept
    { return OSSwapInt16(x); }

    inline int32 swap_bytes(int32 x) noexcept
    { return OSSwapInt32(x); }

    inline uint32 swap_bytes(uint32 x) noexcept
    { return OSSwapInt32(x); }

    inline int64 swap_bytes(int64 x) noexcept
    { return OSSwapInt64(x); }

    inline uint64 swap_bytes(uint64 x) noexcept
    { return OSSwapInt64(x); }
}
#elif defined(__GNUC__) || defined(__clang__)
namespace imp {
  inline int16 swap_bytes(int16 x) noexcept
  { return __bswap_16(x); }

  inline uint16 swap_bytes(uint16 x) noexcept
  { return __bswap_16(x); }

  inline int32 swap_bytes(int32 x) noexcept
  { return __bswap_32(x); }

  inline uint32 swap_bytes(uint32 x) noexcept
  { return __bswap_32(x); }

  inline int64 swap_bytes(int64 x) noexcept
  { return __bswap_64(x); }

  inline uint64 swap_bytes(uint64 x) noexcept
  { return __bswap_64(x); }
}
#endif

namespace imp {
  // Identity functions for completeness
  inline int8 swap_bytes(int8 x) noexcept
  { return x; }

  inline uint8 swap_bytes(uint8 x) noexcept
  { return x; }

#if BYTE_ORDER == LITTLE_ENDIAN
  template <class T>
  inline T little_endian(T x) noexcept
  { return x; }

  template <class T>
  inline T big_endian(T x) noexcept
  { return swap_bytes(x); }
#elif BYTE_ORDER == BIG_ENDIAN
  template <class T>
  inline T little_endian(T x) noexcept
  { return swap_bytes(x); }

  template <class T>
  inline T big_endian(T x) noexcept
  { return x; }
#else
#error "Unknown or unsupported byte order"
#endif

  template <class T>
  inline void set_big_endian(T& x) noexcept
  { x = big_endian(x); }

  template <class T>
  inline void set_little_endian(T& x) noexcept
  { x = little_endian(x); }
}

#endif //__IMP_ENDIAN__41348837
