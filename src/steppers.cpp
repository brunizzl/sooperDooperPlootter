#include "steppers.hpp"

#include "svgHandling.hpp"

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

std::ostream& operator<<(std::ostream& stream, Vec2D& coord)
{
	stream << "(" << coord.x << ", " << coord.y << ")";
	return stream;
}


//stores if the last move command could be exected or was ignored, as it lead putside the view box
static bool prev_in_view_box = true;

void save_draw_to(Vec2D point)
{
	const bool next_in_view_box = view_box::contains(point);
	if (prev_in_view_box && next_in_view_box) {
		//hier bitte basheys sachen aufrufen <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <--
		std::cout << "------" << point << '\n';
	}
	else if (next_in_view_box) {
		//hier bitte basheys sachen aufrufen <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <--
		std::cout << "  ->  " << point << '\n';
	}
	prev_in_view_box = next_in_view_box;
}

void save_go_to(Vec2D point)
{
	const bool next_in_view_box = view_box::contains(point);
	if (next_in_view_box) {
		//hier bitte basheys sachen aufrufen <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <--
		std::cout << "  ->  " << point << '\n';
	}
	prev_in_view_box = next_in_view_box;
}



