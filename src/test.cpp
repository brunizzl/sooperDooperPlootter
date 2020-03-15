
#include <cassert>
#include <cmath>
#include <functional>

#include "test.hpp"
#include "steppers.hpp"
#include "svgHandling.hpp"
#include "libBMP.h"



uint32_t RGB::to_int()
{
	return ((uint32_t)this->red * 256 + this->green) * 256 + this->blue;
}

HSV::HSV(double hue_, double saturation_, double value_)
	:hue(hue_), saturation(saturation_), value(value_)
{
	assert(hue_ >= 0 && hue_ <= 2 * pi);
	assert(saturation_ >= 0 && saturation_ <= 1);
	assert(value_ >= 0 && value_ <= 1);
}

RGB HSV::to_rgb()
{
	//formulas taken from here : https://en.wikipedia.org/wiki/HSL_and_HSV

	const double angle = this->hue * 3 / pi;  //in interval [0, 5]
	const double chroma = this->value * this->saturation;
	const double rest = chroma * (1.0 - std::fabs(std::fmod(angle, 2.0) - 1.0));

	double red, green, blue;

	switch (static_cast<int>(angle)) {
	case 0: red = chroma; green = rest;   blue = 0;      break;
	case 1: red = rest;   green = chroma; blue = 0;      break;
	case 2: red = 0;      green = chroma; blue = rest;   break;
	case 3: red = 0;      green = rest;   blue = chroma; break;
	case 4: red = rest;   green = 0;      blue = chroma; break;
	case 5: red = chroma; green = 0;      blue = rest;   break;
	default: assert(false);
	}

	const double grey_part = this->value - chroma;
	red = (red + grey_part) * 255;
	green = (green + grey_part) * 255;
	blue = (blue + grey_part) * 255;

	assert(red <= 255 && green <= 255 && blue <= 255);
	return RGB{ static_cast<unsigned char>(red), static_cast<unsigned char>(green), static_cast<unsigned char>(blue) };
}


BMP::BMP(uint16_t width_, uint16_t height_, RGB backround)
	:width(width_), height(height_), picture(new uint32_t[width_ * height_])
{
	for (uint16_t x = 0; x < width_; x++) {
		for (uint16_t y = 0; y < height_; y++) {
			this->set_pixel(x, y, backround);
		}
	}
}

BMP::~BMP()
{
	delete[] this->picture;
}

void BMP::set_pixel(uint16_t x, uint16_t y, RGB color)
{
	assert(x < width && y <= height);
	picture[y * width + x] = color.to_int();
}

void BMP::set_pixel(uint16_t x, uint16_t y, HSV color)
{
	assert(x < width && y <= height);
	picture[y * width + x] = color.to_rgb().to_int();
}

void BMP::save_as(const char* const name)
{
	bmp_create(name, this->picture, this->width, this->height);
}

void test::svg_to_bmp(const char * const input_name, const char* const output_name, double board_width, double board_height)
{
	//finding out how often draw_to is called
	unsigned int amount_points = 0;
	std::function count_points = [&amount_points](Board_Vec point) {amount_points++; };
	std::function do_nothing = [](Board_Vec point) {; };

	set_output_functions(count_points, do_nothing);
	draw_from_file(input_name, board_width, board_height);

	const double hue_per_point = 1.99 * pi / amount_points;	//
	HSV point_color(0, 1, 1);

	BMP picture(board_width, board_height, { 130, 130, 130 });

	Board_Vec current(0, 0);


	std::function draw_to = [&](Board_Vec point) {
		double gradient = (point.y - current.y) / (point.x - current.x);
		if (std::abs(gradient) < 1) {
			const double y_axis_offset = point.y - gradient * point.x;
			if (current.x < point.x) {
				for (int x = current.x; x <= point.x; x++) {
					picture.set_pixel(x, gradient * x + y_axis_offset, point_color);
				}
			}
			else {
				for (int x = current.x; x >= point.x; x--) {
					picture.set_pixel(x, gradient * x + y_axis_offset, point_color);
				}
			}
		}
		else {
			gradient = 1 / gradient;
			const double x_axis_offset = point.x - gradient * point.y;
			if (current.y < point.y) {
				for (int y = current.y; y <= point.y; y++) {
					picture.set_pixel(gradient * y + x_axis_offset, y, point_color);
				}
			}
			else {
				for (int y = current.y; y >= point.y; y--) {
					picture.set_pixel(gradient * y + x_axis_offset, y, point_color);
				}
			}
			current = point;
		}

		current = point; 
		point_color.hue += hue_per_point;
	};
	std::function go_to = [&](Board_Vec point) {
		current = point;
	};

	set_output_functions(draw_to, go_to);
	draw_from_file(input_name, board_width, board_height);

	picture.save_as(output_name);
}
