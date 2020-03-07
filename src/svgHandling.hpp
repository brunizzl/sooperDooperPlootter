#pragma once

//#include <svgpp/svgpp.hpp>



#include <vector>
#include "steppers.hpp"

constexpr double pi = 3.1415926535897932384626433832795028841971;

namespace to_path {
	//all data in mm, resolution determines how many lines are used to display the circle
	std::vector<Coord_mm> circle(Coord_mm center, double radius, std::size_t resolution);

	//all points in mm,
	std::vector<Coord_mm> bezier();
}

namespace test {
	void nanosvg();
}