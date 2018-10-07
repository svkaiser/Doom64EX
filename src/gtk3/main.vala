private ConsoleWindow console_window;
private Thread<void*> gui_thread;

public void nui_gtk_init () {
	unowned string[]? argv = null;
	Gtk.init (ref argv);

	gui_thread = new Thread<void*> ("Gtk3 Thread", () => {
			console_window = new ConsoleWindow ();
			console_window.show_all ();

			Gtk.main ();

			return null;
		});
}

public void nui_gtk_quit () {
	Idle.add (() => {
			Gtk.main_quit ();
			return Source.REMOVE;
		});

	gui_thread.join ();
	console_window = null;
}

public void nui_gtk_console_show (bool show) {
	Idle.add (() => {
			if (show) {
				console_window.show ();
			} else {
				console_window.hide ();
			}
			return Source.REMOVE;
		});
}

public void nui_gtk_console_add_line (string line) {
	Idle.add (() => {
			console_window.insert (line);
			return Source.REMOVE;
		});
}

public void nui_gtk_rom_dialog_init () {
	Idle.add (() => {
			var window = new RomDialog ();
			window.show ();

			return Source.REMOVE;
		});
}