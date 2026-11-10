/*
 * openMSX libretro core - Core Options v2
 *
 * The English texts are edited here; the translations in
 * libretro_core_options_intl.h come from Crowdin (intl/).
 */

#ifndef LIBRETRO_CORE_OPTIONS_H__
#define LIBRETRO_CORE_OPTIONS_H__

#include <stdlib.h>
#include <string.h>

#include "libretro.h"

#ifndef HAVE_NO_LANGEXTRA
#include "libretro_core_options_intl.h"
#endif

/*
 ********************************
 * VERSION: 2.0
 ********************************
 *
 * - 2.0: Add support for core options v2 interface
 * - 1.3: Move translations to libretro_core_options_intl.h
 *        - libretro_core_options_intl.h includes BOM and utf-8
 *          fix for MSVC 2010-2013
 *        - Added HAVE_NO_LANGEXTRA flag to disable translations
 *          on platforms/compilers without BOM support
 * - 1.2: Use core options v1 interface when
 *        RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION is >= 1
 *        (previously required RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION == 1)
 * - 1.1: Support generation of core options v0 retro_core_option_value
 *        arrays containing options with a single value
 * - 1.0: First commit
*/

#ifdef __cplusplus
extern "C" {
#endif

/*
 ********************************
 * Core Option Definitions
 ********************************
*/

/* RETRO_LANGUAGE_ENGLISH */

/* Default language:
 * - All other languages must include the same keys and values
 * - Will be used as a fallback in the event that frontend language
 *   is not available
 * - Will be used as a fallback for any missing entries in
 *   frontend language definition
 * - Translations live in libretro_core_options_intl.h, which is
 *   generated from Crowdin - edit only the English texts here
 */

struct retro_core_option_v2_category option_cats_us[] = {
    {"system", "System", "The MSX machine and the cartridges in its slots. Changes take effect when the content is loaded again."},
    {"video", "Video", "The size of the picture and its border."},
    {NULL, NULL, NULL},
};

struct retro_core_option_v2_definition option_defs_us[] = {
    {
        "openmsx_machine", "Machine (Restart)", "Machine",
        "The MSX computer to emulate. The C-BIOS machines need nothing else; a real machine needs its system ROMs in <system>/openmsx/share/systemroms, otherwise C-BIOS MSX2+ starts instead. Europe and the UK run at 50 Hz.",
        NULL, "system",
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
            {NULL, NULL},
        },
        "C-BIOS_MSX2+"
    },
    {
        "openmsx_scale", "Resolution", NULL,
        "The size openMSX draws the picture at, borders included: 320x240 times this. Auto follows the screen mode: 1x for the 256 pixel wide modes, 2x for the 512 wide and interlaced ones.",
        NULL, "video",
        {{"auto", "Auto"}, {"1", "1x (320x240)"}, {"2", "2x (640x480)"}, {"3", "3x (960x720)"}, {"4", "4x (1280x960)"}, {NULL, NULL}},
        "2"
    },
    /* Software by default, unlike openMSX, which has only its OpenGL
     * renderer: it works with any video driver, RetroArch's shaders do what
     * openMSX's scalers and effects would, and run-ahead's state loads are
     * cheapest there (Tatsuya79) */
    {
        "openmsx_video_output", "Video Output (Restart)", NULL,
        "Software hands RetroArch the picture as pixels, for any video driver. OpenGL draws through RetroArch's OpenGL context (the gl or glcore video driver), with openMSX's own scalers and effects.",
        NULL, "video",
        {{"software", "Software"}, {"opengl", "OpenGL"}, {NULL, NULL}},
        "software"
    },
    {
        "openmsx_crop_overscan", "Crop Overscan", NULL,
        "Shows only the part of the screen the MSX displays on, without its border; the size follows the screen mode.",
        NULL, "video",
        {{"disabled", NULL}, {"enabled", NULL}, {NULL, NULL}},
        "disabled"
    },
    {
        "openmsx_pixel_aspect", "Pixel Aspect Ratio", NULL,
        "TV: the picture as wide as on a TV, as openMSX shows it. Square: each pixel as wide as it is tall.",
        NULL, "video",
        {{"tv", "TV"}, {"square", "Square"}, {NULL, NULL}},
        "tv"
    },
    {
        "openmsx_ext1", "Extension 1 (Restart)", NULL,
        "A cartridge or device in a free slot, as openMSX's -ext. Some need their ROMs in <system>/openmsx/share/systemroms.",
        NULL, "system",
        {
            {"none", "None"},
            {"2nd_PSG", "2nd PSG"},
            {"advram", "Ademir Carchano ADVRAM"},
            {"MegaRAM_2MB", "Ademir Carchano MegaRAM (anniversary ed.)"},
            {"ASCII_MSX-DOS2", "ASCII Japanese MSX-DOS2"},
            {"ASCII_Japanese_MSX-Write", "ASCII Japanese MSX-Write"},
            {"ASCII_Japanese_MSX-Write_II", "ASCII Japanese MSX-Write II"},
            {"msxdos2", "ASCII MSX-DOS 2"},
            {"AVT_DPF-550", "AVT Floppy Disk Drive DPF-550"},
            {"mu-PACK", "Bit²/ASCII μ·PACK BM-117"},
            {"Casio_KB-10", "Casio Docking station KB-10"},
            {"Casio_KB-7", "Casio Docking station KB-7"},
            {"Checkmark_FM_Stereo_PAK", "Checkmark FM Stereo PAK"},
            {"Computer_Mates_Disk_Interface", "Computer Mates Disk Interface"},
            {"ROM_Hunter_Mk2", "Daito Micom System ROM Hunter Mk2"},
            {"debugdevice", "Debug Device"},
            {"DDX_3.0", "Digital Design DDX 3.0"},
            {"megaram", "Digital Design MegaRAM"},
            {"MegaRAM_Disk", "Digital Design MegaRAM DISK"},
            {"smxwifi", "DucaSP SM-X WiFi"},
            {"Epson_Chakkari_Copy", "Epson Chakkari Copy"},
            {"ESE_Ramdisk", "ESE Artists' Factory ESE Ramdisk"},
            {"ESE_SCC", "ESE Artists' Factory ESE SCC"},
            {"ESE_WAVE-SCSI", "ESE Artists' Factory ESE WAVE-SCSI"},
            {"ESE_MEGA-SCSI", "ESE Artists' Factory MEGA SCSI"},
            {"FAC_MIDI_Interface", "FAC MIDI Interface"},
            {"rs232", "Generic MSX RS-232C"},
            {"audio", "Generic MSX-AUDIO"},
            {"audio2", "Generic MSX-AUDIO 2 (alternative ports)"},
            {"Gradiente_CT-80E", "Gradiente Cartão 80 Colunas CT-80E"},
            {"ASCII16-X_Mapper_XL", "Grauw ASCII16-X Mapper XL"},
            {"hirestimer", "hirestimer"},
            {"Hitachi_MPC-310H", "Hitachi 2DD Micro Floppydisk Drive + Controller MPC-310H"},
            {"Jun_Soft_DalSoRi_R2", "Jun Soft DalSoRi R2"},
            {"JVC_MSX_MIDI", "JVC MSX MIDI"},
            {"Konami_EC-701", "Konami Japanese Word Processor Cartridge EC-701"},
            {"Konami_EC-702", "Konami Kanji ROM Cartridge EC-702"},
            {"scc", "Konami SCC"},
            {"scc+", "Konami SCC+"},
            {"Konami_SD-Snatcher_Sound_Cartridge", "Konami SD-Snatcher Sound Cartridge RA-011"},
            {"Konami_Snatcher_Sound_Cartridge", "Konami Snatcher Sound Cartridge RA-004"},
            {"Lascom_Kanji", "Lascom Kanji ROM Cartridge"},
            {"OPL3Cartridge_Moonsound_compatible", "Luciano Sturaro OPL3 Cartridge (Moonsound compatible)"},
            {"OPL3Cartridge", "Luciano Sturaro OPL3 Cartridge (MSX-Audio compatible)"},
            {"MIDI_Interface_3", "M. Fujimoto MIDI Interface 3"},
            {"Konami_Ultimate_Collection", "Manuel Pazos Konami Ultimate Collection version 3"},
            {"MegaFlashROM_SCC", "Manuel Pazos MegaFlashROM SCC"},
            {"MegaFlashROM_SCC+", "Manuel Pazos MegaFlashROM SCC+"},
            {"MegaFlashROM_SCC+_SD", "Manuel Pazos MegaFlashROM SCC+ SD"},
            {"ram1mb", "Memory Mapper (1 MB)"},
            {"ram2mb", "Memory Mapper (2 MB)"},
            {"ram4mb", "Memory Mapper (4 MB)"},
            {"ram512k", "Memory Mapper (512 kB)"},
            {"Microsol_CDX-2", "Microsol Disk Drive CDX-2"},
            {"Mitsubishi_ML-30DC_ML-30FD", "Mitsubishi Micro Floppy Disk Controller/Drives ML-30DC/ML-30FD"},
            {"mbstereo", "Moonblaster Stereo"},
            {"Gouda_SCSI", "MSX Club Gouda Gouda Novaxis SCSI"},
            {"MSXPi", "MSXPi"},
            {"OPL3Cartridge2_mono", "MSXpro OPL3 Cartridge v2 (mono)"},
            {"OPL3Cartridge2_stereo", "MSXpro OPL3 Cartridge v2 (stereo)"},
            {"National_CF-SM003", "National Kanji Word Processor Unit CF-SM003"},
            {"Neos_MA-20R", "Neos Version Up Adapter MSX BASIC Vev. 2.0 MA-20 (R)"},
            {"Neos_MA-20V", "Neos Version Up Adapter V MA-20 (V)"},
            {"unapinet", "openMSXnet UnapiNet"},
            {"fmpac", "Panasoft FMPAC SW-M004"},
            {"pac", "Panasoft PAC SW-M001"},
            {"Panasonic_FS-FD1A", "Panasonic 3.5 inch Floppy Disk Drive FS-FD1A"},
            {"Panasonic_FS-CA1", "Panasonic MSX-AUDIO Unit FS-CA1"},
            {"Panasonic_FS-PW1", "Panasonic Word Processor with Kanji ROM Cartridge FS-PW1"},
            {"Password_Cartridge", "Password Cartridge"},
            {"Philips_NMS_1200", "Philips 2DD Micro Floppydisk Drive NMS 1200"},
            {"Philips_VY_0010", "Philips 3.5\" Disk Drive VY-0010"},
            {"Philips_NMS_1205", "Philips Music-Module NMS-1205"},
            {"Philips_VU_0040", "Philips Parallel Printer Interface VU 0040"},
            {"Boosted_audio", "Philips/MSXPro MSX-Audio"},
            {"Musical_Memory_Mapper", "Popolon + Repro Factory Musical Memory Mapper (1 MB)"},
            {"Popolon_Sound_Cartridge", "Popolon Sound Cartridge"},
            {"programmabledevice", "PVM Programmable Device"},
            {"ram16k", "RAM (16 kB)"},
            {"ram32k", "RAM (32 kB)"},
            {"ram64k", "RAM (64 kB)"},
            {"Carnivore2", "RBSC Carnivore2"},
            {"Sanyo_MFD-001", "Sanyo MSX Floppy Disk Unit MFD-001"},
            {"Sanyo_MRS-001", "Sanyo MSX RS-232C Interface Unit MRS-001"},
            {"Sharp_HB-4000", "Sharp Cartão 80 Colunas HB-4000"},
            {"Sharp_HB-3600", "Sharp Dual Disk Drive Controller HB-3600"},
            {"slotexpander", "Slot Expander"},
            {"Beer_IDE", "SOLiD Beer IDE"},
            {"Sony_HBD-20W", "Sony 2DD Micro Floppydisk Drive HBD-20W"},
            {"Sony_HBD-F1", "Sony 2DD Micro Floppydisk Drive HBD-F1"},
            {"Sony_HBI-55", "Sony Data Cartridge 4KBytes HBI-55"},
            {"Sony_HBI-J1", "Sony Kanji ROM Cartridge HBI-J1"},
            {"Sony_HBD-50", "Sony Micro Floppydisk Drive HBD-50"},
            {"Sony_HBK-30", "Sony Micro Floppydisk Drive HBK-30"},
            {"Sony_HBI-232", "Sony RS-232C Interface Cartridge HBI-232"},
            {"SensorKid", "Sony Sensor Kid Cartridge HBJ-H081C"},
            {"Spectravideo_SVI-707", "Spectravideo MSX Disk Drive (320K) SVI-707"},
            {"SunriseIDE_Nextor", "Sunrise ATA-IDE with Nextor"},
            {"ide", "Sunrise ATA-IDE"},
            {"gfx9000", "Sunrise GFX9000"},
            {"moonsound", "Sunrise MoonSound"},
            {"nowind", "Sunrise Nowind USB Interface"},
            {"video9000", "Sunrise Video9000"},
            {"Makoto", "SuperSoniqs / Electric River Hardware Makoto"},
            {"franky-ntsc", "SuperSoniqs Franky NTSC"},
            {"franky-pal", "SuperSoniqs Franky PAL"},
            {"Talent_TDC-600", "Talent disk drive TDC-600"},
            {"Talent_DPF-550", "Talent Floppy Disk Drive DPF-550"},
            {"Tecall_MSX_RAMFILE_TM220", "Tecall MSX RAMFILE TM220"},
            {"SN76489", "Texas Instruments Digital Complex Sound Generator SN76489"},
            {"The_SCC_Alliance_Yamanooto", "The SCC Alliance Yamanooto"},
            {"Toshiba_HX-F101PE", "Toshiba Micro Floppydisk Unit HX-F101PE"},
            {"Toshiba_HX-MU900", "Toshiba MSX Music System HX-MU900"},
            {"Yamaha_FD-03", "Yamaha 3.5inch Floppydisk Drive 1DD FD-03"},
            {"Yamaha_FD-05", "Yamaha 3.5Inch MSX Micro FDD FD-05/FD-051"},
            {"Yamaha_SFG-05", "Yamaha FM Sound Synthesizer Unit mark II SFG-05"},
            {"Yamaha_SFG-01", "Yamaha FM Sound Synthesizer Unit SFG-01"},
            {"Yamaha_SKW-05_embedded", "Yamaha Kanji Word Processor Unit (embedded) SKW-05"},
            {"Yamaha_SKW-01", "Yamaha Kanji Word Processor Unit SKW-01"},
            {NULL, NULL},
        },
        "none"
    },
    {
        "openmsx_ext2", "Extension 2 (Restart)", NULL,
        "A cartridge or device in a free slot, as openMSX's -ext. Some need their ROMs in <system>/openmsx/share/systemroms.",
        NULL, "system",
        {
            {"none", "None"},
            {"2nd_PSG", "2nd PSG"},
            {"advram", "Ademir Carchano ADVRAM"},
            {"MegaRAM_2MB", "Ademir Carchano MegaRAM (anniversary ed.)"},
            {"ASCII_MSX-DOS2", "ASCII Japanese MSX-DOS2"},
            {"ASCII_Japanese_MSX-Write", "ASCII Japanese MSX-Write"},
            {"ASCII_Japanese_MSX-Write_II", "ASCII Japanese MSX-Write II"},
            {"msxdos2", "ASCII MSX-DOS 2"},
            {"AVT_DPF-550", "AVT Floppy Disk Drive DPF-550"},
            {"mu-PACK", "Bit²/ASCII μ·PACK BM-117"},
            {"Casio_KB-10", "Casio Docking station KB-10"},
            {"Casio_KB-7", "Casio Docking station KB-7"},
            {"Checkmark_FM_Stereo_PAK", "Checkmark FM Stereo PAK"},
            {"Computer_Mates_Disk_Interface", "Computer Mates Disk Interface"},
            {"ROM_Hunter_Mk2", "Daito Micom System ROM Hunter Mk2"},
            {"debugdevice", "Debug Device"},
            {"DDX_3.0", "Digital Design DDX 3.0"},
            {"megaram", "Digital Design MegaRAM"},
            {"MegaRAM_Disk", "Digital Design MegaRAM DISK"},
            {"smxwifi", "DucaSP SM-X WiFi"},
            {"Epson_Chakkari_Copy", "Epson Chakkari Copy"},
            {"ESE_Ramdisk", "ESE Artists' Factory ESE Ramdisk"},
            {"ESE_SCC", "ESE Artists' Factory ESE SCC"},
            {"ESE_WAVE-SCSI", "ESE Artists' Factory ESE WAVE-SCSI"},
            {"ESE_MEGA-SCSI", "ESE Artists' Factory MEGA SCSI"},
            {"FAC_MIDI_Interface", "FAC MIDI Interface"},
            {"rs232", "Generic MSX RS-232C"},
            {"audio", "Generic MSX-AUDIO"},
            {"audio2", "Generic MSX-AUDIO 2 (alternative ports)"},
            {"Gradiente_CT-80E", "Gradiente Cartão 80 Colunas CT-80E"},
            {"ASCII16-X_Mapper_XL", "Grauw ASCII16-X Mapper XL"},
            {"hirestimer", "hirestimer"},
            {"Hitachi_MPC-310H", "Hitachi 2DD Micro Floppydisk Drive + Controller MPC-310H"},
            {"Jun_Soft_DalSoRi_R2", "Jun Soft DalSoRi R2"},
            {"JVC_MSX_MIDI", "JVC MSX MIDI"},
            {"Konami_EC-701", "Konami Japanese Word Processor Cartridge EC-701"},
            {"Konami_EC-702", "Konami Kanji ROM Cartridge EC-702"},
            {"scc", "Konami SCC"},
            {"scc+", "Konami SCC+"},
            {"Konami_SD-Snatcher_Sound_Cartridge", "Konami SD-Snatcher Sound Cartridge RA-011"},
            {"Konami_Snatcher_Sound_Cartridge", "Konami Snatcher Sound Cartridge RA-004"},
            {"Lascom_Kanji", "Lascom Kanji ROM Cartridge"},
            {"OPL3Cartridge_Moonsound_compatible", "Luciano Sturaro OPL3 Cartridge (Moonsound compatible)"},
            {"OPL3Cartridge", "Luciano Sturaro OPL3 Cartridge (MSX-Audio compatible)"},
            {"MIDI_Interface_3", "M. Fujimoto MIDI Interface 3"},
            {"Konami_Ultimate_Collection", "Manuel Pazos Konami Ultimate Collection version 3"},
            {"MegaFlashROM_SCC", "Manuel Pazos MegaFlashROM SCC"},
            {"MegaFlashROM_SCC+", "Manuel Pazos MegaFlashROM SCC+"},
            {"MegaFlashROM_SCC+_SD", "Manuel Pazos MegaFlashROM SCC+ SD"},
            {"ram1mb", "Memory Mapper (1 MB)"},
            {"ram2mb", "Memory Mapper (2 MB)"},
            {"ram4mb", "Memory Mapper (4 MB)"},
            {"ram512k", "Memory Mapper (512 kB)"},
            {"Microsol_CDX-2", "Microsol Disk Drive CDX-2"},
            {"Mitsubishi_ML-30DC_ML-30FD", "Mitsubishi Micro Floppy Disk Controller/Drives ML-30DC/ML-30FD"},
            {"mbstereo", "Moonblaster Stereo"},
            {"Gouda_SCSI", "MSX Club Gouda Gouda Novaxis SCSI"},
            {"MSXPi", "MSXPi"},
            {"OPL3Cartridge2_mono", "MSXpro OPL3 Cartridge v2 (mono)"},
            {"OPL3Cartridge2_stereo", "MSXpro OPL3 Cartridge v2 (stereo)"},
            {"National_CF-SM003", "National Kanji Word Processor Unit CF-SM003"},
            {"Neos_MA-20R", "Neos Version Up Adapter MSX BASIC Vev. 2.0 MA-20 (R)"},
            {"Neos_MA-20V", "Neos Version Up Adapter V MA-20 (V)"},
            {"unapinet", "openMSXnet UnapiNet"},
            {"fmpac", "Panasoft FMPAC SW-M004"},
            {"pac", "Panasoft PAC SW-M001"},
            {"Panasonic_FS-FD1A", "Panasonic 3.5 inch Floppy Disk Drive FS-FD1A"},
            {"Panasonic_FS-CA1", "Panasonic MSX-AUDIO Unit FS-CA1"},
            {"Panasonic_FS-PW1", "Panasonic Word Processor with Kanji ROM Cartridge FS-PW1"},
            {"Password_Cartridge", "Password Cartridge"},
            {"Philips_NMS_1200", "Philips 2DD Micro Floppydisk Drive NMS 1200"},
            {"Philips_VY_0010", "Philips 3.5\" Disk Drive VY-0010"},
            {"Philips_NMS_1205", "Philips Music-Module NMS-1205"},
            {"Philips_VU_0040", "Philips Parallel Printer Interface VU 0040"},
            {"Boosted_audio", "Philips/MSXPro MSX-Audio"},
            {"Musical_Memory_Mapper", "Popolon + Repro Factory Musical Memory Mapper (1 MB)"},
            {"Popolon_Sound_Cartridge", "Popolon Sound Cartridge"},
            {"programmabledevice", "PVM Programmable Device"},
            {"ram16k", "RAM (16 kB)"},
            {"ram32k", "RAM (32 kB)"},
            {"ram64k", "RAM (64 kB)"},
            {"Carnivore2", "RBSC Carnivore2"},
            {"Sanyo_MFD-001", "Sanyo MSX Floppy Disk Unit MFD-001"},
            {"Sanyo_MRS-001", "Sanyo MSX RS-232C Interface Unit MRS-001"},
            {"Sharp_HB-4000", "Sharp Cartão 80 Colunas HB-4000"},
            {"Sharp_HB-3600", "Sharp Dual Disk Drive Controller HB-3600"},
            {"slotexpander", "Slot Expander"},
            {"Beer_IDE", "SOLiD Beer IDE"},
            {"Sony_HBD-20W", "Sony 2DD Micro Floppydisk Drive HBD-20W"},
            {"Sony_HBD-F1", "Sony 2DD Micro Floppydisk Drive HBD-F1"},
            {"Sony_HBI-55", "Sony Data Cartridge 4KBytes HBI-55"},
            {"Sony_HBI-J1", "Sony Kanji ROM Cartridge HBI-J1"},
            {"Sony_HBD-50", "Sony Micro Floppydisk Drive HBD-50"},
            {"Sony_HBK-30", "Sony Micro Floppydisk Drive HBK-30"},
            {"Sony_HBI-232", "Sony RS-232C Interface Cartridge HBI-232"},
            {"SensorKid", "Sony Sensor Kid Cartridge HBJ-H081C"},
            {"Spectravideo_SVI-707", "Spectravideo MSX Disk Drive (320K) SVI-707"},
            {"SunriseIDE_Nextor", "Sunrise ATA-IDE with Nextor"},
            {"ide", "Sunrise ATA-IDE"},
            {"gfx9000", "Sunrise GFX9000"},
            {"moonsound", "Sunrise MoonSound"},
            {"nowind", "Sunrise Nowind USB Interface"},
            {"video9000", "Sunrise Video9000"},
            {"Makoto", "SuperSoniqs / Electric River Hardware Makoto"},
            {"franky-ntsc", "SuperSoniqs Franky NTSC"},
            {"franky-pal", "SuperSoniqs Franky PAL"},
            {"Talent_TDC-600", "Talent disk drive TDC-600"},
            {"Talent_DPF-550", "Talent Floppy Disk Drive DPF-550"},
            {"Tecall_MSX_RAMFILE_TM220", "Tecall MSX RAMFILE TM220"},
            {"SN76489", "Texas Instruments Digital Complex Sound Generator SN76489"},
            {"The_SCC_Alliance_Yamanooto", "The SCC Alliance Yamanooto"},
            {"Toshiba_HX-F101PE", "Toshiba Micro Floppydisk Unit HX-F101PE"},
            {"Toshiba_HX-MU900", "Toshiba MSX Music System HX-MU900"},
            {"Yamaha_FD-03", "Yamaha 3.5inch Floppydisk Drive 1DD FD-03"},
            {"Yamaha_FD-05", "Yamaha 3.5Inch MSX Micro FDD FD-05/FD-051"},
            {"Yamaha_SFG-05", "Yamaha FM Sound Synthesizer Unit mark II SFG-05"},
            {"Yamaha_SFG-01", "Yamaha FM Sound Synthesizer Unit SFG-01"},
            {"Yamaha_SKW-05_embedded", "Yamaha Kanji Word Processor Unit (embedded) SKW-05"},
            {"Yamaha_SKW-01", "Yamaha Kanji Word Processor Unit SKW-01"},
            {NULL, NULL},
        },
        "none"
    },
    {
        "openmsx_ext3", "Extension 3 (Restart)", NULL,
        "A cartridge or device in a free slot, as openMSX's -ext. Some need their ROMs in <system>/openmsx/share/systemroms.",
        NULL, "system",
        {
            {"none", "None"},
            {"2nd_PSG", "2nd PSG"},
            {"advram", "Ademir Carchano ADVRAM"},
            {"MegaRAM_2MB", "Ademir Carchano MegaRAM (anniversary ed.)"},
            {"ASCII_MSX-DOS2", "ASCII Japanese MSX-DOS2"},
            {"ASCII_Japanese_MSX-Write", "ASCII Japanese MSX-Write"},
            {"ASCII_Japanese_MSX-Write_II", "ASCII Japanese MSX-Write II"},
            {"msxdos2", "ASCII MSX-DOS 2"},
            {"AVT_DPF-550", "AVT Floppy Disk Drive DPF-550"},
            {"mu-PACK", "Bit²/ASCII μ·PACK BM-117"},
            {"Casio_KB-10", "Casio Docking station KB-10"},
            {"Casio_KB-7", "Casio Docking station KB-7"},
            {"Checkmark_FM_Stereo_PAK", "Checkmark FM Stereo PAK"},
            {"Computer_Mates_Disk_Interface", "Computer Mates Disk Interface"},
            {"ROM_Hunter_Mk2", "Daito Micom System ROM Hunter Mk2"},
            {"debugdevice", "Debug Device"},
            {"DDX_3.0", "Digital Design DDX 3.0"},
            {"megaram", "Digital Design MegaRAM"},
            {"MegaRAM_Disk", "Digital Design MegaRAM DISK"},
            {"smxwifi", "DucaSP SM-X WiFi"},
            {"Epson_Chakkari_Copy", "Epson Chakkari Copy"},
            {"ESE_Ramdisk", "ESE Artists' Factory ESE Ramdisk"},
            {"ESE_SCC", "ESE Artists' Factory ESE SCC"},
            {"ESE_WAVE-SCSI", "ESE Artists' Factory ESE WAVE-SCSI"},
            {"ESE_MEGA-SCSI", "ESE Artists' Factory MEGA SCSI"},
            {"FAC_MIDI_Interface", "FAC MIDI Interface"},
            {"rs232", "Generic MSX RS-232C"},
            {"audio", "Generic MSX-AUDIO"},
            {"audio2", "Generic MSX-AUDIO 2 (alternative ports)"},
            {"Gradiente_CT-80E", "Gradiente Cartão 80 Colunas CT-80E"},
            {"ASCII16-X_Mapper_XL", "Grauw ASCII16-X Mapper XL"},
            {"hirestimer", "hirestimer"},
            {"Hitachi_MPC-310H", "Hitachi 2DD Micro Floppydisk Drive + Controller MPC-310H"},
            {"Jun_Soft_DalSoRi_R2", "Jun Soft DalSoRi R2"},
            {"JVC_MSX_MIDI", "JVC MSX MIDI"},
            {"Konami_EC-701", "Konami Japanese Word Processor Cartridge EC-701"},
            {"Konami_EC-702", "Konami Kanji ROM Cartridge EC-702"},
            {"scc", "Konami SCC"},
            {"scc+", "Konami SCC+"},
            {"Konami_SD-Snatcher_Sound_Cartridge", "Konami SD-Snatcher Sound Cartridge RA-011"},
            {"Konami_Snatcher_Sound_Cartridge", "Konami Snatcher Sound Cartridge RA-004"},
            {"Lascom_Kanji", "Lascom Kanji ROM Cartridge"},
            {"OPL3Cartridge_Moonsound_compatible", "Luciano Sturaro OPL3 Cartridge (Moonsound compatible)"},
            {"OPL3Cartridge", "Luciano Sturaro OPL3 Cartridge (MSX-Audio compatible)"},
            {"MIDI_Interface_3", "M. Fujimoto MIDI Interface 3"},
            {"Konami_Ultimate_Collection", "Manuel Pazos Konami Ultimate Collection version 3"},
            {"MegaFlashROM_SCC", "Manuel Pazos MegaFlashROM SCC"},
            {"MegaFlashROM_SCC+", "Manuel Pazos MegaFlashROM SCC+"},
            {"MegaFlashROM_SCC+_SD", "Manuel Pazos MegaFlashROM SCC+ SD"},
            {"ram1mb", "Memory Mapper (1 MB)"},
            {"ram2mb", "Memory Mapper (2 MB)"},
            {"ram4mb", "Memory Mapper (4 MB)"},
            {"ram512k", "Memory Mapper (512 kB)"},
            {"Microsol_CDX-2", "Microsol Disk Drive CDX-2"},
            {"Mitsubishi_ML-30DC_ML-30FD", "Mitsubishi Micro Floppy Disk Controller/Drives ML-30DC/ML-30FD"},
            {"mbstereo", "Moonblaster Stereo"},
            {"Gouda_SCSI", "MSX Club Gouda Gouda Novaxis SCSI"},
            {"MSXPi", "MSXPi"},
            {"OPL3Cartridge2_mono", "MSXpro OPL3 Cartridge v2 (mono)"},
            {"OPL3Cartridge2_stereo", "MSXpro OPL3 Cartridge v2 (stereo)"},
            {"National_CF-SM003", "National Kanji Word Processor Unit CF-SM003"},
            {"Neos_MA-20R", "Neos Version Up Adapter MSX BASIC Vev. 2.0 MA-20 (R)"},
            {"Neos_MA-20V", "Neos Version Up Adapter V MA-20 (V)"},
            {"unapinet", "openMSXnet UnapiNet"},
            {"fmpac", "Panasoft FMPAC SW-M004"},
            {"pac", "Panasoft PAC SW-M001"},
            {"Panasonic_FS-FD1A", "Panasonic 3.5 inch Floppy Disk Drive FS-FD1A"},
            {"Panasonic_FS-CA1", "Panasonic MSX-AUDIO Unit FS-CA1"},
            {"Panasonic_FS-PW1", "Panasonic Word Processor with Kanji ROM Cartridge FS-PW1"},
            {"Password_Cartridge", "Password Cartridge"},
            {"Philips_NMS_1200", "Philips 2DD Micro Floppydisk Drive NMS 1200"},
            {"Philips_VY_0010", "Philips 3.5\" Disk Drive VY-0010"},
            {"Philips_NMS_1205", "Philips Music-Module NMS-1205"},
            {"Philips_VU_0040", "Philips Parallel Printer Interface VU 0040"},
            {"Boosted_audio", "Philips/MSXPro MSX-Audio"},
            {"Musical_Memory_Mapper", "Popolon + Repro Factory Musical Memory Mapper (1 MB)"},
            {"Popolon_Sound_Cartridge", "Popolon Sound Cartridge"},
            {"programmabledevice", "PVM Programmable Device"},
            {"ram16k", "RAM (16 kB)"},
            {"ram32k", "RAM (32 kB)"},
            {"ram64k", "RAM (64 kB)"},
            {"Carnivore2", "RBSC Carnivore2"},
            {"Sanyo_MFD-001", "Sanyo MSX Floppy Disk Unit MFD-001"},
            {"Sanyo_MRS-001", "Sanyo MSX RS-232C Interface Unit MRS-001"},
            {"Sharp_HB-4000", "Sharp Cartão 80 Colunas HB-4000"},
            {"Sharp_HB-3600", "Sharp Dual Disk Drive Controller HB-3600"},
            {"slotexpander", "Slot Expander"},
            {"Beer_IDE", "SOLiD Beer IDE"},
            {"Sony_HBD-20W", "Sony 2DD Micro Floppydisk Drive HBD-20W"},
            {"Sony_HBD-F1", "Sony 2DD Micro Floppydisk Drive HBD-F1"},
            {"Sony_HBI-55", "Sony Data Cartridge 4KBytes HBI-55"},
            {"Sony_HBI-J1", "Sony Kanji ROM Cartridge HBI-J1"},
            {"Sony_HBD-50", "Sony Micro Floppydisk Drive HBD-50"},
            {"Sony_HBK-30", "Sony Micro Floppydisk Drive HBK-30"},
            {"Sony_HBI-232", "Sony RS-232C Interface Cartridge HBI-232"},
            {"SensorKid", "Sony Sensor Kid Cartridge HBJ-H081C"},
            {"Spectravideo_SVI-707", "Spectravideo MSX Disk Drive (320K) SVI-707"},
            {"SunriseIDE_Nextor", "Sunrise ATA-IDE with Nextor"},
            {"ide", "Sunrise ATA-IDE"},
            {"gfx9000", "Sunrise GFX9000"},
            {"moonsound", "Sunrise MoonSound"},
            {"nowind", "Sunrise Nowind USB Interface"},
            {"video9000", "Sunrise Video9000"},
            {"Makoto", "SuperSoniqs / Electric River Hardware Makoto"},
            {"franky-ntsc", "SuperSoniqs Franky NTSC"},
            {"franky-pal", "SuperSoniqs Franky PAL"},
            {"Talent_TDC-600", "Talent disk drive TDC-600"},
            {"Talent_DPF-550", "Talent Floppy Disk Drive DPF-550"},
            {"Tecall_MSX_RAMFILE_TM220", "Tecall MSX RAMFILE TM220"},
            {"SN76489", "Texas Instruments Digital Complex Sound Generator SN76489"},
            {"The_SCC_Alliance_Yamanooto", "The SCC Alliance Yamanooto"},
            {"Toshiba_HX-F101PE", "Toshiba Micro Floppydisk Unit HX-F101PE"},
            {"Toshiba_HX-MU900", "Toshiba MSX Music System HX-MU900"},
            {"Yamaha_FD-03", "Yamaha 3.5inch Floppydisk Drive 1DD FD-03"},
            {"Yamaha_FD-05", "Yamaha 3.5Inch MSX Micro FDD FD-05/FD-051"},
            {"Yamaha_SFG-05", "Yamaha FM Sound Synthesizer Unit mark II SFG-05"},
            {"Yamaha_SFG-01", "Yamaha FM Sound Synthesizer Unit SFG-01"},
            {"Yamaha_SKW-05_embedded", "Yamaha Kanji Word Processor Unit (embedded) SKW-05"},
            {"Yamaha_SKW-01", "Yamaha Kanji Word Processor Unit SKW-01"},
            {NULL, NULL},
        },
        "none"
    },
    {
        "openmsx_ext4", "Extension 4 (Restart)", NULL,
        "A cartridge or device in a free slot, as openMSX's -ext. Some need their ROMs in <system>/openmsx/share/systemroms.",
        NULL, "system",
        {
            {"none", "None"},
            {"2nd_PSG", "2nd PSG"},
            {"advram", "Ademir Carchano ADVRAM"},
            {"MegaRAM_2MB", "Ademir Carchano MegaRAM (anniversary ed.)"},
            {"ASCII_MSX-DOS2", "ASCII Japanese MSX-DOS2"},
            {"ASCII_Japanese_MSX-Write", "ASCII Japanese MSX-Write"},
            {"ASCII_Japanese_MSX-Write_II", "ASCII Japanese MSX-Write II"},
            {"msxdos2", "ASCII MSX-DOS 2"},
            {"AVT_DPF-550", "AVT Floppy Disk Drive DPF-550"},
            {"mu-PACK", "Bit²/ASCII μ·PACK BM-117"},
            {"Casio_KB-10", "Casio Docking station KB-10"},
            {"Casio_KB-7", "Casio Docking station KB-7"},
            {"Checkmark_FM_Stereo_PAK", "Checkmark FM Stereo PAK"},
            {"Computer_Mates_Disk_Interface", "Computer Mates Disk Interface"},
            {"ROM_Hunter_Mk2", "Daito Micom System ROM Hunter Mk2"},
            {"debugdevice", "Debug Device"},
            {"DDX_3.0", "Digital Design DDX 3.0"},
            {"megaram", "Digital Design MegaRAM"},
            {"MegaRAM_Disk", "Digital Design MegaRAM DISK"},
            {"smxwifi", "DucaSP SM-X WiFi"},
            {"Epson_Chakkari_Copy", "Epson Chakkari Copy"},
            {"ESE_Ramdisk", "ESE Artists' Factory ESE Ramdisk"},
            {"ESE_SCC", "ESE Artists' Factory ESE SCC"},
            {"ESE_WAVE-SCSI", "ESE Artists' Factory ESE WAVE-SCSI"},
            {"ESE_MEGA-SCSI", "ESE Artists' Factory MEGA SCSI"},
            {"FAC_MIDI_Interface", "FAC MIDI Interface"},
            {"rs232", "Generic MSX RS-232C"},
            {"audio", "Generic MSX-AUDIO"},
            {"audio2", "Generic MSX-AUDIO 2 (alternative ports)"},
            {"Gradiente_CT-80E", "Gradiente Cartão 80 Colunas CT-80E"},
            {"ASCII16-X_Mapper_XL", "Grauw ASCII16-X Mapper XL"},
            {"hirestimer", "hirestimer"},
            {"Hitachi_MPC-310H", "Hitachi 2DD Micro Floppydisk Drive + Controller MPC-310H"},
            {"Jun_Soft_DalSoRi_R2", "Jun Soft DalSoRi R2"},
            {"JVC_MSX_MIDI", "JVC MSX MIDI"},
            {"Konami_EC-701", "Konami Japanese Word Processor Cartridge EC-701"},
            {"Konami_EC-702", "Konami Kanji ROM Cartridge EC-702"},
            {"scc", "Konami SCC"},
            {"scc+", "Konami SCC+"},
            {"Konami_SD-Snatcher_Sound_Cartridge", "Konami SD-Snatcher Sound Cartridge RA-011"},
            {"Konami_Snatcher_Sound_Cartridge", "Konami Snatcher Sound Cartridge RA-004"},
            {"Lascom_Kanji", "Lascom Kanji ROM Cartridge"},
            {"OPL3Cartridge_Moonsound_compatible", "Luciano Sturaro OPL3 Cartridge (Moonsound compatible)"},
            {"OPL3Cartridge", "Luciano Sturaro OPL3 Cartridge (MSX-Audio compatible)"},
            {"MIDI_Interface_3", "M. Fujimoto MIDI Interface 3"},
            {"Konami_Ultimate_Collection", "Manuel Pazos Konami Ultimate Collection version 3"},
            {"MegaFlashROM_SCC", "Manuel Pazos MegaFlashROM SCC"},
            {"MegaFlashROM_SCC+", "Manuel Pazos MegaFlashROM SCC+"},
            {"MegaFlashROM_SCC+_SD", "Manuel Pazos MegaFlashROM SCC+ SD"},
            {"ram1mb", "Memory Mapper (1 MB)"},
            {"ram2mb", "Memory Mapper (2 MB)"},
            {"ram4mb", "Memory Mapper (4 MB)"},
            {"ram512k", "Memory Mapper (512 kB)"},
            {"Microsol_CDX-2", "Microsol Disk Drive CDX-2"},
            {"Mitsubishi_ML-30DC_ML-30FD", "Mitsubishi Micro Floppy Disk Controller/Drives ML-30DC/ML-30FD"},
            {"mbstereo", "Moonblaster Stereo"},
            {"Gouda_SCSI", "MSX Club Gouda Gouda Novaxis SCSI"},
            {"MSXPi", "MSXPi"},
            {"OPL3Cartridge2_mono", "MSXpro OPL3 Cartridge v2 (mono)"},
            {"OPL3Cartridge2_stereo", "MSXpro OPL3 Cartridge v2 (stereo)"},
            {"National_CF-SM003", "National Kanji Word Processor Unit CF-SM003"},
            {"Neos_MA-20R", "Neos Version Up Adapter MSX BASIC Vev. 2.0 MA-20 (R)"},
            {"Neos_MA-20V", "Neos Version Up Adapter V MA-20 (V)"},
            {"unapinet", "openMSXnet UnapiNet"},
            {"fmpac", "Panasoft FMPAC SW-M004"},
            {"pac", "Panasoft PAC SW-M001"},
            {"Panasonic_FS-FD1A", "Panasonic 3.5 inch Floppy Disk Drive FS-FD1A"},
            {"Panasonic_FS-CA1", "Panasonic MSX-AUDIO Unit FS-CA1"},
            {"Panasonic_FS-PW1", "Panasonic Word Processor with Kanji ROM Cartridge FS-PW1"},
            {"Password_Cartridge", "Password Cartridge"},
            {"Philips_NMS_1200", "Philips 2DD Micro Floppydisk Drive NMS 1200"},
            {"Philips_VY_0010", "Philips 3.5\" Disk Drive VY-0010"},
            {"Philips_NMS_1205", "Philips Music-Module NMS-1205"},
            {"Philips_VU_0040", "Philips Parallel Printer Interface VU 0040"},
            {"Boosted_audio", "Philips/MSXPro MSX-Audio"},
            {"Musical_Memory_Mapper", "Popolon + Repro Factory Musical Memory Mapper (1 MB)"},
            {"Popolon_Sound_Cartridge", "Popolon Sound Cartridge"},
            {"programmabledevice", "PVM Programmable Device"},
            {"ram16k", "RAM (16 kB)"},
            {"ram32k", "RAM (32 kB)"},
            {"ram64k", "RAM (64 kB)"},
            {"Carnivore2", "RBSC Carnivore2"},
            {"Sanyo_MFD-001", "Sanyo MSX Floppy Disk Unit MFD-001"},
            {"Sanyo_MRS-001", "Sanyo MSX RS-232C Interface Unit MRS-001"},
            {"Sharp_HB-4000", "Sharp Cartão 80 Colunas HB-4000"},
            {"Sharp_HB-3600", "Sharp Dual Disk Drive Controller HB-3600"},
            {"slotexpander", "Slot Expander"},
            {"Beer_IDE", "SOLiD Beer IDE"},
            {"Sony_HBD-20W", "Sony 2DD Micro Floppydisk Drive HBD-20W"},
            {"Sony_HBD-F1", "Sony 2DD Micro Floppydisk Drive HBD-F1"},
            {"Sony_HBI-55", "Sony Data Cartridge 4KBytes HBI-55"},
            {"Sony_HBI-J1", "Sony Kanji ROM Cartridge HBI-J1"},
            {"Sony_HBD-50", "Sony Micro Floppydisk Drive HBD-50"},
            {"Sony_HBK-30", "Sony Micro Floppydisk Drive HBK-30"},
            {"Sony_HBI-232", "Sony RS-232C Interface Cartridge HBI-232"},
            {"SensorKid", "Sony Sensor Kid Cartridge HBJ-H081C"},
            {"Spectravideo_SVI-707", "Spectravideo MSX Disk Drive (320K) SVI-707"},
            {"SunriseIDE_Nextor", "Sunrise ATA-IDE with Nextor"},
            {"ide", "Sunrise ATA-IDE"},
            {"gfx9000", "Sunrise GFX9000"},
            {"moonsound", "Sunrise MoonSound"},
            {"nowind", "Sunrise Nowind USB Interface"},
            {"video9000", "Sunrise Video9000"},
            {"Makoto", "SuperSoniqs / Electric River Hardware Makoto"},
            {"franky-ntsc", "SuperSoniqs Franky NTSC"},
            {"franky-pal", "SuperSoniqs Franky PAL"},
            {"Talent_TDC-600", "Talent disk drive TDC-600"},
            {"Talent_DPF-550", "Talent Floppy Disk Drive DPF-550"},
            {"Tecall_MSX_RAMFILE_TM220", "Tecall MSX RAMFILE TM220"},
            {"SN76489", "Texas Instruments Digital Complex Sound Generator SN76489"},
            {"The_SCC_Alliance_Yamanooto", "The SCC Alliance Yamanooto"},
            {"Toshiba_HX-F101PE", "Toshiba Micro Floppydisk Unit HX-F101PE"},
            {"Toshiba_HX-MU900", "Toshiba MSX Music System HX-MU900"},
            {"Yamaha_FD-03", "Yamaha 3.5inch Floppydisk Drive 1DD FD-03"},
            {"Yamaha_FD-05", "Yamaha 3.5Inch MSX Micro FDD FD-05/FD-051"},
            {"Yamaha_SFG-05", "Yamaha FM Sound Synthesizer Unit mark II SFG-05"},
            {"Yamaha_SFG-01", "Yamaha FM Sound Synthesizer Unit SFG-01"},
            {"Yamaha_SKW-05_embedded", "Yamaha Kanji Word Processor Unit (embedded) SKW-05"},
            {"Yamaha_SKW-01", "Yamaha Kanji Word Processor Unit SKW-01"},
            {NULL, NULL},
        },
        "none"
    },
    {
        "openmsx_ext5", "Extension 5 (Restart)", NULL,
        "A cartridge or device in a free slot, as openMSX's -ext. Some need their ROMs in <system>/openmsx/share/systemroms.",
        NULL, "system",
        {
            {"none", "None"},
            {"2nd_PSG", "2nd PSG"},
            {"advram", "Ademir Carchano ADVRAM"},
            {"MegaRAM_2MB", "Ademir Carchano MegaRAM (anniversary ed.)"},
            {"ASCII_MSX-DOS2", "ASCII Japanese MSX-DOS2"},
            {"ASCII_Japanese_MSX-Write", "ASCII Japanese MSX-Write"},
            {"ASCII_Japanese_MSX-Write_II", "ASCII Japanese MSX-Write II"},
            {"msxdos2", "ASCII MSX-DOS 2"},
            {"AVT_DPF-550", "AVT Floppy Disk Drive DPF-550"},
            {"mu-PACK", "Bit²/ASCII μ·PACK BM-117"},
            {"Casio_KB-10", "Casio Docking station KB-10"},
            {"Casio_KB-7", "Casio Docking station KB-7"},
            {"Checkmark_FM_Stereo_PAK", "Checkmark FM Stereo PAK"},
            {"Computer_Mates_Disk_Interface", "Computer Mates Disk Interface"},
            {"ROM_Hunter_Mk2", "Daito Micom System ROM Hunter Mk2"},
            {"debugdevice", "Debug Device"},
            {"DDX_3.0", "Digital Design DDX 3.0"},
            {"megaram", "Digital Design MegaRAM"},
            {"MegaRAM_Disk", "Digital Design MegaRAM DISK"},
            {"smxwifi", "DucaSP SM-X WiFi"},
            {"Epson_Chakkari_Copy", "Epson Chakkari Copy"},
            {"ESE_Ramdisk", "ESE Artists' Factory ESE Ramdisk"},
            {"ESE_SCC", "ESE Artists' Factory ESE SCC"},
            {"ESE_WAVE-SCSI", "ESE Artists' Factory ESE WAVE-SCSI"},
            {"ESE_MEGA-SCSI", "ESE Artists' Factory MEGA SCSI"},
            {"FAC_MIDI_Interface", "FAC MIDI Interface"},
            {"rs232", "Generic MSX RS-232C"},
            {"audio", "Generic MSX-AUDIO"},
            {"audio2", "Generic MSX-AUDIO 2 (alternative ports)"},
            {"Gradiente_CT-80E", "Gradiente Cartão 80 Colunas CT-80E"},
            {"ASCII16-X_Mapper_XL", "Grauw ASCII16-X Mapper XL"},
            {"hirestimer", "hirestimer"},
            {"Hitachi_MPC-310H", "Hitachi 2DD Micro Floppydisk Drive + Controller MPC-310H"},
            {"Jun_Soft_DalSoRi_R2", "Jun Soft DalSoRi R2"},
            {"JVC_MSX_MIDI", "JVC MSX MIDI"},
            {"Konami_EC-701", "Konami Japanese Word Processor Cartridge EC-701"},
            {"Konami_EC-702", "Konami Kanji ROM Cartridge EC-702"},
            {"scc", "Konami SCC"},
            {"scc+", "Konami SCC+"},
            {"Konami_SD-Snatcher_Sound_Cartridge", "Konami SD-Snatcher Sound Cartridge RA-011"},
            {"Konami_Snatcher_Sound_Cartridge", "Konami Snatcher Sound Cartridge RA-004"},
            {"Lascom_Kanji", "Lascom Kanji ROM Cartridge"},
            {"OPL3Cartridge_Moonsound_compatible", "Luciano Sturaro OPL3 Cartridge (Moonsound compatible)"},
            {"OPL3Cartridge", "Luciano Sturaro OPL3 Cartridge (MSX-Audio compatible)"},
            {"MIDI_Interface_3", "M. Fujimoto MIDI Interface 3"},
            {"Konami_Ultimate_Collection", "Manuel Pazos Konami Ultimate Collection version 3"},
            {"MegaFlashROM_SCC", "Manuel Pazos MegaFlashROM SCC"},
            {"MegaFlashROM_SCC+", "Manuel Pazos MegaFlashROM SCC+"},
            {"MegaFlashROM_SCC+_SD", "Manuel Pazos MegaFlashROM SCC+ SD"},
            {"ram1mb", "Memory Mapper (1 MB)"},
            {"ram2mb", "Memory Mapper (2 MB)"},
            {"ram4mb", "Memory Mapper (4 MB)"},
            {"ram512k", "Memory Mapper (512 kB)"},
            {"Microsol_CDX-2", "Microsol Disk Drive CDX-2"},
            {"Mitsubishi_ML-30DC_ML-30FD", "Mitsubishi Micro Floppy Disk Controller/Drives ML-30DC/ML-30FD"},
            {"mbstereo", "Moonblaster Stereo"},
            {"Gouda_SCSI", "MSX Club Gouda Gouda Novaxis SCSI"},
            {"MSXPi", "MSXPi"},
            {"OPL3Cartridge2_mono", "MSXpro OPL3 Cartridge v2 (mono)"},
            {"OPL3Cartridge2_stereo", "MSXpro OPL3 Cartridge v2 (stereo)"},
            {"National_CF-SM003", "National Kanji Word Processor Unit CF-SM003"},
            {"Neos_MA-20R", "Neos Version Up Adapter MSX BASIC Vev. 2.0 MA-20 (R)"},
            {"Neos_MA-20V", "Neos Version Up Adapter V MA-20 (V)"},
            {"unapinet", "openMSXnet UnapiNet"},
            {"fmpac", "Panasoft FMPAC SW-M004"},
            {"pac", "Panasoft PAC SW-M001"},
            {"Panasonic_FS-FD1A", "Panasonic 3.5 inch Floppy Disk Drive FS-FD1A"},
            {"Panasonic_FS-CA1", "Panasonic MSX-AUDIO Unit FS-CA1"},
            {"Panasonic_FS-PW1", "Panasonic Word Processor with Kanji ROM Cartridge FS-PW1"},
            {"Password_Cartridge", "Password Cartridge"},
            {"Philips_NMS_1200", "Philips 2DD Micro Floppydisk Drive NMS 1200"},
            {"Philips_VY_0010", "Philips 3.5\" Disk Drive VY-0010"},
            {"Philips_NMS_1205", "Philips Music-Module NMS-1205"},
            {"Philips_VU_0040", "Philips Parallel Printer Interface VU 0040"},
            {"Boosted_audio", "Philips/MSXPro MSX-Audio"},
            {"Musical_Memory_Mapper", "Popolon + Repro Factory Musical Memory Mapper (1 MB)"},
            {"Popolon_Sound_Cartridge", "Popolon Sound Cartridge"},
            {"programmabledevice", "PVM Programmable Device"},
            {"ram16k", "RAM (16 kB)"},
            {"ram32k", "RAM (32 kB)"},
            {"ram64k", "RAM (64 kB)"},
            {"Carnivore2", "RBSC Carnivore2"},
            {"Sanyo_MFD-001", "Sanyo MSX Floppy Disk Unit MFD-001"},
            {"Sanyo_MRS-001", "Sanyo MSX RS-232C Interface Unit MRS-001"},
            {"Sharp_HB-4000", "Sharp Cartão 80 Colunas HB-4000"},
            {"Sharp_HB-3600", "Sharp Dual Disk Drive Controller HB-3600"},
            {"slotexpander", "Slot Expander"},
            {"Beer_IDE", "SOLiD Beer IDE"},
            {"Sony_HBD-20W", "Sony 2DD Micro Floppydisk Drive HBD-20W"},
            {"Sony_HBD-F1", "Sony 2DD Micro Floppydisk Drive HBD-F1"},
            {"Sony_HBI-55", "Sony Data Cartridge 4KBytes HBI-55"},
            {"Sony_HBI-J1", "Sony Kanji ROM Cartridge HBI-J1"},
            {"Sony_HBD-50", "Sony Micro Floppydisk Drive HBD-50"},
            {"Sony_HBK-30", "Sony Micro Floppydisk Drive HBK-30"},
            {"Sony_HBI-232", "Sony RS-232C Interface Cartridge HBI-232"},
            {"SensorKid", "Sony Sensor Kid Cartridge HBJ-H081C"},
            {"Spectravideo_SVI-707", "Spectravideo MSX Disk Drive (320K) SVI-707"},
            {"SunriseIDE_Nextor", "Sunrise ATA-IDE with Nextor"},
            {"ide", "Sunrise ATA-IDE"},
            {"gfx9000", "Sunrise GFX9000"},
            {"moonsound", "Sunrise MoonSound"},
            {"nowind", "Sunrise Nowind USB Interface"},
            {"video9000", "Sunrise Video9000"},
            {"Makoto", "SuperSoniqs / Electric River Hardware Makoto"},
            {"franky-ntsc", "SuperSoniqs Franky NTSC"},
            {"franky-pal", "SuperSoniqs Franky PAL"},
            {"Talent_TDC-600", "Talent disk drive TDC-600"},
            {"Talent_DPF-550", "Talent Floppy Disk Drive DPF-550"},
            {"Tecall_MSX_RAMFILE_TM220", "Tecall MSX RAMFILE TM220"},
            {"SN76489", "Texas Instruments Digital Complex Sound Generator SN76489"},
            {"The_SCC_Alliance_Yamanooto", "The SCC Alliance Yamanooto"},
            {"Toshiba_HX-F101PE", "Toshiba Micro Floppydisk Unit HX-F101PE"},
            {"Toshiba_HX-MU900", "Toshiba MSX Music System HX-MU900"},
            {"Yamaha_FD-03", "Yamaha 3.5inch Floppydisk Drive 1DD FD-03"},
            {"Yamaha_FD-05", "Yamaha 3.5Inch MSX Micro FDD FD-05/FD-051"},
            {"Yamaha_SFG-05", "Yamaha FM Sound Synthesizer Unit mark II SFG-05"},
            {"Yamaha_SFG-01", "Yamaha FM Sound Synthesizer Unit SFG-01"},
            {"Yamaha_SKW-05_embedded", "Yamaha Kanji Word Processor Unit (embedded) SKW-05"},
            {"Yamaha_SKW-01", "Yamaha Kanji Word Processor Unit SKW-01"},
            {NULL, NULL},
        },
        "none"
    },
    {
        "openmsx_ext6", "Extension 6 (Restart)", NULL,
        "A cartridge or device in a free slot, as openMSX's -ext. Some need their ROMs in <system>/openmsx/share/systemroms.",
        NULL, "system",
        {
            {"none", "None"},
            {"2nd_PSG", "2nd PSG"},
            {"advram", "Ademir Carchano ADVRAM"},
            {"MegaRAM_2MB", "Ademir Carchano MegaRAM (anniversary ed.)"},
            {"ASCII_MSX-DOS2", "ASCII Japanese MSX-DOS2"},
            {"ASCII_Japanese_MSX-Write", "ASCII Japanese MSX-Write"},
            {"ASCII_Japanese_MSX-Write_II", "ASCII Japanese MSX-Write II"},
            {"msxdos2", "ASCII MSX-DOS 2"},
            {"AVT_DPF-550", "AVT Floppy Disk Drive DPF-550"},
            {"mu-PACK", "Bit²/ASCII μ·PACK BM-117"},
            {"Casio_KB-10", "Casio Docking station KB-10"},
            {"Casio_KB-7", "Casio Docking station KB-7"},
            {"Checkmark_FM_Stereo_PAK", "Checkmark FM Stereo PAK"},
            {"Computer_Mates_Disk_Interface", "Computer Mates Disk Interface"},
            {"ROM_Hunter_Mk2", "Daito Micom System ROM Hunter Mk2"},
            {"debugdevice", "Debug Device"},
            {"DDX_3.0", "Digital Design DDX 3.0"},
            {"megaram", "Digital Design MegaRAM"},
            {"MegaRAM_Disk", "Digital Design MegaRAM DISK"},
            {"smxwifi", "DucaSP SM-X WiFi"},
            {"Epson_Chakkari_Copy", "Epson Chakkari Copy"},
            {"ESE_Ramdisk", "ESE Artists' Factory ESE Ramdisk"},
            {"ESE_SCC", "ESE Artists' Factory ESE SCC"},
            {"ESE_WAVE-SCSI", "ESE Artists' Factory ESE WAVE-SCSI"},
            {"ESE_MEGA-SCSI", "ESE Artists' Factory MEGA SCSI"},
            {"FAC_MIDI_Interface", "FAC MIDI Interface"},
            {"rs232", "Generic MSX RS-232C"},
            {"audio", "Generic MSX-AUDIO"},
            {"audio2", "Generic MSX-AUDIO 2 (alternative ports)"},
            {"Gradiente_CT-80E", "Gradiente Cartão 80 Colunas CT-80E"},
            {"ASCII16-X_Mapper_XL", "Grauw ASCII16-X Mapper XL"},
            {"hirestimer", "hirestimer"},
            {"Hitachi_MPC-310H", "Hitachi 2DD Micro Floppydisk Drive + Controller MPC-310H"},
            {"Jun_Soft_DalSoRi_R2", "Jun Soft DalSoRi R2"},
            {"JVC_MSX_MIDI", "JVC MSX MIDI"},
            {"Konami_EC-701", "Konami Japanese Word Processor Cartridge EC-701"},
            {"Konami_EC-702", "Konami Kanji ROM Cartridge EC-702"},
            {"scc", "Konami SCC"},
            {"scc+", "Konami SCC+"},
            {"Konami_SD-Snatcher_Sound_Cartridge", "Konami SD-Snatcher Sound Cartridge RA-011"},
            {"Konami_Snatcher_Sound_Cartridge", "Konami Snatcher Sound Cartridge RA-004"},
            {"Lascom_Kanji", "Lascom Kanji ROM Cartridge"},
            {"OPL3Cartridge_Moonsound_compatible", "Luciano Sturaro OPL3 Cartridge (Moonsound compatible)"},
            {"OPL3Cartridge", "Luciano Sturaro OPL3 Cartridge (MSX-Audio compatible)"},
            {"MIDI_Interface_3", "M. Fujimoto MIDI Interface 3"},
            {"Konami_Ultimate_Collection", "Manuel Pazos Konami Ultimate Collection version 3"},
            {"MegaFlashROM_SCC", "Manuel Pazos MegaFlashROM SCC"},
            {"MegaFlashROM_SCC+", "Manuel Pazos MegaFlashROM SCC+"},
            {"MegaFlashROM_SCC+_SD", "Manuel Pazos MegaFlashROM SCC+ SD"},
            {"ram1mb", "Memory Mapper (1 MB)"},
            {"ram2mb", "Memory Mapper (2 MB)"},
            {"ram4mb", "Memory Mapper (4 MB)"},
            {"ram512k", "Memory Mapper (512 kB)"},
            {"Microsol_CDX-2", "Microsol Disk Drive CDX-2"},
            {"Mitsubishi_ML-30DC_ML-30FD", "Mitsubishi Micro Floppy Disk Controller/Drives ML-30DC/ML-30FD"},
            {"mbstereo", "Moonblaster Stereo"},
            {"Gouda_SCSI", "MSX Club Gouda Gouda Novaxis SCSI"},
            {"MSXPi", "MSXPi"},
            {"OPL3Cartridge2_mono", "MSXpro OPL3 Cartridge v2 (mono)"},
            {"OPL3Cartridge2_stereo", "MSXpro OPL3 Cartridge v2 (stereo)"},
            {"National_CF-SM003", "National Kanji Word Processor Unit CF-SM003"},
            {"Neos_MA-20R", "Neos Version Up Adapter MSX BASIC Vev. 2.0 MA-20 (R)"},
            {"Neos_MA-20V", "Neos Version Up Adapter V MA-20 (V)"},
            {"unapinet", "openMSXnet UnapiNet"},
            {"fmpac", "Panasoft FMPAC SW-M004"},
            {"pac", "Panasoft PAC SW-M001"},
            {"Panasonic_FS-FD1A", "Panasonic 3.5 inch Floppy Disk Drive FS-FD1A"},
            {"Panasonic_FS-CA1", "Panasonic MSX-AUDIO Unit FS-CA1"},
            {"Panasonic_FS-PW1", "Panasonic Word Processor with Kanji ROM Cartridge FS-PW1"},
            {"Password_Cartridge", "Password Cartridge"},
            {"Philips_NMS_1200", "Philips 2DD Micro Floppydisk Drive NMS 1200"},
            {"Philips_VY_0010", "Philips 3.5\" Disk Drive VY-0010"},
            {"Philips_NMS_1205", "Philips Music-Module NMS-1205"},
            {"Philips_VU_0040", "Philips Parallel Printer Interface VU 0040"},
            {"Boosted_audio", "Philips/MSXPro MSX-Audio"},
            {"Musical_Memory_Mapper", "Popolon + Repro Factory Musical Memory Mapper (1 MB)"},
            {"Popolon_Sound_Cartridge", "Popolon Sound Cartridge"},
            {"programmabledevice", "PVM Programmable Device"},
            {"ram16k", "RAM (16 kB)"},
            {"ram32k", "RAM (32 kB)"},
            {"ram64k", "RAM (64 kB)"},
            {"Carnivore2", "RBSC Carnivore2"},
            {"Sanyo_MFD-001", "Sanyo MSX Floppy Disk Unit MFD-001"},
            {"Sanyo_MRS-001", "Sanyo MSX RS-232C Interface Unit MRS-001"},
            {"Sharp_HB-4000", "Sharp Cartão 80 Colunas HB-4000"},
            {"Sharp_HB-3600", "Sharp Dual Disk Drive Controller HB-3600"},
            {"slotexpander", "Slot Expander"},
            {"Beer_IDE", "SOLiD Beer IDE"},
            {"Sony_HBD-20W", "Sony 2DD Micro Floppydisk Drive HBD-20W"},
            {"Sony_HBD-F1", "Sony 2DD Micro Floppydisk Drive HBD-F1"},
            {"Sony_HBI-55", "Sony Data Cartridge 4KBytes HBI-55"},
            {"Sony_HBI-J1", "Sony Kanji ROM Cartridge HBI-J1"},
            {"Sony_HBD-50", "Sony Micro Floppydisk Drive HBD-50"},
            {"Sony_HBK-30", "Sony Micro Floppydisk Drive HBK-30"},
            {"Sony_HBI-232", "Sony RS-232C Interface Cartridge HBI-232"},
            {"SensorKid", "Sony Sensor Kid Cartridge HBJ-H081C"},
            {"Spectravideo_SVI-707", "Spectravideo MSX Disk Drive (320K) SVI-707"},
            {"SunriseIDE_Nextor", "Sunrise ATA-IDE with Nextor"},
            {"ide", "Sunrise ATA-IDE"},
            {"gfx9000", "Sunrise GFX9000"},
            {"moonsound", "Sunrise MoonSound"},
            {"nowind", "Sunrise Nowind USB Interface"},
            {"video9000", "Sunrise Video9000"},
            {"Makoto", "SuperSoniqs / Electric River Hardware Makoto"},
            {"franky-ntsc", "SuperSoniqs Franky NTSC"},
            {"franky-pal", "SuperSoniqs Franky PAL"},
            {"Talent_TDC-600", "Talent disk drive TDC-600"},
            {"Talent_DPF-550", "Talent Floppy Disk Drive DPF-550"},
            {"Tecall_MSX_RAMFILE_TM220", "Tecall MSX RAMFILE TM220"},
            {"SN76489", "Texas Instruments Digital Complex Sound Generator SN76489"},
            {"The_SCC_Alliance_Yamanooto", "The SCC Alliance Yamanooto"},
            {"Toshiba_HX-F101PE", "Toshiba Micro Floppydisk Unit HX-F101PE"},
            {"Toshiba_HX-MU900", "Toshiba MSX Music System HX-MU900"},
            {"Yamaha_FD-03", "Yamaha 3.5inch Floppydisk Drive 1DD FD-03"},
            {"Yamaha_FD-05", "Yamaha 3.5Inch MSX Micro FDD FD-05/FD-051"},
            {"Yamaha_SFG-05", "Yamaha FM Sound Synthesizer Unit mark II SFG-05"},
            {"Yamaha_SFG-01", "Yamaha FM Sound Synthesizer Unit SFG-01"},
            {"Yamaha_SKW-05_embedded", "Yamaha Kanji Word Processor Unit (embedded) SKW-05"},
            {"Yamaha_SKW-01", "Yamaha Kanji Word Processor Unit SKW-01"},
            {NULL, NULL},
        },
        "none"
    },
    {
        "openmsx_ext7", "Extension 7 (Restart)", NULL,
        "A cartridge or device in a free slot, as openMSX's -ext. Some need their ROMs in <system>/openmsx/share/systemroms.",
        NULL, "system",
        {
            {"none", "None"},
            {"2nd_PSG", "2nd PSG"},
            {"advram", "Ademir Carchano ADVRAM"},
            {"MegaRAM_2MB", "Ademir Carchano MegaRAM (anniversary ed.)"},
            {"ASCII_MSX-DOS2", "ASCII Japanese MSX-DOS2"},
            {"ASCII_Japanese_MSX-Write", "ASCII Japanese MSX-Write"},
            {"ASCII_Japanese_MSX-Write_II", "ASCII Japanese MSX-Write II"},
            {"msxdos2", "ASCII MSX-DOS 2"},
            {"AVT_DPF-550", "AVT Floppy Disk Drive DPF-550"},
            {"mu-PACK", "Bit²/ASCII μ·PACK BM-117"},
            {"Casio_KB-10", "Casio Docking station KB-10"},
            {"Casio_KB-7", "Casio Docking station KB-7"},
            {"Checkmark_FM_Stereo_PAK", "Checkmark FM Stereo PAK"},
            {"Computer_Mates_Disk_Interface", "Computer Mates Disk Interface"},
            {"ROM_Hunter_Mk2", "Daito Micom System ROM Hunter Mk2"},
            {"debugdevice", "Debug Device"},
            {"DDX_3.0", "Digital Design DDX 3.0"},
            {"megaram", "Digital Design MegaRAM"},
            {"MegaRAM_Disk", "Digital Design MegaRAM DISK"},
            {"smxwifi", "DucaSP SM-X WiFi"},
            {"Epson_Chakkari_Copy", "Epson Chakkari Copy"},
            {"ESE_Ramdisk", "ESE Artists' Factory ESE Ramdisk"},
            {"ESE_SCC", "ESE Artists' Factory ESE SCC"},
            {"ESE_WAVE-SCSI", "ESE Artists' Factory ESE WAVE-SCSI"},
            {"ESE_MEGA-SCSI", "ESE Artists' Factory MEGA SCSI"},
            {"FAC_MIDI_Interface", "FAC MIDI Interface"},
            {"rs232", "Generic MSX RS-232C"},
            {"audio", "Generic MSX-AUDIO"},
            {"audio2", "Generic MSX-AUDIO 2 (alternative ports)"},
            {"Gradiente_CT-80E", "Gradiente Cartão 80 Colunas CT-80E"},
            {"ASCII16-X_Mapper_XL", "Grauw ASCII16-X Mapper XL"},
            {"hirestimer", "hirestimer"},
            {"Hitachi_MPC-310H", "Hitachi 2DD Micro Floppydisk Drive + Controller MPC-310H"},
            {"Jun_Soft_DalSoRi_R2", "Jun Soft DalSoRi R2"},
            {"JVC_MSX_MIDI", "JVC MSX MIDI"},
            {"Konami_EC-701", "Konami Japanese Word Processor Cartridge EC-701"},
            {"Konami_EC-702", "Konami Kanji ROM Cartridge EC-702"},
            {"scc", "Konami SCC"},
            {"scc+", "Konami SCC+"},
            {"Konami_SD-Snatcher_Sound_Cartridge", "Konami SD-Snatcher Sound Cartridge RA-011"},
            {"Konami_Snatcher_Sound_Cartridge", "Konami Snatcher Sound Cartridge RA-004"},
            {"Lascom_Kanji", "Lascom Kanji ROM Cartridge"},
            {"OPL3Cartridge_Moonsound_compatible", "Luciano Sturaro OPL3 Cartridge (Moonsound compatible)"},
            {"OPL3Cartridge", "Luciano Sturaro OPL3 Cartridge (MSX-Audio compatible)"},
            {"MIDI_Interface_3", "M. Fujimoto MIDI Interface 3"},
            {"Konami_Ultimate_Collection", "Manuel Pazos Konami Ultimate Collection version 3"},
            {"MegaFlashROM_SCC", "Manuel Pazos MegaFlashROM SCC"},
            {"MegaFlashROM_SCC+", "Manuel Pazos MegaFlashROM SCC+"},
            {"MegaFlashROM_SCC+_SD", "Manuel Pazos MegaFlashROM SCC+ SD"},
            {"ram1mb", "Memory Mapper (1 MB)"},
            {"ram2mb", "Memory Mapper (2 MB)"},
            {"ram4mb", "Memory Mapper (4 MB)"},
            {"ram512k", "Memory Mapper (512 kB)"},
            {"Microsol_CDX-2", "Microsol Disk Drive CDX-2"},
            {"Mitsubishi_ML-30DC_ML-30FD", "Mitsubishi Micro Floppy Disk Controller/Drives ML-30DC/ML-30FD"},
            {"mbstereo", "Moonblaster Stereo"},
            {"Gouda_SCSI", "MSX Club Gouda Gouda Novaxis SCSI"},
            {"MSXPi", "MSXPi"},
            {"OPL3Cartridge2_mono", "MSXpro OPL3 Cartridge v2 (mono)"},
            {"OPL3Cartridge2_stereo", "MSXpro OPL3 Cartridge v2 (stereo)"},
            {"National_CF-SM003", "National Kanji Word Processor Unit CF-SM003"},
            {"Neos_MA-20R", "Neos Version Up Adapter MSX BASIC Vev. 2.0 MA-20 (R)"},
            {"Neos_MA-20V", "Neos Version Up Adapter V MA-20 (V)"},
            {"unapinet", "openMSXnet UnapiNet"},
            {"fmpac", "Panasoft FMPAC SW-M004"},
            {"pac", "Panasoft PAC SW-M001"},
            {"Panasonic_FS-FD1A", "Panasonic 3.5 inch Floppy Disk Drive FS-FD1A"},
            {"Panasonic_FS-CA1", "Panasonic MSX-AUDIO Unit FS-CA1"},
            {"Panasonic_FS-PW1", "Panasonic Word Processor with Kanji ROM Cartridge FS-PW1"},
            {"Password_Cartridge", "Password Cartridge"},
            {"Philips_NMS_1200", "Philips 2DD Micro Floppydisk Drive NMS 1200"},
            {"Philips_VY_0010", "Philips 3.5\" Disk Drive VY-0010"},
            {"Philips_NMS_1205", "Philips Music-Module NMS-1205"},
            {"Philips_VU_0040", "Philips Parallel Printer Interface VU 0040"},
            {"Boosted_audio", "Philips/MSXPro MSX-Audio"},
            {"Musical_Memory_Mapper", "Popolon + Repro Factory Musical Memory Mapper (1 MB)"},
            {"Popolon_Sound_Cartridge", "Popolon Sound Cartridge"},
            {"programmabledevice", "PVM Programmable Device"},
            {"ram16k", "RAM (16 kB)"},
            {"ram32k", "RAM (32 kB)"},
            {"ram64k", "RAM (64 kB)"},
            {"Carnivore2", "RBSC Carnivore2"},
            {"Sanyo_MFD-001", "Sanyo MSX Floppy Disk Unit MFD-001"},
            {"Sanyo_MRS-001", "Sanyo MSX RS-232C Interface Unit MRS-001"},
            {"Sharp_HB-4000", "Sharp Cartão 80 Colunas HB-4000"},
            {"Sharp_HB-3600", "Sharp Dual Disk Drive Controller HB-3600"},
            {"slotexpander", "Slot Expander"},
            {"Beer_IDE", "SOLiD Beer IDE"},
            {"Sony_HBD-20W", "Sony 2DD Micro Floppydisk Drive HBD-20W"},
            {"Sony_HBD-F1", "Sony 2DD Micro Floppydisk Drive HBD-F1"},
            {"Sony_HBI-55", "Sony Data Cartridge 4KBytes HBI-55"},
            {"Sony_HBI-J1", "Sony Kanji ROM Cartridge HBI-J1"},
            {"Sony_HBD-50", "Sony Micro Floppydisk Drive HBD-50"},
            {"Sony_HBK-30", "Sony Micro Floppydisk Drive HBK-30"},
            {"Sony_HBI-232", "Sony RS-232C Interface Cartridge HBI-232"},
            {"SensorKid", "Sony Sensor Kid Cartridge HBJ-H081C"},
            {"Spectravideo_SVI-707", "Spectravideo MSX Disk Drive (320K) SVI-707"},
            {"SunriseIDE_Nextor", "Sunrise ATA-IDE with Nextor"},
            {"ide", "Sunrise ATA-IDE"},
            {"gfx9000", "Sunrise GFX9000"},
            {"moonsound", "Sunrise MoonSound"},
            {"nowind", "Sunrise Nowind USB Interface"},
            {"video9000", "Sunrise Video9000"},
            {"Makoto", "SuperSoniqs / Electric River Hardware Makoto"},
            {"franky-ntsc", "SuperSoniqs Franky NTSC"},
            {"franky-pal", "SuperSoniqs Franky PAL"},
            {"Talent_TDC-600", "Talent disk drive TDC-600"},
            {"Talent_DPF-550", "Talent Floppy Disk Drive DPF-550"},
            {"Tecall_MSX_RAMFILE_TM220", "Tecall MSX RAMFILE TM220"},
            {"SN76489", "Texas Instruments Digital Complex Sound Generator SN76489"},
            {"The_SCC_Alliance_Yamanooto", "The SCC Alliance Yamanooto"},
            {"Toshiba_HX-F101PE", "Toshiba Micro Floppydisk Unit HX-F101PE"},
            {"Toshiba_HX-MU900", "Toshiba MSX Music System HX-MU900"},
            {"Yamaha_FD-03", "Yamaha 3.5inch Floppydisk Drive 1DD FD-03"},
            {"Yamaha_FD-05", "Yamaha 3.5Inch MSX Micro FDD FD-05/FD-051"},
            {"Yamaha_SFG-05", "Yamaha FM Sound Synthesizer Unit mark II SFG-05"},
            {"Yamaha_SFG-01", "Yamaha FM Sound Synthesizer Unit SFG-01"},
            {"Yamaha_SKW-05_embedded", "Yamaha Kanji Word Processor Unit (embedded) SKW-05"},
            {"Yamaha_SKW-01", "Yamaha Kanji Word Processor Unit SKW-01"},
            {NULL, NULL},
        },
        "none"
    },
    {
        "openmsx_ext8", "Extension 8 (Restart)", NULL,
        "A cartridge or device in a free slot, as openMSX's -ext. Some need their ROMs in <system>/openmsx/share/systemroms.",
        NULL, "system",
        {
            {"none", "None"},
            {"2nd_PSG", "2nd PSG"},
            {"advram", "Ademir Carchano ADVRAM"},
            {"MegaRAM_2MB", "Ademir Carchano MegaRAM (anniversary ed.)"},
            {"ASCII_MSX-DOS2", "ASCII Japanese MSX-DOS2"},
            {"ASCII_Japanese_MSX-Write", "ASCII Japanese MSX-Write"},
            {"ASCII_Japanese_MSX-Write_II", "ASCII Japanese MSX-Write II"},
            {"msxdos2", "ASCII MSX-DOS 2"},
            {"AVT_DPF-550", "AVT Floppy Disk Drive DPF-550"},
            {"mu-PACK", "Bit²/ASCII μ·PACK BM-117"},
            {"Casio_KB-10", "Casio Docking station KB-10"},
            {"Casio_KB-7", "Casio Docking station KB-7"},
            {"Checkmark_FM_Stereo_PAK", "Checkmark FM Stereo PAK"},
            {"Computer_Mates_Disk_Interface", "Computer Mates Disk Interface"},
            {"ROM_Hunter_Mk2", "Daito Micom System ROM Hunter Mk2"},
            {"debugdevice", "Debug Device"},
            {"DDX_3.0", "Digital Design DDX 3.0"},
            {"megaram", "Digital Design MegaRAM"},
            {"MegaRAM_Disk", "Digital Design MegaRAM DISK"},
            {"smxwifi", "DucaSP SM-X WiFi"},
            {"Epson_Chakkari_Copy", "Epson Chakkari Copy"},
            {"ESE_Ramdisk", "ESE Artists' Factory ESE Ramdisk"},
            {"ESE_SCC", "ESE Artists' Factory ESE SCC"},
            {"ESE_WAVE-SCSI", "ESE Artists' Factory ESE WAVE-SCSI"},
            {"ESE_MEGA-SCSI", "ESE Artists' Factory MEGA SCSI"},
            {"FAC_MIDI_Interface", "FAC MIDI Interface"},
            {"rs232", "Generic MSX RS-232C"},
            {"audio", "Generic MSX-AUDIO"},
            {"audio2", "Generic MSX-AUDIO 2 (alternative ports)"},
            {"Gradiente_CT-80E", "Gradiente Cartão 80 Colunas CT-80E"},
            {"ASCII16-X_Mapper_XL", "Grauw ASCII16-X Mapper XL"},
            {"hirestimer", "hirestimer"},
            {"Hitachi_MPC-310H", "Hitachi 2DD Micro Floppydisk Drive + Controller MPC-310H"},
            {"Jun_Soft_DalSoRi_R2", "Jun Soft DalSoRi R2"},
            {"JVC_MSX_MIDI", "JVC MSX MIDI"},
            {"Konami_EC-701", "Konami Japanese Word Processor Cartridge EC-701"},
            {"Konami_EC-702", "Konami Kanji ROM Cartridge EC-702"},
            {"scc", "Konami SCC"},
            {"scc+", "Konami SCC+"},
            {"Konami_SD-Snatcher_Sound_Cartridge", "Konami SD-Snatcher Sound Cartridge RA-011"},
            {"Konami_Snatcher_Sound_Cartridge", "Konami Snatcher Sound Cartridge RA-004"},
            {"Lascom_Kanji", "Lascom Kanji ROM Cartridge"},
            {"OPL3Cartridge_Moonsound_compatible", "Luciano Sturaro OPL3 Cartridge (Moonsound compatible)"},
            {"OPL3Cartridge", "Luciano Sturaro OPL3 Cartridge (MSX-Audio compatible)"},
            {"MIDI_Interface_3", "M. Fujimoto MIDI Interface 3"},
            {"Konami_Ultimate_Collection", "Manuel Pazos Konami Ultimate Collection version 3"},
            {"MegaFlashROM_SCC", "Manuel Pazos MegaFlashROM SCC"},
            {"MegaFlashROM_SCC+", "Manuel Pazos MegaFlashROM SCC+"},
            {"MegaFlashROM_SCC+_SD", "Manuel Pazos MegaFlashROM SCC+ SD"},
            {"ram1mb", "Memory Mapper (1 MB)"},
            {"ram2mb", "Memory Mapper (2 MB)"},
            {"ram4mb", "Memory Mapper (4 MB)"},
            {"ram512k", "Memory Mapper (512 kB)"},
            {"Microsol_CDX-2", "Microsol Disk Drive CDX-2"},
            {"Mitsubishi_ML-30DC_ML-30FD", "Mitsubishi Micro Floppy Disk Controller/Drives ML-30DC/ML-30FD"},
            {"mbstereo", "Moonblaster Stereo"},
            {"Gouda_SCSI", "MSX Club Gouda Gouda Novaxis SCSI"},
            {"MSXPi", "MSXPi"},
            {"OPL3Cartridge2_mono", "MSXpro OPL3 Cartridge v2 (mono)"},
            {"OPL3Cartridge2_stereo", "MSXpro OPL3 Cartridge v2 (stereo)"},
            {"National_CF-SM003", "National Kanji Word Processor Unit CF-SM003"},
            {"Neos_MA-20R", "Neos Version Up Adapter MSX BASIC Vev. 2.0 MA-20 (R)"},
            {"Neos_MA-20V", "Neos Version Up Adapter V MA-20 (V)"},
            {"unapinet", "openMSXnet UnapiNet"},
            {"fmpac", "Panasoft FMPAC SW-M004"},
            {"pac", "Panasoft PAC SW-M001"},
            {"Panasonic_FS-FD1A", "Panasonic 3.5 inch Floppy Disk Drive FS-FD1A"},
            {"Panasonic_FS-CA1", "Panasonic MSX-AUDIO Unit FS-CA1"},
            {"Panasonic_FS-PW1", "Panasonic Word Processor with Kanji ROM Cartridge FS-PW1"},
            {"Password_Cartridge", "Password Cartridge"},
            {"Philips_NMS_1200", "Philips 2DD Micro Floppydisk Drive NMS 1200"},
            {"Philips_VY_0010", "Philips 3.5\" Disk Drive VY-0010"},
            {"Philips_NMS_1205", "Philips Music-Module NMS-1205"},
            {"Philips_VU_0040", "Philips Parallel Printer Interface VU 0040"},
            {"Boosted_audio", "Philips/MSXPro MSX-Audio"},
            {"Musical_Memory_Mapper", "Popolon + Repro Factory Musical Memory Mapper (1 MB)"},
            {"Popolon_Sound_Cartridge", "Popolon Sound Cartridge"},
            {"programmabledevice", "PVM Programmable Device"},
            {"ram16k", "RAM (16 kB)"},
            {"ram32k", "RAM (32 kB)"},
            {"ram64k", "RAM (64 kB)"},
            {"Carnivore2", "RBSC Carnivore2"},
            {"Sanyo_MFD-001", "Sanyo MSX Floppy Disk Unit MFD-001"},
            {"Sanyo_MRS-001", "Sanyo MSX RS-232C Interface Unit MRS-001"},
            {"Sharp_HB-4000", "Sharp Cartão 80 Colunas HB-4000"},
            {"Sharp_HB-3600", "Sharp Dual Disk Drive Controller HB-3600"},
            {"slotexpander", "Slot Expander"},
            {"Beer_IDE", "SOLiD Beer IDE"},
            {"Sony_HBD-20W", "Sony 2DD Micro Floppydisk Drive HBD-20W"},
            {"Sony_HBD-F1", "Sony 2DD Micro Floppydisk Drive HBD-F1"},
            {"Sony_HBI-55", "Sony Data Cartridge 4KBytes HBI-55"},
            {"Sony_HBI-J1", "Sony Kanji ROM Cartridge HBI-J1"},
            {"Sony_HBD-50", "Sony Micro Floppydisk Drive HBD-50"},
            {"Sony_HBK-30", "Sony Micro Floppydisk Drive HBK-30"},
            {"Sony_HBI-232", "Sony RS-232C Interface Cartridge HBI-232"},
            {"SensorKid", "Sony Sensor Kid Cartridge HBJ-H081C"},
            {"Spectravideo_SVI-707", "Spectravideo MSX Disk Drive (320K) SVI-707"},
            {"SunriseIDE_Nextor", "Sunrise ATA-IDE with Nextor"},
            {"ide", "Sunrise ATA-IDE"},
            {"gfx9000", "Sunrise GFX9000"},
            {"moonsound", "Sunrise MoonSound"},
            {"nowind", "Sunrise Nowind USB Interface"},
            {"video9000", "Sunrise Video9000"},
            {"Makoto", "SuperSoniqs / Electric River Hardware Makoto"},
            {"franky-ntsc", "SuperSoniqs Franky NTSC"},
            {"franky-pal", "SuperSoniqs Franky PAL"},
            {"Talent_TDC-600", "Talent disk drive TDC-600"},
            {"Talent_DPF-550", "Talent Floppy Disk Drive DPF-550"},
            {"Tecall_MSX_RAMFILE_TM220", "Tecall MSX RAMFILE TM220"},
            {"SN76489", "Texas Instruments Digital Complex Sound Generator SN76489"},
            {"The_SCC_Alliance_Yamanooto", "The SCC Alliance Yamanooto"},
            {"Toshiba_HX-F101PE", "Toshiba Micro Floppydisk Unit HX-F101PE"},
            {"Toshiba_HX-MU900", "Toshiba MSX Music System HX-MU900"},
            {"Yamaha_FD-03", "Yamaha 3.5inch Floppydisk Drive 1DD FD-03"},
            {"Yamaha_FD-05", "Yamaha 3.5Inch MSX Micro FDD FD-05/FD-051"},
            {"Yamaha_SFG-05", "Yamaha FM Sound Synthesizer Unit mark II SFG-05"},
            {"Yamaha_SFG-01", "Yamaha FM Sound Synthesizer Unit SFG-01"},
            {"Yamaha_SKW-05_embedded", "Yamaha Kanji Word Processor Unit (embedded) SKW-05"},
            {"Yamaha_SKW-01", "Yamaha Kanji Word Processor Unit SKW-01"},
            {NULL, NULL},
        },
        "none"
    },
    {
        "openmsx_ext9", "Extension 9 (Restart)", NULL,
        "A cartridge or device in a free slot, as openMSX's -ext. Some need their ROMs in <system>/openmsx/share/systemroms.",
        NULL, "system",
        {
            {"none", "None"},
            {"2nd_PSG", "2nd PSG"},
            {"advram", "Ademir Carchano ADVRAM"},
            {"MegaRAM_2MB", "Ademir Carchano MegaRAM (anniversary ed.)"},
            {"ASCII_MSX-DOS2", "ASCII Japanese MSX-DOS2"},
            {"ASCII_Japanese_MSX-Write", "ASCII Japanese MSX-Write"},
            {"ASCII_Japanese_MSX-Write_II", "ASCII Japanese MSX-Write II"},
            {"msxdos2", "ASCII MSX-DOS 2"},
            {"AVT_DPF-550", "AVT Floppy Disk Drive DPF-550"},
            {"mu-PACK", "Bit²/ASCII μ·PACK BM-117"},
            {"Casio_KB-10", "Casio Docking station KB-10"},
            {"Casio_KB-7", "Casio Docking station KB-7"},
            {"Checkmark_FM_Stereo_PAK", "Checkmark FM Stereo PAK"},
            {"Computer_Mates_Disk_Interface", "Computer Mates Disk Interface"},
            {"ROM_Hunter_Mk2", "Daito Micom System ROM Hunter Mk2"},
            {"debugdevice", "Debug Device"},
            {"DDX_3.0", "Digital Design DDX 3.0"},
            {"megaram", "Digital Design MegaRAM"},
            {"MegaRAM_Disk", "Digital Design MegaRAM DISK"},
            {"smxwifi", "DucaSP SM-X WiFi"},
            {"Epson_Chakkari_Copy", "Epson Chakkari Copy"},
            {"ESE_Ramdisk", "ESE Artists' Factory ESE Ramdisk"},
            {"ESE_SCC", "ESE Artists' Factory ESE SCC"},
            {"ESE_WAVE-SCSI", "ESE Artists' Factory ESE WAVE-SCSI"},
            {"ESE_MEGA-SCSI", "ESE Artists' Factory MEGA SCSI"},
            {"FAC_MIDI_Interface", "FAC MIDI Interface"},
            {"rs232", "Generic MSX RS-232C"},
            {"audio", "Generic MSX-AUDIO"},
            {"audio2", "Generic MSX-AUDIO 2 (alternative ports)"},
            {"Gradiente_CT-80E", "Gradiente Cartão 80 Colunas CT-80E"},
            {"ASCII16-X_Mapper_XL", "Grauw ASCII16-X Mapper XL"},
            {"hirestimer", "hirestimer"},
            {"Hitachi_MPC-310H", "Hitachi 2DD Micro Floppydisk Drive + Controller MPC-310H"},
            {"Jun_Soft_DalSoRi_R2", "Jun Soft DalSoRi R2"},
            {"JVC_MSX_MIDI", "JVC MSX MIDI"},
            {"Konami_EC-701", "Konami Japanese Word Processor Cartridge EC-701"},
            {"Konami_EC-702", "Konami Kanji ROM Cartridge EC-702"},
            {"scc", "Konami SCC"},
            {"scc+", "Konami SCC+"},
            {"Konami_SD-Snatcher_Sound_Cartridge", "Konami SD-Snatcher Sound Cartridge RA-011"},
            {"Konami_Snatcher_Sound_Cartridge", "Konami Snatcher Sound Cartridge RA-004"},
            {"Lascom_Kanji", "Lascom Kanji ROM Cartridge"},
            {"OPL3Cartridge_Moonsound_compatible", "Luciano Sturaro OPL3 Cartridge (Moonsound compatible)"},
            {"OPL3Cartridge", "Luciano Sturaro OPL3 Cartridge (MSX-Audio compatible)"},
            {"MIDI_Interface_3", "M. Fujimoto MIDI Interface 3"},
            {"Konami_Ultimate_Collection", "Manuel Pazos Konami Ultimate Collection version 3"},
            {"MegaFlashROM_SCC", "Manuel Pazos MegaFlashROM SCC"},
            {"MegaFlashROM_SCC+", "Manuel Pazos MegaFlashROM SCC+"},
            {"MegaFlashROM_SCC+_SD", "Manuel Pazos MegaFlashROM SCC+ SD"},
            {"ram1mb", "Memory Mapper (1 MB)"},
            {"ram2mb", "Memory Mapper (2 MB)"},
            {"ram4mb", "Memory Mapper (4 MB)"},
            {"ram512k", "Memory Mapper (512 kB)"},
            {"Microsol_CDX-2", "Microsol Disk Drive CDX-2"},
            {"Mitsubishi_ML-30DC_ML-30FD", "Mitsubishi Micro Floppy Disk Controller/Drives ML-30DC/ML-30FD"},
            {"mbstereo", "Moonblaster Stereo"},
            {"Gouda_SCSI", "MSX Club Gouda Gouda Novaxis SCSI"},
            {"MSXPi", "MSXPi"},
            {"OPL3Cartridge2_mono", "MSXpro OPL3 Cartridge v2 (mono)"},
            {"OPL3Cartridge2_stereo", "MSXpro OPL3 Cartridge v2 (stereo)"},
            {"National_CF-SM003", "National Kanji Word Processor Unit CF-SM003"},
            {"Neos_MA-20R", "Neos Version Up Adapter MSX BASIC Vev. 2.0 MA-20 (R)"},
            {"Neos_MA-20V", "Neos Version Up Adapter V MA-20 (V)"},
            {"unapinet", "openMSXnet UnapiNet"},
            {"fmpac", "Panasoft FMPAC SW-M004"},
            {"pac", "Panasoft PAC SW-M001"},
            {"Panasonic_FS-FD1A", "Panasonic 3.5 inch Floppy Disk Drive FS-FD1A"},
            {"Panasonic_FS-CA1", "Panasonic MSX-AUDIO Unit FS-CA1"},
            {"Panasonic_FS-PW1", "Panasonic Word Processor with Kanji ROM Cartridge FS-PW1"},
            {"Password_Cartridge", "Password Cartridge"},
            {"Philips_NMS_1200", "Philips 2DD Micro Floppydisk Drive NMS 1200"},
            {"Philips_VY_0010", "Philips 3.5\" Disk Drive VY-0010"},
            {"Philips_NMS_1205", "Philips Music-Module NMS-1205"},
            {"Philips_VU_0040", "Philips Parallel Printer Interface VU 0040"},
            {"Boosted_audio", "Philips/MSXPro MSX-Audio"},
            {"Musical_Memory_Mapper", "Popolon + Repro Factory Musical Memory Mapper (1 MB)"},
            {"Popolon_Sound_Cartridge", "Popolon Sound Cartridge"},
            {"programmabledevice", "PVM Programmable Device"},
            {"ram16k", "RAM (16 kB)"},
            {"ram32k", "RAM (32 kB)"},
            {"ram64k", "RAM (64 kB)"},
            {"Carnivore2", "RBSC Carnivore2"},
            {"Sanyo_MFD-001", "Sanyo MSX Floppy Disk Unit MFD-001"},
            {"Sanyo_MRS-001", "Sanyo MSX RS-232C Interface Unit MRS-001"},
            {"Sharp_HB-4000", "Sharp Cartão 80 Colunas HB-4000"},
            {"Sharp_HB-3600", "Sharp Dual Disk Drive Controller HB-3600"},
            {"slotexpander", "Slot Expander"},
            {"Beer_IDE", "SOLiD Beer IDE"},
            {"Sony_HBD-20W", "Sony 2DD Micro Floppydisk Drive HBD-20W"},
            {"Sony_HBD-F1", "Sony 2DD Micro Floppydisk Drive HBD-F1"},
            {"Sony_HBI-55", "Sony Data Cartridge 4KBytes HBI-55"},
            {"Sony_HBI-J1", "Sony Kanji ROM Cartridge HBI-J1"},
            {"Sony_HBD-50", "Sony Micro Floppydisk Drive HBD-50"},
            {"Sony_HBK-30", "Sony Micro Floppydisk Drive HBK-30"},
            {"Sony_HBI-232", "Sony RS-232C Interface Cartridge HBI-232"},
            {"SensorKid", "Sony Sensor Kid Cartridge HBJ-H081C"},
            {"Spectravideo_SVI-707", "Spectravideo MSX Disk Drive (320K) SVI-707"},
            {"SunriseIDE_Nextor", "Sunrise ATA-IDE with Nextor"},
            {"ide", "Sunrise ATA-IDE"},
            {"gfx9000", "Sunrise GFX9000"},
            {"moonsound", "Sunrise MoonSound"},
            {"nowind", "Sunrise Nowind USB Interface"},
            {"video9000", "Sunrise Video9000"},
            {"Makoto", "SuperSoniqs / Electric River Hardware Makoto"},
            {"franky-ntsc", "SuperSoniqs Franky NTSC"},
            {"franky-pal", "SuperSoniqs Franky PAL"},
            {"Talent_TDC-600", "Talent disk drive TDC-600"},
            {"Talent_DPF-550", "Talent Floppy Disk Drive DPF-550"},
            {"Tecall_MSX_RAMFILE_TM220", "Tecall MSX RAMFILE TM220"},
            {"SN76489", "Texas Instruments Digital Complex Sound Generator SN76489"},
            {"The_SCC_Alliance_Yamanooto", "The SCC Alliance Yamanooto"},
            {"Toshiba_HX-F101PE", "Toshiba Micro Floppydisk Unit HX-F101PE"},
            {"Toshiba_HX-MU900", "Toshiba MSX Music System HX-MU900"},
            {"Yamaha_FD-03", "Yamaha 3.5inch Floppydisk Drive 1DD FD-03"},
            {"Yamaha_FD-05", "Yamaha 3.5Inch MSX Micro FDD FD-05/FD-051"},
            {"Yamaha_SFG-05", "Yamaha FM Sound Synthesizer Unit mark II SFG-05"},
            {"Yamaha_SFG-01", "Yamaha FM Sound Synthesizer Unit SFG-01"},
            {"Yamaha_SKW-05_embedded", "Yamaha Kanji Word Processor Unit (embedded) SKW-05"},
            {"Yamaha_SKW-01", "Yamaha Kanji Word Processor Unit SKW-01"},
            {NULL, NULL},
        },
        "none"
    },
    {
        "openmsx_ext10", "Extension 10 (Restart)", NULL,
        "A cartridge or device in a free slot, as openMSX's -ext. Some need their ROMs in <system>/openmsx/share/systemroms.",
        NULL, "system",
        {
            {"none", "None"},
            {"2nd_PSG", "2nd PSG"},
            {"advram", "Ademir Carchano ADVRAM"},
            {"MegaRAM_2MB", "Ademir Carchano MegaRAM (anniversary ed.)"},
            {"ASCII_MSX-DOS2", "ASCII Japanese MSX-DOS2"},
            {"ASCII_Japanese_MSX-Write", "ASCII Japanese MSX-Write"},
            {"ASCII_Japanese_MSX-Write_II", "ASCII Japanese MSX-Write II"},
            {"msxdos2", "ASCII MSX-DOS 2"},
            {"AVT_DPF-550", "AVT Floppy Disk Drive DPF-550"},
            {"mu-PACK", "Bit²/ASCII μ·PACK BM-117"},
            {"Casio_KB-10", "Casio Docking station KB-10"},
            {"Casio_KB-7", "Casio Docking station KB-7"},
            {"Checkmark_FM_Stereo_PAK", "Checkmark FM Stereo PAK"},
            {"Computer_Mates_Disk_Interface", "Computer Mates Disk Interface"},
            {"ROM_Hunter_Mk2", "Daito Micom System ROM Hunter Mk2"},
            {"debugdevice", "Debug Device"},
            {"DDX_3.0", "Digital Design DDX 3.0"},
            {"megaram", "Digital Design MegaRAM"},
            {"MegaRAM_Disk", "Digital Design MegaRAM DISK"},
            {"smxwifi", "DucaSP SM-X WiFi"},
            {"Epson_Chakkari_Copy", "Epson Chakkari Copy"},
            {"ESE_Ramdisk", "ESE Artists' Factory ESE Ramdisk"},
            {"ESE_SCC", "ESE Artists' Factory ESE SCC"},
            {"ESE_WAVE-SCSI", "ESE Artists' Factory ESE WAVE-SCSI"},
            {"ESE_MEGA-SCSI", "ESE Artists' Factory MEGA SCSI"},
            {"FAC_MIDI_Interface", "FAC MIDI Interface"},
            {"rs232", "Generic MSX RS-232C"},
            {"audio", "Generic MSX-AUDIO"},
            {"audio2", "Generic MSX-AUDIO 2 (alternative ports)"},
            {"Gradiente_CT-80E", "Gradiente Cartão 80 Colunas CT-80E"},
            {"ASCII16-X_Mapper_XL", "Grauw ASCII16-X Mapper XL"},
            {"hirestimer", "hirestimer"},
            {"Hitachi_MPC-310H", "Hitachi 2DD Micro Floppydisk Drive + Controller MPC-310H"},
            {"Jun_Soft_DalSoRi_R2", "Jun Soft DalSoRi R2"},
            {"JVC_MSX_MIDI", "JVC MSX MIDI"},
            {"Konami_EC-701", "Konami Japanese Word Processor Cartridge EC-701"},
            {"Konami_EC-702", "Konami Kanji ROM Cartridge EC-702"},
            {"scc", "Konami SCC"},
            {"scc+", "Konami SCC+"},
            {"Konami_SD-Snatcher_Sound_Cartridge", "Konami SD-Snatcher Sound Cartridge RA-011"},
            {"Konami_Snatcher_Sound_Cartridge", "Konami Snatcher Sound Cartridge RA-004"},
            {"Lascom_Kanji", "Lascom Kanji ROM Cartridge"},
            {"OPL3Cartridge_Moonsound_compatible", "Luciano Sturaro OPL3 Cartridge (Moonsound compatible)"},
            {"OPL3Cartridge", "Luciano Sturaro OPL3 Cartridge (MSX-Audio compatible)"},
            {"MIDI_Interface_3", "M. Fujimoto MIDI Interface 3"},
            {"Konami_Ultimate_Collection", "Manuel Pazos Konami Ultimate Collection version 3"},
            {"MegaFlashROM_SCC", "Manuel Pazos MegaFlashROM SCC"},
            {"MegaFlashROM_SCC+", "Manuel Pazos MegaFlashROM SCC+"},
            {"MegaFlashROM_SCC+_SD", "Manuel Pazos MegaFlashROM SCC+ SD"},
            {"ram1mb", "Memory Mapper (1 MB)"},
            {"ram2mb", "Memory Mapper (2 MB)"},
            {"ram4mb", "Memory Mapper (4 MB)"},
            {"ram512k", "Memory Mapper (512 kB)"},
            {"Microsol_CDX-2", "Microsol Disk Drive CDX-2"},
            {"Mitsubishi_ML-30DC_ML-30FD", "Mitsubishi Micro Floppy Disk Controller/Drives ML-30DC/ML-30FD"},
            {"mbstereo", "Moonblaster Stereo"},
            {"Gouda_SCSI", "MSX Club Gouda Gouda Novaxis SCSI"},
            {"MSXPi", "MSXPi"},
            {"OPL3Cartridge2_mono", "MSXpro OPL3 Cartridge v2 (mono)"},
            {"OPL3Cartridge2_stereo", "MSXpro OPL3 Cartridge v2 (stereo)"},
            {"National_CF-SM003", "National Kanji Word Processor Unit CF-SM003"},
            {"Neos_MA-20R", "Neos Version Up Adapter MSX BASIC Vev. 2.0 MA-20 (R)"},
            {"Neos_MA-20V", "Neos Version Up Adapter V MA-20 (V)"},
            {"unapinet", "openMSXnet UnapiNet"},
            {"fmpac", "Panasoft FMPAC SW-M004"},
            {"pac", "Panasoft PAC SW-M001"},
            {"Panasonic_FS-FD1A", "Panasonic 3.5 inch Floppy Disk Drive FS-FD1A"},
            {"Panasonic_FS-CA1", "Panasonic MSX-AUDIO Unit FS-CA1"},
            {"Panasonic_FS-PW1", "Panasonic Word Processor with Kanji ROM Cartridge FS-PW1"},
            {"Password_Cartridge", "Password Cartridge"},
            {"Philips_NMS_1200", "Philips 2DD Micro Floppydisk Drive NMS 1200"},
            {"Philips_VY_0010", "Philips 3.5\" Disk Drive VY-0010"},
            {"Philips_NMS_1205", "Philips Music-Module NMS-1205"},
            {"Philips_VU_0040", "Philips Parallel Printer Interface VU 0040"},
            {"Boosted_audio", "Philips/MSXPro MSX-Audio"},
            {"Musical_Memory_Mapper", "Popolon + Repro Factory Musical Memory Mapper (1 MB)"},
            {"Popolon_Sound_Cartridge", "Popolon Sound Cartridge"},
            {"programmabledevice", "PVM Programmable Device"},
            {"ram16k", "RAM (16 kB)"},
            {"ram32k", "RAM (32 kB)"},
            {"ram64k", "RAM (64 kB)"},
            {"Carnivore2", "RBSC Carnivore2"},
            {"Sanyo_MFD-001", "Sanyo MSX Floppy Disk Unit MFD-001"},
            {"Sanyo_MRS-001", "Sanyo MSX RS-232C Interface Unit MRS-001"},
            {"Sharp_HB-4000", "Sharp Cartão 80 Colunas HB-4000"},
            {"Sharp_HB-3600", "Sharp Dual Disk Drive Controller HB-3600"},
            {"slotexpander", "Slot Expander"},
            {"Beer_IDE", "SOLiD Beer IDE"},
            {"Sony_HBD-20W", "Sony 2DD Micro Floppydisk Drive HBD-20W"},
            {"Sony_HBD-F1", "Sony 2DD Micro Floppydisk Drive HBD-F1"},
            {"Sony_HBI-55", "Sony Data Cartridge 4KBytes HBI-55"},
            {"Sony_HBI-J1", "Sony Kanji ROM Cartridge HBI-J1"},
            {"Sony_HBD-50", "Sony Micro Floppydisk Drive HBD-50"},
            {"Sony_HBK-30", "Sony Micro Floppydisk Drive HBK-30"},
            {"Sony_HBI-232", "Sony RS-232C Interface Cartridge HBI-232"},
            {"SensorKid", "Sony Sensor Kid Cartridge HBJ-H081C"},
            {"Spectravideo_SVI-707", "Spectravideo MSX Disk Drive (320K) SVI-707"},
            {"SunriseIDE_Nextor", "Sunrise ATA-IDE with Nextor"},
            {"ide", "Sunrise ATA-IDE"},
            {"gfx9000", "Sunrise GFX9000"},
            {"moonsound", "Sunrise MoonSound"},
            {"nowind", "Sunrise Nowind USB Interface"},
            {"video9000", "Sunrise Video9000"},
            {"Makoto", "SuperSoniqs / Electric River Hardware Makoto"},
            {"franky-ntsc", "SuperSoniqs Franky NTSC"},
            {"franky-pal", "SuperSoniqs Franky PAL"},
            {"Talent_TDC-600", "Talent disk drive TDC-600"},
            {"Talent_DPF-550", "Talent Floppy Disk Drive DPF-550"},
            {"Tecall_MSX_RAMFILE_TM220", "Tecall MSX RAMFILE TM220"},
            {"SN76489", "Texas Instruments Digital Complex Sound Generator SN76489"},
            {"The_SCC_Alliance_Yamanooto", "The SCC Alliance Yamanooto"},
            {"Toshiba_HX-F101PE", "Toshiba Micro Floppydisk Unit HX-F101PE"},
            {"Toshiba_HX-MU900", "Toshiba MSX Music System HX-MU900"},
            {"Yamaha_FD-03", "Yamaha 3.5inch Floppydisk Drive 1DD FD-03"},
            {"Yamaha_FD-05", "Yamaha 3.5Inch MSX Micro FDD FD-05/FD-051"},
            {"Yamaha_SFG-05", "Yamaha FM Sound Synthesizer Unit mark II SFG-05"},
            {"Yamaha_SFG-01", "Yamaha FM Sound Synthesizer Unit SFG-01"},
            {"Yamaha_SKW-05_embedded", "Yamaha Kanji Word Processor Unit (embedded) SKW-05"},
            {"Yamaha_SKW-01", "Yamaha Kanji Word Processor Unit SKW-01"},
            {NULL, NULL},
        },
        "none"
    },
    {NULL, NULL, NULL, NULL, NULL, NULL, {{NULL, NULL}}, NULL},
};

struct retro_core_options_v2 options_us = {
   option_cats_us,
   option_defs_us
};

/*
 ********************************
 * Language Mapping
 ********************************
*/

#ifndef HAVE_NO_LANGEXTRA
struct retro_core_options_v2 *options_intl[RETRO_LANGUAGE_LAST] = {
   &options_us,       /* RETRO_LANGUAGE_ENGLISH */
   &options_ja,       /* RETRO_LANGUAGE_JAPANESE */
   &options_fr,       /* RETRO_LANGUAGE_FRENCH */
   &options_es,       /* RETRO_LANGUAGE_SPANISH */
   &options_de,       /* RETRO_LANGUAGE_GERMAN */
   &options_it,       /* RETRO_LANGUAGE_ITALIAN */
   &options_nl,       /* RETRO_LANGUAGE_DUTCH */
   &options_pt_br,    /* RETRO_LANGUAGE_PORTUGUESE_BRAZIL */
   &options_pt_pt,    /* RETRO_LANGUAGE_PORTUGUESE_PORTUGAL */
   &options_ru,       /* RETRO_LANGUAGE_RUSSIAN */
   &options_ko,       /* RETRO_LANGUAGE_KOREAN */
   &options_cht,      /* RETRO_LANGUAGE_CHINESE_TRADITIONAL */
   &options_chs,      /* RETRO_LANGUAGE_CHINESE_SIMPLIFIED */
   &options_eo,       /* RETRO_LANGUAGE_ESPERANTO */
   &options_pl,       /* RETRO_LANGUAGE_POLISH */
   &options_vn,       /* RETRO_LANGUAGE_VIETNAMESE */
   &options_ar,       /* RETRO_LANGUAGE_ARABIC */
   &options_el,       /* RETRO_LANGUAGE_GREEK */
   &options_tr,       /* RETRO_LANGUAGE_TURKISH */
   &options_sk,       /* RETRO_LANGUAGE_SLOVAK */
   &options_fa,       /* RETRO_LANGUAGE_PERSIAN */
   &options_he,       /* RETRO_LANGUAGE_HEBREW */
   &options_ast,      /* RETRO_LANGUAGE_ASTURIAN */
   &options_fi,       /* RETRO_LANGUAGE_FINNISH */
   &options_id,       /* RETRO_LANGUAGE_INDONESIAN */
   &options_sv,       /* RETRO_LANGUAGE_SWEDISH */
   &options_uk,       /* RETRO_LANGUAGE_UKRAINIAN */
   &options_cs,       /* RETRO_LANGUAGE_CZECH */
   &options_val,      /* RETRO_LANGUAGE_CATALAN_VALENCIA */
   &options_ca,       /* RETRO_LANGUAGE_CATALAN */
   &options_en,       /* RETRO_LANGUAGE_BRITISH_ENGLISH */
   &options_hu,       /* RETRO_LANGUAGE_HUNGARIAN */
   &options_be,       /* RETRO_LANGUAGE_BELARUSIAN */
   &options_gl,       /* RETRO_LANGUAGE_GALICIAN */
   &options_no,       /* RETRO_LANGUAGE_NORWEGIAN */
   &options_ga,       /* RETRO_LANGUAGE_IRISH */
};
#endif

/*
 ********************************
 * Functions
 ********************************
*/

/* Handles configuration/setting of core options.
 * Should be called as early as possible - ideally inside
 * retro_set_environment(), and no later than retro_load_game()
 * > We place the function body in the header to avoid the
 *   necessity of adding more .c files (i.e. want this to
 *   be as painless as possible for core devs)
 */

static inline void libretro_set_core_options(retro_environment_t environ_cb,
      bool *categories_supported)
{
   unsigned version  = 0;
#ifndef HAVE_NO_LANGEXTRA
   unsigned language = 0;
#endif

   if (!environ_cb || !categories_supported)
      return;

   *categories_supported = false;

   if (!environ_cb(RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION, &version))
      version = 0;

   if (version >= 2)
   {
#ifndef HAVE_NO_LANGEXTRA
      struct retro_core_options_v2_intl core_options_intl;

      core_options_intl.us    = &options_us;
      core_options_intl.local = NULL;

      if (environ_cb(RETRO_ENVIRONMENT_GET_LANGUAGE, &language) &&
          (language < RETRO_LANGUAGE_LAST) && (language != RETRO_LANGUAGE_ENGLISH))
         core_options_intl.local = options_intl[language];

      *categories_supported = environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2_INTL,
            &core_options_intl);
#else
      *categories_supported = environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2,
            &options_us);
#endif
   }
   else
   {
      size_t i, j;
      size_t option_index              = 0;
      size_t num_options               = 0;
      struct retro_core_option_definition
            *option_v1_defs_us         = NULL;
#ifndef HAVE_NO_LANGEXTRA
      size_t num_options_intl          = 0;
      struct retro_core_option_v2_definition
            *option_defs_intl          = NULL;
      struct retro_core_option_definition
            *option_v1_defs_intl       = NULL;
      struct retro_core_options_intl
            core_options_v1_intl;
#endif
      struct retro_variable *variables = NULL;
      char **values_buf                = NULL;

      /* Determine total number of options */
      while (true)
      {
         if (option_defs_us[num_options].key)
            num_options++;
         else
            break;
      }

      if (version >= 1)
      {
         /* Allocate US array */
         option_v1_defs_us = (struct retro_core_option_definition *)
               calloc(num_options + 1, sizeof(struct retro_core_option_definition));

         /* Copy parameters from option_defs_us array */
         for (i = 0; i < num_options; i++)
         {
            struct retro_core_option_v2_definition *option_def_us = &option_defs_us[i];
            struct retro_core_option_value *option_values         = option_def_us->values;
            struct retro_core_option_definition *option_v1_def_us = &option_v1_defs_us[i];
            struct retro_core_option_value *option_v1_values      = option_v1_def_us->values;

            option_v1_def_us->key           = option_def_us->key;
            option_v1_def_us->desc          = option_def_us->desc;
            option_v1_def_us->info          = option_def_us->info;
            option_v1_def_us->default_value = option_def_us->default_value;

            /* Values must be copied individually... */
            while (option_values->value)
            {
               option_v1_values->value = option_values->value;
               option_v1_values->label = option_values->label;

               option_values++;
               option_v1_values++;
            }
         }

#ifndef HAVE_NO_LANGEXTRA
         if (environ_cb(RETRO_ENVIRONMENT_GET_LANGUAGE, &language) &&
             (language < RETRO_LANGUAGE_LAST) && (language != RETRO_LANGUAGE_ENGLISH) &&
             options_intl[language])
            option_defs_intl = options_intl[language]->definitions;

         if (option_defs_intl)
         {
            /* Determine number of intl options */
            while (true)
            {
               if (option_defs_intl[num_options_intl].key)
                  num_options_intl++;
               else
                  break;
            }

            /* Allocate intl array */
            option_v1_defs_intl = (struct retro_core_option_definition *)
                  calloc(num_options_intl + 1, sizeof(struct retro_core_option_definition));

            /* Copy parameters from option_defs_intl array */
            for (i = 0; i < num_options_intl; i++)
            {
               struct retro_core_option_v2_definition *option_def_intl = &option_defs_intl[i];
               struct retro_core_option_value *option_values           = option_def_intl->values;
               struct retro_core_option_definition *option_v1_def_intl = &option_v1_defs_intl[i];
               struct retro_core_option_value *option_v1_values        = option_v1_def_intl->values;

               option_v1_def_intl->key           = option_def_intl->key;
               option_v1_def_intl->desc          = option_def_intl->desc;
               option_v1_def_intl->info          = option_def_intl->info;
               option_v1_def_intl->default_value = option_def_intl->default_value;

               /* Values must be copied individually... */
               while (option_values->value)
               {
                  option_v1_values->value = option_values->value;
                  option_v1_values->label = option_values->label;

                  option_values++;
                  option_v1_values++;
               }
            }
         }

         core_options_v1_intl.us    = option_v1_defs_us;
         core_options_v1_intl.local = option_v1_defs_intl;

         environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_INTL, &core_options_v1_intl);
#else
         environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS, option_v1_defs_us);
#endif
      }
      else
      {
         /* Allocate arrays */
         variables  = (struct retro_variable *)calloc(num_options + 1,
               sizeof(struct retro_variable));
         values_buf = (char **)calloc(num_options, sizeof(char *));

         if (!variables || !values_buf)
            goto error;

         /* Copy parameters from option_defs_us array */
         for (i = 0; i < num_options; i++)
         {
            const char *key                        = option_defs_us[i].key;
            const char *desc                       = option_defs_us[i].desc;
            const char *default_value              = option_defs_us[i].default_value;
            struct retro_core_option_value *values = option_defs_us[i].values;
            size_t buf_len                         = 3;
            size_t default_index                   = 0;

            values_buf[i] = NULL;

            if (desc)
            {
               size_t num_values = 0;

               /* Determine number of values */
               while (true)
               {
                  if (values[num_values].value)
                  {
                     /* Check if this is the default value */
                     if (default_value)
                        if (strcmp(values[num_values].value, default_value) == 0)
                           default_index = num_values;

                     buf_len += strlen(values[num_values].value);
                     num_values++;
                  }
                  else
                     break;
               }

               /* Build values string */
               if (num_values > 0)
               {
                  buf_len += num_values - 1;
                  buf_len += strlen(desc);

                  values_buf[i] = (char *)calloc(buf_len, sizeof(char));
                  if (!values_buf[i])
                     goto error;

                  strcpy(values_buf[i], desc);
                  strcat(values_buf[i], "; ");

                  /* Default value goes first */
                  strcat(values_buf[i], values[default_index].value);

                  /* Add remaining values */
                  for (j = 0; j < num_values; j++)
                  {
                     if (j != default_index)
                     {
                        strcat(values_buf[i], "|");
                        strcat(values_buf[i], values[j].value);
                     }
                  }
               }
            }

            variables[option_index].key   = key;
            variables[option_index].value = values_buf[i];
            option_index++;
         }

         /* Set variables */
         environ_cb(RETRO_ENVIRONMENT_SET_VARIABLES, variables);
      }

error:
      /* Clean up */

      if (option_v1_defs_us)
      {
         free(option_v1_defs_us);
         option_v1_defs_us = NULL;
      }

#ifndef HAVE_NO_LANGEXTRA
      if (option_v1_defs_intl)
      {
         free(option_v1_defs_intl);
         option_v1_defs_intl = NULL;
      }
#endif

      if (values_buf)
      {
         for (i = 0; i < num_options; i++)
         {
            if (values_buf[i])
            {
               free(values_buf[i]);
               values_buf[i] = NULL;
            }
         }

         free(values_buf);
         values_buf = NULL;
      }

      if (variables)
      {
         free(variables);
         variables = NULL;
      }
   }
}

#ifdef __cplusplus
}
#endif

#endif
