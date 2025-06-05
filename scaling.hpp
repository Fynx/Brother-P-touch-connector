#include "png++/png.hpp"

#include <cmath>

// based on code from boost

inline double normalizedSinc(double x)
{
	return std::sin(x * std::numbers::pi) / (x * std::numbers::pi);
}

inline double lanczos(double x, int a)
{
	if (x == 0)
		return 1;
	if (-a < x && x < a)
		return normalizedSinc(x) / normalizedSinc(x / static_cast<double>(a));
	return 0;
}

png::rgb_pixel lanczosAt(const png::image<png::rgb_pixel> &input, int sourceX, int sourceY, int a)
{
	// avoiding using unsigned to dodge the overflow and type comparison traps

	png::rgb_pixel resultPixel{0, 0, 0};

	auto next = input.get_pixel(sourceX, sourceY);

	for (int y = std::max(sourceY - a + 1, 0); y <= std::min(sourceY + a, static_cast<int>(input.get_height()) - 1); ++y) {
		for (int x = std::max(sourceX - a + 1, 0); x <= std::min(sourceX + a, static_cast<int>(input.get_width()) - 1); ++x) {
			double lanczosResponse = lanczos(sourceX - x, a) * lanczos(sourceY - y, a);

			resultPixel.red += next.red * lanczosResponse;
			resultPixel.green += next.green * lanczosResponse;
			resultPixel.blue += next.blue * lanczosResponse;
		}
	}

	return resultPixel;
}

png::image<png::rgb_pixel> scaleLanczos(const png::image<png::rgb_pixel> &input, unsigned height, unsigned width, unsigned a)
{
	png::image<png::rgb_pixel> output{width, height};
	double scaleX = static_cast<double>(width) / static_cast<double>(input.get_width());
	double scaleY = static_cast<double>(height) / static_cast<double>(input.get_height());

	for (png::uint_32 y = 0; y < height; ++y) {
		for (png::uint_32 x = 0; x < width; ++x)
			output.set_pixel(x, y, lanczosAt(input, x / scaleX, y / scaleY, a));
	}

	return output;
}
