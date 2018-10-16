#ifndef __REF__42249360
#define __REF__42249360

#include <stdexcept>
#include "data.hh"

#define IMP_DEFINE_ARITHMETIC_BOTH(_Type, _Op)                          \
    inline _Type operator _Op(const RefBase<_Type>& lhs, const RefBase<_Type>& rhs) \
    { return lhs.get() _Op rhs.get(); }

#define IMP_DEFINE_ARITHMETIC_LEFT(_Type, _Op)                          \
    inline _Type operator _Op(const RefBase<_Type>& lhs, const _Type& rhs) \
    { return lhs.get() _Op rhs; }

#define IMP_DEFINE_ARITHMETIC_RIGHT(_Type, _Op)                         \
    inline _Type operator _Op(const _Type& lhs, const VarBase<_Type>& rhs) \
    { return lhs _Op rhs.get(); }

#define IMP_DECLARE_ASSIGN_ARITHMETIC(_Op)                  \
    RefBase& operator _Op ## =(const VarBase<T>& other)     \
    { return *this = (get() _Op other); }                   \
        RefBase& operator _Op ## =(const RefBase<T>& other) \
        { return *this = (get() _Op other); }               \
            RefBase& operator _Op ## =(const T& other)      \
            { return *this = (get() _Op other); }

#define IMP_DEFINE_ARITHMETIC(_Type, _Op)       \
    IMP_DEFINE_ARITHMETIC_BOTH(_Type, _Op)      \
        IMP_DEFINE_ARITHMETIC_LEFT(_Type, _Op)  \
        IMP_DEFINE_ARITHMETIC_RIGHT(_Type, _Op)

 namespace imp {
   namespace cvar {
     struct ref_error : std::logic_error {
         using std::logic_error::logic_error;
     };

     template <class T>
     class VarBase;

     template <class T>
     class RefBase;

     namespace internal {
       template <class T>
       Optional<RefBase<T>> find(StringView);
     }

     template <class T>
     class RefBase {
         std::shared_ptr<Data> m_data;
         T *m_value;

         void m_not_null() const
         {
             if (!m_data) {
                 throw ref_error("null ref");
             }
         }

#if IMP_DEBUG
         void m_dbg_not_null() const
         { m_not_null(); }
#else
         void m_dbg_not_null() const {}
#endif

     public:
         RefBase() = default;

         RefBase(const RefBase& other) = default;

         RefBase(RefBase&& other) = default;

         RefBase(std::shared_ptr<Data> data):
             m_data(data),
             m_value(&data->get<T>()) {}

         RefBase(VarBase<T>& other):
             m_data(other.m_data),
             m_value(&other.m_value) {}

         RefBase(StringView name)
         {
             auto opt = internal::find<T>(name);
             if (!opt) {
                 throw ref_error("nullopt");
             }
             reset(std::move(*opt));
         }

         void reset(VarBase<T>& other)
         {
             m_data = other.m_data;
             m_value = &other.m_value;
         }

         void reset(RefBase<T>&& other)
         {
             m_data = std::move(other.m_data);
             m_value = other.m_value;
         }

         void set_callback(const std::function<void (const T&)>& f)
         { m_data->set_callback<T>(f); }

         void set_to_default(bool inhibit_callback = false)
         {
             m_data->set_to_default();
             if (!inhibit_callback) {
                 m_data->update();
             }
         }

         const T& get() const
         {
             m_dbg_not_null();
             return *m_value;
         }

         const T& get_default() const
         {
             m_dbg_not_null();
             return m_data->get_default<T>();
         }

         StringView name() const
         {
             m_dbg_not_null();
             return m_data->name();
         }

         StringView desc() const
         {
             m_dbg_not_null();
             return m_data->desc();
         }

         RefBase& operator=(const T& other)
         {
             m_dbg_not_null();
             *m_value = other;
             m_data->update();
             return *this;
         }

         IMP_DECLARE_ASSIGN_ARITHMETIC(+);
         IMP_DECLARE_ASSIGN_ARITHMETIC(-);
         IMP_DECLARE_ASSIGN_ARITHMETIC(*);
         IMP_DECLARE_ASSIGN_ARITHMETIC(/);
         IMP_DECLARE_ASSIGN_ARITHMETIC(%);

         bool is_valid() const
         { return m_data && m_data->is_valid(); }

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
     };

     class RefPtr;

     class Ref {
         std::shared_ptr<Data> m_data;

         friend class RefPtr;

     public:
         Ref() = default;
         Ref(const Ref&) = default;
         Ref(Ref&&) = default;

         Ref(std::shared_ptr<Data> data):
             m_data(data) {}

         template <class T>
         Ref(VarBase<T>& var):
             m_data(var.m_data) {}

         Ref& operator=(StringView new_value)
         {
             m_data->set_value(new_value);
             m_data->update();
             return *this;
         }

         String get() const
         { return m_data->get_value(); }

         String get_default() const
         { return m_data->get_default_string(); }

         void reset(std::shared_ptr<Data> data)
         { m_data = data; }

         void reset(const Ref& ref)
         { m_data = ref.m_data; }

         void reset(std::nullptr_t)
         { m_data = nullptr; }

         void set_to_default(bool inhibit_update = false)
         {
             m_data->set_to_default();
             if (!inhibit_update) {
                 m_data->update();
             }
         }

         StringView name() const
         { return m_data->name(); }

         bool is_valid() const
         { return m_data && m_data->is_valid(); }

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
     };

     using BoolRef = RefBase<bool>;
     using IntRef = RefBase<int>;
     using StringRef = RefBase<String>;
     using FloatRef = RefBase<float>;
   } // cvar namespace
 } // imp namespace

#undef IMP_DECLARE_ASSIGN_ARITHMETIC
#undef IMP_DEFINE_ARITHMETIC
#undef IMP_DEFINE_ARITHMETIC_RIGHT
#undef IMP_DEFINE_ARITHMETIC_LEFT
#undef IMP_DEFINE_ARITHMETIC_BOTH

#endif //__REF__42249360
