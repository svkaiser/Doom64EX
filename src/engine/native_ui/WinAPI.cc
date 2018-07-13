#include <imp/NativeUI>

extern "C" {

void winapi_init();
void winapi_show(bool show);
void winapi_add_text(const char *text);

}

void native_ui::init()
{
    winapi_init();
}

void native_ui::console_show(bool show)
{
    winapi_show(show);
}

void native_ui::console_add_line(StringView line)
{
    String text = line.to_string();
    winapi_add_text(text.c_str());
}
