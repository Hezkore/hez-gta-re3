# reLCS

## Intro

The original re3 is no longer up, its repository was taken down and GitHub answers with a legal notice now.\
This is my fork of its unfinished lcs branch, with my fixes from the other two games ported over.

reLCS aims to bring GTA Liberty City Stories to PC by reversing its engine.
The reversal was left unfinished when the repository went down, so pieces are missing,
but the game starts, plays and no longer falls over on the way there.

## My fixes

The fixes from [the master branch](https://github.com/hezkore/hez-gta-re3/tree/master/#my-fixes) are in here too. On top of those, this branch adds:

* The game no longer turns black when the video mode changes.
* The menus have the text the game files are without, so the graphics page and its options read as they should.
* The PSP hud keeps to its corners and draws at a sane size for a monitor. PSP hud scale under Render in the debug menu adjusts it.

There are no nightlies for this branch, build it yourself.

## Installation

- You **must** own the game. Liberty City Stories was not sold for PC, so that means [the mobile release](https://play.google.com/store/apps/details?id=com.rockstargames.gtalcs) or a PSP or PS2 copy.
- The game data goes in a directory of your choice:
  - the converted files the reLCS project made, [lcs_dist.7z](http://gta.rockstarvision.com/lcs_dist.7z), extracted to the directory, and [lcs_dist_hires.7z](http://gta.rockstarvision.com/lcs_dist_hires.7z) extracted over its `models` if you want the sharper hud textures
  - the music, cutscene and news files from your copy into `audio`: the `AUDIO` folders of a PS2 disc, or the `MUSIC`, `CUTSCENE` and `NEWS` mp3 folders from a mobile install
- Copy the files in `gamefiles` to the directory, then build and put the `reLCS` binary next to them and run it.

## Building from Source

When using premake, you may want to point GTA_LCS_RE_DIR environment variable to GTA LCS root folder if you want the executable to be moved there via post-build script.

Clone the repository with `git clone --recursive -b lcs https://github.com/hezkore/hez-gta-re3.git`. Then `cd hez-gta-re3` into the cloned repository.

<details><summary>Linux Premake</summary>

You need the development files for glfw, OpenAL, libsndfile, mpg123 and OpenGL. On Debian and Ubuntu that is:
```
sudo apt install libglfw3-dev libopenal-dev libsndfile1-dev libmpg123-dev libgl1-mesa-dev
```
Then generate the makefiles and build:
```
./premake5Linux --with-librw gmake2
make -C build config=release_linux-amd64-librw_gl3_glfw-oal
```
The binary ends up in `bin/linux-amd64-librw_gl3_glfw-oal/Release`. Swap `amd64` for `x86`, `arm` or `arm64`, and `release` for `debug`.

Reading the keyboard from the X server needs glfw 3.4 or newer. On anything older the build stops with an error, add `--no-x11-keyboard` to premake and GLFW handles the keys instead.

</details>

<details><summary>Windows</summary>

Assuming you have Visual Studio:
- Run one of the `premake-vsXXXX.cmd` variants on root folder.
- Open the project via Visual Studio.

</details>

There are various settings at the very bottom of [config.h](https://github.com/hezkore/hez-gta-re3/tree/lcs/src/core/config.h), you may want to take a look there.
