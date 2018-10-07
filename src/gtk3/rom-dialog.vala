extern void nui_gtk_rom_select (string path);
extern void nui_gtk_rom_open (string path);
extern bool nui_gtk_rom_valid ();
extern string nui_gtk_rom_version ();

[GtkTemplate(ui = "/com/github/Doom64EX/rom-dialog.ui")]
class RomDialog : Gtk.Dialog {
	[GtkChild]
	private Gtk.Label rom_version;

	[GtkChild]
	private Gtk.Label rom_size;

	[GtkChild]
	private Gtk.Grid info_grid;

	[GtkChild]
	private Gtk.Label info_label;

	[GtkChild]
	private Gtk.Button btn_apply;

	private string rom_path;
	private GLib.File rom_file;

	public RomDialog () {
		this.hide_info ();
	}

	[GtkCallback]
	void on_file_set (Gtk.FileChooserButton button) {
		var uri = button.get_uri ();
		var scheme = GLib.Uri.parse_scheme (uri);
		if (scheme != "file") {
			stdout.printf("Error: %s:// scheme not supported\n", scheme);
			return;
		}

		rom_file = File.new_for_uri (uri);
		rom_path = GLib.Filename.from_uri (uri);

		nui_gtk_rom_open (rom_path);

		show_info ();
	}

	[GtkCallback]
	void on_accept (Gtk.Button button) {
		nui_gtk_rom_select (rom_path);
		destroy ();
	}

	[GtkCallback]
	void on_cancel (Gtk.Button button) {
		destroy ();
	}

	private void hide_info () {
		btn_apply.set_sensitive (false);
		info_grid.hide ();
		info_label.hide ();
	}

	private void show_info () {
		var size = rom_file.query_info ("standard::size", 0).get_size ();

		info_grid.show ();
		rom_version.set_label (nui_gtk_rom_version ());
		rom_size.set_label (pretty_size (size));

		btn_apply.set_sensitive (nui_gtk_rom_valid ());
	}

	private string pretty_size (int64 size) {
		const int DIV = 1000;
		if (size <= DIV) {
			return size.to_string ("%ld bytes");
		}
		if ((size /= DIV) <= DIV) {
			return size.to_string ("%ld kB");
		}
		if ((size /= DIV) <= DIV) {
			return size.to_string ("%ld MB");
		}

		return (size / DIV).to_string ("%ld GB");
	}
}