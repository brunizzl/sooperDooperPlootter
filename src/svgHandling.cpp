
#include "svgHandling.hpp"


#include <cmath>
#include <iostream>
#include <fstream>
#include <streambuf>
#include <cassert>
#include <limits>
#include <algorithm>
#include <charconv>
#include <cstring>

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////functions loal to this file/////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
std::ostream& operator<<(std::ostream& stream, const std::vector<T>& vec)
{
	stream << '{';
	bool first = true;
	for (const auto& elem : vec) {
		if (!std::exchange(first, false)) {
			stream << ", ";
		}
		stream << elem;
	}
	stream << '}';
	return stream;
}

std::ostream& operator<<(std::ostream& stream, const Transform_Matrix& matrix)
{
	return stream << '[' << matrix.a << ", " << matrix.b << ", " << matrix.c << ", " << matrix.d << ", " << matrix.e << ", " << matrix.f << ']';
}


//convinience function used in get_attribute_data() and view_box::set()
std::string_view shorten_to(std::string_view view, std::size_t new_length)
{
	assert(new_length <= view.length());
	return { view.data(), new_length };
}

//convinience function to change angle units from degree to rad
double to_rad(double degree)
{
	return degree * pi / 180.0;
}

//removes leading ' ' and ',' characters if possible
std::string_view remove_leading_seperators(std::string_view view, std::size_t start = 0)
{
	const std::size_t fist_after_seperator = view.find_first_not_of(", ", start);
	if (fist_after_seperator != std::string::npos) {
		view.remove_prefix(fist_after_seperator);
	}
	return view;
}

//made to find matching "</svg>" and "</g>" to the opening ones
//function assumes, that search_zone already misses the opening sequence for searched closing_sequence.
//-> if "</g>" is searched, there is one "</g>" more in search_zone, than there are "<g ..." in search_zone
std::size_t find_closing_elem(std::string_view search_zone, std::string_view opening_sequence, std::string_view closing_sequence)
{
	assert(opening_sequence != closing_sequence);

	std::size_t next_open = read::find_skip_quotations(search_zone, opening_sequence);
	std::size_t next_clsd = read::find_skip_quotations(search_zone, closing_sequence);

	while (next_open < next_clsd) {
		next_open = read::find_skip_quotations(search_zone, opening_sequence, next_open + opening_sequence.length());
		next_clsd = read::find_skip_quotations(search_zone, closing_sequence, next_clsd + closing_sequence.length());
	}
	return next_clsd;
}

//helper for to_scaled()
double to_pixel(read::Unit unit)
{
	switch (unit) {
	case read::Unit::px:	return 1.0;
	case read::Unit::pt:	return 1.25;
	case read::Unit::pc:	return 15.0;
	case read::Unit::mm:	return 3.543307;
	case read::Unit::cm:	return 3.543307;
	case read::Unit::in:	return 90.0;
	}
	assert(false);
	return 0.0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////functions visible from the outside//////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

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
	remove_comments(str);
	std::cout << str << std::endl;

	std::string_view str_view = { str.c_str(), str.length() };

	//this thing here is done incredibly crappy. it will just grab the first viewBox is sees and will never try to change its viewbox ever after.
	std::string_view view_box_data = read::get_attribute_data(str_view, "viewBox=\"");
	if (view_box_data.length()) {
		view_box::set(view_box_data);
	}
	//creating transformation to display viewbox in board coordinate system (mm)
	//DAS IST VIELLEITCH NOCH ABSOLUT NICHT KORREKT BITTE TESTEN BITTE TESTEN BITTE TESTEN BITTE TESTEN BITTE TESTEN BITTE TESTEN BITTE TESTEN BITTE TESTEN
	//const double stretching_factor = std::min(board_width / (view_box::max.x - view_box::min.x), board_height / (view_box::max.y - view_box::min.y));
	Transform_Matrix to_board = scale(1, 1);

	read::evaluate_fragment(str_view, to_board);
}

void remove_comments(std::string& str)
{
	std::size_t comment_start = str.find("<!--");
	while (comment_start != std::string::npos) {
		const std::size_t comment_length = str.find("-->", comment_start) - comment_start + std::strlen("-->");
		str.erase(comment_start, comment_length);
		comment_start = str.find("<!--", comment_start);
	}
}

Vec2D view_box::min = { 0, 0 };
Vec2D view_box::max = { 100, 100 };


void view_box::set(std::string_view data)
{
	const std::vector<double> values = read::from_csv(data);
	assert(values.size() == 4);

	view_box::min.x = values[0];
	view_box::min.y = values[1];
	view_box::max.x = values[0] + values[2];	//min.x + width
	view_box::max.y = values[1] + values[3];	//min.y + height
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
	case Transform::matrix:		return { "matrix" };
	case Transform::translate:	return { "translate" };
	case Transform::scale:		return { "scale" };
	case Transform::rotate:		return { "rotate" };
	case Transform::skew_x:		return { "skewX" };
	case Transform::skew_y:		return { "skewY" };
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

std::size_t read::find_skip_quotations(std::string_view search_zone, std::string_view search_obj, std::size_t start)
{
	std::size_t next_quotation_start = search_zone.find_first_of('\"', start);
	std::size_t prev_quotation_end = start;

	while (next_quotation_start != std::string::npos) {
		const std::string_view search_section = { search_zone.data() + prev_quotation_end, next_quotation_start - prev_quotation_end + 1 };	//including the opening quote itself with + 1
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
	const std::size_t found = find_skip_quotations(search_zone, attr_name);
	if (found == std::string::npos) {
		return "";
	}
	else {
		search_zone.remove_prefix(found + attr_name.length());

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
		csv = remove_leading_seperators(csv, next_seperator);
		next_seperator = csv.find_first_of(", ");
	}

	result.push_back(to_scaled(csv));

	result.shrink_to_fit();
	return result;
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

double read::to_scaled(std::string_view name, double default_val)
{
	double result = default_val;
	const auto [value_end, error] = std::from_chars(name.data(), name.data() + name.size(), result);

	const char* const name_end = name.data() + name.length();
	if (value_end != name_end) {
		const std::string_view rest(value_end, name_end - value_end);
		for (Unit unit : all_units) {
			if (rest == name_of(unit)){
				return result * to_pixel(unit);
			}
		}
		return unknown_unit;
	}
	return result;
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
	std::cout << "\n\nevaluate_fragment():\n" << fragment << std::endl;

	Elem_Data current = next_elem(fragment);
	while (current.type == Elem_Type::unknown) {
		fragment.remove_prefix(find_skip_quotations(fragment, ">") + std::strlen(">"));
		current = next_elem(fragment);
	}

	while (current.type != Elem_Type::end) {
		fragment.remove_prefix(find_skip_quotations(fragment, end_marker(current.type)) + end_marker(current.type).length());

		switch (current.type) {
		case Elem_Type::svg:
			{
				const std::size_t nested_svg_end = find_closing_elem(fragment, { "<svg " }, { "</svg>" });
				const std::string_view nested_svg_fragment = { fragment.data(), nested_svg_end };

				const double x_offset = to_scaled(get_attribute_data(current.content, "x=\""), 0.0);
				const double y_offset = to_scaled(get_attribute_data(current.content, "y=\""), 0.0);
				const Transform_Matrix nested_matrix = transform * translate({ x_offset, y_offset });

				evaluate_fragment(nested_svg_fragment, nested_matrix);
				fragment.remove_prefix(nested_svg_end + std::strlen("</svg>"));
			}
			break;
		case Elem_Type::g:
			{
				const std::size_t group_end = find_closing_elem(fragment, { "<g " }, { "</g>" });
				const std::string_view group_fragment = { fragment.data(), group_end };

				const Transform_Matrix group_matrix = transform * get_transform_matrix(current.content);

				evaluate_fragment(group_fragment, group_matrix);
				fragment.remove_prefix(group_end + std::strlen("</g>"));
			}
			break;
		case Elem_Type::line:		draw::line(transform, current.content);		break;
		case Elem_Type::polyline:	draw::polyline(transform, current.content);	break;
		case Elem_Type::polygon:	draw::polygon(transform, current.content);	break;
		case Elem_Type::rect:		draw::rect(transform, current.content);		break;
		case Elem_Type::ellypse:	draw::ellypse(transform, current.content);	break;
		case Elem_Type::circle:		draw::circle(transform, current.content);	break;
		case Elem_Type::path:		draw::path(transform, current.content);		break;
		}


		current = next_elem(fragment);
		while (current.type == Elem_Type::unknown) {
			fragment.remove_prefix(find_skip_quotations(fragment, ">") + std::strlen(">"));
			current = next_elem(fragment);
		}
	}
}

Transform_Matrix read::get_transform_matrix(std::string_view group_attributes)
{
	Transform_Matrix result_matrix = in_matrix_order(1, 0, 0,
		                                             0, 1, 0);	//starts as identity matrix

	std::string_view transform_list = get_attribute_data(group_attributes, "transform=\"");
	while (transform_list.length()) {
		for (Transform transform : all_transforms) {
			const std::string_view name = name_of(transform);
			if (transform_list.compare(0, name.length(), name) == 0) {
				const std::size_t closing_parenthesis = transform_list.find_first_of(')');
				const std::string_view parameter_view = { transform_list.data() + name.length() + 1, closing_parenthesis - name.length() - 1 };
				const std::vector<double> parameters = from_csv(parameter_view);

				switch (transform) {
				case Transform::matrix:
					assert(parameters.size() == 6);
					result_matrix = result_matrix * Transform_Matrix{ parameters[0], parameters[1], parameters[2], parameters[3], parameters[4], parameters[5] };
					break;
				case Transform::translate: 
					if (parameters.size() == 2) {
						result_matrix = result_matrix * translate({ parameters[0], parameters[1] });
					}
					else {
						assert(parameters.size() == 1);
						result_matrix = result_matrix * translate({ parameters[0], 0.0 });
					}
					break;
				case Transform::scale:
					if (parameters.size() == 2) {
						result_matrix = result_matrix * scale(parameters[0], parameters[1]);
					}
					else {
						assert(parameters.size() == 1);
						result_matrix = result_matrix * scale(parameters[0], parameters[0]);
					}
					break;
				case Transform::rotate:
					if (parameters.size() == 1) {
						result_matrix = result_matrix * rotate(to_rad(parameters[0]));
					}
					else {
						assert(parameters.size() == 3);
						result_matrix = result_matrix * rotate(to_rad(parameters[0]), { parameters[1], parameters[2] });
					}
					break;
				case Transform::skew_x:
					assert(parameters.size() == 1);
					result_matrix = result_matrix * skew_x(to_rad(parameters[0]));
					break;
				case Transform::skew_y:
					assert(parameters.size() == 1);
					result_matrix = result_matrix * skew_y(to_rad(parameters[0]));
					break;
				}

				transform_list.remove_prefix(closing_parenthesis + 1);	//all up to closing parenthesis is removed
				transform_list = remove_leading_seperators(transform_list);
				break;	//transformation is applied and next transformation may be read in
			}
		}
	}
	return result_matrix;
}

void draw::line(Transform_Matrix transform, std::string_view parameters, std::size_t resolution)
{
	std::cout << "male line: " << parameters << '\n';
	std::cout << "mit matrix " << transform << '\n';
}

void draw::rect(Transform_Matrix transform, std::string_view parameters, std::size_t resolution)
{
	std::cout << "male rect: " << parameters << '\n';
	std::cout << "mit matrix " << transform << '\n';
}

void draw::circle(Transform_Matrix transform, std::string_view parameters, std::size_t resolution)
{
	std::cout << "male circle: " << parameters << '\n';
	std::cout << "mit matrix " << transform << '\n';
}

void draw::ellypse(Transform_Matrix transform, std::string_view parameters, std::size_t resolution)
{
	std::cout << "male ellypse: " << parameters << '\n';
	std::cout << "mit matrix " << transform << '\n';
}

void draw::arc(Transform_Matrix transform, std::string_view parameters, std::size_t resolution)
{
	std::cout << "male arc: " << parameters << '\n';
	std::cout << "mit matrix " << transform << '\n';
}

void draw::quadratic_bezier(Transform_Matrix transform, std::string_view parameters, std::size_t resolution)
{
	std::cout << "male quadratic_bezier: " << parameters << '\n';
	std::cout << "mit matrix " << transform << '\n';
}

void draw::cubic_bezier(Transform_Matrix transform, std::string_view parameters, std::size_t resolution)
{
		std::cout << "male cubic_bezier: " << parameters << '\n';
		std::cout << "mit matrix " << transform << '\n';
}

void draw::path(Transform_Matrix transform, std::string_view parameters, std::size_t resolution)
{
	std::cout << "male path: " << parameters << '\n';
	std::cout << "mit matrix " << transform << '\n';
}

void draw::polyline(Transform_Matrix transform, std::string_view parameters, std::size_t resolution)
{
	std::cout << "male polyline: " << parameters << '\n';
	std::cout << "mit matrix " << transform << '\n';
}

void draw::polygon(Transform_Matrix transform, std::string_view parameters, std::size_t resolution)
{
	std::cout << "male polygon: " << parameters << '\n';
	std::cout << "mit matrix " << transform << '\n';
}
