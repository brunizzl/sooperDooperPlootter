#pragma once

#include <vector>
#include <iostream>

struct Vec2D
{
	double x;
	double y;
};

constexpr Vec2D no_value = { std::numeric_limits<double>::max(), std::numeric_limits<double>::max() };

Vec2D operator+(const Vec2D a, const Vec2D b);
Vec2D operator-(const Vec2D a, const Vec2D b);
Vec2D operator-(const Vec2D a);
Vec2D operator*(const double factor, const Vec2D vec);
bool operator==(const Vec2D a, const Vec2D b);

std::ostream& operator<<(std::ostream& stream, const Vec2D& coord);

//draws straight line from current position to point
void draw_to(Vec2D point);

//goes from current position to point without drawing
void go_to(Vec2D point);
