#ifndef LIBRETROSHARE_HH
#define LIBRETROSHARE_HH

#include <string>

namespace openmsx {

/** The share folder the core carries (machines, extensions, scripts, the
  * C-BIOS ROMs; build/libretro_share.py) written to shareDir, unless the
  * same data is there already (its checksum in shareDir/.libretro-share).
  * Files not among it, such as system ROMs put in share/systemroms, stay.
  * Returns the number of files written (0 when none needed to be), or
  * throws MSXException. */
int installShare(const std::string& shareDir);

} // namespace openmsx

#endif
