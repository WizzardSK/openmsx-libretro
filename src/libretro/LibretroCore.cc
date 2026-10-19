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
#include "DeltaBlock.hh"
#include "Display.hh"
#include "Event.hh"
#include "EventDistributor.hh"
#include "GLContext.hh"
#include "GLUtil.hh"
#include "GlobalCommandController.hh"
#include "JoystickDevice.hh"
#include "MSXCliComm.hh"
#include "MSXException.hh"
#include "MSXMotherBoard.hh"
#include "Reactor.hh"
#include "RenderSettings.hh"
#include "StateChange.hh"
#include "StateChangeDistributor.hh"
#include "Thread.hh"
#include "VDP.hh"
#include "Version.hh"
#include "random.hh"
#include "serialize.hh"
#include "strCat.hh"

#include "libretro.h"

#include <SDL.h>

#include <array>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstring>
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
// The fps declared to the frontend; follows the VDP between 60 and 50 Hz
double declaredFps = NTSC_FPS;
int frameTicks = 0;
// The savestate size given to the frontend, the largest seen yet
size_t stateSizeMax = 0;
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

constexpr const char* DEFAULT_MACHINE = "C-BIOS_MSX2+";

struct ExtensionOption { const char* key; const char* name; };
constexpr ExtensionOption extensionOptions[] = {
	{"openmsx_ext_scc", "scc"},
	{"openmsx_ext_sccplus", "scc+"},
	{"openmsx_ext_fmpac", "fmpac"},
	{"openmsx_ext_msxaudio", "audio"},
	{"openmsx_ext_moonsound", "moonsound"},
	{"openmsx_ext_gfx9000", "gfx9000"},
};

std::string optionValue(const char* key, const char* fallback)
{
	retro_variable var = {key, nullptr};
	if (environ_cb && environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) return var.value;
	return fallback;
}

void showMessage(const char* text)
{
	log(RETRO_LOG_WARN, "%s\n", text);
	retro_message msg = {text, 600};
	environ_cb(RETRO_ENVIRONMENT_SET_MESSAGE, &msg);
}

retro_core_option_v2_category optionCategories[] = {
	{"system", "System", "The MSX machine and the cartridges in its slots. Changes take effect when the content is loaded again."},
	{nullptr, nullptr, nullptr},
};

#define EXTENSION_OPTION(key, label, info) \
	{key, label, nullptr, info, nullptr, "system", \
	 {{"disabled", nullptr}, {"enabled", nullptr}, {nullptr, nullptr}}, "disabled"}

retro_core_option_v2_definition optionDefinitions[] = {
	{
		"openmsx_machine", "Machine (Restart)", "Machine",
		"The MSX computer to emulate. The C-BIOS machines need nothing else; a real machine needs its system ROMs in <system>/openmsx/share/systemroms, otherwise C-BIOS MSX2+ starts instead. Europe and the UK run at 50 Hz.",
		nullptr, "system",
		{
			{"C-BIOS_MSX1", "C-BIOS MSX1"},
			{"C-BIOS_MSX1_EU", "C-BIOS MSX1 (Europe)"},
			{"C-BIOS_MSX1_JP", "C-BIOS MSX1 (Japan)"},
			{"C-BIOS_MSX2", "C-BIOS MSX2"},
			{"C-BIOS_MSX2_EU", "C-BIOS MSX2 (Europe)"},
			{"C-BIOS_MSX2_JP", "C-BIOS MSX2 (Japan)"},
			{"C-BIOS_MSX2+", "C-BIOS MSX2+"},
			{"C-BIOS_MSX2+_EU", "C-BIOS MSX2+ (Europe)"},
			{"C-BIOS_MSX2+_JP", "C-BIOS MSX2+ (Japan)"},
			{"Philips_VG_8020", "Philips VG-8020 (MSX1, Europe)"},
			{"Sony_HB-75P", "Sony HB-75P (MSX1, Europe)"},
			{"Toshiba_HX-10", "Toshiba HX-10 (MSX1, UK)"},
			{"Yamaha_CX5M", "Yamaha CX5M (MSX1, Europe)"},
			{"Philips_NMS_8245", "Philips NMS 8245 (MSX2, Europe)"},
			{"Philips_NMS_8250", "Philips NMS 8250 (MSX2, Europe)"},
			{"Sony_HB-F700P", "Sony HB-F700P (MSX2, Europe)"},
			{"Sony_HB-F1XD", "Sony HB-F1XD (MSX2, Japan)"},
			{"Sony_HB-F1XV", "Sony HB-F1XV (MSX2+, Japan)"},
			{"Panasonic_FS-A1WSX", "Panasonic FS-A1WSX (MSX2+, Japan)"},
			{"Panasonic_FS-A1ST", "Panasonic FS-A1ST (turbo R, Japan)"},
			{"Panasonic_FS-A1GT", "Panasonic FS-A1GT (turbo R, Japan)"},
			{nullptr, nullptr},
		},
		DEFAULT_MACHINE
	},
	EXTENSION_OPTION("openmsx_ext_scc", "Konami SCC (Restart)",
		"A cartridge with Konami's SCC sound chip, for music that would otherwise need an SCC game cartridge."),
	EXTENSION_OPTION("openmsx_ext_sccplus", "Konami SCC+ (Restart)",
		"Konami's Sound Cartridge (SCC+), as with Snatcher and SD Snatcher."),
	EXTENSION_OPTION("openmsx_ext_fmpac", "Panasonic FM-PAC (Restart)",
		"MSX-MUSIC (FM sound) with battery-backed SRAM. Needs its ROM in <system>/openmsx/share/systemroms."),
	EXTENSION_OPTION("openmsx_ext_msxaudio", "Philips MSX-Audio (Restart)",
		"The Philips NMS 1205 Music Module (MSX-AUDIO). Needs its ROM in <system>/openmsx/share/systemroms."),
	EXTENSION_OPTION("openmsx_ext_moonsound", "MoonSound (Restart)",
		"The OPL4 sound cartridge. Needs its sample ROM in <system>/openmsx/share/systemroms."),
	EXTENSION_OPTION("openmsx_ext_gfx9000", "GFX9000 (Restart)",
		"The V9990 video cartridge, for the software written for it."),
	{nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, {{nullptr, nullptr}}, nullptr},
};
#undef EXTENSION_OPTION

void setCoreOptions()
{
	retro_core_options_v2 options = {optionCategories, optionDefinitions};
	unsigned version = 0;
	if (environ_cb(RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION, &version) && version >= 2) {
		environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2, &options);
		return;
	}
	// a frontend without options v2: "Label; first|second|..." with the default first
	static std::vector<std::string> strings;
	static std::vector<retro_variable> variables;
	strings.clear();
	variables.clear();
	for (const auto* def = optionDefinitions; def->key; ++def) {
		std::string text = std::string(def->desc) + "; " + def->default_value;
		for (const auto* v = def->values; v->value; ++v) {
			if (std::string(v->value) != def->default_value) text += std::string("|") + v->value;
		}
		strings.push_back(std::move(text));
	}
	size_t i = 0;
	for (const auto* def = optionDefinitions; def->key; ++def) {
		variables.push_back({def->key, strings[i++].c_str()});
	}
	variables.push_back({nullptr, nullptr});
	environ_cb(RETRO_ENVIRONMENT_SET_VARIABLES, variables.data());
}

bool startEmulator(bool useOptions)
{
	try {
		log(RETRO_LOG_INFO, "core built from %s\n", OPENMSX_LIBRETRO_COMMIT);
		randomize();
		if (SDL_Init(SDL_INIT_EVENTS | SDL_INIT_TIMER) < 0) {
			log(RETRO_LOG_ERROR, "SDL_Init: %s\n", SDL_GetError());
			return false;
		}
		Thread::setMainThread();
		reactor = std::make_unique<Reactor>();

		std::vector<std::string> args = {"openmsx"};
		if (useOptions) {
			args.insert(args.end(), {"-machine", optionValue("openmsx_machine", DEFAULT_MACHINE)});
			for (const auto& ext : extensionOptions) {
				if (optionValue(ext.key, "disabled") == "enabled") {
					args.insert(args.end(), {"-ext", ext.name});
				}
			}
		}
		if (!contentPath.empty()) args.push_back(contentPath);
		std::vector<char*> argv;
		for (auto& a : args) argv.push_back(a.data());
		CommandLineParser parser(*reactor);
		parser.parse(argv);
		auto status = parser.getParseStatus();
		if (status == one_of(CommandLineParser::Status::EXIT, CommandLineParser::Status::TEST)) {
			log(RETRO_LOG_ERROR, "the content was not accepted\n");
			reactor.reset();
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
		if (!reactor->getMotherBoard()) {
			log(RETRO_LOG_ERROR, "no MSX machine was started\n");
			reactor.reset();
			return false;
		}
		if (parser.getParseStatus() == CommandLineParser::Status::RUN) {
			reactor->powerOn();
		}
		return true;
	} catch (MSXException& e) {
		log(RETRO_LOG_ERROR, "%s\n", std::string(e.getMessage()).c_str());
	} catch (FatalError& e) {
		// the parser's way of saying a machine or extension did not load
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
	// openMSX itself on Wayland, and the GL functions are loaded all the same.
	// So what counts is whether OpenGL 2.1 is there afterwards.
	if (GLenum err = glewInit(); err != GLEW_OK && !GLEW_VERSION_2_1) {
		log(RETRO_LOG_ERROR, "GLEW: %s\n", reinterpret_cast<const char*>(glewGetErrorString(err)));
		return;
	}
	contextAlive = true;
	if (!reactor) {
		gl::context.emplace();
		bool started = startEmulator(true);
		if (!started) {
			// most often a real machine whose system ROMs are not there
			showMessage("openMSX: the chosen machine or extension could not start (see the log); starting C-BIOS without extensions");
			SDL_Quit();
			started = startEmulator(false);
		}
		if (!started) {
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

void sendKey(bool down, SDL_Scancode scan, SDL_Keycode sym)
{
	SDL_Event evt;
	evt.key = SDL_KeyboardEvent{};
	evt.key.type = down ? SDL_KEYDOWN : SDL_KEYUP;
	evt.key.timestamp = SDL_GetTicks();
	evt.key.state = down ? SDL_PRESSED : SDL_RELEASED;
	evt.key.keysym.scancode = scan;
	evt.key.keysym.sym = sym;
	auto& distributor = reactor->getEventDistributor();
	if (down) {
		distributor.distributeEvent(KeyDownEvent(evt));
	} else {
		distributor.distributeEvent(KeyUpEvent(evt));
	}
}

void RETRO_CALLCONV keyboardEvent(bool down, unsigned keycode, uint32_t /*character*/, uint16_t /*mods*/)
{
	if (!reactor) return;
	for (const auto& k : keyMap) {
		if (k.retro != keycode) continue;
		sendKey(down, k.scan, k.sym);
		return;
	}
}

// What is plugged into the MSX's two joystick ports
constexpr unsigned DEVICE_MSX_JOYSTICK = RETRO_DEVICE_JOYPAD;
constexpr unsigned DEVICE_CURSOR_KEYS = RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_JOYPAD, 0);
constexpr unsigned DEVICE_MSX_MOUSE = RETRO_DEVICE_MOUSE;
constexpr unsigned NUM_PORTS = 2;

std::array<unsigned, NUM_PORTS> portDevice = {DEVICE_MSX_JOYSTICK, DEVICE_MSX_JOYSTICK};
bool portsChanged = true;
MSXMotherBoard* pluggedBoard = nullptr;

struct PortState {
	uint8_t joy = 0;      // MSX joystick bits held (JOY_UP ... JOY_BUTTONB)
	uint32_t keys = 0;    // keys held: padKeys, then cursorKeys
	unsigned mouseButtons = 0;
};
std::array<PortState, NUM_PORTS> portState;

// RetroPad buttons that press MSX keys, with an MSX joystick and with the
// cursor keys alike. Konami's games pause on F1 and ask for Space to start.
struct PadKey { unsigned id; SDL_Scancode scan; SDL_Keycode sym; };
constexpr PadKey padKeys[] = {
	{RETRO_DEVICE_ID_JOYPAD_X, SDL_SCANCODE_SPACE, SDLK_SPACE},
	{RETRO_DEVICE_ID_JOYPAD_Y, SDL_SCANCODE_RETURN, SDLK_RETURN},
	{RETRO_DEVICE_ID_JOYPAD_START, SDL_SCANCODE_F1, SDLK_F1},
	{RETRO_DEVICE_ID_JOYPAD_SELECT, SDL_SCANCODE_F5, SDLK_F5},
	{RETRO_DEVICE_ID_JOYPAD_L, SDL_SCANCODE_ESCAPE, SDLK_ESCAPE},
	{RETRO_DEVICE_ID_JOYPAD_R, SDL_SCANCODE_LSHIFT, SDLK_LSHIFT},
};
// With the cursor keys, the D-pad and A/B as the keyboard games read them:
// trigger A is Space, trigger B is M
constexpr PadKey cursorKeys[] = {
	{RETRO_DEVICE_ID_JOYPAD_UP, SDL_SCANCODE_UP, SDLK_UP},
	{RETRO_DEVICE_ID_JOYPAD_DOWN, SDL_SCANCODE_DOWN, SDLK_DOWN},
	{RETRO_DEVICE_ID_JOYPAD_LEFT, SDL_SCANCODE_LEFT, SDLK_LEFT},
	{RETRO_DEVICE_ID_JOYPAD_RIGHT, SDL_SCANCODE_RIGHT, SDLK_RIGHT},
	{RETRO_DEVICE_ID_JOYPAD_A, SDL_SCANCODE_SPACE, SDLK_SPACE},
	{RETRO_DEVICE_ID_JOYPAD_B, SDL_SCANCODE_M, SDLK_m},
};

/** Plugs into the MSX's joystick ports what the frontend's ports hold. */
void plugPorts(MSXMotherBoard& board)
{
	static constexpr const char* connectors[NUM_PORTS] = {"joyporta", "joyportb"};
	static constexpr const char* joysticks[NUM_PORTS] = {"msxjoystick1", "msxjoystick2"};
	// An MSX joystick keeps its state while unplugged: let go of what it
	// held, unless the machine is new
	if (&board == pluggedBoard) {
		for (unsigned port = 0; port < NUM_PORTS; ++port) {
			if (uint8_t held = portState[port].joy) {
				board.getStateChangeDistributor().distributeNew<MSXJoyState>(
					board.getCurrentTime(), uint8_t(port + 1), uint8_t(0), held);
			}
		}
	}
	// unplug everything first, so the mouse can move from one port to the other
	for (auto* connector : connectors) runCommand(std::string("unplug ") + connector);
	for (unsigned port = 0; port < NUM_PORTS; ++port) {
		portState[port] = {};
		switch (portDevice[port]) {
		case DEVICE_MSX_JOYSTICK:
			runCommand(std::string("plug ") + connectors[port] + ' ' + joysticks[port]);
			break;
		case DEVICE_MSX_MOUSE:
			runCommand(std::string("plug ") + connectors[port] + " mouse");
			break;
		default: // the cursor keys or nothing: the port stays empty
			break;
		}
	}
	pluggedBoard = &board;
	portsChanged = false;
}

bool padButton(unsigned port, unsigned id)
{
	return input_state_cb(port, RETRO_DEVICE_JOYPAD, 0, id) != 0;
}

/** The D-pad, or the left stick pushed beyond a third of its range. */
bool padDirection(unsigned port, unsigned id)
{
	if (padButton(port, id)) return true;
	constexpr int THRESHOLD = 0x2AAA;
	bool vertical = id == one_of(RETRO_DEVICE_ID_JOYPAD_UP, RETRO_DEVICE_ID_JOYPAD_DOWN);
	int value = input_state_cb(port, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT,
		vertical ? RETRO_DEVICE_ID_ANALOG_Y : RETRO_DEVICE_ID_ANALOG_X);
	bool negative = id == one_of(RETRO_DEVICE_ID_JOYPAD_UP, RETRO_DEVICE_ID_JOYPAD_LEFT);
	return negative ? value < -THRESHOLD : value > THRESHOLD;
}

/** Presses and releases the keys in 'map' that changed since the last frame. */
template<size_t N>
void updateKeys(uint32_t now, uint32_t& held, const PadKey (&map)[N], uint32_t shift)
{
	for (size_t i = 0; i < N; ++i) {
		uint32_t bit = 1u << (shift + i);
		if ((now & bit) != (held & bit)) {
			sendKey((now & bit) != 0, map[i].scan, map[i].sym);
		}
	}
	held = (held & ~(((1u << N) - 1) << shift)) | (now & (((1u << N) - 1) << shift));
}

void pollJoystick(MSXMotherBoard& board, unsigned port, PortState& state)
{
	static constexpr unsigned dirs[4] = {
		RETRO_DEVICE_ID_JOYPAD_UP, RETRO_DEVICE_ID_JOYPAD_DOWN,
		RETRO_DEVICE_ID_JOYPAD_LEFT, RETRO_DEVICE_ID_JOYPAD_RIGHT,
	};
	uint8_t now = 0;
	for (unsigned i = 0; i < 4; ++i) {
		if (padDirection(port, dirs[i])) now |= uint8_t(1 << i);
	}
	if (padButton(port, RETRO_DEVICE_ID_JOYPAD_A)) now |= JoystickDevice::JOY_BUTTONA;
	if (padButton(port, RETRO_DEVICE_ID_JOYPAD_B)) now |= JoystickDevice::JOY_BUTTONB;
	if (now != state.joy) {
		// MSXJoystick takes the same state change from its own bindings
		board.getStateChangeDistributor().distributeNew<MSXJoyState>(
			board.getCurrentTime(), uint8_t(port + 1),
			uint8_t(now & ~state.joy), uint8_t(state.joy & ~now));
		state.joy = now;
	}
}

void pollCursorKeys(unsigned port, PortState& state)
{
	uint32_t now = 0;
	for (size_t i = 0; i < std::size(cursorKeys); ++i) {
		bool down = i < 4 ? padDirection(port, cursorKeys[i].id)
		                  : padButton(port, cursorKeys[i].id);
		if (down) now |= 1u << (std::size(padKeys) + i);
	}
	updateKeys(now, state.keys, cursorKeys, std::size(padKeys));
}

void pollPadKeys(unsigned port, PortState& state)
{
	uint32_t now = 0;
	for (size_t i = 0; i < std::size(padKeys); ++i) {
		if (padButton(port, padKeys[i].id)) now |= 1u << i;
	}
	updateKeys(now, state.keys, padKeys, 0);
}

void pollMouse(unsigned port, PortState& state)
{
	auto& distributor = reactor->getEventDistributor();
	int dx = input_state_cb(port, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_X);
	int dy = input_state_cb(port, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_Y);
	if (dx || dy) {
		SDL_Event evt;
		evt.motion = SDL_MouseMotionEvent{};
		evt.motion.type = SDL_MOUSEMOTION;
		evt.motion.timestamp = SDL_GetTicks();
		evt.motion.xrel = dx;
		evt.motion.yrel = dy;
		distributor.distributeEvent(MouseMotionEvent(evt));
	}
	static constexpr struct { unsigned id; uint8_t sdl; } buttons[] = {
		{RETRO_DEVICE_ID_MOUSE_LEFT, SDL_BUTTON_LEFT},
		{RETRO_DEVICE_ID_MOUSE_RIGHT, SDL_BUTTON_RIGHT},
	};
	for (unsigned i = 0; i < std::size(buttons); ++i) {
		bool down = input_state_cb(port, RETRO_DEVICE_MOUSE, 0, buttons[i].id) != 0;
		if (down == bool(state.mouseButtons & (1u << i))) continue;
		state.mouseButtons ^= 1u << i;
		SDL_Event evt;
		evt.button = SDL_MouseButtonEvent{};
		evt.button.type = down ? SDL_MOUSEBUTTONDOWN : SDL_MOUSEBUTTONUP;
		evt.button.timestamp = SDL_GetTicks();
		evt.button.button = buttons[i].sdl;
		evt.button.state = down ? SDL_PRESSED : SDL_RELEASED;
		if (down) {
			distributor.distributeEvent(MouseButtonDownEvent(evt));
		} else {
			distributor.distributeEvent(MouseButtonUpEvent(evt));
		}
	}
}

void pollInput()
{
	auto* board = reactor->getMotherBoard();
	if (!board) return;
	if (portsChanged || board != pluggedBoard) plugPorts(*board);
	for (unsigned port = 0; port < NUM_PORTS; ++port) {
		auto& state = portState[port];
		switch (portDevice[port]) {
		case DEVICE_MSX_JOYSTICK:
			pollJoystick(*board, port, state);
			pollPadKeys(port, state);
			break;
		case DEVICE_CURSOR_KEYS:
			pollCursorKeys(port, state);
			pollPadKeys(port, state);
			break;
		case DEVICE_MSX_MOUSE:
			pollMouse(port, state);
			break;
		default:
			break;
		}
	}
}

void setControllerInfo()
{
	static const retro_controller_description types[] = {
		{"MSX Joystick", DEVICE_MSX_JOYSTICK},
		{"Cursor Keys", DEVICE_CURSOR_KEYS},
		{"MSX Mouse", DEVICE_MSX_MOUSE},
		{"None", RETRO_DEVICE_NONE},
	};
	static const retro_controller_info ports[] = {
		{types, std::size(types)},
		{types, std::size(types)},
		{nullptr, 0},
	};
	environ_cb(RETRO_ENVIRONMENT_SET_CONTROLLER_INFO, const_cast<retro_controller_info*>(ports));
}

void setInputDescriptors()
{
	static const retro_input_descriptor descriptors[] = {
#define PORT_DESCRIPTORS(p) \
		{p, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP, "Up"}, \
		{p, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN, "Down"}, \
		{p, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT, "Left"}, \
		{p, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "Right"}, \
		{p, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A, "Trigger A"}, \
		{p, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B, "Trigger B"}, \
		{p, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X, "Space"}, \
		{p, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y, "Return"}, \
		{p, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START, "F1"}, \
		{p, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT, "F5"}, \
		{p, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L, "Esc"}, \
		{p, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R, "Shift"}, \
		{p, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X, "Left Stick X"}, \
		{p, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y, "Left Stick Y"},
		PORT_DESCRIPTORS(0)
		PORT_DESCRIPTORS(1)
#undef PORT_DESCRIPTORS
		{0, 0, 0, 0, nullptr},
	};
	environ_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, const_cast<retro_input_descriptor*>(descriptors));
}

} // namespace

namespace openmsx {

void libretroFrameEnd(MSXMotherBoard& motherBoard, int ticksPerFrame)
{
	frameDone = true;
	frameTicks = ticksPerFrame;
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
	setControllerInfo();
	setCoreOptions();
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
	info->library_version = "21.0 " OPENMSX_LIBRETRO_COMMIT;
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
	info->timing.fps = declaredFps;
	info->timing.sample_rate = LibretroSoundDriver::FREQUENCY;
}

RETRO_API void retro_set_controller_port_device(unsigned port, unsigned device)
{
	if (port >= NUM_PORTS) return;
	// releases what the previous device held, before its port changes
	if (reactor) {
		auto& state = portState[port];
		if (portDevice[port] == DEVICE_CURSOR_KEYS) updateKeys(0, state.keys, cursorKeys, std::size(padKeys));
		updateKeys(0, state.keys, padKeys, 0);
	}
	portDevice[port] = device;
	portsChanged = true;
}

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
	// A video reinit (refresh rate switching on a 50/60 Hz change) keeps
	// the context, and with it every GL object openMSX made
	hw_render.cache_context = true;
	if (!environ_cb(RETRO_ENVIRONMENT_SET_HW_RENDER, &hw_render)) {
		log(RETRO_LOG_ERROR, "the frontend has no OpenGL context for the core\n");
		return false;
	}

	retro_keyboard_callback keyboard = {keyboardEvent};
	environ_cb(RETRO_ENVIRONMENT_SET_KEYBOARD_CALLBACK, &keyboard);
	setInputDescriptors();

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
	pluggedBoard = nullptr;
	portsChanged = true;
	portState = {};
	declaredFps = NTSC_FPS;
	frameTicks = 0;
	stateSizeMax = 0;
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
		pollInput();
		frameDone = false;
		auto& distributor = reactor->getEventDistributor();
		for (int i = 0; !frameDone && i < 64; ++i) {
			distributor.deliverEvents(0);
			auto* board = reactor->getMotherBoard();
			if (!board || !board->execute()) break;
		}
		distributor.deliverEvents(0);
		if (frameTicks > 0) {
			double fps = double(VDP::TICKS_PER_SECOND) / frameTicks;
			if (std::abs(fps - declaredFps) > 0.5) {
				declaredFps = fps;
				retro_system_av_info av;
				retro_get_system_av_info(&av);
				environ_cb(RETRO_ENVIRONMENT_SET_SYSTEM_AV_INFO, &av);
			}
		}
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

// A savestate: magic, the openMSX version and core commit it was made with,
// the length of the state and the state, a MemOutputArchive of the board with
// every blob inline. That archive has no versioning, so a state loads only
// into the build that made it.
namespace {

constexpr char STATE_MAGIC[8] = {'O', 'M', 'S', 'X', 'L', 'R', '0', '1'};

std::string stateVersion()
{
	// openMSX's own version names no revision in a shallow checkout
	return strCat(Version::full(), ' ', OPENMSX_LIBRETRO_COMMIT);
}

MemBuffer<uint8_t> saveBoard(MSXMotherBoard& board)
{
	LastDeltaBlocks lastDeltaBlocks;
	std::vector<std::shared_ptr<DeltaBlock>> deltaBlocks;
	MemOutputArchive out(lastDeltaBlocks, deltaBlocks, false, true);
	out.serialize("machine", board);
	return std::move(out).releaseBuffer();
}

size_t headerSize()
{
	return sizeof(STATE_MAGIC) + sizeof(uint32_t) + stateVersion().size() + sizeof(uint64_t);
}

} // namespace

RETRO_API size_t retro_serialize_size()
{
	auto* board = reactor ? reactor->getMotherBoard() : nullptr;
	if (!board) return stateSizeMax;
	try {
		// The state grows and shrinks a little with the machine's
		// state; the frontend wants one size, so the largest yet, with
		// room to spare, in 64 kB steps
		size_t need = headerSize() + saveBoard(*board).size() + 0x10000;
		need = (need + 0xFFFF) & ~size_t(0xFFFF);
		if (need > stateSizeMax) stateSizeMax = need;
	} catch (MSXException& e) {
		log(RETRO_LOG_ERROR, "savestate: %s\n", std::string(e.getMessage()).c_str());
	}
	return stateSizeMax;
}

RETRO_API bool retro_serialize(void* data, size_t size)
{
	auto* board = reactor ? reactor->getMotherBoard() : nullptr;
	if (!board) return false;
	try {
		auto state = saveBoard(*board);
		auto version = stateVersion();
		if (headerSize() + state.size() > size) {
			log(RETRO_LOG_ERROR, "savestate: %zu bytes do not fit in %zu\n", headerSize() + state.size(), size);
			return false;
		}
		auto* out = static_cast<uint8_t*>(data);
		memcpy(out, STATE_MAGIC, sizeof(STATE_MAGIC)); out += sizeof(STATE_MAGIC);
		uint32_t versionSize = uint32_t(version.size());
		memcpy(out, &versionSize, sizeof(versionSize)); out += sizeof(versionSize);
		memcpy(out, version.data(), version.size()); out += version.size();
		uint64_t stateSize = state.size();
		memcpy(out, &stateSize, sizeof(stateSize)); out += sizeof(stateSize);
		memcpy(out, state.data(), state.size()); out += state.size();
		memset(out, 0, size - size_t(out - static_cast<uint8_t*>(data)));
		return true;
	} catch (MSXException& e) {
		log(RETRO_LOG_ERROR, "savestate: %s\n", std::string(e.getMessage()).c_str());
	}
	return false;
}

RETRO_API bool retro_unserialize(const void* data, size_t size)
{
	auto* board = reactor ? reactor->getMotherBoard() : nullptr;
	if (!board) return false;
	const auto* in = static_cast<const uint8_t*>(data);
	const auto* end = in + size;
	auto version = stateVersion();
	uint32_t versionSize = 0;
	uint64_t stateSize = 0;
	if (size < sizeof(STATE_MAGIC) + sizeof(versionSize)
	    || memcmp(in, STATE_MAGIC, sizeof(STATE_MAGIC)) != 0) {
		log(RETRO_LOG_ERROR, "savestate: not an openMSX core savestate\n");
		return false;
	}
	in += sizeof(STATE_MAGIC);
	memcpy(&versionSize, in, sizeof(versionSize)); in += sizeof(versionSize);
	if (size_t(end - in) < versionSize + sizeof(stateSize)
	    || std::string_view(reinterpret_cast<const char*>(in), versionSize) != version) {
		log(RETRO_LOG_ERROR, "savestate: made by another build of the core, %s needed\n", version.c_str());
		return false;
	}
	in += versionSize;
	memcpy(&stateSize, in, sizeof(stateSize)); in += sizeof(stateSize);
	if (stateSize > size_t(end - in)) {
		log(RETRO_LOG_ERROR, "savestate: cut short\n");
		return false;
	}
	try {
		// as openMSX's reverse does: a new board from the state, which
		// then replaces the running one
		auto newBoard = reactor->createEmptyMotherBoard();
		newBoard->getMSXCliComm().setSuppressMessages(true);
		MemInputArchive archive(std::span{in, size_t(stateSize)}, {}, true);
		archive.serialize("machine", *newBoard);
		newBoard->getMSXCliComm().setSuppressMessages(false);
		// the state holds the input of its moment; the MSX should see
		// what is held now, which pollInput sends again
		newBoard->getStateChangeDistributor().stopReplay(newBoard->getCurrentTime());
		reactor->replaceBoard(*board, newBoard);
		return true;
	} catch (MSXException& e) {
		log(RETRO_LOG_ERROR, "savestate: %s\n", std::string(e.getMessage()).c_str());
	}
	return false;
}
RETRO_API void retro_cheat_reset() {}
RETRO_API void retro_cheat_set(unsigned /*index*/, bool /*enabled*/, const char* /*code*/) {}
RETRO_API unsigned retro_get_region() { return declaredFps < 55.0 ? RETRO_REGION_PAL : RETRO_REGION_NTSC; }
RETRO_API void* retro_get_memory_data(unsigned /*id*/) { return nullptr; }
RETRO_API size_t retro_get_memory_size(unsigned /*id*/) { return 0; }
