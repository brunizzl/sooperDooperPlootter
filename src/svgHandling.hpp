#pragma once



#include <vector>
#include <string>
#include <string_view>

#include "steppers.hpp"

constexpr double pi = 3.1415926535897932384626433832795028841971;


// "main" function
//reads svg at file path "name" and moves plotter accordingly
// board_width and board_height are the dimensions of the board the plotter draws on in mm
void draw_from_file(const char* const name, double board_width, double board_height);

//assumes str to hold svg and removes everything between "<!--" and "-->"
void remove_comments(std::string& str);

//the svg standard allows for view boxes to be defined inside other view boxes.
//this forces an implementation of the complete standard to not save the view box as rectangle, but as polygon.
//also there may be multible instances of view boxes.
//i ignore all view boxes but the first one and live a happy live.
namespace view_box {
	extern Vec2D min;	//upper left corner of box
	extern Vec2D max;	//lower right corner of box

	void set(std::string_view data);

	bool contains(Vec2D point);
}




struct Transform_Matrix
{
	double a, b, c, d, e, f;
	//corresponds to Matrix
	// a c e
	// b d f
	// 0 0 1
	//as specified here: https://www.w3.org/TR/SVG11/coords.html#TransformMatrixDefined
};

//matrix * matrix, as in math
Transform_Matrix operator*(const Transform_Matrix& fst, const Transform_Matrix& snd);

//matrix * vector, as in math. the third coordinate of vec is always 1.
Vec2D operator*(const Transform_Matrix& matrix, Vec2D vec);

//create matrices from different transformations
//these are also all taken from here: https://www.w3.org/TR/SVG11/coords.html#TransformMatrixDefined
Transform_Matrix translate(Vec2D t);
Transform_Matrix scale(double sx, double sy);
Transform_Matrix rotate(double angle);	//angle in rad
Transform_Matrix rotate(double angle, Vec2D pivot);	//angle in rad (but stored in svg as degree)
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

//all functions used while parsing/reading
namespace read {

	//what is given in the following part of the string (basically everything written in angle brackets, as "<circle ..>" or "<svg ...>"
	//naming based on https://www.w3.org/TR/SVG11/struct.html
	enum class Elem_Type
	{
		//containers
		svg, 
		g,			//group

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
		end,		//if the document is fully read, type "end" is returned
	};

	//array of all types BUT UNKNOWN to be used in range based for loops
	static const Elem_Type all_elem_types[] = { Elem_Type::svg, Elem_Type::line, Elem_Type::polyline, Elem_Type::polygon,
		Elem_Type::rect, Elem_Type::ellypse, Elem_Type::circle, Elem_Type::path, Elem_Type::g, };

	std::string_view name_of(Elem_Type type);

	std::string_view end_marker(Elem_Type type);

	std::size_t find_skip_quotations(std::string_view search_zone, std::string_view find, std::size_t start = 0);

	struct Elem_Data
	{
		Elem_Type type = Elem_Type::unknown;
		std::string_view content;	//only shows inner part, not the enclosing angle brackets and name 
		//example: 
		// "<circle cx="100" cy="200" r="20"> will be stored as:
		// {Elem_Type::circle, "cx=\"100\" cy=\"200\" r=\"20\""}
	};

	//returns view to the part of content describing your attribute
	//example: std::string_view("cx=\"100\" cy=\"200\" r=\"20\"").get_attribute_data("cx=\"") 
	//  yields "100", as this is specified after "cx=\""
	//if attr_name is not found, "" is returned
	std::string_view get_attribute_data(std::string_view search_zone, std::string_view attr_name);

	//multiple commas/ spaces and combinations thereof are read in as a single seperator.
	//example: from_csv("-1, 30 4  6.35,9") yields { -1.0, 30.0, 4.0, 6.35. 9.0 }
	std::vector<double> from_csv(std::string_view csv);

	//units as specified by w3: https://www.w3.org/TR/SVG11/coords.html#Units
	enum class Unit
	{
		/*em, ex,*/ px, pt, pc, mm, cm, in,
	};

	//returns unit code as string view. example: name_of(Unit::cm) yields "cm"
	std::string_view name_of(Unit unit);

	//assumes to have only the numer and (optionally) a unit passed as parameter
	//example: to_scaled("100")   yields 100.0
	//         to_scaled("100cm") yields 3543.307
	//if a unit is not implemented here (the relative ones), the function returns unknown_unit constant 
	//if val_str turns out to be empty, default_val is returned
	double to_scaled(std::string_view val_str, double default_val = 0);

	extern const double unknown_unit;

	static const Unit all_units[] = { Unit::px, Unit::pt, Unit::pc, Unit::mm, Unit::cm, Unit::in };

	Elem_Data next_elem(std::string_view view);

	//reads all of fragment
	void evaluate_fragment(std::string_view fragment, const Transform_Matrix& transform);

	//returns matrix resulting from transformation attributes of group specified in group attributes
	Transform_Matrix get_transform_matrix(std::string_view group_attributes);
}




namespace path {

	//all path elements can ether be specified relative to the last coordinate (lower case)
	//or in absolute coordinates (upper case)
	enum class Path_Elem
	{
		move,					 //M
		vertical_line,			 //V
		horizontal_line,		 //H
		line,					 //L
		arc,					 //A
		quadratic_bezier,		 //Q
		cubic_bezier,			 //C
		closed,					 //Z
	};

	struct Path_Elem_data
	{
		//quite exactly analogous to read::Elem_Data
		std::string_view content;
		Path_Elem type;
		bool absolute_coords;
	};

	//quite exactly analogous to read::next_elem()
	Path_Elem_data next_elem(std::string_view view);
}




//resolution in draw() determines in how many straight lines the shape is split
//transform in draw() is matrix needed to transform from current coordinate system to board system
namespace draw {
	constexpr std::size_t default_res = 3;

	void line				(Transform_Matrix transform_matrix, std::string_view parameters, std::size_t resolution = default_res);
	void rect				(Transform_Matrix transform_matrix, std::string_view parameters, std::size_t resolution = default_res);
	void circle				(Transform_Matrix transform_matrix, std::string_view parameters, std::size_t resolution = default_res);
	void ellypse			(Transform_Matrix transform_matrix, std::string_view parameters, std::size_t resolution = default_res);
	void polyline			(Transform_Matrix transform_matrix, std::string_view parameters, std::size_t resolution = default_res);
	void polygon			(Transform_Matrix transform_matrix, std::string_view parameters, std::size_t resolution = default_res);
	void path				(Transform_Matrix transform_matrix, std::string_view parameters, std::size_t resolution = default_res);

	//the following functions are called mostly from path()
	//IMPORTANT: all lengths and vectors are assumed to already be transformed.

	//arc is part of ellypse between start_angle and end_angle
	//if mathematical_positive is true, the arc from start to end turning counterclockwise is drawn, clockwise if false
	//angles are expected to be in rad
	void arc(Vec2D center, double rx, double ry, double start_angle, double end_angle, 
		bool mathematical_positive, std::size_t resolution = default_res);

	void path_line(Vec2D start, Vec2D end, std::size_t resolution = default_res);
	void quadratic_bezier(Vec2D start, Vec2D control, Vec2D end, std::size_t resolution = default_res);
	void cubic_bezier(Vec2D start, Vec2D control_1, Vec2D control_2, Vec2D end, std::size_t resolution = default_res);
}