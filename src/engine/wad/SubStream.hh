#ifndef DOOM64EX_SUBSTREAM_HH
#define DOOM64EX_SUBSTREAM_HH

#include <istream>

namespace imp {
  class SubstreamBuf : std::basic_streambuf<char> {
      std::istream& parent_;

  protected:
      SubstreamBuf(std::istream& parent):
          std::basic_streambuf<char>(),
          parent_(parent) {}

      
  };

  class Substream : std::basic_istream<char> {

  };
}

#endif //DOOM64EX_SUBSTREAM_HH
