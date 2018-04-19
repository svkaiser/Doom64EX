#include <iostream>
#include <cstdarg>
#include <imp/NativeUI>
#include "logger.hh"

using namespace ::imp::log;

namespace {
  auto s_program_start = std::chrono::steady_clock::now();

  String s_timestamp()
  {
      using namespace std::chrono;
      auto now = steady_clock::now();
      auto sec = duration_cast<seconds>(now - s_program_start).count();
      return fmt::format("[{:>6}] ", sec);
  }

  auto s_ansi_info = "\x1b[37m"_sv; // ANSI White
  auto s_ansi_warn = "\x1b[33m"_sv; // ANSI Yellow
  auto s_ansi_error = "\x1b[1;31m"_sv; // ANSI Bold & Red
  auto s_ansi_fatal = "\x1b[1;30;41m"_sv; // ANSI Bold & Black on Red
  auto s_ansi_debug = "\x1b[34m"_sv; // ANSI Blue
  auto s_ansi_reset = "\x1b[0m"_sv;
}

thread_local Logger log::info { s_ansi_info, std::cout };
thread_local Logger log::warn { s_ansi_warn, std::cerr };
thread_local Logger log::error { s_ansi_error, std::cerr };
thread_local Logger log::fatal { s_ansi_fatal, std::cerr };
thread_local Logger log::debug { s_ansi_debug, std::cerr };

void Logger::m_println(StringView message)
{
    auto prefix = m_ansi_color.to_string() + s_timestamp() + s_ansi_reset.to_string();

    int left {};
    int right { -1 };
    for (;;) {
        left = right + 1;
        right = message.find('\n', left);
        if (static_cast<size_t>(right) == message.npos) {
            right = message.size();
        }
        if (left >= right) {
            break;
        }

        auto line = message.substr(left, right - left);

        m_ostream.write(prefix.data(), prefix.size());
        m_ostream.write(line.data(), line.size());
        m_ostream.put('\n');

        native_ui::console_add_line(line);
    };

    m_ostream.flush();

    if (m_callback) {
        m_callback(message);
    }
}

int Logger::printf(const char* fmt, ...)
{
    char *text;
    va_list ap;
    va_start(ap, fmt);

    auto len = vasprintf(&text, fmt, ap);
    m_println(text);
    free(text);

    va_end(ap);

    return len;
}

Writer::~Writer()
{ m_logger.m_println(m_buffer.str()); }
