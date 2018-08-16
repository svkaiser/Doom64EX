#include <iostream>
#include <atomic>
#include <cstdarg>

#include <imp/NativeUI>
#include <prelude.hh>

using namespace ::imp::log;

extern bool console_initialized;
void CON_AddLine(const char* line, int len);

namespace {
  std::chrono::steady_clock::time_point s_program_start;
  bool s_isatty {};

  String s_strip_ansi(StringView fmt)
  {
      String stripped;
      stripped.reserve(fmt.size());

      bool esc {};

      for (auto c : fmt) {
          if (c == '\x1b') {
              esc = true;
          } else if (esc && c == 'm') {
              esc = false;
          } else if (!esc) {
              stripped.push_back(c);
          }
      }

      return stripped;
  }

  String s_timestamp()
  {
      using namespace std::chrono;
      auto now = steady_clock::now();
      auto sec = duration_cast<seconds>(now - s_program_start).count();
      return fmt::format("[{:>6}] ", sec);
  }

  void s_fatal(StringView message)
  {
      native_ui::error(message.to_string());
      std::exit(0);
  }

  auto s_ansi_info = "\x1b[37m"_sv; // ANSI White
  auto s_ansi_warn = "\x1b[33m"_sv; // ANSI Yellow
  auto s_ansi_error = "\x1b[1;31m"_sv; // ANSI Bold & Red
  auto s_ansi_fatal = "\x1b[1;30;41m"_sv; // ANSI Bold & Black on Red
  auto s_ansi_debug = "\x1b[34m"_sv; // ANSI Blue
  auto s_ansi_reset = "\x1b[0m"_sv;

  template <class Func>
  void s_each_line(StringView message, Func yield)
  {
      for (;;) {
          auto pos = message.find('\n');
          if (pos == message.npos) {
              yield(message);
              break;
          }

          yield(message.substr(0, pos));
          message.remove_prefix(pos + 1);
      };
  }

  void s_write_ansi(std::ostream& s, StringView message, StringView style)
  {
      auto prefix = style.to_string() + s_timestamp() + s_ansi_reset.to_string();

      s_each_line(message, [&](StringView msg) {
          s.write(prefix.data(), prefix.size());
          s.write(msg.data(), msg.size());
          s.put('\n');
      });

      s.flush();
  }

  void s_write(std::ostream& s, StringView message, StringView style)
  {
      auto prefix = style.to_string() + s_timestamp();

      s_each_line(message, [&](StringView msg) {
          s.write(prefix.data(), prefix.size());
          s.write(msg.data(), msg.size());
          s.put('\n');
      });

      s.flush();
  }

#ifdef _WIN32
  //
  // static vasprintf
  //

  int vasprintf(char **buf, const char *fmt, va_list ap)
  {
      // WinAPI doesn't contain an implementation of vasprintf, so let's do it
      // ourselves.
      // https://stackoverflow.com/questions/40159892/using-asprintf-on-windows#40160038

      auto len = _vscprintf(fmt, ap);
      if (len == -1) {
          return -1;
      }

      *buf = reinterpret_cast<char*>(malloc(len + 1));
      if (!*buf) {
          // Out-of-memory. Should probably just die or something.
          return -1;
      }

      return vsprintf_s(*buf, len + 1, fmt, ap);
  }
#endif
}

//
// Globals
//

std::atomic<size_t> Init::m_refcnt {};

Logger log::info;
Logger log::warn;
Logger log::error;
Logger log::fatal;

#ifdef IMP_DEBUG
Logger log::debug;
#else
NullLogger log::debug;
#endif

//
// Init::Init
//

Init::Init()
{
    if (!m_refcnt++) {
        log::info.m_init(s_ansi_info, std::cout);
        log::warn.m_init(s_ansi_warn, std::cerr);
        log::error.m_init(s_ansi_error, std::cerr);
        log::fatal.m_init(s_ansi_fatal, std::cerr);
        log::debug.m_init(s_ansi_debug, std::cerr);

        log::fatal.set_callback(s_fatal);

        s_program_start = std::chrono::steady_clock::now();

#ifndef _WIN32
        s_isatty = true;//isatty(STDOUT_FILENO) && isatty(STDERR_FILENO);
#endif
    }
}

//
// Init::~Init
//

Init::~Init() = default;

//
// Logger::m_init
//

void Logger::m_init(imp::StringView ansi_color, std::ostream& ostream)
{
    m_ansi_color = ansi_color;
    m_ostream = &ostream;
}

//
// Logger::m_println
//

void Logger::m_println(StringView message_ansi)
{
    auto message = s_strip_ansi(message_ansi);

    /* Write to terminal */
    if (s_isatty) {
        s_write_ansi(*m_ostream, message_ansi, m_ansi_color);
    } else {
        s_write(*m_ostream, message, " "_sv);
    }

    /* Write to console */
    if (console_initialized) {
        CON_AddLine(message.c_str(), message.size());
    }

    /* Write to native console */
    native_ui::console_add_line(message);

    /* Hand off to callback */
    if (m_callback) {
        m_callback(message);
    }
}

//
// Logger::printf
//

int Logger::printf(const char* fmt, ...)
{
    char *text;
    va_list ap;
    va_start(ap, fmt);

    auto len = vasprintf(&text, fmt, ap);

    if (text[len - 1] == '\n')
        text[len - 1] = 0;

    m_println(text);
    free(text);

    va_end(ap);

    return len;
}

//
// Stream::~Stream
//

Stream::~Stream()
{
    m_logger.m_println(m_buffer.str());
}
