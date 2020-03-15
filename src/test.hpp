#pragma once

#include <stdint.h>


struct RGB
{
	unsigned char red, green, blue;

	uint32_t to_int();
};

struct HSV
{
	double hue;
	double saturation;
	double value;

public:
	HSV(double hue_, double saturation_, double value_);

	RGB to_rgb();
};


class BMP
{
	uint16_t width;
	uint16_t height;
	uint32_t* picture;

public:
	BMP(uint16_t width_, uint16_t height_, RGB backround);
	~BMP();

	void set_pixel(uint16_t x, uint16_t y, RGB color);
	void set_pixel(uint16_t x, uint16_t y, HSV color);
	void save_as(const char* const name);
};

namespace test {

	void svg_to_bmp(const char * const input_name, const char* const output_name, double board_width, double board_height);
	
}