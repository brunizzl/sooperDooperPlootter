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

//bitmap coordinates are same as board coordinates
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
	void draw_mesh(uint16_t mesh_size, RGB mesh_color);	//draws mesh with two lines seperated by mesh_size - 1 pixels

	void save_as(const char* name);
};

namespace test {

	void svg_to_bmp(const std::string& svg_str, const char* output_name, double board_width, double board_height, 
		uint16_t mesh_size, double scaling_factor = 1);
	void svg_to_bbf(const std::string& svg_str, const char* output_name, double board_width, double board_height);

	void read_string_to_bmp_and_bbf(const char* input_name, double board_width, double board_height);
}