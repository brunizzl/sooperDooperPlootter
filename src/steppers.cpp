#include "steppers.hpp"

#include "svgHandling.hpp"

Vec2D operator+(const Vec2D a, const Vec2D b)
{
	return { a.x + b.x, a.y + b.y };
}

Vec2D operator-(const Vec2D a, const Vec2D b)
{
	return { a.x - b.x, a.y - b.y };
}

Vec2D operator-(const Vec2D a)
{
	return { -a.x, -a.y };
}

Vec2D operator*(const double factor, const Vec2D vec)
{
	return { factor * vec.x, factor * vec.y };
}

std::ostream& operator<<(std::ostream& stream, const Vec2D& coord)
{
	stream << "(" << coord.x << ", " << coord.y << ")";
	return stream;
}



static bool prev_in_view_box = true;

void draw_to(Vec2D point)
{
	if (std::exchange(prev_in_view_box, view_box::contains(point))) {
		//hier bitte basheys sachen aufrufen <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <--
		std::cout << "------" << point << '\n';
	}
	else {
		//altough the previous point was not contained by the view_box, this one might be. hence we need to go there.
		go_to(point);
	}
}

void go_to(Vec2D point)
{
	const bool in_box = view_box::contains(point);
	if (in_box) {
		//hier bitte basheys sachen aufrufen <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <--
		std::cout << "  ->  " << point << '\n';
	}
	prev_in_view_box = in_box;
}



