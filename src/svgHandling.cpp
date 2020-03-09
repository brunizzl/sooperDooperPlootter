
#include "svgHandling.hpp"


#include <cmath>
#include <iostream>
#include <fstream>
#include <streambuf>
#include <cassert>
#include <limits>
#include <algorithm>
#include <charconv>


//convinience function used in get_attribute_data() and view_box::set()
std::string_view shorten_to(std::string_view view, std::size_t new_length)
{
	assert(new_length <= view.length());
	return { view.data(), new_length };
}

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

	//reading in head data (such as viewbox)
	std::size_t current_pos = 0;	//the part of the string starting at current_pos is parsed in this moment. 
	read::Elem_Data head;
	do {
		head = read::next_elem({ str.c_str() + current_pos, str.length() - current_pos });
		current_pos = head.content.data() - str.c_str();
	} while (head.type != read::Elem_Type::svg);

	//updating view_box borders
	std::string_view view_box_data = read::get_attribute_data(head.content, "viewBox");
	if (view_box_data.length()) {
		view_box::set(view_box_data);
	}

	//creating transformation to display viewbox in board coordinate system (mm)
	double stretching_factor = std::min(board_width / (view_box::max.x - view_box::min.x), board_height / (view_box::max.y - view_box::min.y));
	Transform_Matrix to_board = scale(stretching_factor, stretching_factor);
	
	std::string_view picture = { str.c_str() + current_pos, str.length() - current_pos };
	picture = shorten_to(picture, read::find_skip_quotations(picture, "</svg>"));
	read::evaluate_fragment(picture, to_board);
}

Vec2D view_box::min = { 0, 0 };
Vec2D view_box::max = { 100, 100 };

//initialized to be max possible coordinate in both x and y direction
const Vec2D view_box::outside = { std::numeric_limits<double>::max(), std::numeric_limits<double>::max() };

void view_box::set(std::string_view data)
{
	const std::size_t end_min_x = data.find_first_of(", ");	//marks end of the first number (witch specifies view_box::min.x)
	view_box::min.x = read::to_scaled(shorten_to(data, end_min_x));
	data.remove_prefix(data.find_first_not_of(", ", end_min_x));	//now starts with second number

	const std::size_t end_min_y = data.find_first_of(", ");
	view_box::min.y = read::to_scaled(shorten_to(data, end_min_y));
	data.remove_prefix(data.find_first_not_of(", ", end_min_y));

	const std::size_t end_width = data.find_first_of(", ");
	view_box::max.x = read::to_scaled(shorten_to(data, end_width)) + view_box::min.x;
	data.remove_prefix(data.find_first_not_of(", ", end_width));

	//data only contains the last number
	view_box::max.y = read::to_scaled(data) + view_box::min.y;
}


//allows to initialize a matrix by writing it out as in math
//because   a c e
//          b d f
//          0 0 1  is not the order of the alphabet :(
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

Vec2D operator*(const Transform_Matrix& matrix, Vec2D vec)
{
	const double& a = matrix.a, b = matrix.b, c = matrix.c, d = matrix.d, e = matrix.e, f = matrix.f,
		          x = vec.x, y = vec.y;

	return Vec2D{ a * x + c * y + e,
	                 b * x + d * y + f };
}

Vec2D to_board_system(const Transform_Matrix& transform, Vec2D point)
{
	auto [x, y] = transform * point;
	if (x < view_box::min.x || x > view_box::max.x || y < view_box::min.y || y > view_box::max.y) {
		return view_box::outside;
	}
	else {
		return { x, y };
	}
}

Transform_Matrix translate(Vec2D t)
{
	return { 1, 0, 0, 1, t.x, t.y };
}

Transform_Matrix scale(double sx, double sy)
{
	return { sx, 0, 0, sy, 0, 0 };
}

Transform_Matrix rotate(double angle)
{
	return in_matrix_order(std::cos(angle), -std::sin(angle), 0,
	                       std::sin(angle),  std::cos(angle), 0);
}

Transform_Matrix rotate(double angle, Vec2D pivot)
{
	return translate(pivot) * rotate(angle) * translate(-pivot);
}

Transform_Matrix skew_x(double angle)
{
	return { 1, 0, std::tan(angle), 1, 0, 0 };
}

Transform_Matrix skew_y(double angle)
{
	return { 1, std::tan(angle), 0, 1, 0, 0 };
}

std::string_view name_of(Transform transform)
{
	switch (transform) {
	case Transform::translate: return { "translate" };
	case Transform::scale: return { "scale" };
	case Transform::rotate: return { "rotate" };
	case Transform::skew_x: return { "skewX" };
	case Transform::skew_y: return { "skewY" };
	}
	assert(false);
	return {};
}

using namespace read;

std::string_view read::name_of(Elem_Type type)
{
	switch (type) {
	case Elem_Type::svg:		return { "svg" };
	case Elem_Type::g:			return { "g" };
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
	case Elem_Type::g:			
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

std::string_view read::get_attribute_data(std::string_view search_zone, std::string_view attr_name)
{
	const std::size_t found = search_zone.find(attr_name);
	if (found == std::string::npos) {
		return "";
	}
	else {
		search_zone.remove_prefix(found + attr_name.length());
		assert(search_zone[0] == '=' && search_zone[1] == '\"');
		search_zone.remove_prefix(2);

		const size_t closing_quote = search_zone.find_first_of('\"');
		return shorten_to(search_zone, closing_quote);
	}
}

std::vector<double> read::from_csv(std::string_view csv)
{
	std::vector<double> result;
	result.reserve(std::count_if(csv.begin(), csv.end(), [](char c) { return c == ' ' || c == ','; }) + 1);

	std::size_t next_seperator = csv.find_first_of(", ");
	while (next_seperator != std::string::npos) {
		result.push_back(to_scaled(shorten_to(csv, next_seperator)));
		const std::size_t val_after_seperator = csv.find_first_not_of(", ", next_seperator);
		csv.remove_prefix(val_after_seperator);	//everything bevore begin of next value is removed
		next_seperator = csv.find_first_of(", ");
	}

	result.push_back(to_scaled(csv));

	result.shrink_to_fit();
	return result;
}

double read::to_pixel(Unit unit)
{
	switch (unit) {
	case Unit::px:	return 1.0;
	case Unit::pt:	return 1.25;
	case Unit::pc:	return 15.0;
	case Unit::mm:	return 3.543307;
	case Unit::cm:	return 3.543307;
	case Unit::in:	return 90.0;
	}
	assert(false);
	return 0.0;
}

std::string_view read::name_of(Unit unit)
{
	switch (unit) {
	case Unit::px:	return {"px"};
	case Unit::pt:	return {"pt"};
	case Unit::pc:	return {"pc"};
	case Unit::mm:	return {"mm"};
	case Unit::cm:	return {"cm"};
	case Unit::in:	return {"in"};
	}
	assert(false);
	return {};
}

double read::to_scaled(std::string_view name)
{
	double factor = read::unknown_unit;
	const auto [value_end, error] = std::from_chars(name.data(), name.data() + name.size(), factor);

	const char* const name_end = name.data() + name.length();
	if (value_end != name_end) {
		const std::string_view rest(value_end, name_end - value_end);
		for (Unit unit : all_units) {
			if (rest == name_of(unit)){
				return factor * to_pixel(unit);
			}
		}
		return unknown_unit;
	}
	return factor;
}

const double read::unknown_unit = std::numeric_limits<double>::max();

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
	Elem_Data current = next_elem(fragment);
	do {
		while (current.type == Elem_Type::unknown) {
			fragment.remove_prefix(find_skip_quotations(fragment, ">"));
			current = next_elem(fragment);
		}
		switch (current.type) {
		case Elem_Type::svg:
		case Elem_Type::g:
		case Elem_Type::line:
		case Elem_Type::polyline:
		case Elem_Type::polygon:
		case Elem_Type::rect:
		case Elem_Type::ellypse:
		case Elem_Type::circle:
		case Elem_Type::path:
			current;///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		}
		fragment.remove_prefix(find_skip_quotations(fragment, end_marker(current.type)));
		current = next_elem(fragment);

	} while (current.type != Elem_Type::end);
}

Transform_Matrix read::group_transform(std::string_view group_attributes)
{
	std::string_view transform_list = get_attribute_data(group_attributes, "transform");
	if (transform_list == "") {
		return in_matrix_order(1, 0, 0,
		                       0, 1, 0);	//return identity matrix
	}
	else {
	}
}
