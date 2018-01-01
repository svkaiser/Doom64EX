// -*- mode: c++ -*-
#ifndef __IMP_GUARD__26964644
#define __IMP_GUARD__26964644

namespace imp {
  /**
   * \brief A generic RAII guard
   *
   * \tparam Deleter A Callable type that will be called in {\ref Guard::~Guard}
   *
   * A generic RAII guard that calls the deleter on the Guard going out of scope
   */
  template <class Deleter>
  class Guard {
      bool executed_ {};
      Deleter deleter_;

  public:
      Guard(Deleter&& deleter):
          deleter_(std::move_if_noexcept(deleter)) {}

      ~Guard()
      {
          if (!executed_)
              deleter_();
      }

       /*!
       * Execute the deleter deliberately prior to the Guard object going out of scope
       */
      void execute()
      {
          if (!executed_) {
              deleter_();
              executed_ = true;
          }
      }

      /*!
       * Prevent deleter being executed on destruction
       */
      void release()
      { executed_ = true; }
  };

  template <class Deleter>
  inline Guard<Deleter> make_guard(Deleter&& deleter)
  { return { std::forward<Deleter>(deleter) }; }
}

#endif //__IMP_GUARD__26964644
