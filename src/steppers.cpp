#include "steppers.hpp"

Coord_mm operator+(const Coord_mm a, const Coord_mm b)
{
	return { a.x + b.x, a.y + b.y };
}

Coord_mm operator-(const Coord_mm a, const Coord_mm b)
{
	return { a.x - b.x, a.y - b.y };
}

Coord_mm operator*(const double factor, const Coord_mm vec)
{
	return { factor * vec.x, factor * vec.y };
}

std::ostream& operator<<(std::ostream& stream, const Coord_mm& coord)
{
	stream << "(" << coord.x << ", " << coord.y << ")";
	return stream;
}
