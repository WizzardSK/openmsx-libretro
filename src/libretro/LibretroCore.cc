/*
 * openMSX libretro core: the libretro API.
 *
 * openMSX draws with OpenGL only, so the core asks the frontend for an OpenGL
 * (2.1 compatibility) context and paints each frame into the frontend's
 * framebuffer. Each retro_run emulates until the VDP finishes a frame.
 */
#include "LibretroHooks.hh"
#include "LibretroSoundDriver.hh"
#include "LibretroVideoSystem.hh"

#include "CommandLineParser.hh"
#include "Display.hh"
#include "Event.hh"
#include "EventDistributor.hh"
#include "GLContext.hh"
#include "GLUtil.hh"
#include "GlobalCommandController.hh"
#include "MSXException.hh"
#include "MSXMotherBoard.hh"
#include "Reactor.hh"
#include "RenderSettings.hh"
#include "Thread.hh"
#include "random.hh"

#include "libretro.h"

#include <SDL.h>

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string>
#include <vector>

using namespace openmsx;

namespace {

retro_environment_t environ_cb;
retro_video_refresh_t video_cb;
retro_audio_sample_batch_t audio_batch_cb;
retro_input_poll_t input_poll_cb;
retro_input_state_t input_state_cb;
retro_log_printf_t log_cb;
retro_hw_render_callback hw_render;

std::unique_ptr<Reactor> reactor;
std::string contentPath;
bool frameDone = false;
bool contextAlive = false;

constexpr double NTSC_FPS = 59.9227434;
constexpr unsigned BASE_WIDTH = 640, BASE_HEIGHT = 480;
constexpr unsigned MAX_WIDTH = 1920, MAX_HEIGHT = 1440;

void log(retro_log_level level, const char* fmt, ...)
{
	char buf[2048];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);
	if (log_cb) {
		log_cb(level, "[openMSX] %s", buf);
	} else {
		fprintf(stderr, "[openMSX] %s", buf);
	}
}

std::string directory(unsigned cmd)
{
	const char* dir = nullptr;
	if (environ_cb && environ_cb(cmd, &dir) && dir) return dir;
	return ".";
}

/** openMSX's data (machines, extensions, scripts, ROMs) is the 'share'
  * folder of an openMSX release, in <system>/openmsx/share; its user folder
  * (settings, SRAM, saved states) is <saves>/openmsx. */
void setupDirectories()
{
	std::string system = directory(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY) + "/openmsx";
	std::string saves = directory(RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY) + "/openmsx";
	setenv("OPENMSX_SYSTEM_DATA", (system + "/share").c_str(), 1);
	setenv("OPENMSX_HOME", saves.c_str(), 1);
}

void runCommand(const std::string& command)
{
	try {
		reactor->getGlobalCommandController().executeCommand(command);
	} catch (MSXException& e) {
		log(RETRO_LOG_WARN, "'%s': %s\n", command.c_str(), std::string(e.getMessage()).c_str());
	}
}

bool startEmulator()
{
	try {
		randomize();
		if (SDL_Init(SDL_INIT_EVENTS | SDL_INIT_TIMER) < 0) {
			log(RETRO_LOG_ERROR, "SDL_Init: %s\n", SDL_GetError());
			return false;
		}
		Thread::setMainThread();
		reactor = std::make_unique<Reactor>();

		std::vector<std::string> args = {"openmsx"};
		if (!contentPath.empty()) args.push_back(contentPath);
		std::vector<char*> argv;
		for (auto& a : args) argv.push_back(a.data());
		CommandLineParser parser(*reactor);
		parser.parse(argv);
		auto status = parser.getParseStatus();
		if (status == one_of(CommandLineParser::Status::EXIT, CommandLineParser::Status::TEST)) {
			log(RETRO_LOG_ERROR, "the content was not accepted\n");
			return false;
		}
		reactor->runStartupScripts(parser);

		// The frontend paces the frames: no throttling or frame skipping
		runCommand("set throttle off");
		runCommand("set minframeskip 0");
		runCommand("set maxframeskip 0");

		auto& render = reactor->getDisplay().getRenderSettings().getRendererSetting();
		if (render.getEnum() == RenderSettings::RendererID::UNINITIALIZED) {
			render.setValue(render.getDefaultValue());
			reactor->getEventDistributor().deliverEvents(0);
		}
		if (parser.getParseStatus() == CommandLineParser::Status::RUN) {
			reactor->powerOn();
		}
		return true;
	} catch (MSXException& e) {
		log(RETRO_LOG_ERROR, "%s\n", std::string(e.getMessage()).c_str());
	} catch (std::exception& e) {
		log(RETRO_LOG_ERROR, "%s\n", e.what());
	}
	reactor.reset();
	return false;
}

void contextReset()
{
	glewExperimental = GL_TRUE;
	// GLEW loads the GL functions first and then EGL's, which needs an EGL
	// display: with a GLX context from the frontend there is none, as with
	// openMSX itself on Wayland, and the GL functions are loaded all the same
	if (GLenum err = glewInit(); err != GLEW_OK && err != GLEW_ERROR_NO_GLX_DISPLAY) {
		log(RETRO_LOG_ERROR, "GLEW: %s\n", reinterpret_cast<const char*>(glewGetErrorString(err)));
		return;
	}
	contextAlive = true;
	if (!reactor) {
		gl::context.emplace();
		if (!startEmulator()) {
			environ_cb(RETRO_ENVIRONMENT_SHUTDOWN, nullptr);
		}
	} else {
		// TODO: a lost context needs every GL object made again
		log(RETRO_LOG_WARN, "the OpenGL context was reset; the picture may be lost\n");
	}
}

void contextDestroy()
{
	contextAlive = false;
}

// RetroArch's keyboard to SDL keys, as openMSX's keyboard emulation reads them
struct KeyMap { unsigned retro; SDL_Scancode scan; SDL_Keycode sym; };

constexpr KeyMap keyMap[] = {
	{RETROK_BACKSPACE, SDL_SCANCODE_BACKSPACE, SDLK_BACKSPACE},
	{RETROK_TAB, SDL_SCANCODE_TAB, SDLK_TAB},
	{RETROK_RETURN, SDL_SCANCODE_RETURN, SDLK_RETURN},
	{RETROK_PAUSE, SDL_SCANCODE_PAUSE, SDLK_PAUSE},
	{RETROK_ESCAPE, SDL_SCANCODE_ESCAPE, SDLK_ESCAPE},
	{RETROK_SPACE, SDL_SCANCODE_SPACE, SDLK_SPACE},
	{RETROK_QUOTE, SDL_SCANCODE_APOSTROPHE, SDLK_QUOTE},
	{RETROK_COMMA, SDL_SCANCODE_COMMA, SDLK_COMMA},
	{RETROK_MINUS, SDL_SCANCODE_MINUS, SDLK_MINUS},
	{RETROK_PERIOD, SDL_SCANCODE_PERIOD, SDLK_PERIOD},
	{RETROK_SLASH, SDL_SCANCODE_SLASH, SDLK_SLASH},
	{RETROK_0, SDL_SCANCODE_0, SDLK_0}, {RETROK_1, SDL_SCANCODE_1, SDLK_1},
	{RETROK_2, SDL_SCANCODE_2, SDLK_2}, {RETROK_3, SDL_SCANCODE_3, SDLK_3},
	{RETROK_4, SDL_SCANCODE_4, SDLK_4}, {RETROK_5, SDL_SCANCODE_5, SDLK_5},
	{RETROK_6, SDL_SCANCODE_6, SDLK_6}, {RETROK_7, SDL_SCANCODE_7, SDLK_7},
	{RETROK_8, SDL_SCANCODE_8, SDLK_8}, {RETROK_9, SDL_SCANCODE_9, SDLK_9},
	{RETROK_SEMICOLON, SDL_SCANCODE_SEMICOLON, SDLK_SEMICOLON},
	{RETROK_EQUALS, SDL_SCANCODE_EQUALS, SDLK_EQUALS},
	{RETROK_LEFTBRACKET, SDL_SCANCODE_LEFTBRACKET, SDLK_LEFTBRACKET},
	{RETROK_BACKSLASH, SDL_SCANCODE_BACKSLASH, SDLK_BACKSLASH},
	{RETROK_RIGHTBRACKET, SDL_SCANCODE_RIGHTBRACKET, SDLK_RIGHTBRACKET},
	{RETROK_BACKQUOTE, SDL_SCANCODE_GRAVE, SDLK_BACKQUOTE},
	{RETROK_a, SDL_SCANCODE_A, SDLK_a}, {RETROK_b, SDL_SCANCODE_B, SDLK_b},
	{RETROK_c, SDL_SCANCODE_C, SDLK_c}, {RETROK_d, SDL_SCANCODE_D, SDLK_d},
	{RETROK_e, SDL_SCANCODE_E, SDLK_e}, {RETROK_f, SDL_SCANCODE_F, SDLK_f},
	{RETROK_g, SDL_SCANCODE_G, SDLK_g}, {RETROK_h, SDL_SCANCODE_H, SDLK_h},
	{RETROK_i, SDL_SCANCODE_I, SDLK_i}, {RETROK_j, SDL_SCANCODE_J, SDLK_j},
	{RETROK_k, SDL_SCANCODE_K, SDLK_k}, {RETROK_l, SDL_SCANCODE_L, SDLK_l},
	{RETROK_m, SDL_SCANCODE_M, SDLK_m}, {RETROK_n, SDL_SCANCODE_N, SDLK_n},
	{RETROK_o, SDL_SCANCODE_O, SDLK_o}, {RETROK_p, SDL_SCANCODE_P, SDLK_p},
	{RETROK_q, SDL_SCANCODE_Q, SDLK_q}, {RETROK_r, SDL_SCANCODE_R, SDLK_r},
	{RETROK_s, SDL_SCANCODE_S, SDLK_s}, {RETROK_t, SDL_SCANCODE_T, SDLK_t},
	{RETROK_u, SDL_SCANCODE_U, SDLK_u}, {RETROK_v, SDL_SCANCODE_V, SDLK_v},
	{RETROK_w, SDL_SCANCODE_W, SDLK_w}, {RETROK_x, SDL_SCANCODE_X, SDLK_x},
	{RETROK_y, SDL_SCANCODE_Y, SDLK_y}, {RETROK_z, SDL_SCANCODE_Z, SDLK_z},
	{RETROK_DELETE, SDL_SCANCODE_DELETE, SDLK_DELETE},
	{RETROK_KP0, SDL_SCANCODE_KP_0, SDLK_KP_0}, {RETROK_KP1, SDL_SCANCODE_KP_1, SDLK_KP_1},
	{RETROK_KP2, SDL_SCANCODE_KP_2, SDLK_KP_2}, {RETROK_KP3, SDL_SCANCODE_KP_3, SDLK_KP_3},
	{RETROK_KP4, SDL_SCANCODE_KP_4, SDLK_KP_4}, {RETROK_KP5, SDL_SCANCODE_KP_5, SDLK_KP_5},
	{RETROK_KP6, SDL_SCANCODE_KP_6, SDLK_KP_6}, {RETROK_KP7, SDL_SCANCODE_KP_7, SDLK_KP_7},
	{RETROK_KP8, SDL_SCANCODE_KP_8, SDLK_KP_8}, {RETROK_KP9, SDL_SCANCODE_KP_9, SDLK_KP_9},
	{RETROK_KP_PERIOD, SDL_SCANCODE_KP_PERIOD, SDLK_KP_PERIOD},
	{RETROK_KP_DIVIDE, SDL_SCANCODE_KP_DIVIDE, SDLK_KP_DIVIDE},
	{RETROK_KP_MULTIPLY, SDL_SCANCODE_KP_MULTIPLY, SDLK_KP_MULTIPLY},
	{RETROK_KP_MINUS, SDL_SCANCODE_KP_MINUS, SDLK_KP_MINUS},
	{RETROK_KP_PLUS, SDL_SCANCODE_KP_PLUS, SDLK_KP_PLUS},
	{RETROK_KP_ENTER, SDL_SCANCODE_KP_ENTER, SDLK_KP_ENTER},
	{RETROK_UP, SDL_SCANCODE_UP, SDLK_UP}, {RETROK_DOWN, SDL_SCANCODE_DOWN, SDLK_DOWN},
	{RETROK_RIGHT, SDL_SCANCODE_RIGHT, SDLK_RIGHT}, {RETROK_LEFT, SDL_SCANCODE_LEFT, SDLK_LEFT},
	{RETROK_INSERT, SDL_SCANCODE_INSERT, SDLK_INSERT},
	{RETROK_HOME, SDL_SCANCODE_HOME, SDLK_HOME}, {RETROK_END, SDL_SCANCODE_END, SDLK_END},
	{RETROK_PAGEUP, SDL_SCANCODE_PAGEUP, SDLK_PAGEUP},
	{RETROK_PAGEDOWN, SDL_SCANCODE_PAGEDOWN, SDLK_PAGEDOWN},
	{RETROK_F1, SDL_SCANCODE_F1, SDLK_F1}, {RETROK_F2, SDL_SCANCODE_F2, SDLK_F2},
	{RETROK_F3, SDL_SCANCODE_F3, SDLK_F3}, {RETROK_F4, SDL_SCANCODE_F4, SDLK_F4},
	{RETROK_F5, SDL_SCANCODE_F5, SDLK_F5}, {RETROK_F6, SDL_SCANCODE_F6, SDLK_F6},
	{RETROK_F7, SDL_SCANCODE_F7, SDLK_F7}, {RETROK_F8, SDL_SCANCODE_F8, SDLK_F8},
	{RETROK_F9, SDL_SCANCODE_F9, SDLK_F9}, {RETROK_F10, SDL_SCANCODE_F10, SDLK_F10},
	{RETROK_F11, SDL_SCANCODE_F11, SDLK_F11}, {RETROK_F12, SDL_SCANCODE_F12, SDLK_F12},
	{RETROK_CAPSLOCK, SDL_SCANCODE_CAPSLOCK, SDLK_CAPSLOCK},
	{RETROK_RSHIFT, SDL_SCANCODE_RSHIFT, SDLK_RSHIFT},
	{RETROK_LSHIFT, SDL_SCANCODE_LSHIFT, SDLK_LSHIFT},
	{RETROK_RCTRL, SDL_SCANCODE_RCTRL, SDLK_RCTRL},
	{RETROK_LCTRL, SDL_SCANCODE_LCTRL, SDLK_LCTRL},
	{RETROK_RALT, SDL_SCANCODE_RALT, SDLK_RALT},
	{RETROK_LALT, SDL_SCANCODE_LALT, SDLK_LALT},
};

void RETRO_CALLCONV keyboardEvent(bool down, unsigned keycode, uint32_t /*character*/, uint16_t /*mods*/)
{
	if (!reactor) return;
	for (const auto& k : keyMap) {
		if (k.retro != keycode) continue;
		SDL_Event evt;
		evt.key = SDL_KeyboardEvent{};
		evt.key.type = down ? SDL_KEYDOWN : SDL_KEYUP;
		evt.key.timestamp = SDL_GetTicks();
		evt.key.state = down ? SDL_PRESSED : SDL_RELEASED;
		evt.key.keysym.scancode = k.scan;
		evt.key.keysym.sym = k.sym;
		auto& distributor = reactor->getEventDistributor();
		if (down) {
			distributor.distributeEvent(KeyDownEvent(evt));
		} else {
			distributor.distributeEvent(KeyUpEvent(evt));
		}
		return;
	}
}

} // namespace

namespace openmsx {

void libretroFrameEnd(MSXMotherBoard& motherBoard)
{
	frameDone = true;
	motherBoard.exitCPULoopSync();
}

} // namespace openmsx

RETRO_API void retro_set_environment(retro_environment_t cb)
{
	environ_cb = cb;
	retro_log_callback logging;
	if (cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &logging)) {
		log_cb = logging.log;
	}
}

RETRO_API void retro_set_video_refresh(retro_video_refresh_t cb) { video_cb = cb; }
RETRO_API void retro_set_audio_sample(retro_audio_sample_t /*cb*/) {}
RETRO_API void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb) { audio_batch_cb = cb; }
RETRO_API void retro_set_input_poll(retro_input_poll_t cb) { input_poll_cb = cb; }
RETRO_API void retro_set_input_state(retro_input_state_t cb) { input_state_cb = cb; }

RETRO_API unsigned retro_api_version() { return RETRO_API_VERSION; }

RETRO_API void retro_init() {}

RETRO_API void retro_deinit() {}

RETRO_API void retro_get_system_info(retro_system_info* info)
{
	*info = {};
	info->library_name = "openMSX";
	info->library_version = "21.0-git";
	info->valid_extensions = "rom|ri|mx1|mx2|col|sg|sc|dsk|di1|di2|dmk|xsa|cas|wav|zip|gz";
	info->need_fullpath = true;
	info->block_extract = true;
}

RETRO_API void retro_get_system_av_info(retro_system_av_info* info)
{
	*info = {};
	info->geometry.base_width = BASE_WIDTH;
	info->geometry.base_height = BASE_HEIGHT;
	info->geometry.max_width = MAX_WIDTH;
	info->geometry.max_height = MAX_HEIGHT;
	info->geometry.aspect_ratio = 4.0f / 3.0f;
	info->timing.fps = NTSC_FPS;
	info->timing.sample_rate = LibretroSoundDriver::FREQUENCY;
}

RETRO_API void retro_set_controller_port_device(unsigned /*port*/, unsigned /*device*/) {}

RETRO_API void retro_reset()
{
	if (reactor) runCommand("reset");
}

RETRO_API bool retro_load_game(const retro_game_info* game)
{
	contentPath = (game && game->path) ? game->path : "";

	retro_pixel_format fmt = RETRO_PIXEL_FORMAT_XRGB8888;
	environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt);

	hw_render = {};
	hw_render.context_type = RETRO_HW_CONTEXT_OPENGL;
	hw_render.version_major = 2;
	hw_render.version_minor = 1;
	hw_render.context_reset = contextReset;
	hw_render.context_destroy = contextDestroy;
	hw_render.bottom_left_origin = true;
	if (!environ_cb(RETRO_ENVIRONMENT_SET_HW_RENDER, &hw_render)) {
		log(RETRO_LOG_ERROR, "the frontend has no OpenGL context for the core\n");
		return false;
	}

	retro_keyboard_callback keyboard = {keyboardEvent};
	environ_cb(RETRO_ENVIRONMENT_SET_KEYBOARD_CALLBACK, &keyboard);

	setupDirectories();
	// openMSX starts in context_reset, once there is a GL context
	return true;
}

RETRO_API bool retro_load_game_special(unsigned /*type*/, const retro_game_info* /*info*/, size_t /*num*/)
{
	return false;
}

RETRO_API void retro_unload_game()
{
	reactor.reset();
	gl::context.reset();
	SDL_Quit();
}

RETRO_API void retro_run()
{
	input_poll_cb();
	if (!reactor || !contextAlive) {
		video_cb(nullptr, BASE_WIDTH, BASE_HEIGHT, 0);
		return;
	}

	try {
		// Emulate until the VDP finishes a frame. The bound keeps a
		// machine whose VDP produces no frames from hanging the frontend.
		frameDone = false;
		auto& distributor = reactor->getEventDistributor();
		for (int i = 0; !frameDone && i < 64; ++i) {
			distributor.deliverEvents(0);
			auto* board = reactor->getMotherBoard();
			if (!board || !board->execute()) break;
		}
		distributor.deliverEvents(0);
	} catch (MSXException& e) {
		log(RETRO_LOG_ERROR, "%s\n", std::string(e.getMessage()).c_str());
	}

	auto* video = dynamic_cast<LibretroVideoSystem*>(&reactor->getDisplay().getVideoSystem());
	if (video) {
		video->paintTo(unsigned(hw_render.get_current_framebuffer()),
		               gl::ivec2(int(BASE_WIDTH), int(BASE_HEIGHT)));
		video_cb(RETRO_HW_FRAME_BUFFER_VALID, BASE_WIDTH, BASE_HEIGHT, 0);
	} else {
		video_cb(nullptr, BASE_WIDTH, BASE_HEIGHT, 0);
	}

	if (auto* sound = LibretroSoundDriver::current()) {
		auto& samples = sound->pending();
		size_t frames = samples.size() / 2;
		const int16_t* data = samples.data();
		while (frames) {
			size_t done = audio_batch_cb(data, frames);
			if (!done) break;
			frames -= done;
			data += 2 * done;
		}
		samples.clear();
	}
}

RETRO_API size_t retro_serialize_size() { return 0; }
RETRO_API bool retro_serialize(void* /*data*/, size_t /*size*/) { return false; }
RETRO_API bool retro_unserialize(const void* /*data*/, size_t /*size*/) { return false; }
RETRO_API void retro_cheat_reset() {}
RETRO_API void retro_cheat_set(unsigned /*index*/, bool /*enabled*/, const char* /*code*/) {}
RETRO_API unsigned retro_get_region() { return RETRO_REGION_NTSC; }
RETRO_API void* retro_get_memory_data(unsigned /*id*/) { return nullptr; }
RETRO_API size_t retro_get_memory_size(unsigned /*id*/) { return 0; }
