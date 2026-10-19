#include "LibretroVideoSystem.hh"

#include "Display.hh"
#include "GLContext.hh"
#include "GLSnow.hh"
#include "GLUtil.hh"
#include "MSXException.hh"
#include "OSDGUILayer.hh"
#include "PostProcessor.hh"
#include "RenderSettings.hh"
#include "SDLRasterizer.hh"
#include "SMSVDP.hh"
#include "SMSVDPSDLRasterizer.hh"
#include "V9990.hh"
#include "V9990SDLRasterizer.hh"
#include "VDP.hh"
#include "VisibleSurface.hh"

#include "Reactor.hh"

#if COMPONENT_LASERDISC
#include "LaserdiscPlayer.hh"
#include "LDSDLRasterizer.hh"
#endif

#include <cassert>
#include <memory>

namespace openmsx {

LibretroSurface::LibretroSurface(gl::ivec2 logicalSize, gl::ivec2 physicalSize)
{
	resize(logicalSize, physicalSize);
}

void LibretroSurface::resize(gl::ivec2 logicalSize, gl::ivec2 physicalSize)
{
	calculateViewPort(logicalSize, physicalSize);
	gl::context->setupMvpMatrix(gl::vec2(logicalSize));
}

void LibretroSurface::saveScreenshot(const std::string& filename)
{
	// Reads back the framebuffer that is bound, the frontend's one
	VisibleSurface::saveScreenshotGL(*this, filename);
}


LibretroVideoSystem::LibretroVideoSystem(Reactor& reactor_)
	: reactor(reactor_)
	, display(reactor.getDisplay())
{
	auto size = display.getWindowSize();
	surface = std::make_unique<LibretroSurface>(size, size);

	// The snow and the OSD (console, icons) as with the SDL video system;
	// ImGui needs a platform back-end that the core does not have yet.
	snowLayer = std::make_unique<GLSnow>(display);
	osdGuiLayer = std::make_unique<OSDGUILayer>(display.getOSDGUI());
	display.addLayer(*snowLayer);
	display.addLayer(*osdGuiLayer);
}

LibretroVideoSystem::~LibretroVideoSystem()
{
	display.removeLayer(*osdGuiLayer);
	display.removeLayer(*snowLayer);
}

std::unique_ptr<Rasterizer> LibretroVideoSystem::createRasterizer(VDP& vdp)
{
	assert(display.getRenderer() == RenderSettings::RendererID::SDLGL_PP);
	std::string videoSource = (vdp.getName() == "VDP")
	                        ? "MSX" // for backwards compatibility
	                        : vdp.getName();
	auto& motherBoard = vdp.getMotherBoard();
	return std::make_unique<SDLRasterizer>(
		vdp, display,
		std::make_unique<PostProcessor>(
			motherBoard, display, *surface,
			videoSource, 640, 240, true));
}

std::unique_ptr<V9990Rasterizer> LibretroVideoSystem::createV9990Rasterizer(
	V9990& vdp)
{
	assert(display.getRenderer() == RenderSettings::RendererID::SDLGL_PP);
	std::string videoSource = (vdp.getName() == "Sunrise GFX9000")
	                        ? "GFX9000" // for backwards compatibility
	                        : vdp.getName();
	MSXMotherBoard& motherBoard = vdp.getMotherBoard();
	return std::make_unique<V9990SDLRasterizer>(
		vdp, display,
		std::make_unique<PostProcessor>(
			motherBoard, display, *surface,
			videoSource, 1280, 240, true));
}

std::unique_ptr<SMSVDPRasterizer> LibretroVideoSystem::createSMSVDPRasterizer(
	SMSVDP& vdp)
{
	assert(display.getRenderer() == RenderSettings::RendererID::SDLGL_PP);
	MSXMotherBoard& motherBoard = vdp.getMotherBoard();
	return std::make_unique<SMSVDPSDLRasterizer>(
		vdp, display,
		std::make_unique<PostProcessor>(
			motherBoard, display, *surface,
			std::string(SMSVDP::VIDEO_SOURCE), 320, 240, false));
}

#if COMPONENT_LASERDISC
std::unique_ptr<LDRasterizer> LibretroVideoSystem::createLDRasterizer(
	LaserdiscPlayer& ld)
{
	assert(display.getRenderer() == RenderSettings::RendererID::SDLGL_PP);
	std::string videoSource = "Laserdisc"; // TODO handle multiple???
	MSXMotherBoard& motherBoard = ld.getMotherBoard();
	return std::make_unique<LDSDLRasterizer>(
		std::make_unique<PostProcessor>(
			motherBoard, display, *surface,
			videoSource, 640, 480, false));
}
#endif

void LibretroVideoSystem::flush()
{
	// The frontend presents the frame once retro_run hands it over
}

void LibretroVideoSystem::takeScreenShot(const std::string& filename, bool /*withOsd*/)
{
	surface->saveScreenshot(filename);
}

std::optional<gl::ivec2> LibretroVideoSystem::getMouseCoord()
{
	return std::nullopt;
}

std::optional<gl::vec2> LibretroVideoSystem::getMsxPixelSize()
{
	return surface->getViewScale() * gl::vec2(0.5f); // 640 wide logical, 320 MSX pixels
}

OutputSurface* LibretroVideoSystem::getOutputSurface()
{
	return surface.get();
}

void LibretroVideoSystem::showCursor(Cursor /*cursor*/)
{
	// The frontend owns the pointer
}

void LibretroVideoSystem::updateCursor()
{
}

bool LibretroVideoSystem::getCursorEnabled()
{
	return false;
}

std::string LibretroVideoSystem::getClipboardText()
{
	return {};
}

void LibretroVideoSystem::setClipboardText(zstring_view /*text*/)
{
}

std::optional<gl::ivec2> LibretroVideoSystem::getWindowPosition()
{
	return std::nullopt;
}

void LibretroVideoSystem::setWindowPosition(gl::ivec2 /*pos*/)
{
}

void LibretroVideoSystem::repaint()
{
	// A frame is finished; retro_run paints it into the frontend's
	// framebuffer and hands it over, so nothing is drawn here.
}

void LibretroVideoSystem::paintTo(unsigned fbo, gl::ivec2 size, gl::ivec2 offset)
{
	surface->resize(display.getWindowSize(), size);
	surface->moveViewPort(offset);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glViewport(0, 0, size.x, size.y);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	display.repaintImpl(*surface);
}

} // namespace openmsx
