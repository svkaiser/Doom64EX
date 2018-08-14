#ifndef __LOGGER__81425854
#define __LOGGER__81425854

#include <sstream>
#include <iostream>
#include <utility/string_view.hh>

#include "style.hh"

namespace imp {
  namespace log {

  class Init;

    enum struct LogType {
        info,
        warn,
        error,
        fatal,
        stub,
        fixme
    };

    // Forward declare logger class
    class Logger;

    /**
     * Helper
     */
    class Stream {
        Logger& m_logger;
        std::ostringstream m_buffer;

        friend class Logger;

        template <class T>
        Stream(Logger& logger, const T& value):
            m_logger(logger)
        { m_buffer << value; }

    public:
        /**
         * Delete all default constructors and assignment operators.
         */
        Stream() = delete;
        Stream(const Stream&) = delete;
        Stream(Stream&&) = delete;

        Stream& operator=(const Stream&) = delete;
        Stream& operator=(Stream&&) = delete;

        /**
         * Prints and flushes on destruction
         */
        ~Stream();

        /**
         * C++ std::ostream chainable operator
         */
        template <class T>
        Stream& operator<<(const T& obj)
        {
            m_buffer << obj;
            return *this;
        }
    };

    /**
     * \class Logger
     */
    class Logger {
        StringView m_ansi_color {};
        std::ostream* m_ostream {};
        void (*m_callback)(StringView message) {};

        void m_println(StringView message);

        void m_init(StringView ansi_color, std::ostream& stream);

        friend class Stream;
        friend class Init;

    public:
        Logger() = default;

        // Disable copying and moving
        Logger(const Logger&) = delete;
        Logger(Logger&&) = delete;
        Logger& operator=(const Logger&) = delete;
        Logger& operator=(Logger&&) = delete;

        /**
         * fmtlib-style formatting. Recommended for all new code.
         *
         * See http://fmtlib.net/latest/syntax.html for more info.
         */
        template <class... Args>
        void operator()(StringView fmt, Args&&... args)
        { m_println(fmt::format(fmt.to_string(), std::forward<Args>(args)...)); }

        /**
         * Legacy C-style formatting. Used by \def I_Printf and \def I_Error
         * \param fmt C-style format string
         */
        [[gnu::format(printf, 2, 3)]]
        int printf(const char* fmt, ...);

        /**
         * C++ std::ostream-style chain formatting.
         * @tparam T
         * @param value
         * @return
         */
        template <class T>
        Stream operator<<(const T& value)
        { return { *this, value }; }

        void set_callback(void (*callback)(StringView message))
        { m_callback = callback; }
    };
  }
}

#endif //__LOGGER__81425854
