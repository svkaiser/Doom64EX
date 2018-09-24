using Gtk;

namespace NativeUI {
	class Console : Window {
		private ScrolledWindow window;
		private TextBuffer text_buffer;

		public Console () {
			this.title = "Doom64EX Console";
			this.border_width = 10;
			this.window_position = WindowPosition.CENTER;
			this.set_default_size (384, 480);

			var layout = new Box (Orientation.VERTICAL, 4);
			this.add (layout);

			var text_view = new TextView ();
			this.text_buffer = text_view.get_buffer ();
			text_view.cursor_visible = false;
			text_view.editable = false;
			text_view.monospace = true;
			text_view.wrap_mode = WrapMode.NONE;

			window = new ScrolledWindow (text_view.hadjustment, text_view.vadjustment);
			window.add (text_view);
			window.size_allocate.connect(() => {
					var adj = window.vadjustment;
					adj.set_value (adj.upper);
				});
			layout.pack_start (window, true, true, 4);

			var buttons = new ButtonBox (Orientation.HORIZONTAL);
			var btn_copy = new Button.with_label ("Copy");
			var btn_quit = new Button.with_label ("Quit");

			btn_quit.clicked.connect (() => Gtk.main_quit ());

			buttons.add (btn_copy);
			buttons.add (btn_quit);

			layout.pack_start (buttons, false, true, 4);
		}

		public void insert (string text) {
			text_buffer.insert_at_cursor (text, text.length);
			text_buffer.insert_at_cursor ("\n", 1);
		}
	}
}