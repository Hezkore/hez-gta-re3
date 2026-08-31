<img src="https://github.com/hezkore/hez-gta-re3/blob/master/res/images/logo_1024.png?raw=true" alt="re3 logo" width="200">

## Intro

The original re3 is no longer up, its repository was taken down and GitHub answers with a legal notice now.\
This is my fork of the last version of it I had, with my own fixes on top.

In here you'll find the fully reversed source code for GTA III ([master](https://github.com/hezkore/hez-gta-re3/tree/master/) branch) and GTA VC ([miami](https://github.com/hezkore/hez-gta-re3/tree/miami/) branch).

It builds and runs on Windows, Linux, MacOS and FreeBSD, on x86, amd64, arm and arm64.\
Rendering is handled either by original RenderWare (D3D8)
or the reimplementation [librw](https://github.com/aap/librw) (D3D9, OpenGL 2.1 or above, OpenGL ES 2.0 or above).\
Audio is done with MSS (using dlls from original GTA) or OpenAL.

## My fixes

* The game runs at 30 FPS internally, like on the PS2, but draws in between those frames, so it is as smooth as your monitor allows.
* Animations are smoothed the same way, cutscenes included. SMOOTH ANIMATIONS turns it off.
* Motion blur works the same at any framerate, with OFF, FAINT and STRONG.
* LIMIT FPS is a list of framerates.
* Menus run at the full framerate and no longer flash the old page while fading to a new one.
* The camera turns as fast up and down as sideways, and the mouse is slower, so the sensitivity slider has a finer range.
* The left mouse button fires on a Wayland desktop.
* The defaults are the best the game can look: map memory usage high, rim lights, lightmaps and road gloss on, draw distance at max, VSync on, no FPS limit.
* On screens wider than 16:9 the HUD, subtitles and loading screens stay in a 16:9 box, and cutscenes are not zoomed in. EXTEND HUD under Display puts the HUD at the screen edges.
* The game starts on a Wayland desktop instead of quitting with `GLFW Error: X11: Platform not initialized`.

## Installation

- You **must** own the games. re3 needs [a copy of GTA III](https://store.steampowered.com/app/12100/Grand_Theft_Auto_III/), reVC needs [a copy of GTA Vice City](https://store.steampowered.com/app/12110/Grand_Theft_Auto_Vice_City/).
- Build it yourself, or grab the newest [nightly](https://github.com/hezkore/hez-gta-re3/releases). Each nightly has one zip per game:
  - `re3-windows-amd64.zip` and `re3-linux-amd64.zip` for GTA III
  - `reVC-windows-amd64.zip` and `reVC-linux-amd64.zip` for GTA Vice City
- Extract the zip over the game directory and run the binary in it. The zip has the binary, updated and additional gamefiles and the dlls OpenAL needs.

A nightly is built at midnight UTC, but only when a branch has moved since the last one. The ten newest are kept.

## Screenshots

![re3 2021-02-11 22-57-03-23](https://user-images.githubusercontent.com/1521437/107704085-fbdabd00-6cbc-11eb-8406-8951a80ccb16.png)
![re3 2021-02-11 22-43-44-98](https://user-images.githubusercontent.com/1521437/107703339-cbdeea00-6cbb-11eb-8f0b-07daa105d470.png)
![re3 2021-02-11 22-46-33-76](https://user-images.githubusercontent.com/1521437/107703343-cd101700-6cbb-11eb-9ccd-012cb90524b7.png)
![re3 2021-02-11 22-50-29-54](https://user-images.githubusercontent.com/1521437/107703348-d00b0780-6cbb-11eb-8afd-054249c2b95e.png)

## Improvements

We have implemented a number of changes and improvements to the original game.
They can be configured in `core/config.h`.
Some of them can be toggled at runtime, some cannot.

* Fixed a lot of smaller and bigger bugs
* User files (saves and settings) stored in GTA root directory
* Settings stored in re3.ini file instead of gta3.set
* Debug menu to do and change various things (Ctrl-M to open)
* Debug camera (Ctrl-B to toggle)
* Rotatable camera
* XInput controller support (Windows)
* No loading screens between islands ("map memory usage" in menu)
* Skinned ped support (models from Xbox or Mobile)
* Rendering
  * Widescreen support (properly scaled HUD, Menu and FOV)
  * PS2 MatFX (vehicle reflections)
  * PS2 alpha test (better rendering of transparency)
  * PS2 particles
  * Xbox vehicle rendering
  * Xbox world lightmap rendering (needs Xbox map)
  * Xbox ped rim light
  * Xbox screen rain droplets
  * More customizable colourfilter
* Menu
  * Map
  * More options
  * Controller configuration menu
  * ...
* Can load DFFs and TXDs from other platforms, possibly with a performance penalty
* ...

## To-Do

The following things would be nice to have/do:

* Improve performance on lower end devices, especially the OpenGL layer on the Raspberry Pi (if you have experience with this, please get in touch)
* Compare code with PS2 code (tedious, no good decompiler)
* PS2 port
* Xbox port (not quite as important)
* reverse remaining unused/debug functions
* compare CodeWarrior build with original binary for more accurate code (very tedious)

## Modding

Asset modifications (models, texture, handling, script, ...) should work the same way as with original GTA for the most part.

CLEO scripts work with [CLEO Redux](https://github.com/cleolibrary/CLEO-Redux).

Mods that make changes to the code (dll/asi, limit adjusters) will *not* work.
Some things these mods do are already implemented in re3 (much of SkyGFX, GInput, SilentPatch, Widescreen fix),
others can easily be achieved (increasing limis, see `config.h`),
others will simply have to be rewritten and integrated into the code directly.
Sorry for the inconvenience.

## Building from Source  

When using premake, you may want to point GTA_III_RE_DIR environment variable to GTA3 root folder if you want the executable to be moved there via post-build script.

Clone the repository with `git clone --recursive https://github.com/hezkore/hez-gta-re3.git`. Then `cd hez-gta-re3` into the cloned repository.

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

<details><summary>Linux Conan</summary>

Install python and conan, and then run build.
```
conan export vendor/librw librw/master@
mkdir build
cd build
conan install .. re3/master@ -if build -o re3:audio=openal -o librw:platform=gl3 -o librw:gl3_gfxlib=glfw --build missing -s re3:build_type=RelWithDebInfo -s librw:build_type=RelWithDebInfo
conan build .. -if build -bf build -pf package
```
</details>

<details><summary>MacOS Premake</summary>

The old instructions went with the upstream wiki. premake generates the projects the same way it does on Linux, though this has not been built on a Mac in a long time.

</details>

<details><summary>FreeBSD</summary>

The old instructions went with the upstream wiki. premake generates the projects the same way it does on Linux, though this has not been built on FreeBSD in a long time.

</details>

<details><summary>Windows</summary>

Assuming you have Visual Studio 2015/2017/2019:
- Run one of the `premake-vsXXXX.cmd` variants on root folder.
- Open build/re3.sln with Visual Studio and compile the solution.

Visual Studio 2022 builds the vs2019 solution as long as the v142 toolset is installed, which is how the nightlies are built.

Microsoft recently discontinued its downloads of the DX9 SDK. You can download an archived version here: https://archive.org/details/dxsdk_jun10

**If you choose OpenAL on Windows** the exe needs `OpenAL32.dll` and `libmpg123-0.dll` next to it. Both ship with the repository, in `vendor/openal-soft/dist/Win64` and `vendor/mpg123/dist/Win64`.
</details>

> :information_source: premake has an `--with-lto` option if you want the project to be compiled with Link Time Optimization.

> :information_source: There are various settings in [config.h](src/core/config.h), you may want to take a look there.

> :information_source: re3 uses completely homebrew RenderWare-replacement rendering engine; [librw](https://github.com/aap/librw/). librw comes as submodule of re3, but you also can use LIBRW enviorenment variable to specify path to your own librw.

If you feel the need, you can also use CodeWarrior 7 to compile re3 using the supplied codewarrior/re3.mcp project - this requires the original RW33 libraries, and the DX8 SDK. The build is unstable compared to the MSVC builds though, and is mostly meant to serve as a reference.

## Contributing
As long as it's not linux/cross-platform skeleton/compatibility layer, all of the code on the repo that's not behind a preprocessor condition(like FIX_BUGS) are **completely** reversed code from original binaries.  

We **don't** accept custom codes, as long as it's not wrapped via preprocessor conditions, or it's linux/cross-platform skeleton/compatibility layer.

We accept only these kinds of PRs;

- A new feature that exists in at least one of the GTAs (if it wasn't in III/VC then it doesn't have to be decompilation)  
- Game, UI or UX bug fixes (if it's a fix to original code, it should be behind FIX_BUGS)
- Platform-specific and/or unused code that's not been reversed yet
- Makes reversed code more understandable/accurate, as in "which code would produce this assembly".
- A new cross-platform skeleton/compatibility layer, or improvements to them
- Translation fixes, for languages original game supported
- Code that increase maintainability  

We have a [Coding Style](CODING_STYLE.md) document that isn't followed or enforced very well.

Do not use features from C++11 or later.


## History

re3 was started sometime in the spring of 2018,
initially as a way to test reversed collision and physics code
inside the game.
This was done by replacing single functions of the game
with their reversed counterparts using a dll.

After a bit of work the project lay dormant for about a year
and was picked up again and pushed to github in May 2019.
At the time I (aap) had reversed around 10k lines of code and estimated
the final game to have around 200-250k.
Others quickly joined the effort (Fire_Head, shfil, erorcun and Nick007J
in time order, and Serge a bit later) and we made very quick progress
throughout the summer of 2019
after which the pace slowed down a bit.

Due to everyone staying home during the start of the Corona pandemic
everybody had a lot of time to work on re3 again and
we finally got a standalone exe in April 2020 (around 180k lines by then).

After the initial excitement and fixing and polishing the code further,
reVC was started in early May 2020 by starting from re3 code,
not by starting from scratch replacing functions with a dll.
After a few months of mostly steady progress we considered reVC
finished in December.

Since then we have started reLCS, which is currently work in progress.


## License

We don't feel like we're in a position to give this code a license.\
The code should only be used for educational, documentation and modding purposes.\
We do not encourage piracy or commercial use.\
Please keep derivate work open source and give proper credit.
