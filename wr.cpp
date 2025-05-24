#include <cassert>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <png++/png.hpp>

#include "ArgParser.hpp"


const char ESCAPE = static_cast<char>(27);

struct InitCommand {
	uint8_t invalidate[200] = {0};

	char m[2] = {ESCAPE, '@'};
};

struct StatusRequest {
	char v[3] = {ESCAPE, 'i', 'S'};
};

struct SwitchDynamicCommandMode {
	char m[3] = {ESCAPE, 'i', 'a'};

	enum Mode : uint8_t {
		EscP = 0x00,
		Raster = 0x01,
		PtouchTemplate = 0x03,
	} v = Raster;
};

struct JobIdSettingCommands {
};

struct PrintInformationCommand {
	char m[3] = {ESCAPE, 'i', 'z'};
	uint8_t v[10] = {132, 0, 36, 0, 110, 5, 0, 0, 2, 0};
};

struct VariousModeSettings {
	const char m[3] = {ESCAPE, 'i', 'M'};

	enum Flags : uint8_t {
		AutoCut = 0x40,
		MirrorPrinting = 0x80,
	};

	uint8_t value = AutoCut;

	void print()
	{
		std::cerr << std::boolalpha
			<< "\tauto cut: " << static_cast<bool>(value & AutoCut) << "\n"
			<< "\tmirror printing: " << static_cast<bool>(value & MirrorPrinting) << "\n";
	}
};

struct PageNumberInCutEachLabels {
	char m[3] = {ESCAPE, 'i', 'A'};
	uint8_t v = 1;  // cut each label
};

struct AdvancedModeSettings {
	char m[3] = {ESCAPE, 'i', 'K'};
	uint8_t v = 12;
};

struct SpecifyMarginAmount {
	char m[3] = {ESCAPE, 'i', 'd'};
	uint8_t v[2] = {14, 0};
};

struct SelectCompressionMode {
	enum Type : uint8_t {
		NoCompression = 0x00,
		Reserved = 0x01,  // disabled
		Tiff = 0x02,
	};

	char m = 'M';
	uint8_t v = NoCompression;
};

struct PrintCommand {
	InitCommand m_initCommand;
	SwitchDynamicCommandMode m_switchDynamicCommandMode;
	PrintInformationCommand m_printInformationCommand;
	PageNumberInCutEachLabels m_pageNumberInCutEachLabels;
	SpecifyMarginAmount m_specifyMarginAmount;
	SelectCompressionMode m_selectCompressionMode;
	// RasterData m_rasterData;
	// uint8_t m_printCommandWithFeeding = 0x1a;
};

namespace ParseFlags {
	enum Value {
		WithInvalidate = 0x01,
	};
}

inline void printHex(char c)
{
	std::cerr << std::setfill('0') << std::setw(2) << std::hex << static_cast<uint32_t>(c);
	return;
	if (static_cast<uint8_t>(c) > 172)
		c = '#';
	else if (static_cast<uint8_t>(c) > 128)
		c = '*';
	else if (static_cast<uint8_t>(c) > 64)
		c = '+';
	else if (static_cast<uint8_t>(c) > 0)
		c = '.';
	else
		c = ' ';
	std::cerr << c << c;
}

void parse(char *buf, unsigned len, uint32_t flags)
{
	unsigned i = 0;
	if (flags & ParseFlags::WithInvalidate)
		i += 200;  // zero value bytes

	unsigned index;
	bool compressed;

	for (; i < len; ++i) {
		if (buf[i] == 'M') {
			assert(++i < len);
			std::cerr << "select compression mode: " << static_cast<uint32_t>(buf[i]) << "\n";
			compressed = buf[i] == SelectCompressionMode::Tiff;
			if (compressed)
				std::cerr << "\tcompressed\n";
			continue;
		} else if (buf[i] == 'G') {
			assert(++i < len);
			assert(++i < len);
			unsigned bytes = static_cast<uint32_t>(buf[i - 1]) + static_cast<uint32_t>(buf[i]) * 256;
			assert(i + bytes < len);

			// for (uint8_t j = 1; j <= bytes; ++j)
				// printHex(buf[i + j]);
			// std::cerr << "\n";

			// std::cerr << "bytes: " << bytes << "\n";
			for (index = 1; index <= bytes; ++index) {
				int8_t rep = buf[i + index];

				if (!compressed) {
					printHex(buf[i + index]);
				} else if (rep < 0) {
					rep = -rep + 1;
					assert(index + 1 <= bytes);

					for (uint8_t j = 0; j < rep; ++j)
						printHex(buf[i + index + 1]);

					++index;
				} else {
					rep = rep + 1;
					assert(index + rep <= bytes);

					for (uint8_t j = 1; j <= rep; ++j)
						printHex(buf[i + index + j]);

					index += rep;
				}
			}
			std::cerr << "\n";
			i += index - 1;
			continue;
		} else if (buf[i] == 'Z') {
			std::cerr << "empty line\n";
			continue;
		} else if (buf[i] == 0x1a) {
			std::cerr << "last page marker\n";
			return;
		}

		std::cerr << "> " << static_cast<uint32_t>(buf[i]) << "\n";

		assert(buf[i] == ESCAPE);
		assert(++i < len);

		if (buf[i] == '@') {
			std::cerr << "initialize\n";
			continue;
		}

		assert(buf[i] == 'i');
		assert(++i < len);

		switch (buf[i]) {
			case 'S':
				std::cerr << "status request\n";
				break;
			case 'a':
				assert(++i < len);
				std::cerr << "switch dynamic command mode: " << static_cast<uint32_t>(buf[i]) << "\n";
				break;
			case 'z':
				std::cerr << "print information:\n";
				assert(i + 10 < len);
				for (index = 1; index <= 10; ++index)
					std::cerr << "\t" << static_cast<uint32_t>(static_cast<uint8_t>(buf[i + index])) << "\n";
				i += 10;
				break;
			case 'M':
				assert(++i < len);
				std::cerr << "various mode settings:\n";
				std::cerr << "\t" << static_cast<uint32_t>(buf[i]) << "\n";
				break;
			case 'K':
				assert(++i < len);
				std::cerr << "advanced mode settings: " << static_cast<uint32_t>(buf[i]) << "\n";
				break;
			case 'd':
				assert(++i < len);
				assert(++i < len);
				std::cerr << "specify margin amount: " << static_cast<uint32_t>(buf[i - 1]) + static_cast<uint32_t>(buf[i]) * 256  << " (margin amount - dots)\n";
				break;
			case 'A':
				assert(++i < len);
				std::cerr << "specify the page number in 'cut each * labels': " << static_cast<uint32_t>(buf[i]) << "\n";
				break;
			case 'U':
				std::cerr << "'U' ??";
				assert(i + 15 < len);
				for (index = 1; index <= 10; ++index)
					std::cerr << "\t" << static_cast<uint32_t>(static_cast<uint8_t>(buf[i + index])) << "\n";
				i += 15;
				break;
			case 'k':
				std::cerr << "'k' ??\n";
				assert(i + 3 < len);
				for (index = 1; index <= 3; ++index)
					std::cerr << "\t" << static_cast<uint32_t>(static_cast<uint8_t>(buf[i + index])) << "\n";
				i += 3;
				break;
			default:
				std::cerr << "Received '" << buf[i] << "'\n";
				assert(false);
		}
		std::cerr << "\n";
	}
}

template <class T>
void writeStruct(std::ofstream &out, const T &c)
{
	out.write(reinterpret_cast<const char *>(&c), sizeof(c));
}

void writeEncodedLine(std::ofstream &out, uint8_t* line, png::uint_32 height)
{
	uint8_t output[height + 3];
	size_t size = 3, counter = 0;
	uint8_t last = -1;

	uint8_t buffer[height];
	size_t bufferSize = 0;

	for (png::uint_32 y = 0; y < height; ++y) {
		if (line[y] == last) {
			if (bufferSize > 1) {
				output[size++] = bufferSize - 2;
				for (size_t i = 0; i < bufferSize - 1; ++i)
					output[size++] = buffer[i];

				buffer[0] = line[y];
				bufferSize = 1;
				assert(counter == 1);
			}
			++counter;

		} else {
			if (counter > 1) {
				output[size++] = -(counter - 1);
				output[size++] = buffer[0];

				buffer[0] = line[y];
				assert(bufferSize == 1);
				counter = 1;
			}
			last = line[y];
		}
	}
	if (bufferSize > 1) {
		output[size++] = bufferSize - 1;
		for (size_t i = 0; i < bufferSize; ++i)
			output[size++] = buffer[i];
	} else {
		output[size++] = -(counter - 1);
		output[size++] = buffer[0];
	}

	output[0] = 'G';
	output[1] = size;
	output[2] = 0;
	for (size_t i = 0; i < size; ++i)
		out << output[i];
}

void writePng(std::ofstream &out, const png::image<png::rgb_pixel> &img)
{
	auto height = img.get_height();
	uint8_t vline[height];
	bool zeroLine;

	for (png::uint_32 x = 0; x < img.get_width(); ++x) {
		for (unsigned rep = 0; rep < 7; ++rep) {
			zeroLine = true;

			for (png::uint_32 y = 0; y < height; ++y) {
				auto p = img.get_pixel(x, y);  // TODO iterate like a human being
				vline[y] = static_cast<uint8_t>(255 - (p.red + p.green + p.blue) / 3);
				std::cerr << int(p.red) << " " << int(p.green) << " " << int(p.blue) << " => " << int(vline[y]) << "\n";
				zeroLine &= vline[y] == 0;
			}

			if (!zeroLine) {
				out << 'G' << static_cast<uint8_t>(70) << static_cast<uint8_t>(0);
				for (png::uint_32 y = 0; y < height; ++y)
					out << vline[y];
			} else {
				out << 'Z';
			}
		}
	}
}

int main(int argc, char **argv)
{
	enum class Command {
		Parse,
		Print,
		Status,
		Initialise,
	} command;

	if (strcmp(argv[1], "parse") == 0)
		command = Command::Parse;
	else if (strcmp(argv[1], "print") == 0)
		command = Command::Print;
	else if (strcmp(argv[1], "status") == 0)
		command = Command::Status;
	else if (strcmp(argv[1], "initialise") == 0)
		command = Command::Initialise;
	else
		assert(false);

	ArgParser parser;
	switch (command) {
		case Command::Parse:
			parser.addArgument(Arg{"-i"});
			break;
		case Command::Print:
			parser.addArgument(Arg{"-i"});
			parser.addArgument(Arg{"-o"});
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

	if (command == Command::Parse) {
		char buf[100000] = {0};

		std::ifstream in{parser.get("-i"), std::ifstream::binary};
		in.read(buf, 100000);

		parse(buf, in.gcount(), ParseFlags::WithInvalidate);

	} else if (command == Command::Print) {
		std::ofstream out(parser.get("-o"), std::ofstream::binary | std::ios_base::app);

		writeStruct(out, SwitchDynamicCommandMode{});
		writeStruct(out, PrintInformationCommand{});
		writeStruct(out, VariousModeSettings{});
		writeStruct(out, PageNumberInCutEachLabels{});
		writeStruct(out, AdvancedModeSettings{});
		writeStruct(out, SpecifyMarginAmount{});

		SelectCompressionMode compressionMode;
		compressionMode.v = SelectCompressionMode::Tiff;
		writeStruct(out, compressionMode);

		png::image<png::rgb_pixel> image(parser.get("-i"));
		writePng(out, image);
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
