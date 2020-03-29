
#include <cassert>
#include <cmath>
#include <functional>
#include <iostream>
#include <fstream>

#include "test.hpp"
#include "linearAlgebra.hpp"
#include "svgHandling.hpp"
#include "libBMP.h"



uint32_t RGB::to_int()
{
	return ((uint32_t)this->red * 256 + this->green) * 256 + this->blue;
}

HSV::HSV(double hue_, double saturation_, double value_)
	:hue(hue_), saturation(saturation_), value(value_)
{
	assert(hue_ >= 0 && hue_ <= 2 * la::pi);
	assert(saturation_ >= 0 && saturation_ <= 1);
	assert(value_ >= 0 && value_ <= 1);
}

RGB HSV::to_rgb()
{
	//formulas taken from here : https://en.wikipedia.org/wiki/HSL_and_HSV

	const double angle = this->hue * 3 / la::pi;  //in interval [0, 5]
	const double chroma = this->value * this->saturation;
	const double rest = chroma * (1.0 - std::fabs(std::fmod(angle, 2.0) - 1.0));

	double red = 0, green = 0, blue = 0;

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

void BMP::draw_mesh(uint16_t mesh_size, RGB mesh_color)
{
	//drawing vertical lines
	for (uint16_t x = mesh_size; x < this->width; x += mesh_size) {
		for (uint16_t y = 0; y < this->height; y++) {
			this->set_pixel(x, y, mesh_color);
		}
	}
	//drawing horizontal lines
	for (uint16_t y = mesh_size; y < this->height; y += mesh_size) {
		for (uint16_t x = 0; x < this->width; x++) {
			this->set_pixel(x, y, mesh_color);
		}
	}
}

void BMP::save_as(const char* name)
{
	bmp_create(name, this->picture, this->width, this->height);
}

void test::svg_to_bmp(const std::string& svg_str, const char* output_name, double board_width, double board_height, uint16_t mesh_size, double scaling_factor)
{
	//finding out how often draw_to is called
	unsigned int amount_points = 0;
	double distance = 0;
	la::Board_Vec current_point(0, 0);
	bool pen_down = false;
	unsigned int times_pen_moved_down = 0;
	std::function compute_draw_to_values = [&](la::Board_Vec point) {
		amount_points++; 
		distance += abs(current_point - point);
		current_point = point;
		if (!pen_down) {
			times_pen_moved_down++;
		}
		pen_down = true;
	};
	std::function compute_go_to_values = [&](la::Board_Vec point) {
		distance += abs(current_point - point);
		current_point = point;
		pen_down = false;
	};
	set_output_functions(compute_draw_to_values, compute_go_to_values);
	read::evaluate_svg({ svg_str.c_str(), svg_str.length() }, board_width, board_height);

	std::cout << "total distance the plotter moves is " << distance << " mm\n";
	std::cout << "the pen was moved down " << times_pen_moved_down << " times\n";
	const unsigned int time_in_seconds = static_cast<unsigned int>(distance / 4.44 + times_pen_moved_down * 1.0);
	std::cout << "the estimated time to draw is " << time_in_seconds / 60 << " minutes and " << time_in_seconds % 60 << " seconds\n";

	const double hue_per_point = 1.99 * la::pi / amount_points;	//just stay under 2 * pi, to not risk hue beeing slightly over 2 * pi in last point, due to rounding error
	HSV hsv_color(0, 1, 1);

	BMP picture(static_cast<uint16_t>(board_width * scaling_factor), static_cast<uint16_t>(board_height * scaling_factor), { 80, 80, 80 });
	if (mesh_size > 0) {
		picture.draw_mesh(static_cast<uint16_t>(mesh_size * scaling_factor), { 0, 0, 0 });
	}

	la::Board_Vec current(0, 0);


	std::function draw_to = [&](la::Board_Vec point) {
		point = scaling_factor * point;
		double gradient = (point.y - current.y) / (point.x - current.x);
		const RGB random_color = { 
			static_cast<unsigned char>(std::rand() % 255), 
			static_cast<unsigned char>(std::rand() % 255), 
			static_cast<unsigned char>(std::rand() % 255) }; //can be used as an alternative to point_color
		const auto point_color = hsv_color;	//please switch the desired color here
		if (std::abs(gradient) < 1) {
			const double y_axis_offset = point.y - gradient * point.x;
			if (current.x < point.x) {
				for (int x = static_cast<int>(current.x); x <= point.x; x++) {
					picture.set_pixel(x, static_cast<uint16_t>(gradient * x + y_axis_offset), point_color);
				}
			}
			else {
				for (int x = static_cast<int>(current.x); x >= point.x; x--) {
					picture.set_pixel(x, static_cast<uint16_t>(gradient * x + y_axis_offset), point_color);
				}
			}
		}
		else {
			gradient = 1 / gradient;
			const double x_axis_offset = point.x - gradient * point.y;
			if (current.y < point.y) {
				for (int y = static_cast<int>(current.y); y <= point.y; y++) {
					picture.set_pixel(static_cast<uint16_t>(gradient * y + x_axis_offset), y, point_color);
				}
			}
			else {
				for (int y = static_cast<int>(current.y); y >= point.y; y--) {
					picture.set_pixel(static_cast<uint16_t>(gradient * y + x_axis_offset), y, point_color);
				}
			}
		}
		current = point; 
		hsv_color.hue += hue_per_point;
	};
	std::function go_to = [&](la::Board_Vec point) {
		point = scaling_factor * point;
		current = point;
	};


	std::cout << "draw picture..." << std::endl;
	set_output_functions(draw_to, go_to);
	read::evaluate_svg({ svg_str.c_str(), svg_str.length() }, board_width, board_height);

	std::cout << "save picture as " << output_name << " ..." << std::endl;
	picture.save_as(output_name);
}

void test::svg_to_bbf(const std::string& svg_str, const char* output_name, double board_width, double board_height)
{
	//opening new bbf file
	std::ofstream output;
	output.open(output_name);

	std::function go_to = [&](la::Board_Vec point) {
		output << "0 " << point.x << " " << point.y << "\n";
	};
	std::function draw_to = [&](la::Board_Vec point) {
		output << "1 " << point.x << " " << point.y << "\n";
	};

	std::cout << "draw picture..." << std::endl;
	set_output_functions(draw_to, go_to);
	read::evaluate_svg({ svg_str.c_str(), svg_str.length() }, board_width, board_height);

	std::cout << "save picture as " << output_name << " ..." << std::endl;
	output.close();
}

void test::read_string_to_bmp_and_bbf(const char* input_name, double board_width, double board_height)
{
	const std::string svg_name = std::string("samples/")      + std::string(input_name) + std::string(".svg");
	const std::string bbf_name = std::string("samples/bbfs/") + std::string(input_name) + std::string(".bbf");
	const std::string bmp_name = std::string("samples/bmps/") + std::string(input_name) + std::string(".bmp");

	std::cout << "\nreading in " << svg_name << " ..." << std::endl;
	std::string content_str = read::string_from_file(svg_name.c_str());
	preprocess_str(content_str);
	test::svg_to_bmp(content_str, bmp_name.c_str(), board_width, board_height, 100, 2);
	test::svg_to_bbf(content_str, bbf_name.c_str(), board_width, board_height);
}
