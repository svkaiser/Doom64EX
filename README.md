Doom64EX [![Build Status](https://travis-ci.org/svkaiser/Doom64EX.svg?branch=master)](https://travis-ci.org/svkaiser/Doom64EX) [![Build status](https://ci.appveyor.com/api/projects/status/04kswu014uwrljrd/branch/master?svg=true)](https://ci.appveyor.com/project/dotfloat/doom64ex/branch/master)
========

Doom64EX is a reverse-engineering project aimed to recreate Doom64 as close as
possible with additional modding features.

**NOTICE 2nd February 2017** The supplementary data file `kex.wad` has been
renamed to `doom64ex.pk3`.

# Installing

At the moment there are no official binary builds. You can find older
versions [here](https://doom64ex.wordpress.com/downloads/).

# Compiling

It's possible to compile Doom64EX yourself. Officially, only Linux, Windows and
macOS are supported. Patches for alternative operating system are gladly
accepted, however.

## Dependencies

|                                                      | Ubuntu 14.04      | Fedora 24        | Arch Linux / [MSYS2*](http://www.msys2.org/) on Windows | [Homebrew](http://brew.sh/) on macOS        |
|------------------------------------------------------|-------------------|------------------|---------------------------------------------------------|---------------------------------------------|
| C++14 compiler                                       | g++-6             | gcc              | gcc                                                     | [Xcode](https://developer.apple.com/xcode/) |
| [CMake](https://cmake.org/download/)                 | cmake             | cmake            | cmake                                                   | cmake                                       |
| [SDL2](http://libsdl.org/download-2.0.php)           | libsdl2-dev       | SDL2-devel       | sdl2                                                    | sdl2                                        |
| [SDL2_net](https://www.libsdl.org/projects/SDL_net/) | libsdl2_net-dev   | SDL2_net-devel   | sdl2_net                                                | sdl2_net                                    |
| [zlib](http://www.zlib.net/)                         | zlib1g-dev        | zlib-devel       | zlib                                                    | zlib                                        |
| [libpng](http://www.libpng.org/pub/png/libpng.html)  | libpng-dev        | libpng-devel     | libpng                                                  | libpng                                      |
| [FluidSynth**](http://www.fluidsynth.org/)           | libfluidsynth-dev | fluidsynth-devel | fluidsynth                                              | N/A                                         |

\* MSYS2 uses a naming convention similar to the one utilised by Arch, except
packages are prefixed with `mingw-w64-i686-` and `mingw-w64-x86_64-` for 32-bit
and 64-bit packages, respectively.

\** FluidSynth is optional.
The [fluidsynth-lite](https://github.com/dotfloat/fluidsynth-lite) fork can be
used instead.

Note: You may also need to install dynamic libraries separately.

## Using the system-provided FluidSynth library

Doom64EX uses [fluidsynth-lite](https://github.com/dotfloat/fluidsynth-lite) to
reduce the number of dependencies. If you wish to use FluidSynth as provided by
your package-manager, add `-DENABLE_SYSTEM_FLUIDSYNTH=ON` as a cmake argument.

    $ cmake -DENABLE_SYSTEM_FLUIDSYNTH=ON ..

## Compiling on Linux

All of these steps are done using the terminal.

### Prepare the Dependencies

On Ubuntu:

    $ # Add additional toolchains
    $ sudo add-apt repository ppa:ubuntu-toolchain-r/test
    $ sudo apt update
    
    $ # Install GCC
    $ sudo apt install build-essential gcc-6 g++-6
    
    $ # Install dependencies
    $ sudo apt install git cmake libsdl2-dev libsdl2_net-dev zlib1g-dev libpng-dev

On Fedora:

    $ # Install development group
    $ sudo dnf groupinstall "Development Tools and Libraries"
    
    $ # Install dependencies
    $ sudo dnf install git cmake sdl2-devel sdl2_net-devel zlib-devel libpng-devel
    
On Arch Linux:

    $ # Install dependencies
    $ sudo pacman -S git gcc cmake sdl2 sdl2_net zlib libpng

### Clone and Build

Find a suitable place to build the program and navigate there using `cd`.

    $ # Clone this repository (if you haven't done so already)
    $ git clone https://github.com/svkaiser/Doom64EX --recursive
    $ cd Doom64EX

    $ # If you have previously cloned the repository, you'll need to also grab the fluidsynth-lite submodule
    $ git update --init --recursive
    
    $ mkdir build       # Create a build directory within the git repo
    $ cd build          # Change into the new directory
    $ cmake ..          # Generate Makefiles
    $ make              # Build everything
    $ sudo make install # Install Doom64EX to /usr/local
    
You can now launch Doom64EX from the menu or using `doom64ex` from terminal.

**NOTICE** Ubuntu ships with a severely outdated version of CMake, so you'll
need to create the `doom64ex.pk3` file manually.

## Compiling on Windows

Download and install [CMAKE](https://cmake.org/download/). Follow the instructions on
the website and make sure to update the system. Clone the repository in a suitable place to build the program.

Next, download the [Win32 Dependencies](https://github.com/svkaiser/Doom64EX/releases/download/win32dep-2018-04-11/Doom64EX-deps-win32-2018-04-11.zip). Extract the archive into the `extern` directory. Also remember to clone [fluidsynth-lite](https://github.com/dotfloat/fluidsynth-lite) and generate the `.lib` and `.dll` files. Place these in `extern\lib` and `extern\bin`, respectively.

Next, generate the MSVC project files.

    $ mkdir build                           # Create a build directory within the git repo
    $ cd build                              # Change into the new directory
    $ cmake .. -G "Visual Studio 15 2017"   # Generate MSVC 2017 files
    
Visual Studio 2017 project files will now be sitting in the `build` directory. 

## Compiling on macOS

Install [Xcode](https://developer.apple.com/xcode/) for its developer tools.
Follow the instructions to install [Homebrew](http://brew.sh/). You can probably
use other package managers, but Doom64EX has only been tested with Homebrew.

Open `Terminal.app` (or a terminal replacement).

    $ # Install dependencies
    $ brew install git cmake sdl2 sdl2_net libpng zlib
    
Find a suitable place to build the program and navigate there using terminal.

    $ # Clone this repository (if you haven't done so already)
    $ git clone https://github.com/svkaiser/Doom64EX --recursive

    $ # If you have previously cloned the repository, you'll need to also grab the fluidsynth-lite submodule
    $ git update --init --recursive
    
    $ mkdir build       # Create a build directory within the git repo
    $ cd build          # Change into the new directory
    $ cmake ..          # Generate Makefiles
    $ make              # Build everything
    $ sudo make install # Install Doom64EX.app

You will now find Doom64EX in your Applications directory.

## Creating `doom64ex.pk3`

If for some reason CMake refuses to automatically generate the required
`doom64ex.pk3`, you can easily create it yourself.

## Data Files

The data files required by Doom64EX to function are:

* `doom64ex.pk3` (Generated by cmake)
* `doom64.wad`
* `doomsnd.sf2`

To generate the two latter files, acquire a Doom64 ROM and run:

    $ doom64ex -wadgen PATH_TO_ROM

This will generate the required files and place them somewhere where Doom64EX
can find them.

Doom64EX needs the Doom 64 data to be present in any of the following
directories.

### On Linux and BSDs

* Current directory
* The directory in which the `doom64ex` executable resides
* `$XDG_DATA_HOME/doom64ex` (usually `~/.local/share/doom64ex`)
* `/usr/local/share/games/doom64ex`
* `/usr/local/share/doom64ex`
* `/usr/local/share/doom`
* `/usr/share/games/doom64ex`
* `/usr/share/doom64ex`
* `/usr/share/doom`
* `/opt/doom64ex`

### On Windows

* Current directory
* The directory in which the `doom64ex` executable resides

### On macOS

* Current directory
* `~/Library/Application Support/doom64ex`

# Community

**[Official Blog](https://doom64ex.wordpress.com/)**

**[Forum](http://z13.invisionfree.com/Doom64EX/index.php)** 

**[Discord](https://discord.gg/AHd8t33)**

You can join the official IRC channel `#doom64ex` on `irc.oftc.net` (OFTC).
