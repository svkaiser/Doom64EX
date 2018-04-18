#include <dlfcn.h>
#include <imp/NativeUI>

namespace {
  struct {
      void (*init)();
      void (*quit)();
      void (*console_show)(bool);
      void (*console_add_line)(const char*);
  } ftable;

  void *handle_ = nullptr;
}

void native_ui::init()
{
    handle_ = dlopen("./libDoom64EX-gtk3.so", RTLD_LAZY | RTLD_GLOBAL);
    if (!handle_) {
        log::warn("Gtk3: Could not load libDoom64EX-gtk3.so");
        return;
    }

#define DL_FUNC(name)                                                                     \
    ftable.name = reinterpret_cast<decltype(ftable.name)>(dlsym(handle_, #name));         \
    if (!ftable.name) {                                                                   \
        log::fatal("Symbol '" #name "' not found in libDoom64EX-gtk3.so: {}", dlerror()); \
    }

    DL_FUNC(init);
    DL_FUNC(quit);
    DL_FUNC(console_show);
    DL_FUNC(console_add_line);

    ftable.init();
}

void native_ui::console_show(bool show)
{
    if (!handle_)
        return;

    // ftable.console_show(show);
}

void native_ui::console_add_line(StringView line)
{
    if (!handle_)
        return;

    String str { line };
    ftable.console_add_line(str.c_str());
}
