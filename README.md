# openMSX libretro core

A libretro core of [openMSX](https://openmsx.org), the MSX emulator that aims for perfection, for RetroArch and other libretro frontends. This fork adds the core on the `libretro` branch and keeps openMSX itself as upstream has it; openMSX's own documentation is in [README](README) and `doc/`.

The core is preliminary. Builds of it are in the [releases](https://github.com/WizzardSK/openmsx-libretro/releases/tag/libretro-latest).

## Status

| Area | Status |
|---|---|
| Video (OpenGL, drawn into the frontend's framebuffer) | ✅ working |
| Video Output: Software (the picture as pixels, any video driver), the default | ✅ working (Tatsuya79) |
| Sound | ✅ working |
| Keyboard | ✅ working |
| MSX joystick, cursor keys on the RetroPad | ✅ working |
| MSX mouse | ⚠️ untested |
| Core options: machine, extensions, resolution, crop overscan | ✅ working |
| 50 Hz machines (Europe, UK) | ✅ the frame rate follows the machine |
| Savestates, rewind, run-ahead | ✅ working; a savestate loads only into the build that made it |
| Disk control, `.m3u` | ⚠️ untested; needs a machine with a disk drive and its ROMs |
| Cassettes | ⚠️ untested; need a machine with BASIC |
| A lost OpenGL context (another video driver, Android) | ⚠️ untested |
| Linux x86_64, aarch64 | ✅ aarch64 tested in RetroArch |
| Windows x86_64, macOS x86_64 and arm64 | ⚠️ built, untested |
| Android | ❌ openMSX draws with desktop OpenGL only, there is no GLES renderer |
| libretro buildbot | ❌ not yet |

## Installing

- Put the core from the zip for your platform in RetroArch's cores folder, and `openmsx_libretro.info` in its info folder (RetroArch then shows the core's name, its content types and these notes).
- Extract `openmsx-system.zip` into RetroArch's system folder, so that it ends up as `system/openmsx/share`. It holds openMSX's machines, extensions and scripts, with the C-BIOS ROMs, so games start without any other BIOS.
- Real machines and some extensions need their system ROMs in `system/openmsx/share/systemroms`.
- The core hands RetroArch the picture as pixels by default, with any video driver; with Video > Video Output set to OpenGL it draws through RetroArch's OpenGL context, so the video driver has to be `gl` (or `glcore`).

Content: cartridges (`.rom`, `.ri`, `.mx1`, `.mx2`, `.col`, `.sg`, `.sc`), disk images (`.dsk`, `.di1`, `.di2`, `.dmk`, `.xsa`), cassettes (`.cas`, `.wav`), `.m3u` lists of disks, and any of them zipped or gzipped.

## Core options

| Option | Values | Notes |
|---|---|---|
| System > Machine | C-BIOS MSX1, MSX2, MSX2+ (international, Europe, Japan); Philips, Sony, Toshiba, Yamaha and Panasonic MSX1, MSX2, MSX2+ and turbo R machines | at the next start of the content; without a real machine's ROMs the core starts C-BIOS MSX2+ and says so |
| System > Extension 1 to 10 | None, or any of openMSX's extensions (sound, video, disk, RAM and other cartridges) | at the next start of the content; some need their ROMs; after a start as many are shown as there are in use and empty cartridge slots (a Slot Expander's too), before it two; after a start the core shows what is in the cartridge slots, the log has the whole slot map |
| Video > Resolution | Auto (1x for the 256 pixel wide screen modes, 2x for the 512 wide and interlaced ones, switching with the mode), or 1x (320x240) to 4x (1280x960) | on the fly |
| Video > Video Output | Software / OpenGL | at the next start of the content; Software (the default) hands RetroArch the picture as pixels, with no GL context, for any video driver (no openMSX OSD, screenshots through RetroArch); OpenGL adds openMSX's own scalers and effects |
| Video > Crop Overscan | on / off | only the part of the screen the MSX displays on, without its border; the size follows the screen mode |
| Video > Pixel Aspect Ratio | TV / Square | TV: the picture as wide as on a TV, as openMSX shows it; Square: each pixel as wide as it is tall |

## Controls

The device for each port is chosen in Controls > Port 1/2 Controls > Device Type:

| Device | |
|---|---|
| MSX Joystick (default) | D-pad or left stick, A and B are triggers A and B; ports 1 and 2 are the MSX's two joystick ports |
| Cursor Keys | for games played on the keyboard: the D-pad presses the cursor keys, A presses Space and B presses M |
| MSX Mouse | RetroArch's mouse |
| Arkanoid Vaus Paddle | Arkanoid's paddle, turned by the mouse's horizontal motion, the left button fires; Arkanoid games need it, with the MSX mouse the bat jumps across the screen |
| Keyboard | nothing in the MSX port; the port's pad buttons can be remapped to keyboard keys (Quick Menu > Controls), such as F8 |
| None | nothing in that port |

With either RetroPad device, X presses Space, Y Return, Start F1, Select F5, L Esc and R Shift. A keyboard works as the MSX keyboard.

L3 on port 1 is Machine Info (remappable in Quick Menu > Controls): the game stops and the machine is shown in the picture as standalone's Machine > Current setup has it: maker, model, year, type, region, RAM and VRAM size, VDP version, disk drives and cartridge slots, its description, and what is in each slot, at least at 640x480, a page at a time. The same button shows the next page, and after the last one returns to the game; the pages name it Machine Info button, as RetroArch does not tell the core which button it is remapped to. It needs Video Output at Software, the default.

## Building

The core builds with openMSX's own make, against static libraries it builds first (SDL2 without video or audio, Tcl, FreeType, GLEW, libpng, zlib). It needs a C++23 compiler; the Linux and Windows builds use GCC 13, macOS Xcode 16.

```
export LIBRETRO=true
make 3rdparty OPENMSX_TARGET_CPU=x86_64 OPENMSX_TARGET_OS=linux OPENMSX_FLAVOUR=opt
make OPENMSX_TARGET_CPU=x86_64 OPENMSX_TARGET_OS=linux OPENMSX_FLAVOUR=opt 3RDPARTY_FLAG=true
```

The core is then `derived/x86_64-linux-opt-3rd/lib/openmsx_libretro.so`. `OPENMSX_TARGET_CPU` is `x86_64` or `aarch64`, `OPENMSX_TARGET_OS` is `linux`, `mingw-w64` (cross-compiled, giving `openmsx_libretro.dll`) or `darwin` (`openmsx_libretro.dylib`). The workflow in `.github/workflows/libretro.yml` builds all of them, and `.gitlab-ci.yml` is the recipe for the libretro buildbot.

The libretro frontend is in `src/libretro/`; the build switches are in `build/main.mk` and `build/3rdparty.mk` under `LIBRETRO`.

## License

GPL-2.0, as openMSX. C-BIOS, in the system files, has its own license, in `system/openmsx/share/cbios.txt`.
