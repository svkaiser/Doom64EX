#ifndef __RADIX_TREE__79038274
#define __RADIX_TREE__79038274

/**
 * Library stolen from
 * https://github.com/ytakano/radix_tree
 */
#include "radix_tree/radix_tree.hpp"

namespace imp {
  template <class K, class V, class Compare = std::less<K>>
  using radix_tree = ::radix_tree<K, V, Compare>;
}

#endif //__RADIX_TREE__79038274
