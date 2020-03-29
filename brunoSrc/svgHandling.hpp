#pragma once



#include <vector>
#include <string>
#include <string_view>

#include "linearAlgebra.hpp"

//the svg standard allows for view boxes to be defined inside other view boxes.
//this forces an implementation of the complete standard to check every coordinate transformation, if we are still within the view box.
//this removes the possibility of expressing a combination of transformations as matrix multiplication. 
//i therefore only track the outhermost view box.
class View_Box
{
	static la::Board_Vec min;	//upper left corner of box
	static la::Board_Vec max;	//lower right corner of box

	//called by public set functions
	static la::Transform_Matrix private_set(double min_x, double min_y, double width, double height, double board_width, double board_height);

public:

	//sets view box to have aspect ratio as described in data, but stored in Board units (mm)
	//the transformation matrix from the outhermost svg coordinate system to the board is returned
	static la::Transform_Matrix set(std::string_view data, double board_width, double board_height);
	static la::Transform_Matrix set(double width, double height, double board_width, double board_height);

	//checks if point is contained within view box
	static bool contains(la::Board_Vec point);
};





//all functions used while parsing/reading
namespace read {

	//opens file with name file_name and reads full file in as string
	std::string string_from_file(const char* file_name);

	//assumes str to hold svg and removes comments (everything between "<!--" and "-->")
	//also swaps out newlines within quotes to spaces ("...d=\"M100 100 \n L20 30...\"..." becomes "...d=\"M100 100   L20 30...\"...")
	void preprocess_str(std::string& str);

	//what is given in the following part of the string (basically everything written in angle brackets, as "<circle ..>" or "<svg ...>"
	//naming based on https://www.w3.org/TR/SVG11/struct.html
	enum class Elem_Type
	{
		//containers
		svg, 
		g,			//standing for group

		//shapes
		line,
		polyline,
		polygon,
		rect,
		ellipse,
		circle,
		path,

		//other things
		unknown,	//if a type is not listed explicitly in Elem_Type, it will be read as type "unknown"
		end,		//if the document is fully read, type "end" is returned
	};

	//array of all types BUT UNKNOWN to be used in range based for loops
	static const Elem_Type all_elem_types[] = { Elem_Type::svg, Elem_Type::line, Elem_Type::polyline, Elem_Type::polygon,
		Elem_Type::rect, Elem_Type::ellipse, Elem_Type::circle, Elem_Type::path, Elem_Type::g, };

	std::string_view name_of(Elem_Type type);

	//just like search_zone.find(find, start) but ignores everything written in between quotes
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
	//example: std::string_view("cx=\"100\" cy=\"200\" r=\"20\"").get_attribute_data("cx=") 
	//  yields "100", as this is specified after "cx="
	//if attr_name is not found, "" is returned
	std::string_view get_attribute_data(std::string_view search_zone, std::string_view attr_name);

	//multiple commas/ spaces and combinations thereof are read in as a single seperator.
	//example: from_csv("-1, 30 4  6.35,9") yields { -1.0, 30.0, 4.0, 6.35. 9.0 }
	//if always_comma is set, csv will not seperate at space or '-', but only at ','.
	std::vector<double> from_csv(std::string_view csv, bool always_comma = false);

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
	//if a unit is not implemented here (the relative ones), the function returns the value as if unit was px
	//if val_str turns out to be empty, default_val is returned
	double to_scaled(std::string_view val_str, double default_val = 0);

	static const Unit all_units[] = { Unit::px, Unit::pt, Unit::pc, Unit::mm, Unit::cm, Unit::in };

	//returns Elem_Data of next element and shortens view to after the current element
	Elem_Data take_next_elem(std::string_view& view);

	//sets view box and calls functions for elements in svg
	void evaluate_svg(std::string_view svg_view, double board_width, double board_height);

	//reads all of fragment
	void evaluate_fragment(std::string_view fragment, const la::Transform_Matrix& transform);

	//returns matrix resulting from transformation attributes of group specified in group attributes
	la::Transform_Matrix get_transform_matrix(std::string_view group_attributes);
}




namespace path {

	//all path elements can ether be specified relative to the last coordinate (lower case)
	//or in absolute coordinates (upper case)
	//see documentation here https://www.w3.org/TR/SVG11/paths.html#PathElement
	enum class Path_Elem
	{
		move,					 //Mm
		vertical_line,			 //Vv
		horizontal_line,		 //Hh
		line,					 //Ll
		arc,					 //Aa
		quadr_bezier,		     //Qq or Tt
		cubic_bezier,			 //Cc or Ss
		closed,					 //Zz
		end,          //if a path is fully read, "end" is returned as type of next elem
	};

	//upper case letters stand for the command given in absolute coordinates
	//lower case letters stand for the command to be specified relative to the point drawn at the end of the last command
	enum class Coords_Type
	{
		absolute,
		relative,
	};

	struct Path_Elem_data
	{
		//quite exactly analogous to read::Elem_Data
		std::string_view content;
		Path_Elem type = Path_Elem::end;
		Coords_Type coords_type = Coords_Type::absolute;
	};

	//returns Path_Elem_Data of next element and shortens view to after the current element
	Path_Elem_data take_next_elem(std::string_view& view);

	//do the drawing of as much bezier curves as data can deliver
	//current_point is current position of plotter
	//return where they finnished drawing (the new current_point).
	la::Vec2D process_quadr_bezier(const la::Transform_Matrix& transform_matrix, Path_Elem_data data, la::Vec2D current_point);
	la::Vec2D process_cubic_bezier(const la::Transform_Matrix& transform_matrix, Path_Elem_data data, la::Vec2D current_point);

	//control point is given explicitly -> set to expl
	//control point is to be calculated from the last control point -> set to impl
	//see here how to calculate: https://www.w3.org/TR/SVG11/paths.html#PathDataCurveCommands
	enum class Control_Given
	{
		expl,
		impl,
	};

	//if a control point is only given implicitly, it lies on a line with the last control point and the current point.
	//the distance to the current point is equal to the distance of the last control point and the current point, 
	//only the direction is opposite.
	la::Vec2D compute_contol_point(la::Vec2D last_control_point, la::Vec2D mirror);

	struct Bezier_Data
	{
		//only numbers, no mor letters. ready as is to be plugged into from_csv()
		std::string_view content;

		//if all points (but the start) are given explicitly, this flag is set to true
		//-> if 'C', 'c', 'Q' or 'q' are read in, control_data is set to explicit,
		//-> if 'T', 't', 'S' or 's' are read in, control_data is set to implicit
		Control_Given control_data = Control_Given::expl;
		Coords_Type coords_type = Coords_Type::absolute;
	};

	//returns Bezier_Data specified at start of view & removes prefix of view to start after returned data
	//if no data is found, content is set to "". 
	//both quadratic and cubic share this same function
	Bezier_Data take_next_bezier(std::string_view& view);

	//analogous to process_bezier() functions
	//see here how to calculate: https://www.w3.org/TR/SVG11/paths.html#PathDataEllipticalArcCommands
	la::Vec2D process_arc(const la::Transform_Matrix& transform_matrix, Path_Elem_data data, la::Vec2D current_point);
}




//resolution in draw() determines in how many straight lines the shape is split
//although a straight line can already be described by a straight line perfectly, it may also be of advantage to split it, 
// as the linear interpolation on the plotterside to draw from one point to the nex may suffer from the non-linear nature of the
// transformation from kartesian coordinates to cable lengths.
//transform in draw() is matrix needed to transform from current coordinate system to board system
namespace draw {
	constexpr std::size_t default_res = 100;

	void line     (la::Transform_Matrix transform_matrix, std::string_view parameters, std::size_t resolution = default_res);
	void rect     (la::Transform_Matrix transform_matrix, std::string_view parameters, std::size_t resolution = default_res);
	void circle   (la::Transform_Matrix transform_matrix, std::string_view parameters, std::size_t resolution = default_res);
	void ellipse  (la::Transform_Matrix transform_matrix, std::string_view parameters, std::size_t resolution = default_res);
	void polyline (la::Transform_Matrix transform_matrix, std::string_view parameters, std::size_t resolution = default_res);
	void polygon  (la::Transform_Matrix transform_matrix, std::string_view parameters, std::size_t resolution = default_res);
	void path     (la::Transform_Matrix transform_matrix, std::string_view parameters, std::size_t resolution = default_res);

	//the following functions are called mostly from path()

	//arc is part of ellipse between start_angle and start_angle + delta_angle
	//angles are expected to be in rad
	//note: a rotated ellipse can not be described as an unrotated one, hence we need to drag the rotation matrix into this function.
	void arc(const la::Transform_Matrix& transform_matrix, la::Vec2D center, double rx, double ry, double start_angle,
		double delta_angle, std::size_t resolution = default_res);

	void linear_bezier(la::Board_Vec start, la::Board_Vec end, std::size_t resolution = default_res);
	void quadr_bezier(la::Board_Vec start, la::Board_Vec control, la::Board_Vec end, std::size_t resolution = default_res);
	void cubic_bezier(la::Board_Vec start, la::Board_Vec control_1, la::Board_Vec control_2, la::Board_Vec end, std::size_t resolution = default_res);
}
