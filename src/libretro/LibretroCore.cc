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
#include "File.hh"
#include "FileContext.hh"
#include "PostProcessor.hh"
#include "Event.hh"
#include "EventDistributor.hh"
#include "FrameSource.hh"
#include "GLContext.hh"
#include "GLUtil.hh"
#include "GlobalCommandController.hh"
#include "JoystickDevice.hh"
#include "MSXCliComm.hh"
#include "MSXException.hh"
#include "MSXMotherBoard.hh"
#include "Reactor.hh"
#include "RenderSettings.hh"
#include "SDLRasterizer.hh"
#include "StateChange.hh"
#include "StateChangeDistributor.hh"
#include "TTFFont.hh"
#include "TclObject.hh"
#include "Thread.hh"
#include "VDP.hh"
#include "VDPVRAM.hh"
#include "Debuggable.hh"
#include "Debugger.hh"
#include "RealDrive.hh"
#include "CartridgeSlotManager.hh"
#include "one_of.hh"
#include "Version.hh"
#include "random.hh"
#include "serialize.hh"
#include "strCat.hh"

#include "libretro.h"

#include <SDL.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <map>
#include <memory>
#include <span>
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
// The frame's display area has ended and it was handed over: its end (the
// VDP's next frame start) is not where retro_run stops then
bool displayDone = false;
bool stateLoad = false;
bool contextAlive = false;

constexpr double NTSC_FPS = 59.9227434;
// The fps declared to the frontend; follows the VDP between 60 and 50 Hz
double declaredFps = NTSC_FPS;
int frameTicks = 0;
// The savestate size given to the frontend, the largest seen yet
size_t stateSizeMax = 0;
// The frame as it goes to the frontend: openMSX's whole picture at the
// chosen scale, or with Crop Overscan the part of it the MSX displays on
unsigned scale = 2;
bool cropOverscan = false;
// Video Output: Software, no GL context; the frame as pixels (swFrame), at
// 320x240 times the Resolution, filled as each frame finishes
bool softwareOutput = false;
std::vector<uint32_t> swFrame;
unsigned swWidth = 0, swHeight = 0;
unsigned frameWidth = 640, frameHeight = 480;
gl::ivec2 frameOffset;
// openMSX's 320 wide picture, borders included, on a 4:3 screen: its
// default horizontal stretch (284) spreads 284 of the 320 over the whole
// width; the core shows all 320 unstretched, so the picture is that much
// wider than 4:3
constexpr float PICTURE_ASPECT = (4.0f / 3.0f) * (320.0f / 284.0f);
// Pixel Aspect Ratio Square: the 320x240 picture as it is, 4:3 (Tatsuya79)
bool squarePixels = false;
bool aspectChanged = false;
float pictureAspect() { return squarePixels ? 4.0f / 3.0f : PICTURE_ASPECT; }
float frameAspect = PICTURE_ASPECT;
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
	// openMSX reads both with getenv(), as UTF-8 also on Windows
	auto setVariable = [](const char* name, const std::string& value) {
#ifdef _WIN32
		_putenv_s(name, value.c_str());
#else
		setenv(name, value.c_str(), 1);
#endif
	};
	setVariable("OPENMSX_SYSTEM_DATA", system + "/share");
	setVariable("OPENMSX_HOME", saves);
}

void runCommand(const std::string& command)
{
	try {
		reactor->getGlobalCommandController().executeCommand(command);
	} catch (MSXException& e) {
		log(RETRO_LOG_WARN, "'%s': %s\n", command.c_str(), std::string(e.getMessage()).c_str());
	}
}

// Disk control: the disks of the content (one, or the list of an .m3u) for
// the frontend to swap in drive A. A machine needs a disk drive of its own;
// the C-BIOS machines have none.
std::vector<std::string> diskImages;
unsigned diskIndex = 0;
bool diskEjected = false;
unsigned initialDiskIndex = 0;
std::string initialDiskPath;

bool hasExtension(const std::string& path, std::initializer_list<const char*> extensions)
{
	auto dot = path.rfind('.');
	if (dot == std::string::npos) return false;
	std::string ext = path.substr(dot + 1);
	for (auto& c : ext) c = char(tolower(c));
	for (const auto* e : extensions) {
		if (ext == e) return true;
	}
	return false;
}

bool isDiskImage(const std::string& path)
{
	return hasExtension(path, {"dsk", "di1", "di2", "dmk", "xsa"});
}

/** The disks an .m3u lists, relative to it; empty lines and comments skipped. */
std::vector<std::string> readM3u(const std::string& path)
{
	std::vector<std::string> result;
	std::ifstream file(path);
	auto slash = path.find_last_of("/\\");
	std::string dir = slash == std::string::npos ? "" : path.substr(0, slash + 1);
	std::string line;
	while (std::getline(file, line)) {
		while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) line.pop_back();
		if (line.empty() || line[0] == '#') continue;
		bool absolute = line[0] == '/' || line[0] == '\\' || (line.size() > 1 && line[1] == ':');
		result.push_back(absolute ? line : dir + line);
	}
	return result;
}

void insertDisk()
{
	if (!reactor) return;
	if (diskEjected || diskIndex >= diskImages.size() || diskImages[diskIndex].empty()) {
		runCommand("diska eject");
	} else {
		runCommand(std::string(makeTclList("diska", diskImages[diskIndex]).getString()));
	}
}

bool RETRO_CALLCONV diskSetEjectState(bool ejected)
{
	if (ejected == diskEjected) return true;
	diskEjected = ejected;
	insertDisk();
	return true;
}

bool RETRO_CALLCONV diskGetEjectState() { return diskEjected; }

unsigned RETRO_CALLCONV diskGetImageIndex() { return diskIndex; }

bool RETRO_CALLCONV diskSetImageIndex(unsigned index)
{
	// the frontend swaps with the drive open; an index past the end is no disk
	if (!diskEjected) return false;
	diskIndex = index;
	return true;
}

unsigned RETRO_CALLCONV diskGetNumImages() { return unsigned(diskImages.size()); }

bool RETRO_CALLCONV diskReplaceImageIndex(unsigned index, const retro_game_info* info)
{
	if (index >= diskImages.size()) return false;
	if (!info || !info->path) {
		diskImages.erase(diskImages.begin() + index);
		if (diskIndex > index || diskIndex >= diskImages.size()) {
			if (diskIndex > 0) --diskIndex;
		}
	} else {
		diskImages[index] = info->path;
	}
	return true;
}

bool RETRO_CALLCONV diskAddImageIndex()
{
	diskImages.emplace_back();
	return true;
}

bool RETRO_CALLCONV diskSetInitialImage(unsigned index, const char* path)
{
	initialDiskIndex = index;
	initialDiskPath = path ? path : "";
	return true;
}

bool RETRO_CALLCONV diskGetImagePath(unsigned index, char* path, size_t len)
{
	if (index >= diskImages.size() || diskImages[index].empty() || len == 0) return false;
	snprintf(path, len, "%s", diskImages[index].c_str());
	return true;
}

bool RETRO_CALLCONV diskGetImageLabel(unsigned index, char* label, size_t len)
{
	if (index >= diskImages.size() || diskImages[index].empty() || len == 0) return false;
	const auto& image = diskImages[index];
	auto slash = image.find_last_of("/\\");
	snprintf(label, len, "%s", image.c_str() + (slash == std::string::npos ? 0 : slash + 1));
	return true;
}

void setDiskControl()
{
	retro_disk_control_ext_callback ext = {
		diskSetEjectState, diskGetEjectState, diskGetImageIndex, diskSetImageIndex,
		diskGetNumImages, diskReplaceImageIndex, diskAddImageIndex,
		diskSetInitialImage, diskGetImagePath, diskGetImageLabel,
	};
	unsigned version = 0;
	if (environ_cb(RETRO_ENVIRONMENT_GET_DISK_CONTROL_INTERFACE_VERSION, &version) && version >= 1) {
		environ_cb(RETRO_ENVIRONMENT_SET_DISK_CONTROL_EXT_INTERFACE, &ext);
	} else {
		retro_disk_control_callback basic = {
			diskSetEjectState, diskGetEjectState, diskGetImageIndex, diskSetImageIndex,
			diskGetNumImages, diskReplaceImageIndex, diskAddImageIndex,
		};
		environ_cb(RETRO_ENVIRONMENT_SET_DISK_CONTROL_INTERFACE, &basic);
	}
}

/** The content's disks, and the one to start with: the content itself, or
  * the frontend's remembered disk of an .m3u. Returns the path to start. */
std::string setupDisks(const std::string& path)
{
	diskImages.clear();
	diskIndex = 0;
	diskEjected = false;
	if (hasExtension(path, {"m3u"})) {
		diskImages = readM3u(path);
		if (diskImages.empty()) return path;
		if (initialDiskIndex < diskImages.size() && diskImages[initialDiskIndex] == initialDiskPath) {
			diskIndex = initialDiskIndex;
		}
		return diskImages[diskIndex];
	}
	if (isDiskImage(path)) diskImages.push_back(path);
	return path;
}

constexpr const char* DEFAULT_MACHINE = "C-BIOS_MSX2+";

// Extension 1 to 10: as many shown as there are cartridge slots for them
// (showExtensionOptions)
constexpr const char* extensionOptions[] = {
	"openmsx_ext1", "openmsx_ext2", "openmsx_ext3", "openmsx_ext4", "openmsx_ext5",
	"openmsx_ext6", "openmsx_ext7", "openmsx_ext8", "openmsx_ext9", "openmsx_ext10",
};
constexpr int EXTENSIONS_BEFORE_START = 2;

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

std::string evalCommand(const std::string& command)
{
	try {
		return std::string(reactor->getGlobalCommandController().executeCommand(command).getString());
	} catch (MSXException& e) {
		log(RETRO_LOG_WARN, "'%s': %s\n", command.c_str(), std::string(e.getMessage()).c_str());
		return {};
	}
}

/** What is in the cartridge slots on screen, the whole slot map in the log
 * (Tatsuya79) */
void showSlots()
{
	std::string text = evalCommand(
		"set r {}; foreach s [machine_info external_slot] {"
		" set n [lindex [machine_info external_slot $s] 2];"
		" lappend r \"Slot [string toupper [string index $s 4]]: [expr {$n eq {} ? {empty} : $n}]\" };"
		" join $r {, }");
	if (!text.empty()) {
		log(RETRO_LOG_INFO, "%s\n", text.c_str());
		retro_message msg = {text.c_str(), 300};
		environ_cb(RETRO_ENVIRONMENT_SET_MESSAGE, &msg);
	}
	log(RETRO_LOG_INFO, "slot map:\n%s", evalCommand("slotmap").c_str());
}

/** Machine Info (L3 on port 1, Tatsuya79): the game stops and the machine,
 * its description and what is in its slots are drawn into the picture,
 * wrapped to its width, a page at a time; each press shows the next page,
 * and the one after the last returns to the game. At least openMSX's high
 * resolution (640x480), more at a Resolution above 2. Software video output
 * only: the pages are pixels for the frontend. */
struct MachineInfo {
	bool active = false;
	bool held = false;
	unsigned page = 0;
	unsigned width = 0, height = 0;
	std::vector<std::vector<std::string>> pages;
	std::vector<uint32_t> pixels;
} info;

/** A field of the machine's <info> (openmsx_info machines <name>), or "" */
std::string machineField(const char* key)
{
	return evalCommand(strCat(
		"set i [openmsx_info machines [machine_info config_name]];"
		" if {[dict exists $i ", key, "]} { dict get $i ", key, " }"));
}

/** As standalone's Machine > Current setup shows it (Tatsuya79): maker and
 * model, the machine's details and sizes, its description, and what is in
 * the cartridge slots */
std::string machineInfoText()
{
	std::vector<std::pair<std::string, std::string>> rows;
	auto add = [&](const char* label, std::string value) {
		if (!value.empty()) rows.emplace_back(label, std::move(value));
	};
	std::string manufacturer = machineField("manufacturer"), code = machineField("code");
	add("Manufacturer", manufacturer);
	add("Product code", code);
	add("Release year", machineField("release_year"));
	add("Type", machineField("type"));
	add("Region", machineField("region"));
	if (auto* board = reactor->getMotherBoard()) {
		// as ImGuiMachine's amendConfigInfo counts them
		unsigned ramSize = 0;
		for (const auto& [name, debuggable] : board->getDebugger().getDebuggables()) {
			if (debuggable->getDescription() == one_of("memory mapper", "ram")) {
				ramSize += debuggable->getSize();
			}
		}
		add("RAM size", strCat(ramSize / 1024, "kB"));
		if (auto* vdp = dynamic_cast<VDP*>(board->findDevice("VDP"))) {
			add("VRAM size", strCat(vdp->getVRAM().getSize() / 1024, "kB"));
			add("VDP version", std::string(vdp->getVersionString()));
		}
		if (auto drives = RealDrive::getDrivesInUse(*board)) {
			add("Disk drives", strCat(drives->count()));
		}
		add("Cartridge slots", strCat(board->getSlotManager().getNumberOfSlots()));
	}
	size_t labelWidth = 0;
	for (const auto& row : rows) labelWidth = std::max(labelWidth, row.first.size());

	std::string title = strCat(manufacturer, manufacturer.empty() || code.empty() ? "" : " ", code);
	std::string text = title.empty() ? evalCommand("machine_info config_name") : title;
	text += "\n";
	for (const auto& [label, value] : rows) {
		text += strCat('\n', label, std::string(labelWidth + 2 - label.size(), ' '), value);
	}
	if (std::string description = machineField("description"); !description.empty()) {
		text += strCat("\n\n", description);
	}
	text += "\n\nSlots";
	text += evalCommand(
		"set r {}; foreach s [machine_info external_slot] {"
		" set n [lindex [machine_info external_slot $s] 2];"
		" append r \"\nSlot [string toupper [string index $s 4]]: [expr {$n eq {} ? {empty} : $n}]\" };"
		" set r");
	return text;
}

/** The text in lines no wider than 'width' pixels, broken between words */
std::vector<std::string> wrapText(const TTFFont& font, const std::string& text, int width)
{
	std::vector<std::string> lines;
	size_t start = 0;
	while (start <= text.size()) {
		size_t end = text.find('\n', start);
		if (end == std::string::npos) end = text.size();
		std::string paragraph = text.substr(start, end - start);
		std::string line;
		size_t pos = 0;
		while (pos < paragraph.size()) {
			size_t next = paragraph.find(' ', pos);
			if (next == std::string::npos) next = paragraph.size();
			std::string word = paragraph.substr(pos, next - pos);
			std::string candidate = line.empty() ? word : line + ' ' + word;
			if (!line.empty() && font.getSize(candidate).x > width) {
				lines.push_back(line);
				line = word;
			} else {
				line = candidate;
			}
			pos = next + 1;
		}
		lines.push_back(line);
		start = end + 1;
	}
	return lines;
}

constexpr uint32_t INFO_BACKGROUND = 0x101830, INFO_TEXT = 0xF0F0F0, INFO_FOOTER = 0x90A0C0;

void drawText(const TTFFont& font, const std::string& text, int x, int y, uint32_t color)
{
	if (text.empty()) return;
	SDLSurfacePtr surface = font.render(text, uint8_t(color >> 16), uint8_t(color >> 8), uint8_t(color));
	SDL_Surface* s = surface.get();
	if (!s) return;
	SDL_LockSurface(s);
	for (int sy = 0; sy < s->h; ++sy) {
		int py = y + sy;
		if (py < 0 || py >= int(info.height)) continue;
		const auto* row = static_cast<const uint8_t*>(s->pixels) + size_t(sy) * s->pitch;
		for (int sx = 0; sx < s->w; ++sx) {
			int px = x + sx;
			if (px < 0 || px >= int(info.width)) continue;
			uint32_t value;
			std::memcpy(&value, row + size_t(sx) * 4, 4);
			uint8_t r, g, b, a;
			SDL_GetRGBA(value, s->format, &r, &g, &b, &a);
			uint32_t& out = info.pixels[size_t(py) * info.width + px];
			auto mix = [&](unsigned shift, uint8_t c) {
				unsigned bg = (out >> shift) & 0xFF;
				return ((bg * (255 - a) + c * a) / 255) << shift;
			};
			out = mix(16, r) | mix(8, g) | mix(0, b);
		}
	}
	SDL_UnlockSurface(s);
}

/** The pages laid out for the picture's size; false without a font */
bool layOutMachineInfo()
{
	const unsigned s = std::max(scale, 2u);
	info.width = 320 * s;
	info.height = 240 * s;
	info.pages.clear();
	try {
		TTFFont font(systemFileContext().resolve("skins/DejaVuSansMono.ttf.gz"), int(7 * s), 0);
		const int margin = int(8 * s), lineHeight = font.getHeight();
		const int perPage = std::max(1, (int(info.height) - 2 * margin) / lineHeight - 2);
		auto lines = wrapText(font, machineInfoText(), int(info.width) - 2 * margin);
		for (size_t i = 0; i < lines.size(); i += perPage) {
			info.pages.emplace_back(lines.begin() + i, lines.begin() + std::min(lines.size(), i + perPage));
		}
	} catch (MSXException& e) {
		log(RETRO_LOG_WARN, "Machine Info: %s\n", std::string(e.getMessage()).c_str());
		return false;
	}
	return !info.pages.empty();
}

void drawMachineInfoPage()
{
	const unsigned s = std::max(scale, 2u);
	info.pixels.assign(size_t(info.width) * info.height, INFO_BACKGROUND);
	try {
		TTFFont font(systemFileContext().resolve("skins/DejaVuSansMono.ttf.gz"), int(7 * s), 0);
		const int margin = int(8 * s), lineHeight = font.getHeight();
		int y = margin;
		for (const auto& line : info.pages[info.page]) {
			drawText(font, line, margin, y, INFO_TEXT);
			y += lineHeight;
		}
		// by the name Quick Menu > Controls lists: the frontend does not tell a
		// core which button a remap put it on (Tatsuya79)
		std::string footer = strCat("Page ", info.page + 1, '/', info.pages.size(), " - Machine Info button: ",
			info.page + 1 < info.pages.size() ? "next page" : "back to the game");
		drawText(font, footer, margin, int(info.height) - margin - lineHeight, INFO_FOOTER);
	} catch (MSXException& e) {
		log(RETRO_LOG_WARN, "Machine Info: %s\n", std::string(e.getMessage()).c_str());
	}
	retro_game_geometry geometry = {};
	geometry.base_width = info.width;
	geometry.base_height = info.height;
	geometry.max_width = MAX_WIDTH;
	geometry.max_height = MAX_HEIGHT;
	geometry.aspect_ratio = 4.0f / 3.0f;
	environ_cb(RETRO_ENVIRONMENT_SET_GEOMETRY, &geometry);
}

/** L3 on port 1 pressed: into the pages, to the next one, or back to the
 * game; true while the pages are shown */
bool updateMachineInfo()
{
	bool down = input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L3) != 0;
	bool pressed = down && !info.held;
	info.held = down;
	if (!pressed) return info.active;
	if (!info.active) {
		if (!softwareOutput) {
			showMessage("Machine Info needs Video Output Software");
			return false;
		}
		if (!layOutMachineInfo()) return false;
		info.active = true;
		info.page = 0;
	} else if (++info.page >= info.pages.size()) {
		info.active = false;
		info.pixels.clear();
		frameWidth = 0; // the game's geometry to the frontend again
		return false;
	}
	drawMachineInfoPage();
	return true;
}

/** Extension 1 to n shown, the others hidden (Tatsuya79) */
void showExtensionOptions(int n)
{
	int i = 0;
	for (const auto* key : extensionOptions) {
		retro_core_option_display display = {key, i++ < n};
		environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &display);
	}
}

/** After a start, as many Extension options as there are in use and empty
 * cartridge slots, the content's cartridge and a Slot Expander's slots
 * counted; up to the last one in use, so that it can be changed */
void showExtensionOptionsForMachine()
{
	int used = 0, last = 0, i = 0;
	for (const auto* key : extensionOptions) {
		++i;
		if (optionValue(key, "none") != "none") {
			++used;
			last = i;
		}
	}
	int empty = std::atoi(evalCommand(
		"set n 0; foreach s [machine_info external_slot] {"
		" if {[lindex [machine_info external_slot $s] 2] eq {}} {incr n} }; set n").c_str());
	showExtensionOptions(std::max(last, used + empty));
}

retro_core_option_v2_category optionCategories[] = {
	{"system", "System", "The MSX machine and the cartridges in its slots. Changes take effect when the content is loaded again."},
	{"video", "Video", "The size of the picture and its border."},
	{nullptr, nullptr, nullptr},
};

// Every extension in share/extensions, by its maker and name
#define EXTENSION_OPTION(key, label) \
	{key, label, nullptr, \
	 "A cartridge or device in a free slot, as openMSX's -ext. Some need their ROMs in <system>/openmsx/share/systemroms.", \
	 nullptr, "system", \
	 { \
		{"none", "None"}, \
		{"2nd_PSG", "2nd PSG"}, \
		{"advram", "Ademir Carchano ADVRAM"}, \
		{"MegaRAM_2MB", "Ademir Carchano MegaRAM (anniversary ed.)"}, \
		{"ASCII_MSX-DOS2", "ASCII Japanese MSX-DOS2"}, \
		{"ASCII_Japanese_MSX-Write", "ASCII Japanese MSX-Write"}, \
		{"ASCII_Japanese_MSX-Write_II", "ASCII Japanese MSX-Write II"}, \
		{"msxdos2", "ASCII MSX-DOS 2"}, \
		{"AVT_DPF-550", "AVT Floppy Disk Drive DPF-550"}, \
		{"mu-PACK", "Bit²/ASCII μ·PACK BM-117"}, \
		{"Casio_KB-10", "Casio Docking station KB-10"}, \
		{"Casio_KB-7", "Casio Docking station KB-7"}, \
		{"Checkmark_FM_Stereo_PAK", "Checkmark FM Stereo PAK"}, \
		{"Computer_Mates_Disk_Interface", "Computer Mates Disk Interface"}, \
		{"ROM_Hunter_Mk2", "Daito Micom System ROM Hunter Mk2"}, \
		{"debugdevice", "Debug Device"}, \
		{"DDX_3.0", "Digital Design DDX 3.0"}, \
		{"megaram", "Digital Design MegaRAM"}, \
		{"MegaRAM_Disk", "Digital Design MegaRAM DISK"}, \
		{"smxwifi", "DucaSP SM-X WiFi"}, \
		{"Epson_Chakkari_Copy", "Epson Chakkari Copy"}, \
		{"ESE_Ramdisk", "ESE Artists' Factory ESE Ramdisk"}, \
		{"ESE_SCC", "ESE Artists' Factory ESE SCC"}, \
		{"ESE_WAVE-SCSI", "ESE Artists' Factory ESE WAVE-SCSI"}, \
		{"ESE_MEGA-SCSI", "ESE Artists' Factory MEGA SCSI"}, \
		{"FAC_MIDI_Interface", "FAC MIDI Interface"}, \
		{"rs232", "Generic MSX RS-232C"}, \
		{"audio", "Generic MSX-AUDIO"}, \
		{"audio2", "Generic MSX-AUDIO 2 (alternative ports)"}, \
		{"Gradiente_CT-80E", "Gradiente Cartão 80 Colunas CT-80E"}, \
		{"ASCII16-X_Mapper_XL", "Grauw ASCII16-X Mapper XL"}, \
		{"hirestimer", "hirestimer"}, \
		{"Hitachi_MPC-310H", "Hitachi 2DD Micro Floppydisk Drive + Controller MPC-310H"}, \
		{"Jun_Soft_DalSoRi_R2", "Jun Soft DalSoRi R2"}, \
		{"JVC_MSX_MIDI", "JVC MSX MIDI"}, \
		{"Konami_EC-701", "Konami Japanese Word Processor Cartridge EC-701"}, \
		{"Konami_EC-702", "Konami Kanji ROM Cartridge EC-702"}, \
		{"scc", "Konami SCC"}, \
		{"scc+", "Konami SCC+"}, \
		{"Konami_SD-Snatcher_Sound_Cartridge", "Konami SD-Snatcher Sound Cartridge RA-011"}, \
		{"Konami_Snatcher_Sound_Cartridge", "Konami Snatcher Sound Cartridge RA-004"}, \
		{"Lascom_Kanji", "Lascom Kanji ROM Cartridge"}, \
		{"OPL3Cartridge_Moonsound_compatible", "Luciano Sturaro OPL3 Cartridge (Moonsound compatible)"}, \
		{"OPL3Cartridge", "Luciano Sturaro OPL3 Cartridge (MSX-Audio compatible)"}, \
		{"MIDI_Interface_3", "M. Fujimoto MIDI Interface 3"}, \
		{"Konami_Ultimate_Collection", "Manuel Pazos Konami Ultimate Collection version 3"}, \
		{"MegaFlashROM_SCC", "Manuel Pazos MegaFlashROM SCC"}, \
		{"MegaFlashROM_SCC+", "Manuel Pazos MegaFlashROM SCC+"}, \
		{"MegaFlashROM_SCC+_SD", "Manuel Pazos MegaFlashROM SCC+ SD"}, \
		{"ram1mb", "Memory Mapper (1 MB)"}, \
		{"ram2mb", "Memory Mapper (2 MB)"}, \
		{"ram4mb", "Memory Mapper (4 MB)"}, \
		{"ram512k", "Memory Mapper (512 kB)"}, \
		{"Microsol_CDX-2", "Microsol Disk Drive CDX-2"}, \
		{"Mitsubishi_ML-30DC_ML-30FD", "Mitsubishi Micro Floppy Disk Controller/Drives ML-30DC/ML-30FD"}, \
		{"mbstereo", "Moonblaster Stereo"}, \
		{"Gouda_SCSI", "MSX Club Gouda Gouda Novaxis SCSI"}, \
		{"MSXPi", "MSXPi"}, \
		{"OPL3Cartridge2_mono", "MSXpro OPL3 Cartridge v2 (mono)"}, \
		{"OPL3Cartridge2_stereo", "MSXpro OPL3 Cartridge v2 (stereo)"}, \
		{"National_CF-SM003", "National Kanji Word Processor Unit CF-SM003"}, \
		{"Neos_MA-20R", "Neos Version Up Adapter MSX BASIC Vev. 2.0 MA-20 (R)"}, \
		{"Neos_MA-20V", "Neos Version Up Adapter V MA-20 (V)"}, \
		{"unapinet", "openMSXnet UnapiNet"}, \
		{"fmpac", "Panasoft FMPAC SW-M004"}, \
		{"pac", "Panasoft PAC SW-M001"}, \
		{"Panasonic_FS-FD1A", "Panasonic 3.5 inch Floppy Disk Drive FS-FD1A"}, \
		{"Panasonic_FS-CA1", "Panasonic MSX-AUDIO Unit FS-CA1"}, \
		{"Panasonic_FS-PW1", "Panasonic Word Processor with Kanji ROM Cartridge FS-PW1"}, \
		{"Password_Cartridge", "Password Cartridge"}, \
		{"Philips_NMS_1200", "Philips 2DD Micro Floppydisk Drive NMS 1200"}, \
		{"Philips_VY_0010", "Philips 3.5\" Disk Drive VY-0010"}, \
		{"Philips_NMS_1205", "Philips Music-Module NMS-1205"}, \
		{"Philips_VU_0040", "Philips Parallel Printer Interface VU 0040"}, \
		{"Boosted_audio", "Philips/MSXPro MSX-Audio"}, \
		{"Musical_Memory_Mapper", "Popolon + Repro Factory Musical Memory Mapper (1 MB)"}, \
		{"Popolon_Sound_Cartridge", "Popolon Sound Cartridge"}, \
		{"programmabledevice", "PVM Programmable Device"}, \
		{"ram16k", "RAM (16 kB)"}, \
		{"ram32k", "RAM (32 kB)"}, \
		{"ram64k", "RAM (64 kB)"}, \
		{"Carnivore2", "RBSC Carnivore2"}, \
		{"Sanyo_MFD-001", "Sanyo MSX Floppy Disk Unit MFD-001"}, \
		{"Sanyo_MRS-001", "Sanyo MSX RS-232C Interface Unit MRS-001"}, \
		{"Sharp_HB-4000", "Sharp Cartão 80 Colunas HB-4000"}, \
		{"Sharp_HB-3600", "Sharp Dual Disk Drive Controller HB-3600"}, \
		{"slotexpander", "Slot Expander"}, \
		{"Beer_IDE", "SOLiD Beer IDE"}, \
		{"Sony_HBD-20W", "Sony 2DD Micro Floppydisk Drive HBD-20W"}, \
		{"Sony_HBD-F1", "Sony 2DD Micro Floppydisk Drive HBD-F1"}, \
		{"Sony_HBI-55", "Sony Data Cartridge 4KBytes HBI-55"}, \
		{"Sony_HBI-J1", "Sony Kanji ROM Cartridge HBI-J1"}, \
		{"Sony_HBD-50", "Sony Micro Floppydisk Drive HBD-50"}, \
		{"Sony_HBK-30", "Sony Micro Floppydisk Drive HBK-30"}, \
		{"Sony_HBI-232", "Sony RS-232C Interface Cartridge HBI-232"}, \
		{"SensorKid", "Sony Sensor Kid Cartridge HBJ-H081C"}, \
		{"Spectravideo_SVI-707", "Spectravideo MSX Disk Drive (320K) SVI-707"}, \
		{"SunriseIDE_Nextor", "Sunrise ATA-IDE with Nextor"}, \
		{"ide", "Sunrise ATA-IDE"}, \
		{"gfx9000", "Sunrise GFX9000"}, \
		{"moonsound", "Sunrise MoonSound"}, \
		{"nowind", "Sunrise Nowind USB Interface"}, \
		{"video9000", "Sunrise Video9000"}, \
		{"Makoto", "SuperSoniqs / Electric River Hardware Makoto"}, \
		{"franky-ntsc", "SuperSoniqs Franky NTSC"}, \
		{"franky-pal", "SuperSoniqs Franky PAL"}, \
		{"Talent_TDC-600", "Talent disk drive TDC-600"}, \
		{"Talent_DPF-550", "Talent Floppy Disk Drive DPF-550"}, \
		{"Tecall_MSX_RAMFILE_TM220", "Tecall MSX RAMFILE TM220"}, \
		{"SN76489", "Texas Instruments Digital Complex Sound Generator SN76489"}, \
		{"The_SCC_Alliance_Yamanooto", "The SCC Alliance Yamanooto"}, \
		{"Toshiba_HX-F101PE", "Toshiba Micro Floppydisk Unit HX-F101PE"}, \
		{"Toshiba_HX-MU900", "Toshiba MSX Music System HX-MU900"}, \
		{"Yamaha_FD-03", "Yamaha 3.5inch Floppydisk Drive 1DD FD-03"}, \
		{"Yamaha_FD-05", "Yamaha 3.5Inch MSX Micro FDD FD-05/FD-051"}, \
		{"Yamaha_SFG-05", "Yamaha FM Sound Synthesizer Unit mark II SFG-05"}, \
		{"Yamaha_SFG-01", "Yamaha FM Sound Synthesizer Unit SFG-01"}, \
		{"Yamaha_SKW-05_embedded", "Yamaha Kanji Word Processor Unit (embedded) SKW-05"}, \
		{"Yamaha_SKW-01", "Yamaha Kanji Word Processor Unit SKW-01"}, \
		{nullptr, nullptr}, \
	 }, "none"}

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
	{
		"openmsx_scale", "Resolution", nullptr,
		"The size openMSX draws the picture at, borders included: 320x240 times this. Auto follows the screen mode: 1x for the 256 pixel wide modes, 2x for the 512 wide and interlaced ones.",
		nullptr, "video",
		{{"auto", "Auto"}, {"1", "1x (320x240)"}, {"2", "2x (640x480)"}, {"3", "3x (960x720)"}, {"4", "4x (1280x960)"}, {nullptr, nullptr}},
		"2"
	},
	// Software by default, unlike openMSX, which has only its OpenGL
	// renderer: it works with any video driver, RetroArch's shaders do what
	// openMSX's scalers and effects would, and run-ahead's state loads are
	// cheapest there (Tatsuya79)
	{
		"openmsx_video_output", "Video Output (Restart)", nullptr,
		"Software hands RetroArch the picture as pixels, for any video driver. OpenGL draws through RetroArch's OpenGL context (the gl or glcore video driver), with openMSX's own scalers and effects.",
		nullptr, "video",
		{{"software", "Software"}, {"opengl", "OpenGL"}, {nullptr, nullptr}},
		"software"
	},
	{
		"openmsx_crop_overscan", "Crop Overscan", nullptr,
		"Shows only the part of the screen the MSX displays on, without its border; the size follows the screen mode.",
		nullptr, "video",
		{{"disabled", nullptr}, {"enabled", nullptr}, {nullptr, nullptr}},
		"disabled"
	},
	{
		"openmsx_pixel_aspect", "Pixel Aspect Ratio", nullptr,
		"TV: the picture as wide as on a TV, as openMSX shows it. Square: each pixel as wide as it is tall.",
		nullptr, "video",
		{{"tv", "TV"}, {"square", "Square"}, {nullptr, nullptr}},
		"tv"
	},
	EXTENSION_OPTION("openmsx_ext1", "Extension 1 (Restart)"),
	EXTENSION_OPTION("openmsx_ext2", "Extension 2 (Restart)"),
	EXTENSION_OPTION("openmsx_ext3", "Extension 3 (Restart)"),
	EXTENSION_OPTION("openmsx_ext4", "Extension 4 (Restart)"),
	EXTENSION_OPTION("openmsx_ext5", "Extension 5 (Restart)"),
	EXTENSION_OPTION("openmsx_ext6", "Extension 6 (Restart)"),
	EXTENSION_OPTION("openmsx_ext7", "Extension 7 (Restart)"),
	EXTENSION_OPTION("openmsx_ext8", "Extension 8 (Restart)"),
	EXTENSION_OPTION("openmsx_ext9", "Extension 9 (Restart)"),
	EXTENSION_OPTION("openmsx_ext10", "Extension 10 (Restart)"),
	{nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, {{nullptr, nullptr}}, nullptr},
};
#undef EXTENSION_OPTION

void setCoreOptions()
{
	retro_core_options_v2 options = {optionCategories, optionDefinitions};
	unsigned version = 0;
	if (environ_cb(RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION, &version) && version >= 2) {
		environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2, &options);
		showExtensionOptions(EXTENSIONS_BEFORE_START);
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

// Resolution Auto: the scale the screen mode wants, taken once it has held
// for a few frames, so a game switching modes for a frame or two does not
// make openMSX draw at another size each time (Tatsuya79)
bool autoScale = false;
unsigned autoWanted = 0, autoFrames = 0;
constexpr unsigned AUTO_SCALE_FRAMES = 10;

void updateAutoScale()
{
	if (!autoScale || !reactor) return;
	auto* board = reactor->getMotherBoard();
	auto* vdp = board ? dynamic_cast<VDP*>(board->findDevice("VDP")) : nullptr;
	if (!vdp) return;
	unsigned want = (vdp->getDisplayMode().getLineWidth() > 256 || vdp->isInterlaced()) ? 2 : 1;
	if (want == scale) {
		autoFrames = 0;
		return;
	}
	if (want != autoWanted) {
		autoWanted = want;
		autoFrames = 0;
	}
	if (++autoFrames < AUTO_SCALE_FRAMES) return;
	autoFrames = 0;
	scale = want;
	runCommand("set scale_factor " + std::to_string(scale));
}

void readVideoOptions()
{
	std::string scaleValue = optionValue("openmsx_scale", "2");
	autoScale = scaleValue == "auto";
	autoFrames = 0;
	// Auto starts from the size there is and follows the machine from there
	unsigned newScale = autoScale ? scale : unsigned(std::clamp(atoi(scaleValue.c_str()), 1, 4));
	cropOverscan = optionValue("openmsx_crop_overscan", "disabled") == "enabled";
	bool newSquare = optionValue("openmsx_pixel_aspect", "tv") == "square";
	if (newSquare != squarePixels) {
		squarePixels = newSquare;
		aspectChanged = true; // to the frontend with the next frame
	}
	if (newScale != scale && reactor) {
		runCommand("set scale_factor " + std::to_string(newScale));
	}
	scale = newScale;
}

/** Where openMSX's picture is cut for the frontend this frame: all of it,
  * or the MSX's display area. In output pixels, y up from the bottom. */
void updateFrameGeometry()
{
	const int w = int(320 * scale), h = int(240 * scale);
	int x0 = 0, x1 = w, y0 = 0, y1 = h;
	auto* board = reactor ? reactor->getMotherBoard() : nullptr;
	auto* vdp = board ? dynamic_cast<VDP*>(board->findDevice("VDP")) : nullptr;
	if (cropOverscan && vdp) {
		// The VDP's ticks to openMSX's 320 wide picture, as the
		// rasterizer places them, then the horizontal stretch that
		// widens the middle of it over the whole output
		auto toPicture = [](int ticks) {
			constexpr int origin = 100 + 102 + (VDP::TICKS_PER_LINE - 100 - 102 - 27) / 2 - 1;
			return float(ticks - origin) / 4.0f + 160.0f;
		};
		float stretch = reactor->getDisplay().getRenderSettings().getHorizontalStretch();
		auto toOutput = [&](float x) {
			return int(std::lround((x - (320.0f - stretch) / 2.0f) * float(w) / stretch));
		};
		x0 = std::clamp(toOutput(toPicture(vdp->getLeftSprites())), 0, w);
		x1 = std::clamp(toOutput(toPicture(vdp->getRightBorder())), 0, w);
		// At the end of a frame the VDP has begun the next one, and until
		// its display line counter resets, the display start is the
		// frame just shown, counted back from the new frame's start
		int lineZero = vdp->getLineZero();
		if (lineZero < 0) lineZero += vdp->getLinesPerFrame();
		int top = lineZero - SDLRasterizer::getLineRenderTop(vdp->isPalTiming());
		int lines = vdp->getNumberOfLines();
		int top0 = std::clamp(top, 0, 240), bottom0 = std::clamp(top + lines, 0, 240);
		y1 = h - top0 * int(scale);
		y0 = h - bottom0 * int(scale);
		if (x1 <= x0 || y1 <= y0) { x0 = 0; x1 = w; y0 = 0; y1 = h; }
	}
	unsigned newWidth = unsigned(x1 - x0), newHeight = unsigned(y1 - y0);
	frameOffset = gl::ivec2(-x0, -y0);
	if (newWidth != frameWidth || newHeight != frameHeight || aspectChanged) {
		aspectChanged = false;
		log(RETRO_LOG_INFO, "picture %ux%u from %d,%d of %dx%d (crop %s, VDP %s)\n",
		    newWidth, newHeight, x0, y0, w, h, cropOverscan ? "on" : "off", vdp ? "found" : "not found");
		frameWidth = newWidth;
		frameHeight = newHeight;
		retro_game_geometry geometry = {};
		geometry.base_width = frameWidth;
		geometry.base_height = frameHeight;
		geometry.max_width = MAX_WIDTH;
		geometry.max_height = MAX_HEIGHT;
		// the whole picture is pictureAspect(), a part of it in proportion
		frameAspect = pictureAspect() * (float(frameWidth) / float(w)) / (float(frameHeight) / float(h));
		geometry.aspect_ratio = frameAspect;
		environ_cb(RETRO_ENVIRONMENT_SET_GEOMETRY, &geometry);
	}
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
			for (const auto* key : extensionOptions) {
				if (auto ext = optionValue(key, "none"); ext != "none") {
					args.insert(args.end(), {"-ext", ext});
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
		runCommand("set scale_factor " + std::to_string(scale));
		// The picture as the VDP draws it: openMSX's own blur (50) and
		// scanlines (20) are on by default, and left the frame soft next to
		// blueMSX's (Tatsuya79); RetroArch's shaders do those
		runCommand("set blur 0");
		runCommand("set scanline 0");
		// Each MSX pixel the same width: openMSX stretches 284 of its 320
		// wide picture over the output by default, which at 320 wide gave
		// uneven pixels next to blueMSX's (Tatsuya79); RetroArch scales to
		// the aspect ratio
		runCommand("set horizontal_stretch 320");

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

/** The emulator with the core options, else C-BIOS without extensions */
void startWithFallback()
{
	bool started = startEmulator(true);
	if (!started) {
		// most often a real machine whose system ROMs are not there
		showMessage("openMSX: the chosen machine or extension could not start (see the log); starting C-BIOS without extensions");
		SDL_Quit();
		started = startEmulator(false);
	}
	if (!started) {
		environ_cb(RETRO_ENVIRONMENT_SHUTDOWN, nullptr);
		return;
	}
	showSlots();
	showExtensionOptionsForMachine();
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
		startWithFallback();
	} else {
		// a new context after a lost one: openMSX makes its GL objects again
		gl::context.emplace();
		reactor->getDisplay().restoreVideoSystem();
	}
}

void contextDestroy()
{
	// With cache_context the frontend calls this only when the context is
	// really lost (another video driver, an Android app paused) or at
	// unload: the GL objects go while it is still current
	contextAlive = false;
	if (reactor) {
		reactor->getDisplay().releaseVideoSystem();
	}
	PostProcessor::libretroDropSpareGL();
	gl::context.reset();
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
// Arkanoid's paddle, turned by the mouse's horizontal motion: with the MSX
// mouse the bat jumps across the screen, in standalone too (Tatsuya79)
constexpr unsigned DEVICE_ARKANOID_PAD = RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_MOUSE, 0);
// Nothing in the MSX port; RetroArch then offers the keyboard's keys in the
// port's remap list, so a pad button can press F8 and the like (Tatsuya79)
constexpr unsigned DEVICE_KEYBOARD = RETRO_DEVICE_KEYBOARD;
constexpr unsigned NUM_PORTS = 2;

std::array<unsigned, NUM_PORTS> portDevice = {DEVICE_MSX_JOYSTICK, DEVICE_MSX_JOYSTICK};
bool portsChanged = true;
MSXMotherBoard* pluggedBoard = nullptr;

struct PortState {
	uint8_t joy = 0;      // MSX joystick bits held (JOY_UP ... JOY_BUTTONB)
	uint32_t keys = 0;    // keys held: padKeys, then cursorKeys
	unsigned mouseButtons = 0;
	int paddleRest = 0;   // the Vaus Paddle's motion not handed over yet
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
		case DEVICE_ARKANOID_PAD:
			runCommand(std::string("plug ") + connectors[port] + " arkanoidpad");
			break;
		default: // the cursor keys, the keyboard or nothing: the port stays empty
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

/** The Vaus Paddle turns by half the motion of each event and drops the
 * rest (ArkanoidPad.cc, SCALE): fine for standalone's many small events,
 * but RetroArch's one motion a frame lost a slow move altogether, which
 * felt like lag. The rest is kept here for the next frame. */
constexpr int PADDLE_SCALE = 2;

void pollMouse(unsigned port, PortState& state, bool paddle)
{
	auto& distributor = reactor->getEventDistributor();
	int dx = input_state_cb(port, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_X);
	int dy = input_state_cb(port, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_Y);
	if (paddle) {
		dx += state.paddleRest;
		state.paddleRest = dx % PADDLE_SCALE;
		dx -= state.paddleRest;
	}
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
			pollMouse(port, state, false);
			break;
		case DEVICE_ARKANOID_PAD:
			pollMouse(port, state, true);
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
		{"Arkanoid Vaus Paddle", DEVICE_ARKANOID_PAD},
		{"Keyboard", DEVICE_KEYBOARD},
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
		{0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L3, "Machine Info"},
		PORT_DESCRIPTORS(1)
#undef PORT_DESCRIPTORS
		{0, 0, 0, 0, nullptr},
	};
	environ_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, const_cast<retro_input_descriptor*>(descriptors));
}

} // namespace

namespace openmsx {

bool libretroSoftwareOutput()
{
	return softwareOutput;
}

void libretroSoftwareFrame(const FrameSource& frame)
{
	// openMSX's 640x480 view of the frame (hi-res modes keep their pixels),
	// sampled to the output size with the horizontal stretch the GL path
	// applies: the middle `stretch` of openMSX's 320 wide picture fills it
	const unsigned w = 320 * scale, h = 240 * scale;
	swFrame.resize(size_t(w) * h);
	swWidth = w;
	swHeight = h;
	const float stretch = reactor->getDisplay().getRenderSettings().getHorizontalStretch();
	const float left = (320.0f - stretch) / 2.0f;
	std::vector<unsigned> srcX(w);
	for (unsigned x = 0; x < w; ++x) {
		float sx = (left + (float(x) + 0.5f) * stretch / float(w)) * 2.0f;
		srcX[x] = unsigned(std::clamp(int(sx), 0, 639));
	}
	std::array<FrameSource::Pixel, 640> line;
	unsigned lastSy = ~0u;
	std::span<const FrameSource::Pixel> src;
	for (unsigned y = 0; y < h; ++y) {
		unsigned sy = std::min(y * 480 / h, 479u);
		if (sy != lastSy) {
			src = frame.getLinePtr640_480(sy, line);
			lastSy = sy;
		}
		uint32_t* out = &swFrame[size_t(y) * w];
		for (unsigned x = 0; x < w; ++x) {
			uint32_t p = src[srcX[x]];
			// openMSX's pixels are R in the low byte, RetroArch's XRGB8888
			out[x] = ((p & 0xFF) << 16) | (p & 0xFF00) | ((p >> 16) & 0xFF);
		}
	}
}

void libretroFrameEnd(MSXMotherBoard& motherBoard, int ticksPerFrame)
{
	frameTicks = ticksPerFrame;
	if (!displayDone) {
		frameDone = true;
		motherBoard.exitCPULoopSync();
	}
	displayDone = false;
}

bool libretroStateLoad()
{
	return stateLoad;
}

std::string_view libretroSystemFile(const std::string& filename)
{
	static std::map<std::string, std::string, std::less<>> files;
	auto it = files.find(filename);
	if (it == files.end()) {
		auto buf = File(filename).mmap<const char>();
		it = files.emplace(filename, std::string(buf.data(), buf.size())).first;
	}
	return it->second;
}

void libretroDisplayDone(MSXMotherBoard& motherBoard)
{
	displayDone = true;
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
	setControllerInfo();
	setCoreOptions();
	setDiskControl();
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
	info->valid_extensions = "rom|ri|mx1|mx2|col|sg|sc|dsk|di1|di2|dmk|xsa|cas|wav|m3u|zip|gz";
	info->need_fullpath = true;
	info->block_extract = true;
}

RETRO_API void retro_get_system_av_info(retro_system_av_info* info)
{
	*info = {};
	info->geometry.base_width = frameWidth;
	info->geometry.base_height = frameHeight;
	info->geometry.max_width = MAX_WIDTH;
	info->geometry.max_height = MAX_HEIGHT;
	info->geometry.aspect_ratio = frameAspect;
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

// RetroArch names a file inside an archive "archive.zip#member": a playlist
// made by its scanner, or a ROM picked inside the zip in the file browser.
// openMSX opens a zip (or gz) itself, by the archive's own name, and tells
// the type by the file inside (Tatsuya79: "Invalid ROM file" for every zip)
std::string archivePath(const std::string& path)
{
	for (const char* ext : {".zip#", ".gz#"}) {
		std::string lower = path;
		std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return std::tolower(c); });
		auto pos = lower.find(ext);
		if (pos != std::string::npos) return path.substr(0, pos + strlen(ext) - 1);
	}
	return path;
}

RETRO_API bool retro_load_game(const retro_game_info* game)
{
	contentPath = setupDisks(archivePath((game && game->path) ? game->path : ""));

	retro_pixel_format fmt = RETRO_PIXEL_FORMAT_XRGB8888;
	environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt);

	readVideoOptions();
	frameWidth = 320 * scale;
	frameHeight = 240 * scale;
	frameAspect = pictureAspect();

	retro_keyboard_callback keyboard = {keyboardEvent};
	environ_cb(RETRO_ENVIRONMENT_SET_KEYBOARD_CALLBACK, &keyboard);
	setInputDescriptors();
	setupDirectories();

	// Software: no GL context to wait for, openMSX starts now
	softwareOutput = optionValue("openmsx_video_output", "software") == "software";
	if (softwareOutput) {
		log(RETRO_LOG_INFO, "video output: software\n");
		contextAlive = true;
		startWithFallback();
		return reactor != nullptr;
	}

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
	libretroClearRomCache();
	PostProcessor::libretroDropSpareGL();
	gl::context.reset();
	SDL_Quit();
}

RETRO_API void retro_run()
{
	input_poll_cb();
	if (!reactor || !contextAlive) {
		video_cb(nullptr, frameWidth, frameHeight, 0);
		return;
	}
	bool updated = false;
	if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &updated) && updated) {
		readVideoOptions();
	}

	// the game stands still while Machine Info's pages are shown
	if (updateMachineInfo()) {
		video_cb(info.pixels.data(), info.width, info.height, info.width * sizeof(uint32_t));
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
		updateAutoScale();
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
	if (softwareOutput) {
		// the part of the picture shown, as the GL path cuts it (y up)
		updateFrameGeometry();
		const unsigned w = 320 * scale;
		if (swWidth == w && !swFrame.empty()) {
			unsigned x0 = unsigned(-frameOffset.x);
			unsigned top = swHeight - unsigned(-frameOffset.y) - frameHeight;
			video_cb(&swFrame[size_t(top) * swWidth + x0], frameWidth, frameHeight,
			         swWidth * sizeof(uint32_t));
		} else {
			video_cb(nullptr, frameWidth, frameHeight, 0);
		}
	} else if (video) {
		updateFrameGeometry();
		video->paintTo(unsigned(hw_render.get_current_framebuffer()),
		               gl::ivec2(int(320 * scale), int(240 * scale)), frameOffset);
		video_cb(RETRO_HW_FRAME_BUFFER_VALID, frameWidth, frameHeight, 0);
	} else {
		video_cb(nullptr, frameWidth, frameHeight, 0);
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
	// (libretroStateLoad, for the SRAMs of both machines)
	stateLoad = true;
	struct StateLoadEnd { ~StateLoadEnd() { stateLoad = false; } } stateLoadEnd;
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
