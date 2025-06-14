#include <algorithm>
#include <cassert>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>

#include "ArgParser.hpp"
#include "constants.hpp"
#include "scaling.hpp"


const char ESCAPE = static_cast<char>(27);

struct InitCommand {
	const uint8_t invalidate[200] = {0};

	const char m[2] = {ESCAPE, '@'};
};

struct StatusRequest {
	const char v[3] = {ESCAPE, 'i', 'S'};
};

struct SwitchDynamicCommandMode {
	const char m[3] = {ESCAPE, 'i', 'a'};

	enum Mode : uint8_t {
		EscP = 0x00,
		Raster = 0x01,
		PtouchTemplate = 0x03,
	} v = Raster;
};

struct PrintInformationCommand {
	const char m[3] = {ESCAPE, 'i', 'z'};

	uint8_t usedFlags = 132;
	uint8_t mediaType = 0;
	uint8_t mediaWidth;
	uint8_t mediaLength = 0;

	uint8_t rasterNumber[4];

	enum PageIndex : uint8_t {
		Starting = 0,
		Other = 1,
		Last = 2,
	} pageIndex = Last;

	uint8_t unused = 0;

	void setRasterNumber(uint32_t r)
	{
		for (unsigned i = 0; i < 4; ++i)
			rasterNumber[i] = (r & (0xff << (i << 3))) >> (i << 3);
	}
};

struct VariousModeSettings {
	const char m[3] = {ESCAPE, 'i', 'M'};

	enum Flags : uint8_t {
		AutoCut = 0x40,
		MirrorPrinting = 0x80,
	};

	uint8_t v = 0;
};

struct PageNumberInCutEachLabels {
	const char m[3] = {ESCAPE, 'i', 'A'};
	uint8_t v = 1;  // cut each label
};

struct AdvancedModeSettings {
	const char m[3] = {ESCAPE, 'i', 'K'};
	bool draftPrinting : 1 = false;
	bool unused1 : 1 = false;
	bool halfCut : 1 = true;
	bool noChainPrinting : 1 = true;
	bool specialTapeNoCutting : 1 = false;
	bool unused2 : 1 = false;
	bool highResolutionPrinting : 1 = false;
	bool noBufferCleaningWhenPrinting : 1 = false;
};

struct SpecifyMarginAmount {
	const char m[3] = {ESCAPE, 'i', 'd'};
	uint8_t v[2] = {14, 0};
};

struct SelectCompressionMode {
	enum Type : uint8_t {
		NoCompression = 0x00,
		Reserved = 0x01,  // disabled
		Tiff = 0x02,
	};

	const char m = 'M';
	uint8_t v = NoCompression;
};

template <class T>
void writeStruct(std::ofstream &out, const T &c)
{
	// for (size_t i = 0; i < sizeof(c); ++i)
	// 	std::cerr << std::hex << std::setfill('0') << std::setw(2) << uint32_t(uint8_t(((char *)&c)[i]));
	// std::cerr << "\n";
	out.write(reinterpret_cast<const char *>(&c), sizeof(c));
}

void writeEncodedLine(std::ofstream &out, uint8_t* line, png::uint_32 height)
{
	// std::cerr << "print line: ";
	// for (png::uint_32 i = 0; i < height; ++i)
		// std::cerr << std::hex << std::setfill('0') << std::setw(2) << static_cast<uint32_t>(static_cast<uint8_t>(line[i]));
	// std::cerr << "\n";

	static const png::uint_32 AuxSize = 20;

	uint8_t output[height + 3 + AuxSize];
	size_t size = 3, counter = 1;
	uint8_t last = -1;

	uint8_t buffer[height];
	size_t bufferSize = 0;

	for (png::uint_32 y = 0; y < height; ++y) {
		if (line[y] == last && y != 0) {
			if (bufferSize > 1) {
				output[size++] = bufferSize - 2;
				// std::cerr << "write buffer (" << bufferSize - 1 << "): ";
				for (size_t i = 0; i < bufferSize - 1; ++i) {
					output[size++] = buffer[i];
					// std::cerr << std::hex << std::setfill('0') << std::setw(2) << static_cast<uint32_t>(static_cast<uint8_t>(buffer[i]));
				}
				// std::cerr << "\n";
				buffer[0] = line[y];
				bufferSize = 1;
				assert(counter == 1);
			}
			++counter;

		} else {
			if (counter > 1) {
				output[size++] = -(counter - 1);
				output[size++] = buffer[0];
				// std::cerr << "write repeated (" << std::hex << static_cast<uint32_t>(static_cast<uint8_t>(buffer[0])) << ") x " << std::dec << counter << "\n";

				buffer[0] = line[y];
				counter = 1;
				assert(bufferSize == 1);
				bufferSize = 0;
			}
			buffer[bufferSize++] = line[y];
			last = line[y];
		}
	}
	if (bufferSize > 1) {
		output[size++] = bufferSize - 1;
		// std::cerr << "write buffer (" << bufferSize << "): ";
		for (size_t i = 0; i < bufferSize; ++i) {
			output[size++] = buffer[i];
			// std::cerr << std::hex << std::setfill('0') << std::setw(2) << static_cast<uint32_t>(static_cast<uint8_t>(buffer[i]));
		}
		// std::cerr << "\n";
	} else {
		output[size++] = -(counter - 1);
		output[size++] = buffer[0];
		// std::cerr << "write repeated (" << std::hex << static_cast<uint32_t>(static_cast<uint8_t>(buffer[0])) << ") x " << std::dec << counter << "\n";
	}

	output[0] = 'G';
	output[1] = size - 3;
	output[2] = 0;
	// std::cerr << "buffer:     ";
	for (size_t i = 0; i < size; ++i) {
		// std::cerr << std::hex << std::setfill('0') << std::setw(2) << static_cast<uint32_t>(static_cast<uint8_t>(output[i]));
		out << output[i];
	}
	// std::cerr << "\n\n";
}

struct Flags {
	enum Value : uint8_t {
		Compressed = 0x01,
		Center = 0x02,
		LastImage = 0x40,
		Test = 0x80,
	};
};

static unsigned TestImageWidth = 16 * 8;

struct Exec {
	std::string error;

	inline operator bool() const { return error.empty(); }
};

namespace {
	// uint16_t randomMask(uint8_t bitsOn)
	// {
	// 	static auto rng = std::default_random_engine{};
	//
	// 	unsigned mask[16];
	// 	for (unsigned i = 0; i < 16; ++i)
	// 		mask[i] = i;
	// 	std::ranges::shuffle(mask, rng);
	//
	// 	uint16_t p = 0;
	// 	for (uint8_t i = 0; i < bitsOn; ++i)
	// 		p |= 1 << mask[i];
	// 	return p;
	// };
	//
	// uint16_t patternMask(uint8_t bitsOn)
	// {
	// 	static const unsigned Mask[16] = {6, 9, 0, 15, 12, 3, 5, 10, 4, 11, 2, 13, 7, 8, 1, 14};
	//
	// 	uint16_t p = 0;
	// 	for (uint8_t i = 0; i < bitsOn; ++i)
	// 		p |= 1 << Mask[i];
	// 	return p;
	// };

	uint16_t alternatingPatternMask(uint8_t bitsOn)
	{
		static const unsigned Mask[16] = {6, 9, 0, 15, 12, 3, 5, 10, 4, 11, 2, 13, 7, 8, 1, 14};
		static bool transpose = false;

		uint16_t p = 0;
		for (uint8_t i = 0; i < 4; ++i) {
			for (uint8_t j = 0; j < 4; ++j) {
				if (transpose) {
					if (i * 4 + j < bitsOn)
						p |= 1 << Mask[4 * i + j];
				} else {
					if (j * 4 + i < bitsOn)
						p |= 1 << Mask[4 * j + i];
				}
			}
		}

		transpose = !transpose;

		return p;
	};
}

struct Margins {
	unsigned leftMargin;
	unsigned rightMargin;
	unsigned height;  // pixels

	static const unsigned Pins = 560;

	Margins(std::string_view mediaWidth)
	{
		const auto margin = bp::margins().at(mediaWidth);
		// left and right margins are swapped as the data is mirrored
		leftMargin = margin.second;
		rightMargin = margin.first;
		rightMargin += 4 - (leftMargin + rightMargin) % 4;  // adjust to full bytes
		height = (Pins - leftMargin - rightMargin) / 4;
	}
};

Exec writePng(std::ofstream &out, const png::image<png::rgb_pixel> &img, std::string_view mediaWidth, unsigned imageWidth, uint8_t flags)
{
	static const unsigned Height = 70;

	if (!bp::margins().contains(mediaWidth))
		return Exec{std::format("writePng: unrecognised media width {}", mediaWidth)};

	Margins margins{mediaWidth};
	unsigned leftMargin = margins.leftMargin;
	unsigned rightMargin = margins.rightMargin;

	png::uint_32 width = imageWidth;
	png::uint_32 height = margins.height;
	if (!(flags & Flags::Test))
		height = img.get_height();

	if (Margins::Pins < leftMargin + height * 4 + rightMargin)
		return Exec{std::format("Height of the image too large: left margin = {} pins, right margin = {} pins, expected = {} pixels ({} pins), received {} pixels ({} pins)", rightMargin, leftMargin, (Margins::Pins - leftMargin - rightMargin) / 4, Margins::Pins - leftMargin - rightMargin, height, height * 4)};

	if (flags & Flags::Center) {
		leftMargin += (margins.height - height) * 4 / 2;
		rightMargin += Margins::Pins - height * 4 - leftMargin - rightMargin;
	}

	if (Margins::Pins != leftMargin + height * 4 + rightMargin)
		return Exec{std::format("Height of the image doesn't match the tape: left margin = {} pins, right margin = {} pins, expected at most = {} pixels ({} pins), received {} pixels ({} pins)", rightMargin, leftMargin, (Margins::Pins - leftMargin - rightMargin) / 4, Margins::Pins - leftMargin - rightMargin, height, height * 4)};

	uint8_t vline[4][Height];
	bool zeroLine[4];

	auto intensity = [](const png::basic_rgb_pixel<unsigned char> &p) { return 15 - (p.red + p.green + p.blue) / 3 / 16; };
	auto maskFn = ::alternatingPatternMask;

	for (png::uint_32 x = 0; x < width; ++x) {
		std::memset(vline, 0, sizeof vline);
		*(reinterpret_cast<uint32_t *>(zeroLine)) = 0;

		for (png::uint_32 y = 0; y < height; ++y) {
			uint16_t pixel;
			if (flags & Flags::Test)
				pixel = maskFn(x / 8 + 1);
			else
				pixel = maskFn(intensity(img.get_pixel(x, y)));

			for (unsigned i = 0; i < 4; ++i) {
				for (unsigned j = 0; j < 4; ++j) {
					if (pixel & (1 << (4 * i + j))) {
						unsigned pin = leftMargin + y * 4 + j;
						unsigned byteNr = pin / 8;
						unsigned bitInPixel = 7 - pin % 8;
						vline[i][byteNr] |= 1 << bitInPixel;
						zeroLine[i] = false;
					}
				}
			}
		}

		for (unsigned i = 0; i < 4; ++i) {
			if (zeroLine[i]) {
				out << 'Z';
			} else if (flags & Flags::Compressed) {
				writeEncodedLine(out, vline[i], Height);
			} else {
				out << 'G' << static_cast<uint8_t>(70) << static_cast<uint8_t>(0);
				for (png::uint_32 y = 0; y < Height; ++y)
					out << vline[i][y];
			}
		}
	}

	return Exec{};
}

Exec writePrintRequest(std::ofstream &out, const ArgParser &parser, const png::image<png::rgb_pixel> &image, unsigned imageWidth, uint8_t flags)
{
	auto tapeWidth = parser.value("--tape-width");
	if (!bp::tapeWidth().contains(tapeWidth))
		return Exec(std::format("writePrintRequest: unrecognised tape width ", tapeWidth));

	unsigned copies = std::stoi(parser.value("--copies"));
	for (unsigned copyIndex = 0; copyIndex < copies; ++copyIndex) {
		writeStruct(out, SwitchDynamicCommandMode{});

		PrintInformationCommand printInformationCommand;
		printInformationCommand.mediaWidth = bp::tapeWidth().at(tapeWidth);
		printInformationCommand.setRasterNumber(4 * imageWidth);
		if (copyIndex + 1 == copies)
			printInformationCommand.pageIndex = PrintInformationCommand::Last;
		else if (copyIndex == 0)
			printInformationCommand.pageIndex = PrintInformationCommand::Starting;
		else
			printInformationCommand.pageIndex = PrintInformationCommand::Other;
		writeStruct(out, printInformationCommand);

		VariousModeSettings variousModeSettings;
		if (!parser.has("--no-auto-cut"))
			variousModeSettings.v |= VariousModeSettings::AutoCut;
		if (parser.has("--mirror-printing"))
			variousModeSettings.v |= VariousModeSettings::MirrorPrinting;
		writeStruct(out, variousModeSettings);

		writeStruct(out, PageNumberInCutEachLabels{});

		AdvancedModeSettings advancedModeSettings;
		if (parser.has("--no-half-cut"))
			advancedModeSettings.halfCut = false;
		if (parser.has("--chain-printing"))
			advancedModeSettings.noChainPrinting = false;
		writeStruct(out, advancedModeSettings);

		SpecifyMarginAmount specifyMarginAmount;
		specifyMarginAmount.v[0] = std::stoi(parser.value("--set-length-margin"));
		writeStruct(out, specifyMarginAmount);

		SelectCompressionMode compressionMode;
		if (flags & Flags::Compressed)
			compressionMode.v = SelectCompressionMode::Tiff;
		else
			compressionMode.v = SelectCompressionMode::NoCompression;
		writeStruct(out, compressionMode);

		auto exec = writePng(out, image, parser.value("--tape-width"), imageWidth, flags);
		if (!exec)
			return exec;

		if (copyIndex + 1 < copies || !(flags & Flags::LastImage))
			out << static_cast<uint8_t>(0x0c);  // page end marker
		else
			out << static_cast<uint8_t>(0x1a);  // final page marker
	}

	return Exec{};
}

png::image<png::rgb_pixel> enlargeImage(const png::image<png::rgb_pixel> &image)
{
	auto width = image.get_width();
	auto height = image.get_height();
	png::image<png::rgb_pixel> img{width * 2, height * 2};
	for (png::uint_32 x = 0; x < width; ++x) {
		for (png::uint_32 y = 0; y < height; ++y) {
			const auto &pixel = image.get_pixel(y, x);
			img.set_pixel(y * 2, x * 2, pixel);
			img.set_pixel(y * 2 + 1, x * 2, pixel);
			img.set_pixel(y * 2, x * 2 + 1, pixel);
			img.set_pixel(y * 2 + 1, x * 2 + 1, pixel);
		}
	}

	return img;
}

int main(int argc, char **argv)
{
	enum class Command {
		Print,
		Status,
		Initialise,
	} command;

	if (argc < 2) {
		std::cerr << std::format("usage: {} print/status/initialise <options>\n", argv[0]);
		return 1;
	}

	if (strcmp(argv[1], "print") == 0) {
		command = Command::Print;
	} else if (strcmp(argv[1], "status") == 0) {
		command = Command::Status;
	} else if (strcmp(argv[1], "initialise") == 0) {
		command = Command::Initialise;
	} else {
		std::cerr << std::format("Unrecognised option: {}, expected: print/status/initialise\n", argv[1]);
		return 1;
	}

	ArgParser parser;
	switch (command) {
		case Command::Print:
			parser.addArgument(Arg{"-i"}.setRepeatable());
			parser.addArgument(Arg{"-o"});
			parser.addArgument(Arg{"--copies"});
			parser.addArgument(Arg{"--compression"});
			parser.addArgument(Arg{"--tape-type"});
			parser.addArgument(Arg{"--tape-width"});
			parser.addArgument(Arg{"--set-length-margin"});
			parser.addArgument(Arg{"--no-auto-cut"}.setOptional().setCount(0));
			parser.addArgument(Arg{"--no-half-cut"}.setOptional().setCount(0));
			parser.addArgument(Arg{"--chain-printing"}.setOptional().setCount(0));
			parser.addArgument(Arg{"--mirror-printing"}.setOptional().setCount(0));
			parser.addArgument(Arg{"--scale-down"}.setOptional().setCount(0));
			parser.addArgument(Arg{"--scale-up"}.setOptional().setCount(0));
			parser.addArgument(Arg{"--center"}.setOptional().setCount(0));
			break;
		case Command::Status:
			parser.addArgument(Arg{"-o"});
			break;
		case Command::Initialise:
			parser.addArgument(Arg{"-o"});
			break;
	}

	parser.parse(argc - 2, argv + 2);
	if (!parser.isValid()) {
		std::cerr << parser.getErrorMsg() << "\n";
		return 1;
	}

	if (command == Command::Print) {
		std::ofstream out(parser.value("-o"), std::ofstream::binary | std::ios_base::app);

		for (unsigned pathIndex = 0; pathIndex < parser.values("-i").size(); ++pathIndex) {
			const auto &path = parser.values("-i")[pathIndex];

			uint8_t flags = 0;
			if (pathIndex == parser.values("-i").size() - 1)
				flags |= Flags::LastImage;

			png::image<png::rgb_pixel> image;
			unsigned imageWidth;
			if (path == "test") {
				flags |= Flags::Test;
				imageWidth = TestImageWidth;
			} else {
				image.read(path);
				imageWidth = image.get_width();

				unsigned imageHeight = image.get_height();
				if (parser.has("--scale-up")) {
					auto expectedHeight = Margins{parser.value("--tape-width")}.height;
					while (imageHeight * 2 <= expectedHeight) {
						std::cerr << std::format("height: {}, expected height: {}, enlarging image\n", imageHeight, expectedHeight);
						image = enlargeImage(image);
						imageHeight = image.get_height();
						imageWidth = image.get_width();
					}
				}
				if (parser.has("--scale-down")) {
					auto expectedHeight = Margins{parser.value("--tape-width")}.height;
					auto expectedWidth = static_cast<unsigned>(static_cast<double>(expectedHeight) / static_cast<double>(imageHeight) * static_cast<double>(imageWidth));
					static const unsigned FilterSize = 3;
					if (imageHeight > expectedHeight) {
						image = scaleLanczos(image, expectedHeight, expectedWidth, FilterSize);
						imageHeight = image.get_height();
						imageWidth = image.get_width();
					}
				}
				std::string previewPath{"/tmp/preview.png"};
				std::cerr << "preview: " << previewPath << "\n";
				image.write(previewPath);
			}

			if (parser.value("--compression") == "tiff") {
				flags |= Flags::Compressed;
			} else if (parser.value("--compression") != "no compression") {
				std::cerr << "Invalid compression mode: " << parser.value("--compression") << "\n";
				return 1;
			}

			if (parser.has("--center"))
				flags |= Flags::Center;

			auto exec = writePrintRequest(out, parser, image, imageWidth, flags);
			if (!exec) {
				std::cerr << exec.error << "\n";
				return 1;
			}
			out.flush();
		}
	} else if (command == Command::Status) {
		std::ofstream out(parser.value("-o"), std::ofstream::binary | std::ios_base::app);
		writeStruct(out, StatusRequest{});

	} else if (command == Command::Initialise) {
		std::ofstream out(parser.value("-o"), std::ofstream::binary | std::ios_base::app);
		writeStruct(out, InitCommand{});
	}

	return 0;
}
