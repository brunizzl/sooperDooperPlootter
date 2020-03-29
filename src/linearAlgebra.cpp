#include <cmath>
#include <cassert>

#include "linearAlgebra.hpp"
#include "svgHandling.hpp"

namespace la {

	const Vec2D Vec2D::no_value = { std::numeric_limits<double>::max(), std::numeric_limits<double>::max() };

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

	double abs(Board_Vec vec)
	{
		return std::sqrt(vec.x * vec.x + vec.y * vec.y);
	}


	Board_Vec operator*(const Transform_Matrix& matrix, Vec2D vec)
	{
		const double& a = matrix.a, b = matrix.b, c = matrix.c, d = matrix.d, e = matrix.e, f = matrix.f,
			x = vec.x, y = vec.y;

		return Board_Vec(a * x + c * y + e,
			             b * x + d * y + f);
	}

	Transform_Matrix skew_x(double angle) 
	{ 
		return { 1, 0, std::tan(angle), 1, 0, 0 }; 
	}

	Transform_Matrix skew_y(double angle) 
	{ 
		return { 1, std::tan(angle), 0, 1, 0, 0 }; 
	}

	std::string_view name_of(Transform transform)
	{
		switch (transform) {
		case Transform::matrix:		return { "matrix" };
		case Transform::translate:	return { "translate" };
		case Transform::scale:		return { "scale" };
		case Transform::rotate:		return { "rotate" };
		case Transform::skew_x:		return { "skewX" };
		case Transform::skew_y:		return { "skewY" };
		}
		assert(false);	//if this assert is hit, you may update the switchcase above.
		return {};
	}


}//namespace la


using namespace la;

std::ostream& operator<<(std::ostream& stream, Vec2D& coord)
{
	stream << "(" << coord.x << ", " << coord.y << ")";
	return stream;
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
