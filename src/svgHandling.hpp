#pragma once



#include <vector>
#include <string>
#include <string_view>

#include "steppers.hpp"

constexpr double pi = 3.1415926535897932384626433832795028841971;


// "main" function
//reads svg at file path "name" and moves robot accordingly
// board_width and board_height are the dimensions of the board the robot draws on in mm
void draw_from_file(const char* const name, double board_width, double board_height);

struct Transform_Matrix
{
	double a, c, e,
		   b, d, f;
	//corresponds to Matrix
	// a c e
	// b d f
	// 0 0 1
	//as specified here: https://www.w3.org/TR/SVG11/coords.html#TransformMatrixDefined
};

//matrix * matrix, as in math
Transform_Matrix operator*(const Transform_Matrix& fst, const Transform_Matrix& snd);

//matrix * vector, as in math. the third coordinate of vec is always 1.
Coord_mm operator*(const Transform_Matrix& matrix, Coord_mm vec);


//here are only arributes listed, that are relevant for plotting.
//style attributes are therefore missing in this struct.
struct Container_Attributes
{
	struct View_Box
	{
		double min_x = 0;		//x-coordinate of the left border
		double min_y = 0;		//y-coordinate of the upper border (positive y is down)
		double width = 100;
		double height = 100;
	} view_box;

	Transform_Matrix transformation;

	//returns attributes of this combined with attributes specified in arguments
	Container_Attributes combine(const View_Box* inner_box = nullptr,
		const Transform_Matrix* inner_transformation = nullptr) const;

	//translates point from system within container to system outside
	//if point lies outside viewbox, not_displayed is returned
	Coord_mm translate(Coord_mm point) const;

	const static Coord_mm not_displayed;
};

//all functions used while parsing/reading
namespace read {

	//what is given in the following part of the string (basically everything written in angle brackets, as "<circle ..>" or "<svg ...>"
	//naming based on https://www.w3.org/TR/SVG11/struct.html
	enum class Elem_Type
	{
		//containers
		svg, 
		svg_end,	// "/svg"
		g,
		g_end,		// "/g"

		//shapes
		line,
		polyline,
		polygon,
		rect,
		ellypse,
		circle,
		path,

		//other things
		unknown,	//if a type is not listed explicitly in Elem_Type, it will be read as type "unknown"
		end,		//if no more elements can be read, because the document is fully read, type "end" is returned
	};

	//array of all types BUT UNKNOWN to be used in range based for loops
	static const Elem_Type all_elem_types[] = { Elem_Type::svg, Elem_Type::svg_end, Elem_Type::line, Elem_Type::polyline, Elem_Type::polygon,
		Elem_Type::rect, Elem_Type::ellypse, Elem_Type::circle, Elem_Type::path, Elem_Type::g, Elem_Type::g_end };

	std::string_view name_of(Elem_Type type);

	std::string_view end_marker(Elem_Type type);

	std::size_t find_skip_quotations(std::string_view search_zone, std::string_view find);

	struct Elem_Data
	{
		Elem_Type type;
		std::string_view content;	//only shows inner part, not the enclosing angle brackets and name 
		//example: 
		// "<circle cx="100" cy="200" r="20"> will be stored as:
		// {Elem_Type::circle, "cx=\"100\" cy=\"200\" r=\"20\""}
	};

	Elem_Data next_elem(std::string_view view);

	void evaluate_part(std::string_view part, const Container_Attributes& attributes);
}

//all lengths in mm, angles in rad
//resolution in draw() determines in how many straight lines the shape is split
namespace shape {

	//straight line from start to end
	struct Line
	{
		Coord_mm start;
		Coord_mm end;

		void draw(const Container_Attributes& attributes) const;
	};

	struct Rectangle
	{
		Coord_mm upper_left;	//position of upper left corner
		double height;
		double width;
		double radius_x;		//corners may be rounded of by a quarter ellypse (== Arc)
		double radius_y;

		void draw(const Container_Attributes& attributes, std::size_t resolution) const;
	};

	struct Circle
	{
		Coord_mm center;
		double radius;

		void draw(const Container_Attributes& attributes, std::size_t resolution) const;
	};

	struct Ellypse
	{
		Coord_mm center;
		double radius_x;
		double radius_y;

		void draw(const Container_Attributes& attributes, std::size_t resolution) const;
	};

	struct Arc
	{
		Ellypse ellypse;
		double start_angle;
		double end_angle;
		bool draw_positive;	//if true, the plotter moves around center of ellypse in mathematical positive direction (counterclockwise)

		void draw(const Container_Attributes& attributes, std::size_t resolution) const;
	};

	struct Quadratic_Bezier
	{
		Coord_mm start;
		Coord_mm control;
		Coord_mm end;

		void draw(const Container_Attributes& attributes, std::size_t resolution) const;
	};

	struct Cubic_Bezier
	{
		Coord_mm start;
		Coord_mm control_1;
		Coord_mm control_2;
		Coord_mm end;

		void draw(const Container_Attributes& attributes, std::size_t resolution) const;
	};
}