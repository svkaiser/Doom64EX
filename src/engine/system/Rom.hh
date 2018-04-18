#ifndef __IMP_ROM__65058706
#define __IMP_ROM__65058706

#include <sstream>

namespace imp {
  namespace rom {
    void init();

    std::istringstream wad();
    std::istringstream sn64();
    std::istringstream sseq();
    std::istringstream pcm();
  }
}

#endif //__IMP_ROM__65058706
