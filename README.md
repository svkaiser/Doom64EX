Doom64EX
========

Doom64EX is a reverse-engineering project aimed to recreate Doom64 as close as possible with additional modding features.

### Compiling on Linux

Clone this repo

    $ git clone https://github.com/svkaiser/Doom64EX

Create a new directory inside the repo root where compilation will take place

    $ mkdir Doom64EX/temp
    $ cd Doom64EX/temp

Run cmake and make

    $ cmake .. && make

(`make install` is currently unsupported)

Run Doom64EX! You'll need `doom64.wad` and `doomsnd.sf2`.

    $ ./doom64ex
