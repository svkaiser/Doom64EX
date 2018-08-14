#ifndef __STORE_ITERATOR__99202760
#define __STORE_ITERATOR__99202760

#include <unordered_map>
#include <boost/compressed_pair.hpp>

#include "ref.hh"
#include "predicates.hh"

namespace imp::cvar {
  class RefPtr {
      Ref m_ref;

  public:
      RefPtr() = delete;

      RefPtr(const RefPtr&) = default;

      RefPtr(RefPtr&&) = default;

      RefPtr(const Ref& ref):
          m_ref(ref) {}

      RefPtr(Ref&& ref):
          m_ref(std::move(ref)) {}

      RefPtr& operator=(const RefPtr& other)
      {
          m_ref.reset(other.m_ref.m_data);
          return *this;
      }

      RefPtr& operator=(RefPtr&& other)
      {
          m_ref.reset(std::move(other.m_ref.m_data));
          return *this;
      }

      Ref& operator*()
      { return m_ref; }

      const Ref& operator*() const
      { return m_ref; }

      Ref* operator->()
      { return &m_ref; }

      const Ref* operator->() const
      { return &m_ref; }
  };

  template <class Predicate = DummyPred>
  class StoreIterator {
      using iterator = typename std::unordered_map<String, std::weak_ptr<Data>>::iterator;
      using predicate = Predicate;

      struct iter_pair {
          iterator iter;
          const iterator end;

          iter_pair(iterator iter, const iterator end):
              iter(iter), end(end) {}

          void next()
          { ++iter; }

          bool good() const
          { return iter != end; }
      };
      boost::compressed_pair<iter_pair, predicate> m_iter;

      Ref m_ref() const
      { return m_iter.first().iter->second.lock(); }

      void m_next()
      {
          m_iter.first().next();
          while (m_iter.first().good()) {
              if (m_iter.second()(m_iter.first().iter->first, m_iter.first().iter->second))
                  break;
              m_iter.first().next();
          }
      }

  public:
      using value_type        = Ref;
      using reference         = Ref;
      using const_reference   = const Ref;
      using pointer           = RefPtr;
      using const_pointer     = const RefPtr;
      using difference_type   = typename iterator::difference_type;
      using iterator_category = typename iterator::iterator_category;

      StoreIterator(iterator iter, const iterator end, predicate pred = {}):
          m_iter({ iter, end }, pred) {}

      StoreIterator(const StoreIterator&) = default;

      StoreIterator& operator=(const StoreIterator&) = default;

      reference operator*()
      { return m_ref(); }

      const_reference operator*() const
      { return m_ref(); }

      pointer operator->()
      { return m_ref(); }

      const_pointer operator->() const
      { return m_ref(); }

      StoreIterator& operator++()
      {
          m_next();
          return *this;
      }

      StoreIterator operator++(int)
      {
          auto iter = *this;
          m_next();
          return iter;
      }

      bool operator==(const StoreIterator& other) const
      { return m_iter.first().iter == other.m_iter.first().iter; }

      bool operator!=(const StoreIterator& other) const
      { return m_iter.first().iter != other.m_iter.first().iter; }
  };
}

#endif //__STORE_ITERATOR__99202760
