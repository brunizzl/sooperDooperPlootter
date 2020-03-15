#include <iostream>

#include "svgHandling.hpp"
#include "test.hpp"

int main(int argc, char* argv[])
{
	const double width = 1000;
	const double height = 500;
	//draw_from_file("samples/w3test1.svg", width, height);
	test::svg_to_bmp("samples/rect01.svg", "samples/bmps/rect01.bmp", width, height);
	test::svg_to_bmp("samples/rect02.svg", "samples/bmps/rect02.bmp", width, height);

	//std::cin.get();
}

