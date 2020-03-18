#include <cmath>

#include "steppers.hpp"
#include "svgHandling.hpp"

const Vec2D Vec2D::no_value = { std::numeric_limits<double>::max(), std::numeric_limits<double>::max() };

Vec2D operator+(Vec2D a, Vec2D b)
{
	return { a.x + b.x, a.y + b.y };
}

Vec2D operator-(Vec2D a, Vec2D b)
{
	return { a.x - b.x, a.y - b.y };
}

Vec2D operator-(Vec2D a)
{
	return { -a.x, -a.y };
}

Vec2D operator*(double factor, Vec2D vec)
{
	return { factor * vec.x, factor * vec.y };
}

bool operator==(Vec2D a, Vec2D b)
{
	return (a.x == b.x && a.y == b.y);
}

double dot(Vec2D u, Vec2D v)
{
	return u.x * v.x + u.y * v.y;
}

double abs(Vec2D vec)
{
	return std::sqrt(vec.x * vec.x + vec.y * vec.y);
}

double angle(Vec2D u, Vec2D v)
{
	const double unsigned_angle = std::acos(dot(u, v) / (abs(u) * abs(v)));
	if (std::isnan(unsigned_angle)) {
		return pi;
	}
	else {
		const double sign = u.x * v.y - u.y - v.x >= 0.0 ? 1.0 : -1.0;
		return sign * unsigned_angle;
	}
}

std::ostream& operator<<(std::ostream& stream, Vec2D& coord)
{
	stream << "(" << coord.x << ", " << coord.y << ")";
	return stream;
}



Vec2D operator*(Matrix2X2 A, Vec2D v)
{
	return Vec2D{ A.a * v.x + A.b * v.y,
				  A.c * v.x + A.d * v.y };
}






Board_Vec::Board_Vec(double x_, double y_)
	:x(x_), y(y_)
{
}

Board_Vec operator+(Board_Vec a, Board_Vec b)
{
	return Board_Vec(a.x + b.x, a.y + b.y);
}

Board_Vec operator-(Board_Vec a, Board_Vec b)
{
	return Board_Vec(a.x - b.x, a.y - b.y);
}

Board_Vec operator-(Board_Vec a)
{
	return Board_Vec(-a.x, -a.y);
}

Board_Vec operator*(double factor, Board_Vec vec)
{
	return Board_Vec(factor * vec.x, factor * vec.y);
}

bool operator==(Board_Vec a, Board_Vec b)
{
	return (a.x == b.x && a.y == b.y);
}

std::ostream& operator<<(std::ostream& stream, Board_Vec& coord)
{
	stream << "(" << coord.x << ", " << coord.y << ")";
	return stream;
}


//stores if the last move command could be exected or was ignored because it lead outside the view box
static bool prev_in_view_box = true;


static std::function<void(Board_Vec)> draw_to = [](Board_Vec point) {std::cout << "------" << point << '\n'; };
static std::function<void(Board_Vec)> go_to   = [](Board_Vec point) {std::cout << "  ->  " << point << '\n'; };

void save_draw_to(Board_Vec point)
{
	const bool next_in_view_box = View_Box::contains(point);
	if (prev_in_view_box && next_in_view_box) {
		draw_to(point);
	}
	else if (next_in_view_box) {
		go_to(point);
	}
	prev_in_view_box = next_in_view_box;
}

void save_go_to(Board_Vec point)
{
	const bool next_in_view_box = View_Box::contains(point);
	if (next_in_view_box) {
		go_to(point);
	}
	prev_in_view_box = next_in_view_box;
}

void set_output_functions(std::function<void (Board_Vec)> new_draw_to, std::function<void (Board_Vec)> new_go_to)
{
	draw_to = new_draw_to;
	go_to   = new_go_to;
}
