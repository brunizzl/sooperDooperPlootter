
#include "svgHandling.hpp"


#include <cmath>
#include <iostream>
#include <fstream>
#include <streambuf>
#include <cassert>
#include <limits>
#include <algorithm>


void draw_from_file(const char* const name, double board_width, double board_height)
{
	std::string str;
	{
		//taken from: https://stackoverflow.com/questions/2602013/read-whole-ascii-file-into-c-stdstring
		std::ifstream filestream(name);

		filestream.seekg(0, std::ios::end);
		str.reserve(filestream.tellg());
		filestream.seekg(0, std::ios::beg);

		str.assign((std::istreambuf_iterator<char>(filestream)), std::istreambuf_iterator<char>());
	}
	std::cout << str << std::endl;



	auto test = read::next_elem({ str.c_str(), str.length() });
	while (test.type != read::Elem_Type::end) {
		if (test.type != read::Elem_Type::unknown) {
			std::cout << name_of(test.type) << ":\n" << test.content << "\n\n";
		}
		test = read::next_elem({ test.content.data() + test.content.length() });
	}
}

static Coord_mm view_box::min = { 0, 0 };
static Coord_mm view_box::max = { 100, 100 };

//initialized to be max possible coordinate in both x and y direction
static const Coord_mm view_box::outside = { std::numeric_limits<double>::max(), std::numeric_limits<double>::max() };

//allows to initialize a matrix by writing it out as in math
constexpr Transform_Matrix in_matrix_order(double a, double  c, double  e, double  b, double  d, double  f)
{
	return Transform_Matrix{ a, b, c, d, e, f };
}

Transform_Matrix operator*(const Transform_Matrix& fst, const Transform_Matrix& snd)
{
	const double& A = fst.a, B = fst.b, C = fst.c, D = fst.d, E = fst.e, F = fst.f,
	              a = snd.a, b = snd.b, c = snd.c, d = snd.d, e = snd.e, f = snd.f;

	return in_matrix_order(A * a + C * b,    A * c + C * d,    E + C * f + A * e,
	                       B * a + D * b,    B * c + D * d,    F + D * f + B * e);
}

Coord_mm operator*(const Transform_Matrix& matrix, Coord_mm vec)
{
	const double& a = matrix.a, b = matrix.b, c = matrix.c, d = matrix.d, e = matrix.e, f = matrix.f,
		          x = vec.x, y = vec.y;

	return Coord_mm{ a * x + c * y + e,
	                 b * x + d * y + f };
}

Coord_mm to_board_system(const Transform_Matrix& transform, Coord_mm point)
{
	auto [x, y] = transform * point;
	if (x < view_box::min.x || x > view_box::max.x || y < view_box::min.y || y > view_box::max.y) {
		return view_box::outside;
	}
	else {
		return { x, y };
	}
}

using namespace read;

std::string_view read::name_of(Elem_Type type)
{
	switch (type) {
	case Elem_Type::svg:		return { "svg" };
	case Elem_Type::svg_end:	return { "/svg" };
	case Elem_Type::g:			return { "g" };
	case Elem_Type::g_end:		return { "/g" };
	case Elem_Type::line:		return { "line" };
	case Elem_Type::polyline:	return { "polyline" };
	case Elem_Type::polygon:	return { "polygon" };
	case Elem_Type::rect:		return { "rect" };
	case Elem_Type::ellypse:	return { "ellypse" };
	case Elem_Type::circle:		return { "circle" };
	case Elem_Type::path:		return { "path" };
	case Elem_Type::unknown:	return { "unknown" };
	case Elem_Type::end:		return { "end" };
	}
	assert(false);	//if this assert is hit, you may update the switchcase above.
	return {};
}

std::string_view read::end_marker(Elem_Type type)
{
	switch (type) {
	case Elem_Type::svg:			
	case Elem_Type::svg_end:		
	case Elem_Type::g:				
	case Elem_Type::g_end:			
		return { ">" };
	case Elem_Type::line:			
	case Elem_Type::polyline:		
	case Elem_Type::polygon:		
	case Elem_Type::rect:			
	case Elem_Type::ellypse:		
	case Elem_Type::circle:			
	case Elem_Type::path:		
		return { "/>" };
	case Elem_Type::unknown:	
		return { ">" };		//just assume the least restricting marker
	}
	assert(false);	//if this assert is hit, you may update the switchcase above.
	return {};
}

std::size_t read::find_skip_quotations(std::string_view search_zone, std::string_view search_obj)
{
	std::size_t next_quotation_start = search_zone.find_first_of('\"');
	std::size_t prev_quotation_end = 0;

	while (next_quotation_start != std::string::npos) {
		const std::string_view search_section = { search_zone.data() + prev_quotation_end, next_quotation_start - prev_quotation_end };
		const std::size_t found = search_section.find(search_obj);
		if (found != std::string::npos) {
			return found + prev_quotation_end;	//search_section has offset of prev_quotation_end from search_zone.
		}
		prev_quotation_end = search_zone.find_first_of('\"', next_quotation_start + 1);
		if (prev_quotation_end == std::string::npos) {
			throw std::exception("function read::find_skip_quotations(): quotation (using \"\") was started, but not ended.");
		}
		next_quotation_start = search_zone.find_first_of('\"', prev_quotation_end + 1);
	}

	return search_zone.find(search_obj, prev_quotation_end);
}

Elem_Data read::next_elem(std::string_view view)
{
	const std::size_t open_bracket = find_skip_quotations(view, "<");
	if (open_bracket == std::string::npos) {
		return { Elem_Type::end, "" };
	}
	view.remove_prefix(open_bracket + 1);	//if view started as "\n <circle cx=...", it now is "circle cx=..."

	for (Elem_Type type : all_elem_types) {
		const std::string_view type_name = name_of(type);
		if (view.compare(0, type_name.length(), type_name) == 0) {
			view.remove_prefix(type_name.length());		//"circle cx=..." becomes " cx=..."
			const std::size_t end = find_skip_quotations(view, end_marker(type));
			if (end == std::string::npos) {
				throw std::exception("function read::next_elem(): element type identified, but end marker of element not found");
			}
			view.remove_suffix(view.length() - end);
			return { type, view };
		}
	}
	return { Elem_Type::unknown, "" };
}

void read::evaluate_fragment(std::string_view fragment, const Transform_Matrix& transform)
{
	Elem_Data next = next_elem(fragment);
	while (next.type == Elem_Type::unknown) {
		fragment.remove_prefix(find_skip_quotations(fragment, ">"));
		next = next_elem(fragment);
	}
}


