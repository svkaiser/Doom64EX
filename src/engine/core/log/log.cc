#include <iostream>
#include <atomic>
#include <cstdarg>

#include <imp/NativeUI>
#include <prelude.hh>

using namespace ::imp::log;

namespace {
  std::chrono::steady_clock::time_point s_program_start;

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
}

//
// Globals
//

std::atomic<size_t> Init::m_refcnt;

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

void Logger::m_println(StringView message)
{
    auto prefix = m_ansi_color.to_string() + s_timestamp() + s_ansi_reset.to_string();

    size_t left {};
    size_t right = String::npos;
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

        m_ostream->write(prefix.data(), prefix.size());
        m_ostream->write(line.data(), line.size());
        m_ostream->put('\n');

        native_ui::console_add_line(line);
    };

    m_ostream->flush();

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
    m_println(text);
    free(text);

    va_end(ap);

    return len;
}

//
// Stream::~Stream
//

Stream::~Stream()
{ m_logger.m_println(m_buffer.str()); }
