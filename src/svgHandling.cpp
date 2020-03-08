
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

	Container_Attributes default_attributes;

	auto test = read::next_elem({ str.c_str(), str.length() });
	while (test.type != read::Elem_Type::end) {
		if (test.type != read::Elem_Type::unknown) {
			std::cout << name_of(test.type) << ":\n" << test.content << "\n\n";
		}
		test = read::next_elem({ test.content.data() + test.content.length() });
	}
}

Transform_Matrix operator*(const Transform_Matrix& fst, const Transform_Matrix& snd)
{
	const double& A = fst.a, B = fst.b, C = fst.c, D = fst.d, E = fst.e, F = fst.f,
	              a = snd.a, b = snd.b, c = snd.c, d = snd.d, e = snd.e, f = snd.f;

	return Transform_Matrix{ A * a + C * b,    A * c + C * d,    E + C * f + A * e,
	                         B * a + D * b,    B * c + D * d,    F + D * f + B * e };
}

Coord_mm operator*(const Transform_Matrix& matrix, Coord_mm vec)
{
	const double& a = matrix.a, b = matrix.b, c = matrix.c, d = matrix.d, e = matrix.e, f = matrix.f,
		          x = vec.x, y = vec.y;

	return Coord_mm{ a * x + c * y + e,
	                 b * x + d * y + f };
}


Container_Attributes Container_Attributes::combine(const View_Box* inner_box, const Transform_Matrix* inner_transformation) const
{
	Container_Attributes combined_attributes = *this;
	if (inner_box) {	//only the intersection of both windows is allowed
		const double& min_x = combined_attributes.view_box.min_x = std::max(this->view_box.min_x, inner_box->min_x);
		const double max_x = std::min(this->view_box.min_x + this->view_box.width, inner_box->min_x + inner_box->width);
		combined_attributes.view_box.width  = std::max(max_x - min_x, 0.0);

		const double& min_y = combined_attributes.view_box.min_y = std::max(this->view_box.min_y, inner_box->min_y);
		const double max_y = std::min(this->view_box.min_y + this->view_box.height, inner_box->min_y + inner_box->height);		
		combined_attributes.view_box.height = std::max(max_y - min_y, 0.0);
	}
	if (inner_transformation) {	
		combined_attributes.transformation = this->transformation * (*inner_transformation);
	}
	return combined_attributes;
}

Coord_mm Container_Attributes::translate(Coord_mm point) const
{
	auto [x, y] = this->transformation * point;

	if (x < this->view_box.min_x || x > this->view_box.min_x + this->view_box.width || y < this->view_box.min_y || y > this->view_box.min_y + this->view_box.height) {
		return not_displayed;
	}
	else {
		return { x, y };
	}
}

//initialized to be max possible coordinate in both x and y direction
const Coord_mm Container_Attributes::not_displayed = { std::numeric_limits<double>::max(), std::numeric_limits<double>::max() };

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

void read::evaluate_part(std::string_view part, const Container_Attributes& attributes)
{
	Elem_Data next = next_elem(part);
	while (next.type == Elem_Type::unknown) {
		part.remove_prefix(find_skip_quotations(part, ">"));
		next = next_elem(part);
	}
}


