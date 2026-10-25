#ifndef LIBRETROHOOKS_HH
#define LIBRETROHOOKS_HH

#include <string>
#include <string_view>

namespace openmsx {

class FrameSource;
class MSXMotherBoard;

/** The core's Software output (core option): openMSX makes no GL objects
  * then, and the PostProcessor hands each finished frame over as pixels
  * instead of uploading it to a texture. */
[[nodiscard]] bool libretroSoftwareOutput();
void libretroSoftwareFrame(const FrameSource& frame);

/** Called by the VDP renderer at the end of every frame. The frame's length
  * in VDP ticks tells 50 Hz from 60 Hz. retro_run stops there only when the
  * frame had no end of display (libretroDisplayDone), as in overscan. */
void libretroFrameEnd(MSXMotherBoard& motherBoard, int ticksPerFrame);

/** Called by the VDP renderer at the end of the display area, once the frame
  * is handed over: retro_run stops the emulation there, so each run emulates
  * one frame, from one VBLANK to the next. */
void libretroDisplayDone(MSXMotherBoard& motherBoard);

/** True while retro_unserialize builds the machine of a savestate and drops
  * the one it replaces (run-ahead does it every frame or input change):
  * the SRAMs (an RTC's CMOS, a game's saves) are neither read from nor
  * written to their files then; the state holds their content, and the
  * machine that goes is a timeline left behind. */
[[nodiscard]] bool libretroStateLoad();

/** Forgets the ROM files Rom keeps for the machines savestate loads build
  * (the content is closed: another one may have other files). */
void libretroClearRomCache();

/** A system file's content (a keyboard's unicode map), read once: every
  * machine reads them, and a savestate load builds one. */
[[nodiscard]] std::string_view libretroSystemFile(const std::string& filename);

} // namespace openmsx

#endif
