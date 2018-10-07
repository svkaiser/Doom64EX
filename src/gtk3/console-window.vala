extern void nui_gtk_console_eval (string cmd);

[GtkTemplate(ui="/com/github/Doom64EX/console-window.ui")]
class ConsoleWindow : Gtk.Window {
	[GtkChild]
	private Gtk.TextBuffer text_buffer;

	public ConsoleWindow () {
	}

	public void insert (string text) {
		text_buffer.insert_at_cursor (text, text.length);
		text_buffer.insert_at_cursor ("\n", 1);
	}

	[GtkCallback]
	private void on_eval (Gtk.Entry entry) {
		nui_gtk_console_eval (entry.get_text ());
		entry.set_text ("");
	}
}