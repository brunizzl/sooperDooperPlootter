#include "steppers.hpp"

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
