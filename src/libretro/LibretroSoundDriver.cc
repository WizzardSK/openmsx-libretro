#include "LibretroSoundDriver.hh"

#include "Mixer.hh"

#include <algorithm>
#include <cmath>

namespace openmsx {

static LibretroSoundDriver* currentDriver = nullptr;

LibretroSoundDriver::LibretroSoundDriver()
{
	currentDriver = this;
}

LibretroSoundDriver::~LibretroSoundDriver()
{
	if (currentDriver == this) currentDriver = nullptr;
}

LibretroSoundDriver* LibretroSoundDriver::current()
{
	return currentDriver;
}

void LibretroSoundDriver::mute()
{
	muted = true;
}

void LibretroSoundDriver::unmute()
{
	muted = false;
}

unsigned LibretroSoundDriver::getFrequency() const
{
	return FREQUENCY;
}

unsigned LibretroSoundDriver::getSamples() const
{
	return SAMPLES;
}

void LibretroSoundDriver::uploadBuffer(std::span<const StereoFloat> buffer)
{
	if (muted) return;
	auto toS16 = [](float f) {
		return int16_t(std::clamp(std::lround(f * 32767.0f), -32768L, 32767L));
	};
	samples.reserve(samples.size() + 2 * buffer.size());
	for (const auto& s : buffer) {
		samples.push_back(toS16(s.left));
		samples.push_back(toS16(s.right));
	}
}

} // namespace openmsx
