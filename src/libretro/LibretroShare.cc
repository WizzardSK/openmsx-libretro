#include "LibretroShare.hh"

#include "File.hh"
#include "FileException.hh"
#include "FileOperations.hh"
#include "MSXException.hh"
#include "strCat.hh"

#include <zlib.h>

#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace {
#include "libretro_share.ii"
}

namespace openmsx {

static std::string marker(const std::string& shareDir)
{
	return strCat(shareDir, "/.libretro-share");
}

static bool upToDate(const std::string& shareDir, std::string_view stamp)
{
	try {
		File file(marker(shareDir));
		std::vector<uint8_t> data(file.getSize());
		file.read(std::span{data});
		return std::string_view(reinterpret_cast<const char*>(data.data()), data.size()) == stamp;
	} catch (FileException&) {
		return false;
	}
}

int installShare(const std::string& shareDir)
{
	const std::string stamp = strCat("crc ", shareCrc, '\n');
	if (upToDate(shareDir, stamp)) return 0;

	std::vector<uint8_t> raw(shareRawSize);
	uLongf rawSize = uLongf(raw.size());
	if (uncompress(raw.data(), &rawSize, shareData, uLong(sizeof(shareData))) != Z_OK
	    || rawSize != raw.size()) {
		throw MSXException("the share data the core carries is damaged");
	}

	auto take = [&](size_t& at, size_t bytes) {
		if (bytes > raw.size() - at) {
			throw MSXException("the share data the core carries is damaged");
		}
		std::span<const uint8_t> part(raw.data() + at, bytes);
		at += bytes;
		return part;
	};
	auto number = [&](size_t& at, size_t bytes) {
		auto part = take(at, bytes);
		uint64_t value = 0;
		for (size_t i = 0; i < part.size(); ++i) {
			value |= uint64_t(part[i]) << (8 * i);
		}
		return value;
	};

	int written = 0;
	size_t at = 0;
	while (at < raw.size()) {
		auto nameBytes = take(at, number(at, 4));
		std::string_view name(reinterpret_cast<const char*>(nameBytes.data()), nameBytes.size());
		auto data = take(at, number(at, 8));
		std::string path = strCat(shareDir, '/', name);
		FileOperations::mkdirp(std::string(FileOperations::getDirName(path)));
		File(path, File::OpenMode::TRUNCATE).write(data);
		++written;
	}
	// last, so that one cut short is written again at the next start
	File(marker(shareDir), File::OpenMode::TRUNCATE).write(std::span{stamp});
	return written;
}

} // namespace openmsx
