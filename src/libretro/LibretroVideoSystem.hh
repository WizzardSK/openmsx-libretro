#ifndef LIBRETROVIDEOSYSTEM_HH
#define LIBRETROVIDEOSYSTEM_HH

#include "OutputSurface.hh"
#include "VideoSystem.hh"

#include "components.hh"

#include <memory>

namespace openmsx {

class Display;
class Layer;
class Reactor;

/** The surface the libretro core paints into: RetroArch's framebuffer of the
  * hardware-rendered (OpenGL) context, bound before every repaint. */
class LibretroSurface final : public OutputSurface
{
public:
	LibretroSurface(gl::ivec2 logicalSize, gl::ivec2 physicalSize);
	void resize(gl::ivec2 logicalSize, gl::ivec2 physicalSize);
	using OutputSurface::moveViewPort;
	void saveScreenshot(const std::string& filename) override;
};

/** VideoSystem of the libretro core: no window of its own, the frame is
  * painted into the frontend's framebuffer when the core hands it over. */
class LibretroVideoSystem final : public VideoSystem
{
public:
	explicit LibretroVideoSystem(Reactor& reactor);
	~LibretroVideoSystem() override;

	// VideoSystem interface:
	[[nodiscard]] std::unique_ptr<Rasterizer> createRasterizer(VDP& vdp) override;
	[[nodiscard]] std::unique_ptr<V9990Rasterizer> createV9990Rasterizer(
		V9990& vdp) override;
	[[nodiscard]] std::unique_ptr<SMSVDPRasterizer> createSMSVDPRasterizer(
		SMSVDP& vdp) override;
#if COMPONENT_LASERDISC
	[[nodiscard]] std::unique_ptr<LDRasterizer> createLDRasterizer(
		LaserdiscPlayer& ld) override;
#endif
	void flush() override;
	void takeScreenShot(const std::string& filename, bool withOsd) override;
	[[nodiscard]] std::optional<gl::ivec2> getMouseCoord() override;
	[[nodiscard]] std::optional<gl::vec2> getMsxPixelSize() override;
	[[nodiscard]] OutputSurface* getOutputSurface() override;
	void showCursor(Cursor cursor) override;
	void updateCursor() override;
	[[nodiscard]] bool getCursorEnabled() override;
	[[nodiscard]] std::string getClipboardText() override;
	void setClipboardText(zstring_view text) override;
	[[nodiscard]] std::optional<gl::ivec2> getWindowPosition() override;
	void setWindowPosition(gl::ivec2 pos) override;
	void repaint() override;

	/** Paints the current frame, 'size' pixels in all, into the
	  * framebuffer the frontend gave (fbo), moved by 'offset': a part of
	  * the frame that starts at -offset then lands in the bottom-left
	  * corner, which is what the frontend shows of a smaller frame. */
	void paintTo(unsigned fbo, gl::ivec2 size, gl::ivec2 offset);

private:
	Reactor& reactor;
	Display& display;
	std::unique_ptr<LibretroSurface> surface;
	std::unique_ptr<Layer> snowLayer;
	std::unique_ptr<Layer> osdGuiLayer;
};

} // namespace openmsx

#endif
