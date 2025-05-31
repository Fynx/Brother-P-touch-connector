#pragma once

#include <string_view>
#include <unordered_map>
#include <utility>

namespace {
	static const std::unordered_map<std::string_view, std::pair<unsigned, unsigned> > Margins {
		{ "3.5 mm", { 248, 264 } },
		{ "6 mm", { 240, 256 } },
		{ "9 mm", { 219, 235 } },
		{ "12 mm", { 197, 213 } },
		{ "18 mm", { 155, 171 } },
		{ "24 mm", { 112, 128 } },
		{ "36 mm", { 45, 61 } },
		{ "HS 5.8 mm", { 244, 260 } },
		{ "HS 8.8 mm", { 224, 240 } },
		{ "HS 11.7 mm", { 206, 222 } },
		{ "HS 17.7 mm", { 166, 182 } },
		{ "HS 23.6 mm", { 144, 160 } },
		// ??
		// FLe 21 mm x 45 mm
		// HS 9.0 mm, HS 11.2 mm, HS 21.0 mm, HS 31.0 mm
	};

	static const std::unordered_map<std::string_view, unsigned> TapeWidth {
		{ "3.5 mm", 4 },
		{ "6 mm", 6 },
		{ "9 mm", 9 },
		{ "12 mm", 12 },
		{ "18 mm", 18 },
		{ "24 mm", 24 },
		{ "36 mm", 36 },
		{ "HS 5.8 mm", 6 },
		{ "HS 8.8 mm", 9 },
		{ "HS 11.7 mm", 12 },
		{ "HS 17.7 mm", 18 },
		{ "HS 23.6 mm", 24 },
		{ "FLe 21 mm x 45 mm", 21 },
	};
}

namespace bp {

const auto & margins()
{
	return ::Margins;
}

const auto & tapeWidth()
{
	return ::TapeWidth;
}

}
