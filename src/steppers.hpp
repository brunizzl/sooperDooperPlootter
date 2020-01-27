#pragma once

#include <vector>


struct Coord
{
	double x;
	double y;
};

//draws straight line from start to end
void draw_line(Coord start, Coord end);

//draws staight lines from start to waypoints[1], 
void draw_path(Coord start, const std::vector<Coord>& waypoints);

void draw_circle(Coord center, double radius);

Coord get_position();
