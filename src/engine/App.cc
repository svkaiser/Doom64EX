#include <map>
#include <fstream>
#include <imp/App>
#include <imp/Wad>
#include <sys/stat.h>

#include "SDL.h"

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

  String _base_dir { "." };
  String _data_dir { "." };
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
                  println(stderr, "Unknown parameter -{}", arg);
              } else {
                  if (param) {
                      println("");
                  }
                  param = it->second;
                  param->set_have();
                  print("{:16s} = ", arg);
                  if (param->arity() == Arity::nullary) {
                      param = nullptr;
                      println(" true");
                  }
              }
          } else {
              if (!param) {
                  _rest.emplace_back(arg);
                  return;
              }

              param->add(arg);
              print(" {}", arg);
              if (param->arity() != Arity::nary) {
                  param = nullptr;
                  println("");
              }
          }
      }
  };
}

[[noreturn]]
void app::main(int argc, char **argv)
{
    using Arity = app::Param::Arity;
    myargc = argc;
    myargv = argv;

    _program = argv[0];

    auto base_dir = SDL_GetBasePath();
    if (base_dir) {
        _base_dir = base_dir;
        SDL_free(base_dir);
    }

    // Data files have to be in the cwd on Windows for compatibility reasons.
#ifdef _WIN32
    auto data_dir = SDL_GetBasePath();
#else
    auto data_dir = SDL_GetPrefPath("", "doom64ex");
#endif
    if (data_dir) {
        _data_dir = data_dir;
        SDL_free(data_dir);
    }

    /* Process command-line arguments */
    println("Parameters:");
    ParamsParser parser;
    for (int i = 1; i < argc; ++i) {
        auto arg = argv[i];

        if (arg[0] == '@') {
            std::ifstream f(arg + 1);

            if (!f.is_open()) {
                // TODO: Print error
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
    if (parser.param && parser.param->arity() == Arity::nary)
        println("");

    /* Print rest params */
    if (!_rest.empty()) {
        print("{:16s} = ", "rest");
        for (const auto &s : _rest)
            print(" {}", s);
        println("");
    }

    if (_wadgen_param) {
        WGen_WadgenMain();
    } else {
        D_DoomMain();
    }
}

bool app::file_exists(StringView path)
{
    struct stat st;
    return !stat(path.data(), &st) && S_ISREG(st.st_mode);
}

Optional<String> app::find_data_file(StringView name, StringView dir_hint)
{
    String path;

    if (!dir_hint.empty()) {
        path = format("{}{}", dir_hint, name);
        if (app::file_exists(path))
            return { inplace, path };
    }

    path = format("{}{}", _base_dir, name);
    if (app::file_exists(path))
        return { inplace, path };

    path = format("{}{}", _data_dir, name);
    if (app::file_exists(path))
        return { inplace, path };

#if defined(__LINUX__) || defined(__OpenBSD__)
    int i;
    const char *paths[] = {
        "/usr/local/share/games/doom64ex/",
        "/usr/local/share/doom64ex/",
        "/usr/local/share/doom/",
        "/usr/share/games/doom64ex/",
        "/usr/share/doom64ex/",
        "/usr/share/doom/",
        "/opt/doom64ex/",
    };

    for (auto p : paths) {
        path = format("{}{}", p, name);
        if (app::file_exists(path))
            return { inplace, path };
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
    return _params.count(name);
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

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    int argc = __argc;
    char** argv = __argv;
    app::main(argc, argv);
}
#else

int main(int argc, char **argv)
{
    app::main(argc, argv);
}

#endif
