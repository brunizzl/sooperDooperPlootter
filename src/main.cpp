#include <iostream>

#include "svgHandling.hpp"

std::string_view in_between(std::string_view original, std::size_t fst, std::size_t snd);

int main(int argc, char* argv[])
{
	///draw_from_file("samples/w3test1.svg", 1000, 500);
	std::string_view test = "halloIbims1Bruno";
	auto o = test.find_first_of('o');
	auto B = test.find_first_of('v');
	std::cout << in_between(test, o, B);
}

