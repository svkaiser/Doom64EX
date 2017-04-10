using NativeUI;

private Console console;
private Thread<void*> gui_thread;

public void init () {
	unowned string[]? argv = null;
	Gtk.init (ref argv);

	gui_thread = new Thread<void*> ("Gtk3 Thread", () => {
			console = new NativeUI.Console ();
			console.show_all ();

			Gtk.main ();

			return null;
		});
}

public void quit () {
	Idle.add (() => {
			Gtk.main_quit ();
			return Source.REMOVE;
		});

	gui_thread.join ();
	console = null;
}

public void console_add_line (string line) {
	Idle.add (() => {
			console.insert (line);
			return Source.REMOVE;
		});
}