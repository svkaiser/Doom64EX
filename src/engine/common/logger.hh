#ifndef __LOGGER__81425854
#define __LOGGER__81425854

#include <thread>
#include <mutex>
#include <ostream>
#include <prelude.hh>

namespace imp {
  namespace log {
    enum struct LogType {
        info,
        warn,
        error,
        fatal,
        stub,
        fixme
    };

    class Logger;

    class Writer {
        Logger& m_logger;
        std::ostringstream m_buffer;

        friend class Logger;

        Writer(Logger& logger):
            m_logger(logger) {}

        template <class T>
        Writer(Logger& logger, const T& value):
            Writer(logger)
        { m_buffer << value; }

    public:
        ~Writer();

        template <class T>
        Writer& operator<<(const T& obj)
        {
            m_buffer << obj;
            return *this;
        }
    };

    class Logger {
        StringView m_ansi_color;
        std::ostream& m_ostream;
        void (*m_callback)(StringView message);

        friend class Writer;

        void m_println(StringView);

    public:
        Logger(StringView ansi_color, std::ostream& ostream):
            m_ansi_color(ansi_color),
            m_ostream(ostream) {}

        template <class... Args>
        void operator()(StringView fmt, Args&&... args)
        { m_println(fmt::format(fmt.to_string(), std::forward<Args>(args)...)); }

        int printf(const char* fmt, ...);

        template <class T>
        Writer operator<<(const T& value)
        { return { *this, value }; }

        void set_callback(void (*callback)(StringView message))
        { m_callback = callback; }
    };

    extern thread_local Logger info;
    extern thread_local Logger warn;
    extern thread_local Logger error;
    extern thread_local Logger fatal;
    extern thread_local Logger debug;
  }
}

#endif //__LOGGER__81425854
