#include <iostream>

#include "svgHandling.hpp"

int main(int argc, char* argv[])
{
	std::cout << read::find_skip_quotations("\"hi\" hi", "hi");
	//draw_from_file("samples/test1.svg");
	std::cin.get();
}

