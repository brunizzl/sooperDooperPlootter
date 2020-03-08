#pragma once

#include <vector>


struct Coord_mm
{
	double x;
	double y;
};

//draws straight line from start to end
void draw_line(Coord_mm start, Coord_mm end);

//draws staight lines from start to waypoints[1], 
void draw_path(Coord_mm start, const std::vector<Coord_mm>& waypoints);