
#ifndef __DOOM64EX_KEXDEF_HH__
#define __DOOM64EX_KEXDEF_HH__

#ifndef __cplusplus
# warning "A C++ compiler is required to use this header file"
#endif //__cplusplus

#include <string>
#include <vector>
#include <memory>

using std::enable_if;
using std::unique_ptr;
using std::move;
using std::forward;

using byte_t = uint8_t;

template <class...>
using void_t = void;

#endif //__DOOM64EX_KEXDEF_HH__
