#ifndef __NULL_LOGGER__81990626
#define __NULL_LOGGER__81990626

#include <utility/string_view.hh>

namespace imp {
  namespace log {
    /**
     * Dummy class that pretends to be a C++ std::ostream-style chain format, but doesn't write any data.
     */
    class NullStream {
    public:
        template<class T>
        NullStream &operator<<(const T &) { return *this; }
    };

    /**
     * Dummy class that pretends to be a Logger, but is a no-op. Used by log::debug in Release mode.
     */
    class NullLogger {
        void m_println(StringView) {}

        void m_init(StringView, std::ostream &) {}

        friend class Init;

    public:
        NullLogger() = default;

        // Disable copying and moving
        NullLogger(const NullLogger &) = delete;

        NullLogger(NullLogger &&) = delete;

        NullLogger &operator=(const NullLogger &) = delete;

        NullLogger &operator=(NullLogger &&) = delete;

        template<class... Args>
        void operator()(StringView, Args &&...) {}

        [[gnu::format(printf, 2, 3)]]
        int printf(const char *, ...)
        { return 0; }

        template<class T>
        NullStream operator<<(const T &) { return {}; }

        void set_callback(void (*)(StringView message)) {}
    };
  }
}

#endif //__NULL_LOGGER__81990626
