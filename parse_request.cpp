#include <cassert>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>

#include "ArgParser.hpp"


const char ESCAPE = static_cast<char>(27);

struct ParseFlags {
	enum Value {
		WithInvalidate = 0x01,
	};
};

struct PrintFlags {
	enum Value {
		WithoutData = 0x01,
	};
};

struct SelectCompressionMode {
	enum Type : uint8_t {
		NoCompression = 0x00,
		Reserved = 0x01,  // disabled
		Tiff = 0x02,
	};
};

inline uint32_t ctou(char c)
{
	return static_cast<uint32_t>(static_cast<uint8_t>((c)));
}

inline void printHex(char c, bool asciiart = false)
{
	if (!asciiart) {
		std::cerr << std::setfill('0') << std::hex << std::setw(2) << ctou(c);
		return;
	}

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

void parse(char *buf, unsigned len, uint8_t parseFlags, uint8_t printFlags)
{
	unsigned i = 0;
	if (parseFlags & ParseFlags::WithInvalidate)
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
			if (printFlags & PrintFlags::WithoutData) {
				std::cerr << "\nSkipping data check!\n";
				return;
			}
			continue;
		} else if (buf[i] == 'G') {
			assert(++i < len);
			assert(++i < len);
			unsigned bytes = static_cast<uint32_t>(buf[i - 1]) + static_cast<uint32_t>(buf[i]) * 256;
			assert(i + bytes < len);

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
			// std::cerr << "empty line\n";
			for (unsigned j = 0; j < 70; ++j)
				printHex(static_cast<char>(0));
			std::cerr << "\n";
			continue;
		} else if (buf[i] == 0x1a) {
			std::cerr << "last page marker\n";
			return;
		} else if (buf[i] == 0x0c) {
			std::cerr << "continue data marker\n";
			continue;
		}

		std::cerr << "> " << std::hex << std::setfill('0') << std::setw(2) << "0x" << ctou(buf[i]) << "\n";

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
				std::cerr << "switch dynamic command mode: " << ctou(buf[i]) << "\n";
				break;
			case 'z':
				std::cerr << "print information:\n";
				assert(i + 10 < len);
				std::cerr
					<< "\t" << "used flags " << std::boolalpha
					<< "media_type=" << static_cast<bool>(buf[i + 1] & 0x02)
					<< ", media_width=" << static_cast<bool>(buf[i + 1] & 0x04)
					<< ", media_length=" << static_cast<bool>(buf[i + 1] & 0x08)
					<< ", unused:print_quality_priority=" << static_cast<bool>(buf[i + 1] & 0x40)
					<< ", recovery_always_on=" << static_cast<bool>(buf[i + 1] & 0x80) << std::noboolalpha << "\n";

				std::cerr << "\tmedia_type = ";
				switch (ctou(buf[i + 2])) {
					case 0x00: std::cerr << "Laminated/Non-laminated tape\n"; break;
					case 0x11: std::cerr << "Heat-Shrink Tube (HS 2:1)\n"; break;
					case 0x17: std::cerr << "Heat-Shrink Tube (HS 3:1)\n"; break;
					case 0x13: std::cerr << "Fle tape\n"; break;
					case 0xff: std::cerr << "Incompatible tape\n"; break;
				}

				std::cerr << "\tmedia_width = 0x" << std::hex << std::setfill('0') << std::setw(2) << ctou(buf[i + 3]) << "\n";
				std::cerr << "\tmedia_length = 0x" << std::hex << std::setfill('0') << std::setw(2) << ctou(buf[i + 4]) << "\n";
				std::cerr << "\traster number = 0x";
				printHex(buf[i + 8]);
				printHex(buf[i + 7]);
				printHex(buf[i + 6]);
				printHex(buf[i + 5]);
				std::cerr << "\n";

				switch (buf[i + 9]) {
					case 0: std::cerr << "\tstarting page\n"; break;
					case 1: std::cerr << "\tother pages\n"; break;
					case 2: std::cerr << "\tlast page\n"; break;
				}

				// buf[i + 10] == 0

				i += 10;
				break;
			case 'M':
				assert(++i < len);
				std::cerr << "various mode settings:\n";
				if (buf[i] & 0x40)
					std::cerr << "\tauto cut\n";
				if (buf[i] & 0x80)
					std::cerr << "\tmirror printing\n";
				break;
			case 'K':
				assert(++i < len);
				std::cerr << "advanced mode settings:\n";

				if (buf[i] & 0x01)
					std::cerr << "\tdraft printing on\n";
				else
					std::cerr << "\tdraft printing off\n";

				if (buf[i] & 0x04)
					std::cerr << "\thalf cut on\n";
				else
					std::cerr << "\thalf cut off\n";

				if (buf[i] & 0x08)
					std::cerr << "\tchain printing off\n";
				else
					std::cerr << "\tchain printing on\n";

				if (buf[i] & 0x10)
					std::cerr << "\tspecial tape (no cutting) on\n";
				else
					std::cerr << "\tspecial tape (no cutting) off\n";

				if (buf[i] & 0x40)
					std::cerr << "\thigh-resolution printing on\n";
				else
					std::cerr << "\thigh-resolution printing off\n";

				if (buf[i] & 0x80)
					std::cerr << "\tno buffer clearing while printing on\n";
				else
					std::cerr << "\tno buffer clearing while printing off\n";

				break;
			case 'd':
				assert(++i < len);
				assert(++i < len);
				std::cerr << "specify margin amount: " << std::dec << static_cast<uint32_t>(buf[i - 1]) + static_cast<uint32_t>(buf[i]) * 256  << " (margin amount - dots)\n";
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

int main(int argc, char **argv)
{
	ArgParser parser;
	parser.addPositionalArgument(Arg{"input"});
	parser.addArgument(Arg{"--no-data"}.setCount(0).setOptional());

	parser.parse(argc - 1, argv + 1);
	if (!parser.isValid()) {
		std::cerr << parser.getErrorMsg() << "\n";
		return 1;
	}

	char buf[100000] = {0};

	std::ifstream in{parser.value("input"), std::ifstream::binary};
	in.read(buf, 100000);

	uint8_t parseFlags = ParseFlags::WithInvalidate;

	uint8_t printFlags = 0;
	if (parser.has("--no-data"))
		printFlags |= PrintFlags::WithoutData;

	parse(buf, in.gcount(), parseFlags, printFlags);

	return 0;
}
