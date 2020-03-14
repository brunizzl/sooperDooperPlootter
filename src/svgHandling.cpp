
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

//displays vector in console (expects vectors local_view (type T) to already be able to be drawn to console)
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

//displays matrix on console
std::ostream& operator<<(std::ostream& stream, const Transform_Matrix& matrix)
{
	return stream << '[' << matrix.a << ", " << matrix.b << ", " << matrix.c << ", " << matrix.d << ", " << matrix.e << ", " << matrix.f << ']';
}


//returns view shortened to new_length
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

//returns part of original in between fst and snd
//if snd is std::string::npos, the view from after fst to end is returned.
std::string_view in_between(std::string_view original, std::size_t fst, std::size_t snd)
{
	if (snd != std::string::npos) {
		return { original.data() + fst + 1, snd - fst - 1 };
	}
	else {
		return { original.data() + fst + 1, original.length() - fst - 1 };
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////functions visible from the outside//////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void draw_from_file(const char* const name, double board_width, double board_height)
{
	std::string str;
	{
		//taken from: http://insanecoding.blogspot.com/2011/11/how-to-read-in-file-in-c.html
		std::ifstream filestream(name);
		if (!filestream) {
			std::cout << "Error: expected to find file \"" << name << "\" relative to programm path.\n";
			throw std::exception("Could not open SVG");
		}

		filestream.seekg(0, std::ios::end);
		str.resize(static_cast<const std::size_t>(filestream.tellg()));		//static_cast silences type shortening warning
		filestream.seekg(0, std::ios::beg);
		filestream.read(&str[0], str.size());
	}
	preprocess_str(str);
	std::cout << str << std::endl;

	std::string_view str_view = { str.c_str(), str.length() };

	//this thing here is done incredibly crappy. it will just grab the first viewBox is sees and will never try to change its viewbox ever after.
	std::string_view view_box_data = read::get_attribute_data(str_view, "viewBox=\"");
	if (view_box_data.length()) {
		view_box::set(view_box_data);
	}
	//creating transformation to display viewbox in board coordinate system (mm)
	//hier muss noch die richtige transformation zum board hin <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <--
	Transform_Matrix to_board = scale(1, 1) * translate({ 0.0, 0.0 });

	read::evaluate_fragment(str_view, to_board);
}

void preprocess_str(std::string& str)
{
	std::size_t comment_start = str.find("<!--");
	while (comment_start != std::string::npos) {
		const std::size_t comment_length = str.find("-->", comment_start) - comment_start + std::strlen("-->");
		str.erase(comment_start, comment_length);
		comment_start = str.find("<!--", comment_start);
	}

	//this part is mainly, because the path attribute may include newlines. 
	//to keep some readability for debugging, only the newlines inside quotes are replaced.
	bool inside_quotes = false;
	for (char& ch : str) {
		switch (ch) {
		case '\"':
			inside_quotes = !inside_quotes;
			break;
		case '\n':
			if (inside_quotes) {
				ch = ' ';
			}
			break;
		}
	}
	assert(!inside_quotes);	//there should be an even number of '\"' in the string.
}

//default values expected to be changed in function draw_from_file()
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

bool view_box::contains(Board_Vec point)
{
	return point.x >= view_box::min.x && point.x <= view_box::max.x &&
	       point.y >= view_box::min.y && point.y <= view_box::max.y;
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

Board_Vec operator*(const Transform_Matrix& matrix, Vec2D vec)
{
	const double& a = matrix.a, b = matrix.b, c = matrix.c, d = matrix.d, e = matrix.e, f = matrix.f,
		          x = vec.x, y = vec.y;

	return Board_Vec{ a * x + c * y + e,
	                     b * x + d * y + f };
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
	assert(false);	//if this assert is hit, you may update the switchcase above.
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
		return { ">" };		//just assume the least restricting marker if the type is unknown 
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
	std::size_t found = find_skip_quotations(search_zone, attr_name);
	while (found != std::string::npos) {
		//this is very trashy and may be changed later. i am aware of that
		if (found != 0 && (search_zone[found - 1] == ' ' || search_zone[found - 1] == ',') || found == 0) {	//guarantee that attr_name is not just suffix of some other atribute
			search_zone.remove_prefix(found + attr_name.length());

			const size_t closing_quote = search_zone.find_first_of('\"');
			return shorten_to(search_zone, closing_quote);
		}
		search_zone.remove_prefix(found + 1); 
		found = find_skip_quotations(search_zone, attr_name);
	}
	return { "" };
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

	if (csv != "") {
		result.push_back(to_scaled(csv));
	}

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
		//hier muesste noch fehlerbehandlung falls ne nicht bekannte einheit genutzt wird hin <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <--
	}
	return result;
}

Elem_Data read::take_next_elem(std::string_view& view)
{
	const std::size_t open_bracket = find_skip_quotations(view, "<");
	if (open_bracket == std::string::npos) {
		return { Elem_Type::end, "" };
	}
	view.remove_prefix(open_bracket + 1);	//if view started as "\n <circle cx=...", view is "circle cx=..." now 

	for (Elem_Type type : all_elem_types) {
		const std::string_view type_name = name_of(type);
		if (view.compare(0, type_name.length(), type_name) == 0) {
			view.remove_prefix(type_name.length());		//"circle cx=..." becomes " cx=..."
			const std::size_t end = find_skip_quotations(view, end_marker(type));
			const std::string_view content = shorten_to(view, end);		//it is just expected that an end marker was found.
			view.remove_prefix(end + std::strlen(">"));
			return { type, content };
		}
	}

	const std::size_t end = find_skip_quotations(view, ">");
	view.remove_prefix(end + std::strlen(">"));	//it is just expected that an end marker was found.
	return { Elem_Type::unknown, "" };
}

void read::evaluate_fragment(std::string_view fragment, const Transform_Matrix& transform)
{
	std::cout << "\n\nevaluate_fragment():\n" << fragment << std::endl;

	Elem_Data next;
	do {
		next = take_next_elem(fragment);
	} while (next.type == Elem_Type::unknown);

	while (next.type != Elem_Type::end) {
		switch (next.type) {
		case Elem_Type::svg:
			{
				const std::size_t nested_svg_end = find_closing_elem(fragment, { "<svg " }, { "</svg>" });
				const std::string_view nested_svg_fragment = { fragment.data(), nested_svg_end };

				const double x_offset = to_scaled(get_attribute_data(next.content, { "x=\"" }), 0.0);
				const double y_offset = to_scaled(get_attribute_data(next.content, { "y=\"" }), 0.0);
				const Transform_Matrix nested_matrix = transform * translate({ x_offset, y_offset });

				evaluate_fragment(nested_svg_fragment, nested_matrix);
				fragment.remove_prefix(nested_svg_end + std::strlen("</svg>"));
			}
			break;

		case Elem_Type::g:
			{
				const std::size_t group_end = find_closing_elem(fragment, { "<g " }, { "</g>" });
				const std::string_view group_fragment = { fragment.data(), group_end };

				const Transform_Matrix group_matrix = transform * get_transform_matrix(next.content);

				evaluate_fragment(group_fragment, group_matrix);
				fragment.remove_prefix(group_end + std::strlen("</g>"));
			}
			break;

		case Elem_Type::line:		draw::line(transform, next.content);		break;
		case Elem_Type::polyline:	draw::polyline(transform, next.content);	break;
		case Elem_Type::polygon:	draw::polygon(transform, next.content);	break;
		case Elem_Type::rect:		draw::rect(transform, next.content);		break;
		case Elem_Type::ellypse:	draw::ellypse(transform, next.content);	break;
		case Elem_Type::circle:		draw::circle(transform, next.content);	break;
		case Elem_Type::path:		draw::path(transform, next.content);		break;
		}


		do {
			next = take_next_elem(fragment);
		} while (next.type == Elem_Type::unknown);
	}
}

Transform_Matrix read::get_transform_matrix(std::string_view group_attributes)
{
	Transform_Matrix result_matrix = in_matrix_order(1, 0, 0,
		                                             0, 1, 0);	//starts as identity matrix

	std::string_view transform_list = get_attribute_data(group_attributes, { "transform=\"" });
	while (transform_list.length()) {
		for (Transform transform : all_transforms) {
			const std::string_view name = name_of(transform);
			if (transform_list.compare(0, name.length(), name) == 0) {
				const std::size_t closing_parenthesis = transform_list.find_first_of(')');
				//name does not include parentheses, but the length is one bigger than the biggest index in name. hence name.length() returns the right number
				const std::string_view parameter_view = in_between(transform_list, name.length(), closing_parenthesis); 
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
				default:
					assert(false);	//if this assert is hit, you may want to update the switch case above
				}

				transform_list.remove_prefix(closing_parenthesis + 1);	//all up to closing parenthesis is removed
				transform_list = remove_leading_seperators(transform_list);
				break;	//transformation is applied and next transformation may be read in -> breaking for() loop
			}
		}
	}
	return result_matrix;
}




using namespace path;

Path_Elem_data path::take_next_elem(std::string_view& view)
{
	const std::size_t fst_letter_pos = view.find_first_of("MmVvHhLlAaQqTtCcSsZz");
	if (fst_letter_pos == std::string::npos) {
		return { "", Path_Elem::end, Coords_Type::absolute };
	}
	std::size_t snd_letter_pos = view.find_first_of("MmVvHhLlAaQqTtCcSsZz", fst_letter_pos + 1);
	std::string_view content = in_between(view, fst_letter_pos, snd_letter_pos);
	Path_Elem_data result;

	switch (view[fst_letter_pos]) {
	case 'M': result = { content, Path_Elem::move, Coords_Type::absolute };				break;
	case 'm': result = { content, Path_Elem::move, Coords_Type::relative };				break;
	case 'V': result = { content, Path_Elem::vertical_line, Coords_Type::absolute };	break;
	case 'v': result = { content, Path_Elem::vertical_line, Coords_Type::relative };	break;
	case 'H': result = { content, Path_Elem::horizontal_line, Coords_Type::absolute };	break;
	case 'h': result = { content, Path_Elem::horizontal_line, Coords_Type::relative };	break;
	case 'L': result = { content, Path_Elem::line, Coords_Type::absolute };				break;
	case 'l': result = { content, Path_Elem::line, Coords_Type::relative };				break;
	case 'A': result = { content, Path_Elem::arc, Coords_Type::absolute };				break;
	case 'a': result = { content, Path_Elem::arc, Coords_Type::relative };				break;
	case 'Q':
	case 'q':
	case 'T':
	case 't':
		snd_letter_pos = view.find_first_of("MmVvHhLlAaCcSsZz", snd_letter_pos + 1);	//QqTt is missing
		content = in_between(view, fst_letter_pos - 1, snd_letter_pos);	//-1 as bezier needs to know what kind
		result = { content, Path_Elem::quadr_bezier, Coords_Type::absolute };		
		break;
	case 'C':
	case 'c':
	case 'S':
	case 's':
		snd_letter_pos = view.find_first_of("MmVvHhLlAaQqTtZz", snd_letter_pos + 1); //CcSs is missing
		content = in_between(view, fst_letter_pos - 1, snd_letter_pos);	//-1 as bezier needs to know what kind
		result = { content, Path_Elem::cubic_bezier, Coords_Type::absolute };		
		break;
	case 'Z':
	case 'z':
		result = { "", Path_Elem::closed, Coords_Type::absolute };
		break;
	default:
		assert(false);
	}

	if (snd_letter_pos == std::string::npos) view = "";
	else view.remove_prefix(snd_letter_pos + 1);

	return result;
}

//current_point_of_reference is point relative coordiantes are specified relative to.
//in the begin, this is also the current position of the plotter.
Vec2D path::process_quadr_bezier(const Transform_Matrix& transform_matrix, Path_Elem_data data, Vec2D current_point_of_reference)
{
	Bezier_Data curve = take_next_bezier(data.content);
	Vec2D last_control_point = Vec2D::no_value;
	while (curve.content != "") {
		const std::vector<double> points = from_csv(curve.content);

		if (curve.control_data == Control_Given::expl) {
			assert(points.size() % 4 == 0);

			Vec2D current_pos = current_point_of_reference;		//relevant to draw next bezier, starting at current_pos
			for (std::size_t i = 0; i < points.size(); i += 4) {
				const Vec2D control = curve.coords_type == Coords_Type::absolute ?
					Vec2D{ points[i], points[i + 1] } :
					current_point_of_reference + Vec2D{ points[i], points[i + 1] };

				const Vec2D end = curve.coords_type == Coords_Type::absolute ?
					Vec2D{ points[i + 2], points[i + 3] } :
					current_point_of_reference + Vec2D{ points[i + 2], points[i + 3] };

				draw::quadr_bezier(transform_matrix * current_pos, transform_matrix * control, transform_matrix * end);
				current_pos = end;
				last_control_point = control;	//not used in this loop, but the loop for implicit control points
			}
			current_point_of_reference = current_pos;	//polybezier is finished -> current position becomes new point of reference for relative coords
		}
		if (curve.control_data == Control_Given::impl) {
			assert(points.size() % 2 == 0);
			if (last_control_point == Vec2D::no_value) {
				last_control_point = current_point_of_reference;	//current_point_of_reference is also current position
			}

			Vec2D current_pos = current_point_of_reference;		//relevant to draw next bezier, starting at current_pos
			for (std::size_t i = 0; i < points.size(); i += 2) {
				const Vec2D control = calculate_contol_point(last_control_point, current_pos);

				const Vec2D end = curve.coords_type == Coords_Type::absolute ?
					Vec2D{ points[i], points[i + 1] } :
					current_point_of_reference + Vec2D{ points[i], points[i + 1] };

				draw::quadr_bezier(transform_matrix * current_pos, transform_matrix * control, transform_matrix * end);
				current_pos = end;
				last_control_point = control;
			}
			current_point_of_reference = current_pos;	//polybezier is finished -> current position becomes new point of reference for relative coords
		}

		curve = take_next_bezier(data.content);
	}
	return current_point_of_reference;
}

//current_point_of_reference is point relative coordiantes are specified relative to.
//in the begin, this is also the current position of the plotter.
Vec2D path::process_cubic_bezier(const Transform_Matrix& transform_matrix, Path_Elem_data data, Vec2D current_point_of_reference)
{
	Bezier_Data curve = take_next_bezier(data.content);
	Vec2D last_control_point = Vec2D::no_value;
	while (curve.content != "") {
		const std::vector<double> points = from_csv(curve.content);

		if (curve.control_data == Control_Given::expl) {
			assert(points.size() % 6 == 0);

			Vec2D current_pos = current_point_of_reference;		//relevant to draw next bezier, starting at current_pos
			for (std::size_t i = 0; i < points.size(); i += 6) {
				const Vec2D control_1 = curve.coords_type == Coords_Type::absolute ?
					Vec2D{ points[i], points[i + 1] } :
					current_point_of_reference + Vec2D{ points[i], points[i + 1] };

				const Vec2D control_2 = curve.coords_type == Coords_Type::absolute ?
					Vec2D{ points[i + 2], points[i + 3] } :
					current_point_of_reference + Vec2D{ points[i + 2], points[i + 3] };

				const Vec2D end = curve.coords_type == Coords_Type::absolute ?
					Vec2D{ points[i + 4], points[i + 5] } :
					current_point_of_reference + Vec2D{ points[i + 4], points[i + 5] };

				draw::cubic_bezier(transform_matrix * current_pos, transform_matrix * control_1, transform_matrix * control_2, transform_matrix * end);
				current_pos = end;
				last_control_point = control_2;	//not used in this loop, but the loop for implicit control points
			}
			current_point_of_reference = current_pos;	//polybezier is finished -> current position becomes new point of reference for relative coords
		}
		if (curve.control_data == Control_Given::impl) {
			assert(points.size() % 4 == 0);
			if (last_control_point == Vec2D::no_value) {
				last_control_point = current_point_of_reference;	//current_point_of_reference is also current position
			}

			Vec2D current_pos = current_point_of_reference;		//relevant to draw next bezier, starting at current_pos
			for (std::size_t i = 0; i < points.size(); i += 4) {
				const Vec2D control_1 = calculate_contol_point(last_control_point, current_pos);

				const Vec2D control_2 = curve.coords_type == Coords_Type::absolute ?
					Vec2D{ points[i], points[i + 1] } :
					current_point_of_reference + Vec2D{ points[i], points[i + 1] };

				const Vec2D end = curve.coords_type == Coords_Type::absolute ?
					Vec2D{ points[i + 2], points[i + 3] } :
					current_point_of_reference + Vec2D{ points[i + 2], points[i + 3] };

				draw::cubic_bezier(transform_matrix * current_pos, transform_matrix * control_1, transform_matrix * control_2, transform_matrix * end);
				current_pos = end;
				last_control_point = control_2;
			}
			current_point_of_reference = current_pos;	//polybezier is finished -> current position becomes new point of reference for relative coords
		}
	}

	return current_point_of_reference;
}

Vec2D path::calculate_contol_point(Vec2D last_control_point, Vec2D mirror)
{
	return 2.0 * mirror - last_control_point;
}

Bezier_Data path::take_next_bezier(std::string_view& view)
{
	const std::size_t identifier_pos = view.find_first_of("CcQqTtSs");
	if (identifier_pos == std::string::npos) {
		return { "", Control_Given::expl, Coords_Type::absolute };
	}
	const std::size_t next_pos = view.find_first_of("CcQqTtSs", identifier_pos + 1);
	const std::string_view content = in_between(view, identifier_pos, next_pos);
	const char identifier = view[identifier_pos];

	if (next_pos == std::string::npos) {
		view = "";
	}
	else {
		view.remove_prefix(next_pos);
	}
	switch (identifier) {
	case 'C': case 'Q': return { content, Control_Given::expl, Coords_Type::absolute };
	case 'c': case 'q':	return { content, Control_Given::expl, Coords_Type::relative };
	case 'T': case 'S':	return { content, Control_Given::impl, Coords_Type::absolute };
	case 't': case 's':	return { content, Control_Given::impl, Coords_Type::relative };
	}
	assert(false);
	return { "", Control_Given::expl, Coords_Type::absolute };
}

Vec2D path::process_arc(const Transform_Matrix& transform_matrix, Path_Elem_data data, Vec2D current_point_of_reference)
{
	//<-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <--<-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <--
	return current_point_of_reference;
}




using namespace draw;

void draw::line(Transform_Matrix transform_matrix, std::string_view parameters, std::size_t resolution)
{
	const std::string_view transform_data = get_attribute_data(parameters, { "transform=\"" });
	if (transform_data != "") {
		transform_matrix = transform_matrix * read::get_transform_matrix(transform_data);
	}

	const double x1 = read::to_scaled(read::get_attribute_data(parameters, { "x1=\"" }), 0.0);
	const double y1 = read::to_scaled(read::get_attribute_data(parameters, { "y1=\"" }), 0.0);
	const double x2 = read::to_scaled(read::get_attribute_data(parameters, { "x2=\"" }), 0.0);
	const double y2 = read::to_scaled(read::get_attribute_data(parameters, { "y2=\"" }), 0.0);

	const Board_Vec start = transform_matrix * Vec2D{ x1, y1 };
	const Board_Vec end = transform_matrix * Vec2D{ x1, y1 };
	save_go_to(start);
	path_line(start, end);
}

void draw::rect(Transform_Matrix transform_matrix, std::string_view parameters, std::size_t resolution)
{
	const std::string_view transform_data = get_attribute_data(parameters, { "transform=\"" });
	if (transform_data != "") {
		transform_matrix = transform_matrix * read::get_transform_matrix(transform_data);
	}

	const double x = read::to_scaled(read::get_attribute_data(parameters, { "x=\"" }), 0.0);
	const double y = read::to_scaled(read::get_attribute_data(parameters, { "y=\"" }), 0.0);
	const double width = read::to_scaled(read::get_attribute_data(parameters, { "width=\"" }), 0.0);
	const double height = read::to_scaled(read::get_attribute_data(parameters, { "height=\"" }), 0.0);
	double rx = read::to_scaled(read::get_attribute_data(parameters, { "rx=\"" }), 0.0);
	double ry = read::to_scaled(read::get_attribute_data(parameters, { "ry=\"" }), 0.0);

	if (rx != 0 && ry == 0) ry = rx;
	if (rx == 0 && ry != 0) rx = ry;
	if (rx > width / 2) rx = width / 2;
	if (ry > height / 2) ry = height / 2;

	if (rx == 0) {	//draw normal rectangle
		const Board_Vec upper_right = transform_matrix * Vec2D{ x + width, y };
		const Board_Vec upper_left =  transform_matrix * Vec2D{ x, y };
		const Board_Vec lower_left =  transform_matrix * Vec2D{ x, y + height };
		const Board_Vec lower_right = transform_matrix * Vec2D{ x + width, y + height };

		//draws in order       |start
		//                     v
		//            ---(1)---
		//           |         |
		//          (2)       (4)
		//           |         |
		//            ---(3)---
		
		save_go_to(upper_right);								//start
		path_line(upper_right, upper_left, resolution);			//(1)
		path_line(upper_left, lower_left, resolution);			//(2)
		path_line(lower_left, lower_right, resolution);			//(3)
		path_line(lower_right, upper_right, resolution);		//(4)
	}
	else {	//draw rectangle with corners rounded of
		//these two points are the center point of the upper right ellypse (arc) and the lower left ellypse respectively
		const Board_Vec center_upper_right = transform_matrix * Vec2D{ x + width - rx, y + ry };
		const Board_Vec center_lower_left = transform_matrix * Vec2D{ x + rx, y + height - ry };
		const double right_x = center_upper_right.x;
		const double left_x = center_lower_left.x;
		const double upper_y = center_upper_right.y;
		const double lower_y = center_lower_left.y;

		//draws in order           | start
		//                         v
		//              (2)--(1)--(8)
		//               | X       |
		//              (3)       (7)
		//               |       X |
		//              (4)--(5)--(6)		
		//with X beeing center_upper_right and center_lower_left

		save_go_to({ right_x, upper_y });							          //start
		path_line({ right_x, upper_y - ry }, { left_x, upper_y - ry });	      //(1)
		arc({ left_x, upper_y }, rx, ry, pi / 2, pi, Rotation::positive);     //(2)
		path_line({ left_x - rx, upper_y }, { left_x - rx, lower_y });	      //(3)
		arc({ left_x, lower_y }, rx, ry, pi, -pi / 2, Rotation::positive);	  //(4)
		path_line({ left_x, lower_y + ry }, { right_x, lower_y + ry });	      //(5)
		arc({ right_x, lower_y }, rx, ry, -pi / 2, 0, Rotation::positive);	  //(6)
		path_line({ right_x + rx, lower_y }, { right_x + rx, upper_y });      //(7)
		arc({ right_x, upper_y }, rx, ry, 0, pi / 2, Rotation::positive);     //(8)
	}
}

void draw::circle(Transform_Matrix transform_matrix, std::string_view parameters, std::size_t resolution)
{
	const std::string_view transform_data = get_attribute_data(parameters, { "transform=\"" });
	if (transform_data != "") {
		transform_matrix = transform_matrix * read::get_transform_matrix(transform_data);
	}

	const double cx = read::to_scaled(read::get_attribute_data(parameters, { "cx=\"" }), 0.0);
	const double cy = read::to_scaled(read::get_attribute_data(parameters, { "cy=\"" }), 0.0);
	const double r  = read::to_scaled(read::get_attribute_data(parameters, { "r=\"" }), 0.0);

	save_go_to(transform_matrix * (Vec2D{ cx, cy } +Vec2D{ r, 0.0 }));	//intersection of positive x-axis and circle is starting point
	for (std::size_t step = 1; step <= resolution; step++) {
		const double angle = 2 * pi * (resolution - step);
		save_draw_to(transform_matrix * Vec2D{ cx + std::cos(angle) * r, cy + std::sin(angle) * r });
	}
}

void draw::ellypse(Transform_Matrix transform_matrix, std::string_view parameters, std::size_t resolution)
{
	const std::string_view transform_data = get_attribute_data(parameters, { "transform=\"" });
	if (transform_data != "") {
		transform_matrix = transform_matrix * read::get_transform_matrix(transform_data);
	}

	const double cx = read::to_scaled(read::get_attribute_data(parameters, { "cx=\"" }), 0.0);
	const double cy = read::to_scaled(read::get_attribute_data(parameters, { "cy=\"" }), 0.0);
	const double rx = read::to_scaled(read::get_attribute_data(parameters, { "rx=\"" }), 0.0);
	const double ry = read::to_scaled(read::get_attribute_data(parameters, { "ry=\"" }), 0.0);

	save_go_to(transform_matrix * (Vec2D{ cx, cy } + Vec2D{ rx, 0.0 }));	//intersection of positive x-axis and ellypse is starting point
	for (std::size_t step = 1; step <= resolution; step++) {
		const double angle = 2 * pi * (resolution - step);
		save_draw_to(transform_matrix * Vec2D{ cx + std::cos(angle) * rx, cy + std::sin(angle) * ry });
	}
}

void draw::polyline(Transform_Matrix transform_matrix, std::string_view parameters, std::size_t resolution)
{
	const std::string_view transform_data = get_attribute_data(parameters, { "transform=\"" });
	if (transform_data != "") {
		transform_matrix = transform_matrix * read::get_transform_matrix(transform_data);
	}

	const std::string_view points_view = get_attribute_data(parameters, { "points=\"" });
	const std::vector<double> points = from_csv(points_view);
	assert(points.size() % 2 == 0);

	Board_Vec start = transform_matrix * Vec2D{ points[0], points[1] };
	save_go_to(start);
	for (std::size_t i = 2; i < points.size(); i += 2) {
		const Board_Vec end = transform_matrix * Vec2D{ points[i], points[i + 1] };
		path_line(start, end);
		start = end;
	}
}

void draw::polygon(Transform_Matrix transform_matrix, std::string_view parameters, std::size_t resolution)
{
	const std::string_view transform_data = get_attribute_data(parameters, { "transform=\"" });
	if (transform_data != "") {
		transform_matrix = transform_matrix * read::get_transform_matrix(transform_data);
	}

	const std::string_view points_view = get_attribute_data(parameters, { "points=\"" });
	const std::vector<double> points = from_csv(points_view);
	assert(points.size() % 2 == 0);

	Board_Vec start = transform_matrix * Vec2D{ points[0], points[1] };
	Board_Vec end;
	save_go_to(start);
	for (std::size_t i = 2; i < points.size(); i += 2) {
		end = transform_matrix * Vec2D{ points[i], points[i + 1] };
		path_line(start, end);
		start = end;
	}
	end = transform_matrix * Vec2D{ points[0], points[1] };	//polygon is closed -> last operation is to connect to first point
	path_line(start, end);
}

void draw::path(Transform_Matrix transform_matrix, std::string_view parameters, std::size_t resolution)
{
	const std::string_view transform_data = get_attribute_data(parameters, { "transform=\"" });
	if (transform_data != "") {
		transform_matrix = transform_matrix * read::get_transform_matrix(transform_data);
	}

	std::string_view data_view = get_attribute_data(parameters, { "data=\"" });
	Vec2D current_point = { 0.0, 0.0 };	//as a path always continues from the last point, this is the point the last path element ended (this is not yet transformed)
	Vec2D current_subpath_begin = { 0.0, 0.0 };
	bool new_subpath = true;	//information for move to maybe update current_subpath_begin
	Path_Elem_data next_elem = path::take_next_elem(data_view);

	while (next_elem.type != Path_Elem::end) {
		std::vector<double> data;
		if (next_elem.type != Path_Elem::quadr_bezier && next_elem.type != Path_Elem::cubic_bezier) {
			data = from_csv(next_elem.content);
		}

		switch (next_elem.type) {
		case Path_Elem::move:
			assert(data.size() % 2 == 0);
			{
				//only the first two coordinates are moved to, the rest are implicit line commands
				const Vec2D next_point = next_elem.coords_type == Coords_Type::absolute ?
					Vec2D{ data[0], data[1] } :
					current_point + Vec2D{ data[0], data[1] };
				save_go_to(transform_matrix * next_point);
				current_point = next_point;
				if (new_subpath) {
					current_subpath_begin = current_point;
				}
			}
			for (std::size_t i = 2; i < data.size(); i += 2) {
				const Vec2D next_point = next_elem.coords_type == Coords_Type::absolute ?
					Vec2D{ data[i], data[i + 1] } :
					current_point + Vec2D{ data[i], data[i + 1] };
				draw::path_line(transform_matrix * current_point, transform_matrix * next_point);
				current_point = next_point;
			}
			new_subpath = false;
			break;

		case Path_Elem::vertical_line:
			{
				//although that makes no sense, there can be multiple vertical lines stacked -> we directly draw to the end
				const Vec2D next_point = next_elem.coords_type == Coords_Type::absolute ?
					Vec2D{ current_point.x, data.back() } :
					Vec2D{ current_point.x, current_point.y + data.back() };
				draw::path_line(transform_matrix * current_point, transform_matrix * next_point);
				current_point = next_point;
			}
			new_subpath = false;
			break;

		case Path_Elem::horizontal_line:
			{
				//although that makes no sense, there can be multiple horizontal lines stacked -> we directly draw to the end
				const Vec2D next_point = next_elem.coords_type == Coords_Type::absolute ?
					Vec2D{ data.back(), current_point.y } :
					Vec2D{ current_point.x + data.back(), current_point.y };
				draw::path_line(transform_matrix * current_point, transform_matrix * next_point);
				current_point = next_point;
			}
			new_subpath = false;
			break;

		case Path_Elem::line:
			assert(data.size() % 2 == 0);
			{
				Vec2D next_point = { 0.0, 0.0 };
				for (std::size_t i = 0; i < data.size(); i += 2) {
					next_point = next_elem.coords_type == Coords_Type::absolute ?
						Vec2D{ data[i], data[i + 1] } :
						current_point + Vec2D{ data[i], data[i + 1] };
					draw::path_line(transform_matrix * current_point, transform_matrix * next_point);
				}
				current_point = next_point;	//w3 specifies a polyline drawn relative has all positions relative to the start of the polyline -> only update current here
			}
			new_subpath = false;
			break;

		case Path_Elem::arc: 
			current_point = process_arc(transform_matrix, next_elem, current_point);
			new_subpath = false;
			break;
		case Path_Elem::quadr_bezier: 
			current_point = process_quadr_bezier(transform_matrix, next_elem, current_point);
			new_subpath = false;
			break;
		case Path_Elem::cubic_bezier:
			current_point = process_cubic_bezier(transform_matrix, next_elem, current_point);
			new_subpath = false;
			break;
		case Path_Elem::closed:
			draw::path_line(transform_matrix * current_point, transform_matrix * current_subpath_begin);
			//current_subpath_begin is same as begin of last (just finished) subpath, but may be changed from directly following move command
			new_subpath = true;
			break;
		}
		next_elem = path::take_next_elem(data_view);
	}
}

void draw::arc(Board_Vec center, double rx, double ry, double start_angle, double end_angle, Rotation rotation, std::size_t resolution)
{
	const double angle_per_step =rotation == Rotation::positive ? 
		(end_angle - start_angle) / resolution : 
		(2.0 * pi - (end_angle - start_angle)) / resolution;

	for (std::size_t step = 1; step <= resolution; step++) {
		const double angle = start_angle + angle_per_step * step;
		save_draw_to({ center.x + std::cos(angle) * rx, center.y + std::sin(angle) * ry });
	}
}

void draw::path_line(Board_Vec start, Board_Vec end, std::size_t resolution)
{
	for (std::size_t step = 1; step <= resolution; step++) {
		//as given in wikipedia for linear bezier curves: https://en.wikipedia.org/wiki/B%C3%A9zier_curve#Linear_B%C3%A9zier_curves
		const double t = step / (double)resolution;
		save_draw_to(start + t * (end - start));
	}
}

void draw::quadr_bezier(Board_Vec start, Board_Vec control, Board_Vec end, std::size_t resolution)
{
	for (std::size_t step = 1; step <= resolution; step++) {
		//formula taken from https://en.wikipedia.org/wiki/B%C3%A9zier_curve#Quadratic_B%C3%A9zier_curves
		const double t = step / (double)resolution;
		const Board_Vec waypoint = (1 - t) * (1 - t) * start + 2 * (1 - t) * t * control + t * t * end;
		save_draw_to(waypoint);
	}
}

void draw::cubic_bezier(Board_Vec start, Board_Vec control_1, Board_Vec control_2, Board_Vec end, std::size_t resolution)
{
	for (std::size_t step = 1; step <= resolution; step++) {
		//formula taken from https://en.wikipedia.org/wiki/B%C3%A9zier_curve#Cubic_B%C3%A9zier_curves
		const double t = step / (double)resolution;
		const double tpow2 = t * t;
		const double onet = (1 - t);
		const double onetpow2 = onet * onet;
		const Board_Vec waypoint = onetpow2 * (onet * start + 3 * t * control_1) + tpow2 * (3 * onet * control_2 + t * end);
		//const Vec2D waypoint = (1 - t) * (1 - t) * (1 - t) * start + 3 * (1 - t) * (1 - t) * t * control_1 + 3 * (1 - t) * t * t * control_2 + t * t * t * end;
		save_draw_to(waypoint);
	}
}
