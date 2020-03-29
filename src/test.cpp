
#include <cassert>
#include <cmath>
#include <functional>
#include <iostream>
#include <fstream>

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
	:width(width_), height(height_), picture(new uint32_t[(width_ + 1) * (height_ + 1)])
{
	for (uint16_t x = 0; x <= width_; x++) {
		for (uint16_t y = 0; y <= height_; y++) {
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
	assert(x <= width && y <= height);
	picture[y * width + x] = color.to_int();
}

void BMP::set_pixel(uint16_t x, uint16_t y, HSV color)
{
	assert(x <= width && y <= height);
	picture[y * width + x] = color.to_rgb().to_int();
}

void BMP::save_as(const char* name)
{
	bmp_create(name, this->picture, this->width, this->height);
}

void test::svg_to_bmp(const char * input_name, const char* output_name, double board_width, double board_height)
{
	//reading in file
	std::cout << "\nreading in " << input_name << " ..." << std::endl;
	std::string str = read::string_from_file(input_name);
	preprocess_str(str);

	//finding out how often draw_to is called
	unsigned int amount_points = 0;
	double distance = 0;
	Board_Vec current_point(0, 0);
	std::function count_points_and_measure_distance = [&](Board_Vec point) {
		amount_points++; 
		distance += abs(current_point - point);
		current_point = point;
	};
	std::function measure_distance = [&](Board_Vec point) {
		distance += abs(current_point - point);
		current_point = point;
	};
	set_output_functions(count_points_and_measure_distance, measure_distance);
	read::evaluate_svg({ str.c_str(), str.length() }, board_width, board_height);
	std::cout << "total distance the plotter moves to draw " << input_name << " is " << distance << " mm\n";

	const double hue_per_point = 1.99 * pi / amount_points;	//just stay under 2 * pi, to not risk hue beeing slightly over 2 * pi in last point, due to rounding error
	HSV hsv_color(0, 1, 1);

	BMP picture(static_cast<uint16_t>(board_width), static_cast<uint16_t>(board_height), { 80, 80, 80 });

	Board_Vec current(0, 0);


	std::function draw_to = [&](Board_Vec point) {
		double gradient = (point.y - current.y) / (point.x - current.x);
		const RGB random_color = { std::rand() % 255, std::rand() % 255, std::rand() % 255 }; //can be used as an alternative to point_color
		const auto point_color = hsv_color;	//please switch the desired color here
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
		}
		current = point; 
		hsv_color.hue += hue_per_point;
	};
	std::function go_to = [&](Board_Vec point) {
		current = point;
	};


	std::cout << "draw picture..." << std::endl;
	set_output_functions(draw_to, go_to);
	read::evaluate_svg({ str.c_str(), str.length() }, board_width, board_height);

	std::cout << "save picture as " << output_name << " ..." << std::endl;
	picture.save_as(output_name);
}

void test::svg_to_bbf(const char* input_name, const char* output_name, double board_width, double board_height)
{
	//reading in file
	std::cout << "\nreading in " << input_name << " ..." << std::endl;
	std::string str = read::string_from_file(input_name);
	preprocess_str(str);
	
	//opening new bbf file
	std::ofstream output;
	output.open(output_name);


	std::function go_to = [&](Board_Vec point) {
		output << "0 " << point.x << " " << point.y << "\n";
	};
	std::function draw_to = [&](Board_Vec point) {
		output << "1 " << point.x << " " << point.y << "\n";
	};

	std::cout << "draw picture..." << std::endl;
	set_output_functions(draw_to, go_to);
	read::evaluate_svg({ str.c_str(), str.length() }, board_width, board_height);

	std::cout << "save picture as " << output_name << " ..." << std::endl;
	output.close();
}
