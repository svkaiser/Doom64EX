#include <dlfcn.h>
#include <imp/NativeUI>

namespace {
  using Init = void ();
  using Quit = void ();
  using ConsoleShow = void (bool);
  using ConsoleAddLine = void (const char *);

  Init* init_ {};
  Quit* quit_ {};
  ConsoleShow* console_show_ {};
  ConsoleAddLine* console_add_line_ {};

  void *handle_ = nullptr;
}

void native_ui::init()
{
    handle_ = dlopen("./libDoom64EX-gtk3.so", RTLD_LAZY | RTLD_GLOBAL);
    if (!handle_) {
        log::warn("Gtk3: Could not load libDoom64EX-gtk3.so");
        return;
    }

    init_ = reinterpret_cast<Init*>(dlsym(handle_, "init"));

    if (!init_) {
        log::fatal("Error loading Gtk3 UI: {}", dlerror());
        exit(0);
    }
    quit_ = reinterpret_cast<Quit*>(dlsym(handle_, "quit"));
    console_add_line_ = reinterpret_cast<ConsoleAddLine*>(dlsym(handle_, "console_add_line"));

    console_show_ = reinterpret_cast<ConsoleShow*>(dlsym(handle_, "console_show"));

    assert(quit_);
    assert(console_add_line_);

    init_();
}

void native_ui::console_show(bool show)
{
    if (!handle_)
        return;

    console_show_(show);
}

void native_ui::console_add_line(StringView line)
{
    if (!handle_)
        return;

    String str { line };
    console_add_line_(str.c_str());
}
