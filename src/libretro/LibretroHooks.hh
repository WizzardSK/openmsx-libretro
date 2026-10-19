#ifndef LIBRETROHOOKS_HH
#define LIBRETROHOOKS_HH

namespace openmsx {

class MSXMotherBoard;

/** Called by the VDP renderer at the end of every frame: retro_run stops the
  * emulation there, so each run emulates exactly one frame. The frame's
  * length in VDP ticks tells 50 Hz from 60 Hz. */
void libretroFrameEnd(MSXMotherBoard& motherBoard, int ticksPerFrame);

} // namespace openmsx

#endif
