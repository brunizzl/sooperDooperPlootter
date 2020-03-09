#include <iostream>

#include "svgHandling.hpp"

template <typename T>
std::ostream& operator<<(std::ostream& stream, const std::vector<T>& vec)
{
	stream << '{';
	bool first = true;
	for (const auto& elem : vec) {
		if (!std::exchange(first, false)) {
			stream << ", ";
		}
		stream << elem;
	}
	stream << '}';
	return stream;
}

int main(int argc, char* argv[])
{
	//draw_from_file("samples/sandbox.svg", 100, 100);
	//std::cout << read::to_scaled(read::get_attribute_data("cx=\"100\" cy=\"200\" r=\"20mm\"", "r")) << std::endl;
	//view_box::set("-1, 0 500,500");
	//std::cout << view_box::min << " " << view_box::max << std::endl;
	std::cout << read::from_csv(/*"-1, 0 500,500"*/"") << std::endl;
	std::cin.get();
}

