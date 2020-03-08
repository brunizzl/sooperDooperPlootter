#include <iostream>

#include "svgHandling.hpp"

int main(int argc, char* argv[])
{
	draw_from_file("samples/sandbox.svg", 100, 100);
	std::cin.get();
}

