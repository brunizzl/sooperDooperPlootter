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

//used to simplify some Vector equations
struct Matrix2X2
{
	double a, b, c, d;
	//corresponds to Matrix 
	// a b
	// c d
};

Vec2D operator*(Matrix2X2 A, Vec2D v);


//exactly analogous to Vec2D, but different type to make sure, 
//only transformed vectors are drawn
struct Board_Vec
{
	double x;
	double y;

	explicit Board_Vec(double x_, double y_);	//do not allow brace enclosed initialisation for Board_Vec
};

Board_Vec operator+(Board_Vec a, Board_Vec b);
Board_Vec operator-(Board_Vec a, Board_Vec b);
Board_Vec operator-(Board_Vec a);
Board_Vec operator*(double factor, Board_Vec vec);
bool operator==(Board_Vec a, Board_Vec b);

std::ostream& operator<<(std::ostream& stream, const Board_Vec& coord);






//current point and next point are both inside view box -> draws straight line from current position to next point 
//only next point is inside view box -> goes to next point (does not draw in that case)
//otherwise does nothing
void save_draw_to(Board_Vec point);

//goes to point if this point resides inside view box (does not draw)
void save_go_to(Board_Vec point);
