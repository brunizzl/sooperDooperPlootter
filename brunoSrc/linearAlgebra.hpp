#pragma once

#include <vector>
#include <iostream>
#include <functional>

namespace la {	//la standing for linear algebra

	constexpr double pi = 3.1415926535897932384626433832795028841971;

	struct Vec2D
	{
		double x;
		double y;

		//constant to indicate a Vec2D has not yet been set
		static const Vec2D no_value;
	};

	//all operators are intended to work as in math.
	constexpr Vec2D operator+(Vec2D a, Vec2D b) { return { a.x + b.x, a.y + b.y }; }
	constexpr Vec2D operator-(Vec2D a, Vec2D b) { return { a.x - b.x, a.y - b.y }; }
	constexpr Vec2D operator-(Vec2D a) { return { -a.x, -a.y }; }
	constexpr Vec2D operator*(double factor, Vec2D vec) { return { factor * vec.x, factor * vec.y }; }
	constexpr bool operator==(Vec2D a, Vec2D b) { return (a.x == b.x && a.y == b.y); }

	//returns dot-product of u and v
	constexpr double dot(Vec2D u, Vec2D v) { return u.x * v.x + u.y * v.y; }

	//returns 2-Norm of vec
	double abs(Vec2D vec);

	//signed angle between u and v
	double angle(Vec2D u, Vec2D v);

	//used to simplify some Vector equations
	struct Matrix2X2
	{
		double a, b, c, d;
		//corresponds to Matrix 
		// a b
		// c d
	};

	constexpr Vec2D operator*(Matrix2X2 A, Vec2D v)
	{
		return Vec2D{ A.a * v.x + A.b * v.y,
					  A.c * v.x + A.d * v.y };
	}


	//exactly analogous to Vec2D, but different type to make sure, 
	//only transformed vectors are drawn
	struct Board_Vec
	{
		double x;
		double y;

		explicit Board_Vec(double x_, double y_);	//do not allow brace enclosed initialisation for Board_Vec
	};

	//these can not be constexpr, beacuse Board_Ves hac an explicit constructor :(
	Board_Vec operator+(Board_Vec a, Board_Vec b);
	Board_Vec operator-(Board_Vec a, Board_Vec b);
	Board_Vec operator-(Board_Vec a);
	Board_Vec operator*(double factor, Board_Vec vec);
	bool operator==(Board_Vec a, Board_Vec b);

	//returns 2-Norm of vec
	double abs(Board_Vec vec);




	struct Transform_Matrix
	{
		double a, b, c, d, e, f;
		//corresponds to Matrix
		// a c e
		// b d f
		// 0 0 1
		//as specified here: https://www.w3.org/TR/SVG11/coords.html#TransformMatrixDefined
	};

	//allows to initialize a matrix by writing it out as in math
	//because   a c e
	//          b d f
	//          0 0 1  (written as a c e b d f) is not the order of the alphabet
	constexpr Transform_Matrix in_matrix_order(double a, double  c, double  e, double  b, double  d, double  f)
	{
		return Transform_Matrix{ a, b, c, d, e, f };
	}

	//matrix * matrix, as in math
	constexpr Transform_Matrix operator*(const Transform_Matrix& fst, const Transform_Matrix& snd)
	{
		const double& A = fst.a, B = fst.b, C = fst.c, D = fst.d, E = fst.e, F = fst.f,
		              a = snd.a, b = snd.b, c = snd.c, d = snd.d, e = snd.e, f = snd.f;

		return in_matrix_order(A * a + C * b, A * c + C * d, E + C * f + A * e,
		                       B * a + D * b, B * c + D * d, F + D * f + B * e);
	}

	//matrix * vector, as in math. the third coordinate of vec is always 1.
	Board_Vec operator*(const Transform_Matrix& matrix, Vec2D vec);

	//create matrices from different transformations
	//these are also all taken from here: https://www.w3.org/TR/SVG11/coords.html#TransformMatrixDefined
	constexpr Transform_Matrix translate(Vec2D t) { return { 1, 0, 0, 1, t.x, t.y }; }
	constexpr Transform_Matrix scale(double sx, double sy) { return { sx, 0, 0, sy, 0, 0 }; }

	//angle expected in rad
	constexpr Transform_Matrix rotate(double angle)
	{
		return in_matrix_order(std::cos(angle), -std::sin(angle), 0,
		                       std::sin(angle),  std::cos(angle), 0);
	}
	constexpr Transform_Matrix rotate(double angle, Vec2D pivot) { return translate(pivot) * rotate(angle) * translate(-pivot); }

	Transform_Matrix skew_x(double angle);
	Transform_Matrix skew_y(double angle);

	enum class Transform
	{
		matrix,
		translate,
		scale,
		rotate,
		skew_x,
		skew_y,
	};

	std::string_view name_of(Transform transform);

	static const Transform all_transforms[] = { Transform::matrix, Transform::translate, Transform::scale,
		Transform::rotate, Transform::skew_x, Transform::skew_y, };

} //namespace la

std::ostream& operator<<(std::ostream& stream, const la::Vec2D& coord);
std::ostream& operator<<(std::ostream& stream, const la::Board_Vec& coord);

//current point and next point are both inside view box -> draws straight line from current position to next point 
//only next point is inside view box -> goes to next point (does not draw in that case)
//otherwise does nothing
void save_draw_to(la::Board_Vec point);

//goes to point if this point resides inside view box (does not draw)
void save_go_to(la::Board_Vec point);

//allows testing to redirect the output to other places
void set_output_functions(std::function<void (la::Board_Vec)> new_draw_to, std::function<void (la::Board_Vec)> new_go_to);