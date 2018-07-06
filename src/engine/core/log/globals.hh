#ifndef __GLOBALS__49513101
#define __GLOBALS__49513101

#include <atomic>

namespace imp {
  namespace log {
    class Logger;
    class NullLogger;

/*!
 * Init class that paralells std::ios::Init.
 */
    class Init {
        static std::atomic<std::size_t> m_refcnt;

    public:
        Init();

        ~Init();
    };

    static Init __ioinit{};

    extern Logger info;
    extern Logger warn;
    extern Logger error;
    extern Logger fatal;

#ifdef IMP_DEBUG
    extern Logger debug;
# define DEBUG(_Fmt, ...) ::imp::log::debug(_Fmt, ##__VA_ARGS__)
# define STUB(_Msg) ::imp::log::debug("STUB {}: {} ({}:{})", __FUNCTION__, _Msg, __LINE__, __FILE__)
# define FIXME(_Msg) ::imp::log::debug("FIXME {}: {} ({}:{})", __FUNCTION__, _Msg, __LINE__, __FILE__)
#else
    extern NullLogger debug;
# define DEBUG(_Fmt, ...)
# define STUB(_Msg)
# define FIXME(_Msg)
#endif

  }
}

#define I_Printf(_Fmt, ...) ::imp::log::info.printf(_Fmt, ##__VA_ARGS__)
#define I_Error(_Fmt, ...) ::imp::log::fatal.printf(_Fmt, ##__VA_ARGS__)

#endif //__GLOBALS__49513101
