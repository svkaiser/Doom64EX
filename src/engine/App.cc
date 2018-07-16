#include <map>
#include <fstream>
#include <platform/app.hh>
#include <sys/stat.h>
#include <imp/detail/Pixel.hh>

#include <cxxabi.h>

#include "SDL.h"

#ifdef main
#undef main
#else
#define SDL_main main
#endif

String imp::type_to_string(const std::type_info& type)
{
#ifdef __GNUC__
    int status {};
    auto real_name = abi::__cxa_demangle(type.name(), nullptr, nullptr, &status);

    if (status == 0) {
        String name = real_name;
        std::free(real_name);
        return name;
    }

    return type.name();
#else
    return type.name();
#endif
}

[[noreturn]]
void D_DoomMain();

[[noreturn]]
void WGen_WadgenMain();

int myargc{};

char **myargv{};

namespace {
  auto &_gparams()
  {
      static std::map<StringView, app::Param *> params;
      return params;
  }

  auto &_params = _gparams();

  String _base_dir { "./" };
  String _data_dir { "./" };
  StringView _program;

  Vector<String> _rest;

  app::StringParam _wadgen_param("wadgen");

  struct ParamsParser {
      using Arity = app::Param::Arity;
      app::Param *param{};

      void process(StringView arg)
      {
          if (arg[0] == '-') {
              arg.remove_prefix(1);
              auto it = _params.find(arg);
              if (it == _params.end()) {
                  log::warn("Unknown parameter -{}", arg);
              } else {
                  if (param) {
                  }
                  param = it->second;
                  param->set_have();
                  if (param->arity() == Arity::nullary) {
                      param = nullptr;
                  }
              }
          } else {
              if (!param) {
                  _rest.emplace_back(arg);
                  return;
              }

              param->add(arg);
              if (param->arity() != Arity::nary) {
                  param = nullptr;
              }
          }
      }
  };
}

[[noreturn]]
void app::main(int argc, char **argv)
{
    myargc = argc;
    myargv = argv;

    _program = argv[0];

    auto base_dir = SDL_GetBasePath();
    if (base_dir) {
        _base_dir = base_dir;
        SDL_free(base_dir);
    }

    // Data files have to be in the cwd on Windows for compatibility reasons.
#ifndef _WIN32
    auto data_dir = SDL_GetPrefPath("", "doom64ex");
    if (data_dir) {
        _data_dir = data_dir;
        SDL_free(data_dir);
    }
#endif

    /* Process command-line arguments */
    log::debug("Parameters:");
    ParamsParser parser;
    for (int i = 1; i < argc; ++i) {
        auto arg = argv[i];

        if (arg[0] == '@') {
            std::ifstream f(arg + 1);

            if (!f.is_open()) {
                log::fatal("Could not open '{}'", arg + 1);
                continue;
            }

            while (!f.eof()) {
                String arg;
                f >> arg;
                parser.process(arg);
            }
        } else {
            parser.process(arg);
        }
    }

    /* Print rest params */
    if (!_rest.empty()) {
    }

    D_DoomMain();
}

bool app::file_exists(StringView path)
{
#ifdef _WIN32
    return std::ifstream(path.to_string()).is_open();
#else
    struct stat st;

    if (stat(path.data(), &st) == -1) {
        return false;
    }

    return S_ISREG(st.st_mode);
#endif
}

Optional<String> app::find_data_file(StringView name, StringView dir_hint)
{
    String path;

    if (!dir_hint.empty()) {
        path = fmt::format("{}{}", dir_hint, name);
        if (app::file_exists(path))
            return path;
    }

    path = _base_dir + name.to_string();
    if (app::file_exists(path))
        return path;

    path = _data_dir + name.to_string();
    if (app::file_exists(path))
        return path;

#if defined(__LINUX__) || defined(__OpenBSD__)
    const char *paths[] = {
        "/usr/local/share/games/doom64ex/",
        "/usr/local/share/doom64ex/",
        "/usr/local/share/doom/",
        "/usr/share/games/doom64ex/",
        "/usr/share/doom64ex/",
        "/usr/share/doom/",
        "/opt/doom64ex/",

        /* flatpak */
        "/app/share/games/doom64ex/"
    };

    for (auto p : paths) {
        path = fmt::format("{}{}", p, name);
        if (app::file_exists(path))
            return path;
    }
#endif

    return nullopt;
}

StringView app::program()
{
    return _program;
}

bool app::have_param(StringView name)
{
    return _params.count(name) == 1;
}

const app::Param *app::param(StringView name)
{
    auto it = _params.find(name);
    return it != _params.end() ? it->second : nullptr;
}

app::Param::Param(StringView name, app::Param::Arity arity) :
    arity_(arity)
{
    _gparams()[name] = this;
}

int SDL_main(int argc, char **argv)
{
    app::main(argc, argv);
}
