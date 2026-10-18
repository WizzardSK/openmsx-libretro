#ifndef LIBRETROHOOKS_HH
#define LIBRETROHOOKS_HH

namespace openmsx {

class MSXMotherBoard;

/** Called by the VDP renderer at the end of every frame: retro_run stops the
  * emulation there, so each run emulates exactly one frame. */
void libretroFrameEnd(MSXMotherBoard& motherBoard);

} // namespace openmsx

#endif
