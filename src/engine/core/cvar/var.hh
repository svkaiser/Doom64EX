#ifndef __VAR__98822993
#define __VAR__98822993

#include <prelude.hh>
#include "data.hh"

#define IMP_DEFINE_ARITHMETIC_BOTH(_Op)                                 \
    template <class T, class U>                                         \
    inline auto operator _Op(const VarBase<T>& lhs, const VarBase<U>& rhs) \
    { return lhs.get() _Op rhs.get(); }

#define IMP_DEFINE_ARITHMETIC_LEFT(_Op)                           \
    template <class T, class U>                                   \
    inline auto operator _Op(const VarBase<T>& lhs, const U& rhs) \
    { return lhs.get() _Op rhs; }

#define IMP_DEFINE_ARITHMETIC_RIGHT(_Op)                          \
    template <class T, class U>                                   \
    inline auto operator _Op(const T& lhs, const VarBase<U>& rhs) \
    { return lhs _Op rhs.get(); }

#define IMP_DECLARE_ASSIGN_ARITHMETIC(_Op)              \
    VarBase& operator _Op ## =(const VarBase<T>& other) \
    { return *this = (m_value _Op other); }             \
        VarBase& operator _Op ## =(const T& other)      \
        { return *this = (m_value _Op other); }

#define IMP_DEFINE_ARITHMETIC(_Op)       \
    IMP_DEFINE_ARITHMETIC_BOTH(_Op)      \
        IMP_DEFINE_ARITHMETIC_LEFT(_Op)  \
        IMP_DEFINE_ARITHMETIC_RIGHT(_Op)

 namespace imp {
   namespace cvar {
     template <class T>
     class RefBase;
     class Ref;
     class Store;

     template <class T>
     class VarBase {
         std::shared_ptr<Data> m_data;
         T& m_value;

         friend class RefBase<T>;
         friend class Ref;
         friend class Store;

     public:
         using value_type = T;

         VarBase():
             m_data(std::make_shared<Data>(T {})),
             m_value(m_data->get<T>()) {}

         VarBase(const T& default_):
             m_data(std::make_shared<Data>(default_)),
             m_value(m_data->get<T>()) {}

         VarBase(T&& default_):
             m_data(std::make_shared<Data>(std::move(default_))),
             m_value(m_data->get<T>()) {}

         VarBase& operator=(const T& new_value)
         {
             m_value = new_value;
             m_data->update();
             return *this;
         }

         VarBase& operator=(T&& new_value)
         {
             m_value = std::move(new_value);
             m_data->update();
             return *this;
         }

         ~VarBase()
         { m_data->set_valid(false); }

         IMP_DECLARE_ASSIGN_ARITHMETIC(+);
         IMP_DECLARE_ASSIGN_ARITHMETIC(-);
         IMP_DECLARE_ASSIGN_ARITHMETIC(*);
         IMP_DECLARE_ASSIGN_ARITHMETIC(/);
         IMP_DECLARE_ASSIGN_ARITHMETIC(%);

         void set_to_default(bool inhibit_callback = false)
         {
             m_data->set_to_default();
             if (!inhibit_callback) {
                 m_data->update();
             }
         }

         void set_callback(const std::function<void (const T&)>& f)
         { m_data->set_callback(f); }

         /**! Don't save this variable to config */
         bool is_noconfig() const
         { return m_data->test_flag(Flag::noconfig); }

         /**! Don't show this variable to the user */
         bool is_hidden() const
         { return m_data->test_flag(Flag::hidden); }

         /**! Server var - Send it to clients */
         bool is_server() const
         { return m_data->test_flag(Flag::server); }

         /**! Client var - Send it to server */
         bool is_client() const
         { return m_data->test_flag(Flag::client); }

         /**! Remember the user config in case this var is dropped and readded
          * later */
         bool is_dynamic() const
         { return m_data->test_flag(Flag::dynamic); }

         /**! Tell the user that this cvar will only take effect after restart */
         bool is_restart() const
         { return m_data->test_flag(Flag::restart); }

         operator const T&() const
         { return m_value; }

         const T& get() const
         { return m_value; }

         const T& get_default() const
         { return m_data->get_default<T>(); }

         const T& operator*() const
         { return m_value; }

         const T* operator->() const
         { return &m_value; }
     };

     IMP_DEFINE_ARITHMETIC(+);
     IMP_DEFINE_ARITHMETIC(-);
     IMP_DEFINE_ARITHMETIC(*);
     IMP_DEFINE_ARITHMETIC(/);
     IMP_DEFINE_ARITHMETIC(%);

     using BoolVar   = VarBase<bool>;
     using IntVar    = VarBase<int>;
     using FloatVar  = VarBase<float>;
     using StringVar = VarBase<String>;
   } // cvar namespace
 } // imp namespace

#undef IMP_DEFINE_ARITHMETIC
#undef IMP_DECLARE_ASSIGN_ARITHMETIC
#undef IMP_DEFINE_ARITHMETIC_RIGHT
#undef IMP_DEFINE_ARITHMETIC_LEFT
#undef IMP_DEFINE_ARITHMETIC_BOTH

#endif //__VAR__98822993
