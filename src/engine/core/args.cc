#include <fstream>

#include "args.hh"

namespace {
  Vector<String> s_args;
}

//
// args::parse
//
void args::parse(int argc, char **argv)
{
    /* Read program arguments */
    log::debug("Parameters:");
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
                s_args.push_back(arg);
                log::debug("\t{}\n", arg);
            }
        } else {
            s_args.push_back(arg);
            log::debug("\t{}\n", arg);
        }
    }
}

//
// args::all_args
//
ArrayView<String> args::all_args()
{
    return s_args;
}