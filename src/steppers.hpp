#pragma once

#include <vector>
#include <iostream>

struct Coord_mm
{
	double x;
	double y;
};

Coord_mm operator+(const Coord_mm a, const Coord_mm b);
Coord_mm operator-(const Coord_mm a, const Coord_mm b);
Coord_mm operator*(const double factor, const Coord_mm vec);

std::ostream& operator<<(std::ostream& stream, const Coord_mm& coord);

//draws straight line from current position to point
void draw_to(Coord_mm point);

//goes from current position to point without drawing
void go_to(Coord_mm point);
