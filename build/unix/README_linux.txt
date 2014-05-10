It's now possible to build and run Doom 64 EX on 32-bit x86 and 64-bit
x86_64 Linux systems.

You need recent versions of gcc, GNU make, CMake, SDL (and SDL_net if
it's a separate package on your system), libpng, and Mesa. If your distro
keeps headers in separate packages, don't forget to install them (these
are usually called -dev or -devel packages). You also need FluidSynth
version 1.1.3 or 1.1.5 (*not* 1.1.1!) and its headers. Later versions
of FluidSynth may or may not be OK (at the time of this writing, 1.1.5
is the latest).

You also need hardware-accelerated OpenGL support in your X server. Don't
bother trying to use Mesa's software rendering, the game will run about
1 frame per second, if it doesn't just crash.

To compile the game the easy way:

# sh linux_install.sh [DESTDIR]

This will build the game and the WadGen utility, and install them to
[DESTDIR]/usr/bin. If no DESTDIR is given, / (the root of the filesystem)
is assumed (in which case you'll need root access, or run the command
with sudo).

If you're doing a distro package, linux_install.sh supports Mafefile-like
variables in the environment (PREFIX, BINDIR, MANDIR, etc). See the
script itself for details.

To compile the game manually:

## From the top-level doom64ex directory:
$ rm -rf build; mkdir build ; cd build ; cmake ../src/kex ; make

To compile the WadGen tool manually:

## From the top-level doom64ex directory:
$ rm -rf build; mkdir build ; cd build ; cmake ../src/wadgen ; make

There is no "make install" target. To install, copy the doom64ex
and WadGen binaries to /usr/local/bin (or wherever you like), and
copy wadgen/Content/* to either /usr/share/games/doom64/Content or
/usr/local/share/games/doom64/Content. Also see the contents of the linux/
directory (includes a .desktop file, PNG icon, and a man page).

Wad File and SoundFont:

To play the game, you'll need a Doom64 ROM image for the N64. Run
"doom64ex-wadgen [rom_image]" to create DOOM64.WAD and DOOMSND.SF2,
then copy or move these files to /usr/share/games/doom64 (or
/usr/local/share/games/doom64).

Running the game:

Type "doom64ex" in a terminal to run the game, or select it from your
desktop's applications menu, if you installed the .desktop file. If the
game fails to run from the desktop menu, try running it in a terminal
to see what error message(s) it's emitting.

Linux-specific issues:

DOOM64.WAD and DOOMSND.SF2 need to be named in uppercase (which
is how wadgen creates them). Don't rename them to doom64.wad and
doomsnd.sf2!  The search path is hard-coded: the files are only
searched for in the current directory, in /usr/share/games/doom64, and
in /usr/local/share/games/doom64.  In the future this should be a cvar,
and maybe settable via the environment like other dooms do.

If your distribution doesn't provide a FluidSynth version >=1.1.3,
you can compile it yourself, install locally, and tell cmake to use it,
something like:

$ ccmake -DFLUIDSYNTH_INCLUDE_DIR=/home/myuser/fluid/include \
         -DFLUIDSYNTH_LIBRARIES=/home/myuser/fluid/lib \
         ../src/kex

You can ignore these warnings:
  fluidsynth: warning: Requested a period size of 64, got 940 instead
  fluidsynth: warning: Failed to set thread to high priority
  fluidsynth: warning: Failed to pin the sample data to RAM; swapping is possible.

I get this warning using nouveau, but it doesn't seem to hurt anything:
  nv50_screen_get_param:162 -  Unknown PIPE_CAP 11

If the sound is completely wrong, you probably tried to use FluidSynth
1.1.1. This is known not to work. 1.1.3 and 1.1.5 are known to work,
other versions haven't been tested.

Currently, the FluidSynth driver is hard-coded to "alsa", which is pretty
much standard on all modern Linux systems. If you need to use something
else (OSS or PortAudio or JACK), edit the file i_audio.h and change the
definition of DEFAULT_FLUID_DRIVER. In the future, this may become a
cvar, saved in the config.cfg.

Doom64EX/Linux Maintainer:

B. Watson (yalhcru@gmail.com)

If you get doom64ex to compile & run on non-intel architectures
(sparc, powerpc, etc), I'd love to hear from you, so I can update this
README. Patches are welcome.

