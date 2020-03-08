#pragma once



#include <vector>
#include <string>
#include <string_view>

#include "steppers.hpp"

constexpr double pi = 3.1415926535897932384626433832795028841971;


// "main" function
//reads svg at file path "name" and moves robot accordingly
void draw_from_file(const char* const name);

//all functions used while parsing/reading
namespace read {

	//what is given in the following part of the string (basically everything written in angle brackets, as "<circle ..>" or "<svg ...>"
	//names based on https://www.w3.org/TR/SVG11/struct.html
	enum class Elem_Type
	{
		//outhermost element
		svg, 
		svg_end,	// "/svg"

		//shapes
		line,
		polyline,
		polygon,
		rect,
		ellypse,
		circle,
		path,

		//other things
		transform,
		g,
		g_end,		// "/g"
		unknown,	//if a type is not listed explicitly here, it will be read as type "unknown"
	};

	//array of all types BUT UNKNOWN to be used in range based for loops
	static const Elem_Type all_elem_types[] = { Elem_Type::svg, Elem_Type::svg_end, Elem_Type::line, Elem_Type::polyline, Elem_Type::polygon,
		Elem_Type::rect, Elem_Type::ellypse, Elem_Type::circle, Elem_Type::path, Elem_Type::g, Elem_Type::g_end, Elem_Type::transform };

	std::string_view name_of(Elem_Type type);

	std::size_t find_skip_quotations(std::string_view search_zone, std::string_view find);

	struct Transformation
	{
		Coord_mm translation = { 0, 0 };
		double rotation = 0;
		double scale = 1;

		//translates to untransformed coordinates
		Coord_mm translate(Coord_mm) const;
	};

	struct Elem_Data
	{
		Elem_Type type;
		std::string_view content;	//only shows inner part, not the enclosing angle brackets and name 
		//example: 
		// "<circle cx="100" cy="200" r="20"> will be stored as:
		// {Elem_Type::circle, "cx=\"100\" cy=\"200\" r=\"20\""}
	};

	Elem_Data next_elem(std::string_view view);
}

namespace draw {
	//all data in mm, resolution determines how many lines are used to display the circle
	std::vector<Coord_mm> circle(Coord_mm center, double radius, std::size_t resolution);

	//all points in mm,
	std::vector<Coord_mm> bezier();
}

namespace test {

}