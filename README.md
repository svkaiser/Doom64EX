This is a fork of svkaiser's Doom 64 EX project, initially released in 2008.
Since the development is still on going, albiet slowly since most of the people who worked on it are now doing other things in the world, I decided to continue the project, albiet in an unofficial status, in the hopes that one day my changes will be merged with the mainline project.
Once a stable build is made it will be called v3.0.0, even though I consider the Doom 64 remaster something of a sucessor to Doom 64 EX.

//.plan
1. Tend to a lot of issues so that this release will be as bug-free as possible.
2. Upgrade OpenGL api to 4.6 to gain some more modern features compared to the last official release of this project, version 2.5
3. Add + remove some code, using the complete reverse engineering project by Erick194 and Team GEC https://github.com/Erick194/DOOM64-RE
4. Anything else that I can come up with (placeholder)

//prerequisites
You will need the following to build Doom 64 EX from git. (No official builds at this time):
    1. A C compiler
    2. Cmake for creating project files for multiple platforms
    3. SDL2 and SDL2_net
    4. zlib
    5. libpng
    6. Fluidsynth (either the system Fluidsynth, or the alternative Fluidsynth-lite fork by Dotfloat)
