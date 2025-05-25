#include <cassert>
#include <fstream>
#include <iomanip>
#include <iostream>

#include "png++/png.hpp"

#include "ArgParser.hpp"


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
	uint8_t mediaWidth = 36;
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

	uint8_t v = AutoCut;
};

struct PageNumberInCutEachLabels {
	const char m[3] = {ESCAPE, 'i', 'A'};
	uint8_t v = 1;  // cut each label
};

struct AdvancedModeSettings {
	const char m[3] = {ESCAPE, 'i', 'K'};
	uint8_t v = 12;
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
		Test = 0x80,
	};
};

static unsigned HeightScale = 3;
static unsigned TestImageWidth = 16;

void writePng(std::ofstream &out, const png::image<png::rgb_pixel> &img, uint8_t flags)
{
	static const unsigned Height = 70;
	// TODO
	static const unsigned LeftMargin = 7;
	static const unsigned RightMargin = 5;

	png::uint_32 height = Height - (LeftMargin + RightMargin);
	png::uint_32 width = TestImageWidth;
	if (!(flags & Flags::Test)) {
		height = img.get_height() / 2;  // one byte takes 2 pixels
		assert(Height - height == LeftMargin + RightMargin);
		width = img.get_width();
	}

	uint8_t vline[Height];
	bool zeroLine;

	auto byteValue = [](uint8_t value1, uint8_t value2) { return ((15 - value1) << 4) + 15 - value2; };  // values have to be between 0 and 15
	auto intensity = [](const png::basic_rgb_pixel<unsigned char> &p) { return (p.red + p.green + p.blue) / 3 / 16; };

	for (png::uint_32 x = 0; x < width; ++x) {
		for (unsigned rep = 0; rep < HeightScale; ++rep) {
			zeroLine = true;
			png::uint_32 y = 0;

			for (; y < LeftMargin; ++y)
				vline[y] = 0;

			if (flags & Flags::Test) {
				for (; y < LeftMargin + height; ++y) {
					vline[y] = byteValue(x, x);
					zeroLine = false;
				}
			} else {
				for (; y < LeftMargin + height; ++y) {
					// TODO iterate like a human being
					auto p1 = img.get_pixel(x, (y - LeftMargin) * 2);
					auto p2 = img.get_pixel(x, (y - LeftMargin) * 2 + 1);
					vline[y] = byteValue(intensity(p1), intensity(p2));
					zeroLine &= vline[y] == 0;
				}
			}

			for (; y < Height; ++y)
				vline[y] = 0;

			if (zeroLine) {
				out << 'Z';
			} else if (flags & Flags::Compressed) {
				writeEncodedLine(out, vline, Height);
			} else {
				out << 'G' << static_cast<uint8_t>(70) << static_cast<uint8_t>(0);
				for (png::uint_32 y = 0; y < Height; ++y)
					out << vline[y];
			}
		}
	}

	// out << 'Z' << 'Z';
}

int main(int argc, char **argv)
{
	enum class Command {
		Print,
		Status,
		Initialise,
	} command;

	if (strcmp(argv[1], "print") == 0)
		command = Command::Print;
	else if (strcmp(argv[1], "status") == 0)
		command = Command::Status;
	else if (strcmp(argv[1], "initialise") == 0)
		command = Command::Initialise;
	else
		assert(false);

	ArgParser parser;
	switch (command) {
		case Command::Print:
			parser.addArgument(Arg{"-i"});
			parser.addArgument(Arg{"-o"});
			parser.addArgument(Arg{"--compression"});
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
		uint8_t flags = 0;

		png::image<png::rgb_pixel> image;
		unsigned imageWidth;
		if (parser.get("-i") == "test") {
			flags |= Flags::Test;
			imageWidth = TestImageWidth;
		} else {
			image.read(parser.get("-i"));
			imageWidth = image.get_width();
		}

		std::ofstream out(parser.get("-o"), std::ofstream::binary | std::ios_base::app);

		writeStruct(out, SwitchDynamicCommandMode{});

		PrintInformationCommand printInformationCommand;
		printInformationCommand.setRasterNumber(HeightScale * imageWidth);
		writeStruct(out, printInformationCommand);

		VariousModeSettings variousModeSettings;
		variousModeSettings.v = VariousModeSettings::AutoCut;
		writeStruct(out, variousModeSettings);

		writeStruct(out, PageNumberInCutEachLabels{});
		writeStruct(out, AdvancedModeSettings{});
		writeStruct(out, SpecifyMarginAmount{});

		SelectCompressionMode compressionMode;
		if (parser.get("--compression") == "no compression") {
			compressionMode.v = SelectCompressionMode::NoCompression;
		} else if (parser.get("--compression") == "tiff") {
			compressionMode.v = SelectCompressionMode::Tiff;
			flags |= Flags::Compressed;
		} else {
			std::cerr << "Invalid compression mode: " << parser.get("--compression") << "\n";
			return 1;
		}
		writeStruct(out, compressionMode);

		writePng(out, image, flags);

		out << static_cast<uint8_t>(0x1a);  // last page marker

	} else if (command == Command::Status) {
		std::ofstream out(parser.get("-o"), std::ofstream::binary | std::ios_base::app);
		writeStruct(out, StatusRequest{});

	} else if (command == Command::Initialise) {
		std::ofstream out(parser.get("-o"), std::ofstream::binary | std::ios_base::app);
		writeStruct(out, InitCommand{});
	}

	return 0;
}
