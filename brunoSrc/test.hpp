#pragma once

#include <stdint.h>
#include <fstream>

#include "linearAlgebra.hpp"

struct RGB
{
	unsigned char red, green, blue;

	uint32_t to_int();
};

struct HSV
{
	double hue;			//assumed to be in [0, 2 * pi]
	double saturation;	//assumed to be in [0, 1]
	double value;		//assumed to be in [0, 1]

	RGB to_rgb() const;
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

//this is not meant as real svg exporter, but just as a way to visualize output data
class SVG	
{
	std::ofstream document;

public:
	//min and max are used to set view box in svg
	SVG(const char* file_name, la::Board_Vec min, la::Board_Vec max);
	~SVG();

	void move_to(la::Board_Vec point);
	void draw_to(la::Board_Vec point);
};

namespace test {

	void svg_to_bmp(const std::string& svg_str, const char* output_name, double board_width, double board_height, 
		uint16_t mesh_size, double scaling_factor = 1);
	void svg_to_bbf(const std::string& svg_str, const char* output_name, double board_width, double board_height);

	void svg_to_svg(const std::string& svg_str, const char* output_name, double board_width, double board_height);

	//this function expects a specific folder structure
	void read_string_to_all(const char* input_name, double board_width, double board_height);
}