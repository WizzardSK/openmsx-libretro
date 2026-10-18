#ifndef LIBRETROSOUNDDRIVER_HH
#define LIBRETROSOUNDDRIVER_HH

#include "SoundDriver.hh"

#include <cstdint>
#include <vector>

namespace openmsx {

/** Collects what the MSX mixer produces during a retro_run; the core hands
  * it to the frontend as 16-bit stereo at the end of the run. */
class LibretroSoundDriver final : public SoundDriver
{
public:
	static constexpr unsigned FREQUENCY = 48000;
	// Samples per fragment: the mixer produces one fragment at a time and
	// leaves the CPU loop after each, so smaller is more even
	static constexpr unsigned SAMPLES = 256;

	LibretroSoundDriver();
	~LibretroSoundDriver() override;

	void mute() override;
	void unmute() override;
	[[nodiscard]] unsigned getFrequency() const override;
	[[nodiscard]] unsigned getSamples() const override;
	void uploadBuffer(std::span<const StereoFloat> buffer) override;

	/** The driver the mixer uses at the moment, or nullptr */
	[[nodiscard]] static LibretroSoundDriver* current();

	/** Interleaved stereo produced since the last take(), then cleared */
	std::vector<int16_t>& pending() { return samples; }

private:
	std::vector<int16_t> samples;
	bool muted = true;
};

} // namespace openmsx

#endif
