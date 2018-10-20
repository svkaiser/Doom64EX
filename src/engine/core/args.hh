#ifndef __ARGS__36067090
#define __ARGS__36067090

#include "prelude.hh"

namespace imp {
  namespace args {
    /**
     * Parse command-line arguments
     * \param argc Number of arguments (including program name)
     * \param argv Vector of argument strings
     */
    void parse(int argc, char **argv);

    /**
     * \return All program arguments
     */
    ArrayView<String> all_args();
  }
}

#endif //__ARGS__36067090
