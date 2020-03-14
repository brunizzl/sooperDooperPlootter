#pragma once

#include <vector>
#include <iostream>

struct Vec2D
{
	double x;
	double y;

	//constant to indicate a Vec2D has not yet been set
	static const Vec2D no_value;
};




Vec2D operator+(Vec2D a, Vec2D b);
Vec2D operator-(Vec2D a, Vec2D b);
Vec2D operator-(Vec2D a);
Vec2D operator*(double factor, Vec2D vec);
bool operator==(Vec2D a, Vec2D b);

std::ostream& operator<<(std::ostream& stream, const Vec2D& coord);


//current point and next point are both inside view box -> draws straight line from current position to next point 
//only next point is inside view box -> goes to next point (does not draw in that case)
//otherwise does nothing
void save_draw_to(Vec2D point);

//goes to point if this point resides inside view box (does not draw)
void save_go_to(Vec2D point);
