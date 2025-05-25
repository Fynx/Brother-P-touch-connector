#include <cassert>
#include <fstream>
#include <iomanip>
#include <iostream>

#include "ArgParser.hpp"


const char ESCAPE = static_cast<char>(27);

namespace ParseFlags {
	enum Value {
		WithInvalidate = 0x01,
	};
}

struct SelectCompressionMode {
	enum Type : uint8_t {
		NoCompression = 0x00,
		Reserved = 0x01,  // disabled
		Tiff = 0x02,
	};
};

inline void printHex(char c, bool asciiart = false)
{
	if (!asciiart) {
		std::cerr << std::setfill('0') << std::hex << std::setw(2) << static_cast<uint32_t>(static_cast<uint8_t>((c)));
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

int main(int argc, char **argv)
{
	ArgParser parser;
	parser.addArgument(Arg{"-i"});

	parser.parse(argc - 1, argv + 1);
	if (!parser.isValid()) {
		std::cerr << parser.getErrorMsg() << "\n";
		return 1;
	}

	char buf[100000] = {0};

	std::ifstream in{parser.get("-i"), std::ifstream::binary};
	in.read(buf, 100000);

	parse(buf, in.gcount(), ParseFlags::WithInvalidate);

	return 0;
}
