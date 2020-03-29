
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
std::ostream& operator<<(std::ostream& stream, const la::Transform_Matrix& matrix)
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
	return degree * la::pi / 180.0;
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

void draw_from_file(const char* name, double board_width, double board_height)
{
	std::string str = read::string_from_file(name);
	preprocess_str(str);
	read::evaluate_svg({ str.c_str(), str.length() }, board_width, board_height);
}

void preprocess_str(std::string& str)
{
	std::size_t comment_start = str.find("<!--");
	while (comment_start != std::string::npos) {
		const std::size_t comment_length = str.find("-->", comment_start) - comment_start + std::strlen("-->");
		str.erase(comment_start, comment_length);
		comment_start = str.find("<!--", comment_start);
	}

	bool inside_quotes = false;
	bool inside_elem = false;
	for (auto& ch : str) {
		switch (ch) {
		case '\"':
		case '\'':
			inside_quotes = !inside_quotes;
			break;
		case '<':
			inside_elem = true;
			break;
		case '>':
			inside_elem = false;
			break;
		case '\n':
			if (inside_elem) {	      //this part is mainly, because the path attribute may include newlines. 
				ch = ' ';			  //to keep some readability for debugging, only the newlines inside elements are replaced.
			}
			break;
		}
	}
	assert(!inside_quotes && !inside_elem);	//there should be an even number of '\"' (or '\'') in the string.
}

//default values (get replaced by set() anyway)
la::Board_Vec View_Box::min = la::Board_Vec(0, 0);
la::Board_Vec View_Box::max = la::Board_Vec(100, 100);


la::Transform_Matrix View_Box::private_set(double min_x, double min_y, double width, double height, double board_width, double board_height)
{
	if (width / height < board_width / board_height) {	//view box has taller aspect ratio than board -> leaving space on right and left side of board
		const double scaling_factor = board_height / height;	//y-direction limits size
		const double view_width_in_board_units = width * scaling_factor;	//width of view box given in board coordinates
		const double x_offset = (board_width - view_width_in_board_units) / 2;	//x-coordinate of left boundary of view box given in board coordinates

		View_Box::min.x = x_offset;
		View_Box::max.x = x_offset + view_width_in_board_units;
		View_Box::min.y = 0;
		View_Box::max.y = board_height;

		//     shift to middle of board     scaling to board units                  translate in svg units to (0, 0)
		return la::translate({ x_offset, 0 }) * la::scale(scaling_factor, scaling_factor) * la::translate({ -min_x, -min_y });
	}
	else {	//view box is wider than board -> full use of board_width, but only use upper part of board
		const double scaling_factor = board_width / width;
		const double view_height_in_board_units = height * scaling_factor;

		View_Box::min.x = 0;
		View_Box::max.x = board_width;
		View_Box::min.y = 0;
		View_Box::max.y = view_height_in_board_units;

		//     scaling to board units                  translate in svg units to (0, 0)
		return la::scale(scaling_factor, scaling_factor) * la::translate({ -min_x, -min_y });
	}
}

la::Transform_Matrix View_Box::set(std::string_view data, double board_width, double board_height)
{
	const std::vector<double> values = read::from_csv(data);
	assert(values.size() == 4);

	const double min_x = values[0];
	const double min_y = values[1];
	const double width = values[2];
	const double height = values[3];

	return private_set(min_x, min_y, width, height, board_width, board_height);
}

la::Transform_Matrix View_Box::set(double width, double height, double board_width, double board_height)
{
	return private_set(0, 0, width, height, board_width, board_height);
}

bool View_Box::contains(la::Board_Vec point)
{
	return point.x > View_Box::min.x && point.x < View_Box::max.x 
	    && point.y > View_Box::min.y && point.y < View_Box::max.y;
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
	case Elem_Type::ellipse:	return { "ellipse" };
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
	case Elem_Type::ellipse:		
	case Elem_Type::circle:			
	case Elem_Type::path:		
		return { "/>" };
	case Elem_Type::unknown:	
		return { ">" };		//just assume the less restricting marker if the type is unknown 
	}
	assert(false);	//if this assert is hit, you may update the switchcase above.
	return {};
}

std::size_t read::find_skip_quotations(std::string_view search_zone, std::string_view search_obj, std::size_t start)
{
	std::size_t next_quotation_start = search_zone.find_first_of("\"'", start);
	std::size_t prev_quotation_end = start;

	while (next_quotation_start != std::string::npos) {
		const std::string_view search_section = { search_zone.data() + prev_quotation_end, next_quotation_start - prev_quotation_end + 1 };	//including the opening quote itself with + 1
		const std::size_t found = search_section.find(search_obj);
		if (found != std::string::npos) {
			return found + prev_quotation_end;	//search_section has offset of prev_quotation_end from search_zone.
		}
		prev_quotation_end = search_zone.find_first_of("\"'", next_quotation_start + 1);
		if (prev_quotation_end == std::string::npos) {
			throw std::exception("function read::find_skip_quotations(): quotation (using \"\" or '') was started, but not ended.");
		}
		next_quotation_start = search_zone.find_first_of("\"'", prev_quotation_end + 1);
	}

	return search_zone.find(search_obj, prev_quotation_end);
}

std::string_view read::get_attribute_data(std::string_view search_zone, std::string_view attr_name)
{
	std::size_t found = find_skip_quotations(search_zone, attr_name);
	while (found != std::string::npos) {
		//this is very trashy and may be changed later. i am aware of that
		if (found != 0 && (search_zone[found - 1] == ' ' || search_zone[found - 1] == ',') || found == 0) {	//guarantee that attr_name is not just suffix of some other atribute
			search_zone.remove_prefix(found + attr_name.length() + 1);

			const size_t closing_quote = search_zone.find_first_of("\"'");
			return shorten_to(search_zone, closing_quote);
		}
		search_zone.remove_prefix(found + 1); 
		found = find_skip_quotations(search_zone, attr_name);
	}
	return { "" };
}

std::vector<double> read::from_csv(std::string_view csv, bool always_comma)
{
	std::vector<double> result;
	result.reserve(std::count_if(csv.begin(), csv.end(), [](char c) { return c == ' ' || c == ','; }) + 1);

	std::size_t next_value_start = csv.find_first_of("0123456789.+-");
	while (next_value_start != std::string::npos) {
		const std::size_t next_seperator = csv.find_first_of((always_comma? "," : ", -"), next_value_start + 1);	//minus may also be used as seperator
		const std::string_view next_value = in_between(csv, next_value_start - 1, next_seperator);
		result.push_back(to_scaled(next_value));
		next_value_start = csv.find_first_of("0123456789.+-", next_seperator);
	}

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

std::string read::string_from_file(const char* file_name)
{
	std::string str;
	{
		//taken from: http://insanecoding.blogspot.com/2011/11/how-to-read-in-file-in-c.html
		std::ifstream filestream(file_name);
		if (!filestream) {
			std::cout << "Error: expected to find file \"" << file_name << "\" relative to programm path.\n";
			throw std::exception("Could not open SVG");
		}

		filestream.seekg(0, std::ios::end);
		str.resize(static_cast<const std::size_t>(filestream.tellg()));		//static_cast silences type shortening warning
		filestream.seekg(0, std::ios::beg);
		filestream.read(&str[0], str.size());
	}
	return str;
}

void read::evaluate_svg(std::string_view svg_view, double board_width, double board_height)
{
	la::Transform_Matrix to_board = la::in_matrix_order(1, 0, 0,
	                                                    0, 1, 0);
	const std::string_view view_box_data = read::get_attribute_data(svg_view, "viewBox=");
	if (view_box_data != "") {
		to_board = View_Box::set(view_box_data, board_width, board_height);
	}
	else {
		const double width = read::to_scaled(read::get_attribute_data(svg_view, "width="));
		const double height = read::to_scaled(read::get_attribute_data(svg_view, "height="));
		to_board = View_Box::set(width, height, board_width, board_height);
	}

	read::evaluate_fragment(svg_view, to_board);
}

void read::evaluate_fragment(std::string_view fragment, const la::Transform_Matrix& transform)
{
	Elem_Data next = take_next_elem(fragment);	//function removes prefix belonging to next also
	while (next.type != Elem_Type::end) {
		switch (next.type) {
		case Elem_Type::svg:
			{
				const std::size_t nested_svg_end = find_closing_elem(fragment, { "<svg " }, { "</svg>" });
				const std::string_view nested_svg_fragment = { fragment.data(), nested_svg_end };

				const double x_offset = to_scaled(get_attribute_data(next.content, { "x=" }), 0.0);
				const double y_offset = to_scaled(get_attribute_data(next.content, { "y=" }), 0.0);
				const la::Transform_Matrix nested_matrix = transform * la::translate({ x_offset, y_offset });

				evaluate_fragment(nested_svg_fragment, nested_matrix);
				fragment.remove_prefix(nested_svg_end + std::strlen("</svg>"));
			}
			break;

		case Elem_Type::g:
			{
				const std::size_t group_end = find_closing_elem(fragment, { "<g " }, { "</g>" });
				const std::string_view group_fragment = { fragment.data(), group_end };

				const la::Transform_Matrix group_matrix = transform * get_transform_matrix(next.content);

				evaluate_fragment(group_fragment, group_matrix);
				fragment.remove_prefix(group_end + std::strlen("</g>"));
			}
			break;

		case Elem_Type::line:		draw::line(transform, next.content);		break;
		case Elem_Type::polyline:	draw::polyline(transform, next.content);	break;
		case Elem_Type::polygon:	draw::polygon(transform, next.content);		break;
		case Elem_Type::rect:		draw::rect(transform, next.content);		break;
		case Elem_Type::ellipse:	draw::ellipse(transform, next.content);		break;
		case Elem_Type::circle:		draw::circle(transform, next.content);		break;
		case Elem_Type::path:		draw::path(transform, next.content);		break;

		case Elem_Type::unknown: break;	//just read in the next element and do nothing
		}
		next = take_next_elem(fragment);	//function removes prefix belonging to next also
	}
}

la::Transform_Matrix read::get_transform_matrix(std::string_view group_attributes)
{
	la::Transform_Matrix result_matrix = la::in_matrix_order(1, 0, 0,
		                                                     0, 1, 0);	//starts as identity matrix

	std::string_view transform_list = get_attribute_data(group_attributes, { "transform=" });
	while (transform_list.length()) {
		for (la::Transform transform : la::all_transforms) {
			const std::string_view name = name_of(transform);
			if (transform_list.compare(0, name.length(), name) == 0) {
				const std::size_t closing_parenthesis = transform_list.find_first_of(')');
				//name does not include parentheses, but the length is one bigger than the biggest index in name. hence name.length() returns the right number
				const std::string_view parameter_view = in_between(transform_list, name.length(), closing_parenthesis); 
				const std::vector<double> parameters = from_csv(parameter_view, true);

				switch (transform) {
				case la::Transform::matrix:
					assert(parameters.size() == 6);
					result_matrix = result_matrix * la::Transform_Matrix{ parameters[0], parameters[1], parameters[2], parameters[3], parameters[4], parameters[5] };
					break;
				case la::Transform::translate:
					if (parameters.size() == 2) {
						result_matrix = result_matrix * la::translate({ parameters[0], parameters[1] });
					}
					else {
						assert(parameters.size() == 1);
						result_matrix = result_matrix * la::translate({ parameters[0], 0.0 });
					}
					break;
				case la::Transform::scale:
					if (parameters.size() == 2) {
						result_matrix = result_matrix * la::scale(parameters[0], parameters[1]);
					}
					else {
						assert(parameters.size() == 1);
						result_matrix = result_matrix * la::scale(parameters[0], parameters[0]);
					}
					break;
				case la::Transform::rotate:
					if (parameters.size() == 1) {
						result_matrix = result_matrix * la::rotate(to_rad(parameters[0]));
					}
					else {
						assert(parameters.size() == 3);
						result_matrix = result_matrix * la::rotate(to_rad(parameters[0]), { parameters[1], parameters[2] });
					}
					break;
				case la::Transform::skew_x:
					assert(parameters.size() == 1);
					result_matrix = result_matrix * la::skew_x(to_rad(parameters[0]));
					break;
				case la::Transform::skew_y:
					assert(parameters.size() == 1);
					result_matrix = result_matrix * la::skew_y(to_rad(parameters[0]));
					break;
				default:
					assert(false);	//if this assert is hit, you may want to update the switch case above
				}

				transform_list.remove_prefix(closing_parenthesis + 1);	//all up to closing parenthesis is removed
				const std::size_t next_not_seperator = transform_list.find_first_not_of(", ");
				if (next_not_seperator != std::string::npos) {
					transform_list.remove_prefix(next_not_seperator);
				}
				else {
					transform_list = "";
				}
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
		snd_letter_pos = view.find_first_of("MmVvHhLlAaCcSsZz", snd_letter_pos);	//QqTt is missing
		content = in_between(view, fst_letter_pos - 1, snd_letter_pos);	//-1 as bezier needs to know what kind
		result = { content, Path_Elem::quadr_bezier, Coords_Type::absolute };		
		break;
	case 'C':
	case 'c':
	case 'S':
	case 's':
		snd_letter_pos = view.find_first_of("MmVvHhLlAaQqTtZz", snd_letter_pos); //CcSs is missing
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
	else view.remove_prefix(snd_letter_pos);

	return result;
}

la::Vec2D path::process_quadr_bezier(const la::Transform_Matrix& transform_matrix, Path_Elem_data data, la::Vec2D current_point)
{
	Bezier_Data curve = take_next_bezier(data.content);
	la::Vec2D last_control_point = la::Vec2D::no_value;
	while (curve.content != "") {
		const std::vector<double> points = from_csv(curve.content);

		if (curve.control_data == Control_Given::expl) {
			assert(points.size() % 4 == 0);

			for (std::size_t i = 0; i < points.size(); i += 4) {
				const la::Vec2D control = curve.coords_type == Coords_Type::absolute ?
					la::Vec2D{ points[i], points[i + 1] } :
					current_point + la::Vec2D{ points[i], points[i + 1] };

				const la::Vec2D end = curve.coords_type == Coords_Type::absolute ?
					la::Vec2D{ points[i + 2], points[i + 3] } :
					current_point + la::Vec2D{ points[i + 2], points[i + 3] };

				draw::quadr_bezier(transform_matrix * current_point, transform_matrix * control, transform_matrix * end);
				current_point = end;
				last_control_point = control;	//not used in this loop, but the loop for implicit control points
			}
		}
		if (curve.control_data == Control_Given::impl) {
			assert(points.size() % 2 == 0);
			if (last_control_point == la::Vec2D::no_value) {
				last_control_point = current_point;	//current_point is also current position
			}

			for (std::size_t i = 0; i < points.size(); i += 2) {
				const la::Vec2D control = compute_contol_point(last_control_point, current_point);

				const la::Vec2D end = curve.coords_type == Coords_Type::absolute ?
					la::Vec2D{ points[i], points[i + 1] } :
					current_point + la::Vec2D{ points[i], points[i + 1] };

				draw::quadr_bezier(transform_matrix * current_point, transform_matrix * control, transform_matrix * end);
				current_point = end;
				last_control_point = control;
			}
		}

		curve = take_next_bezier(data.content);
	}
	return current_point;
}

la::Vec2D path::process_cubic_bezier(const la::Transform_Matrix& transform_matrix, Path_Elem_data data, la::Vec2D current_point)
{
	Bezier_Data curve = take_next_bezier(data.content);
	la::Vec2D last_control_point = la::Vec2D::no_value;
	while (curve.content != "") {
		const std::vector<double> points = from_csv(curve.content);

		if (curve.control_data == Control_Given::expl) {
			assert(points.size() % 6 == 0);

			for (std::size_t i = 0; i < points.size(); i += 6) {
				const la::Vec2D control_1 = curve.coords_type == Coords_Type::absolute ?
					la::Vec2D{ points[i], points[i + 1] } :
					current_point + la::Vec2D{ points[i], points[i + 1] };

				const la::Vec2D control_2 = curve.coords_type == Coords_Type::absolute ?
					la::Vec2D{ points[i + 2], points[i + 3] } :
					current_point + la::Vec2D{ points[i + 2], points[i + 3] };

				const la::Vec2D end = curve.coords_type == Coords_Type::absolute ?
					la::Vec2D{ points[i + 4], points[i + 5] } :
					current_point + la::Vec2D{ points[i + 4], points[i + 5] };

				draw::cubic_bezier(transform_matrix * current_point, transform_matrix * control_1, transform_matrix * control_2, transform_matrix * end);
				current_point = end;
				last_control_point = control_2;	//not used in this loop, but the loop for implicit control points
			}
		}
		if (curve.control_data == Control_Given::impl) {
			assert(points.size() % 4 == 0);
			if (last_control_point == la::Vec2D::no_value) {
				last_control_point = current_point;
			}

			for (std::size_t i = 0; i < points.size(); i += 4) {
				const la::Vec2D control_1 = compute_contol_point(last_control_point, current_point);

				const la::Vec2D control_2 = curve.coords_type == Coords_Type::absolute ?
					la::Vec2D{ points[i], points[i + 1] } :
					current_point + la::Vec2D{ points[i], points[i + 1] };

				const la::Vec2D end = curve.coords_type == Coords_Type::absolute ?
					la::Vec2D{ points[i + 2], points[i + 3] } :
					current_point + la::Vec2D{ points[i + 2], points[i + 3] };

				draw::cubic_bezier(transform_matrix * current_point, transform_matrix * control_1, transform_matrix * control_2, transform_matrix * end);
				current_point = end;
				last_control_point = control_2;
			}
		}
		curve = take_next_bezier(data.content);
	}

	return current_point;
}

la::Vec2D path::compute_contol_point(la::Vec2D last_control_point, la::Vec2D mirror)
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

la::Vec2D path::process_arc(const la::Transform_Matrix& transform_matrix, Path_Elem_data data, la::Vec2D current_point)
{
	const std::vector<double> points = from_csv(data.content);	//data would be a better name than points, but this name is already taken.
	assert(points.size() % 7 == 0);

	for (std::size_t i = 0; i < points.size(); i += 7) {
		double rx = std::abs(points[i]);			//needs to be able to be updated if to small
		double ry = std::abs(points[i + 1]);		//needs to be able to be updated if to small
		const double phi = to_rad(std::fmod(points[i + 2], 360.0));		//phi is often called x_axis_rotation by w3
		const bool large_arc_flag = points[i + 3];	//w3 says any nonzero value is meant as true
		const bool sweep_flag = points[i + 4];
		const double x2 = data.coords_type == Coords_Type::absolute ? points[i + 5] : current_point.x + points[i + 5];
		const double y2 = data.coords_type == Coords_Type::absolute ? points[i + 6] : current_point.y + points[i + 6];

		const double x1 = current_point.x;	//names as in reference
		const double y1 = current_point.y;

		//the following is taken from here: https://www.w3.org/TR/SVG11/implnote.html#ArcImplementationNotes

		//going from  x1 y1 x2 y2 fA fS rx ry phi  to  cx cy theta1 delta theta: (described a little down on that side)

		//step 1:
		const auto [x1_prime, y1_prime] = la::Matrix2X2{ std::cos(phi), std::sin(phi),
		                                                -std::sin(phi), std::cos(phi) } *la::Vec2D{ (x1 - x2) / 2,
		                                                                                            (y1 - y2) / 2 };
		//error correction for to small radii:
		const double lambda = (x1_prime * x1_prime) / (rx * rx) + (y1_prime * y1_prime) / (ry * ry);
		if (lambda > 1.0) {
			rx *= std::sqrt(lambda);
			ry *= std::sqrt(lambda);
		}

		//step 2:
		const double rx2 = rx * rx;						//the 2s stand for squared
		const double ry2 = ry * ry;
		const double x1_prime2 = x1_prime * x1_prime;
		const double y1_prime2 = y1_prime * y1_prime;
		const double sign = large_arc_flag != sweep_flag ? 1.0 : -1.0;
		const la::Vec2D center_prime = sign * std::sqrt(std::abs((rx2 * ry2 - rx2 * y1_prime2 - ry2 * x1_prime2) / 	//in an ideal world abs() is not needed, but negative values may arise from rounding errors.
		                                                          (rx2 * y1_prime2 + ry2 * x1_prime2)))       * la::Vec2D{ rx * y1_prime / ry,
		                                                                                                                  -ry * x1_prime / rx	};
		//step 3:
		const la::Vec2D center = la::Matrix2X2{ std::cos(phi), -std::sin(phi),
		                                        std::sin(phi),  std::cos(phi) } * center_prime + la::Vec2D{ (x1 + x2) / 2,
		                                                                                                    (y1 + y2) / 2 };
		//step 4:	
		const la::Vec2D v1 = la::Vec2D{ (x1_prime - center_prime.x) / rx,
							        	(y1_prime - center_prime.y) / ry };
		const la::Vec2D v2 = la::Vec2D{ (-x1_prime - center_prime.x) / rx,
							        	(-y1_prime - center_prime.y) / ry };
		const double start_angle = angle({ 1, 0 }, v1);			//called theta 1 by w3
		double delta_angle = angle(v1, v2);		                //called delta theta by w3
		if (large_arc_flag) {
			delta_angle += (delta_angle > 0 ? -2 * la::pi : 2 * la::pi);	//function angle() is guaranteed to return a value in interval (-pi, pi]
		}
		if (sweep_flag == (delta_angle < 0)) {	//either sweep_flag is set and delta_angle is smaller than 0 or both are false to enter the condition
			delta_angle *= -1;
		}

		const la::Transform_Matrix from_arc_coordinates = transform_matrix * la::rotate(phi, center);
		draw::arc(from_arc_coordinates, center, rx, ry, start_angle, delta_angle);
		current_point = { x2, y2 };
	}

	return current_point;
}




using namespace draw;

void draw::line(la::Transform_Matrix transform_matrix, std::string_view parameters, std::size_t resolution)
{
	transform_matrix = transform_matrix * read::get_transform_matrix(parameters);

	const double x1 = read::to_scaled(read::get_attribute_data(parameters, { "x1=" }), 0.0);
	const double y1 = read::to_scaled(read::get_attribute_data(parameters, { "y1=" }), 0.0);
	const double x2 = read::to_scaled(read::get_attribute_data(parameters, { "x2=" }), 0.0);
	const double y2 = read::to_scaled(read::get_attribute_data(parameters, { "y2=" }), 0.0);

	const la::Board_Vec start = transform_matrix * la::Vec2D{ x1, y1 };
	const la::Board_Vec end = transform_matrix * la::Vec2D{ x2, y2 };
	save_go_to(start);
	linear_bezier(start, end);
}

void draw::rect(la::Transform_Matrix transform_matrix, std::string_view parameters, std::size_t resolution)
{
	transform_matrix = transform_matrix * read::get_transform_matrix(parameters);

	const double x = read::to_scaled(read::get_attribute_data(parameters, { "x=" }), 0.0);
	const double y = read::to_scaled(read::get_attribute_data(parameters, { "y=" }), 0.0);
	const double width = read::to_scaled(read::get_attribute_data(parameters, { "width=" }), 0.0);
	const double height = read::to_scaled(read::get_attribute_data(parameters, { "height=" }), 0.0);
	double rx = read::to_scaled(read::get_attribute_data(parameters, { "rx=" }), 0.0);
	double ry = read::to_scaled(read::get_attribute_data(parameters, { "ry=" }), 0.0);

	if (rx != 0 && ry == 0) ry = rx;
	if (rx == 0 && ry != 0) rx = ry;
	if (rx > width / 2) rx = width / 2;
	if (ry > height / 2) ry = height / 2;

	if (rx == 0) {	//draw normal rectangle
		const la::Board_Vec upper_right = transform_matrix * la::Vec2D{ x + width, y };
		const la::Board_Vec upper_left =  transform_matrix * la::Vec2D{ x, y };
		const la::Board_Vec lower_left =  transform_matrix * la::Vec2D{ x, y + height };
		const la::Board_Vec lower_right = transform_matrix * la::Vec2D{ x + width, y + height };

		//draws in order       |start
		//                     v
		//            ---(1)---
		//           |         |
		//          (2)       (4)
		//           |         |
		//            ---(3)---
		
		save_go_to(upper_right);								    //start
		linear_bezier(upper_right, upper_left, resolution);			//(1)
		linear_bezier(upper_left, lower_left, resolution);			//(2)
		linear_bezier(lower_left, lower_right, resolution);			//(3)
		linear_bezier(lower_right, upper_right, resolution);		//(4)
	}
	else {	//draw rectangle with corners rounded of
		//these two points are the center point of the upper right ellipse (arc) and the lower left ellipse respectively
		const la::Vec2D center_upper_right = { x + width - rx, y + ry };
		const la::Vec2D center_lower_left = { x + rx, y + height - ry };
		const double right_x = center_upper_right.x;
		const double left_x = center_lower_left.x;
		const double upper_y = center_upper_right.y;
		const double lower_y = center_lower_left.y;

		//draws in order          | start
		//                        v
		//              (2)--(1)--(8)
		//               | X       |
		//              (3)       (7)
		//               |       X |
		//              (4)--(5)--(6)		
		//with X beeing center_upper_right and center_lower_left

		//note: as the y-values become bigger, as one goes down, we have a negative rotation when drawing the rectangle as we do.
		//also: the command (2) and (6) have starting angles one would intuitively change the sign of because of the y-axis direction

		save_go_to(transform_matrix * la::Vec2D{ right_x, upper_y - ry });															 //start
		linear_bezier(transform_matrix * la::Vec2D{ right_x, upper_y - ry }, transform_matrix * la::Vec2D{ left_x, upper_y - ry });	 //(1)
		arc(transform_matrix, { left_x, upper_y }, rx, ry, -la::pi / 2, -la::pi / 2);                                                //(2)
		linear_bezier(transform_matrix * la::Vec2D{ left_x - rx, upper_y }, transform_matrix * la::Vec2D{ left_x - rx, lower_y });	 //(3)
		arc(transform_matrix, { left_x, lower_y }, rx, ry, la::pi, -la::pi / 2);	                                                 //(4)
		linear_bezier(transform_matrix * la::Vec2D{ left_x, lower_y + ry }, transform_matrix * la::Vec2D{ right_x, lower_y + ry });	 //(5)
		arc(transform_matrix, { right_x, lower_y }, rx, ry, la::pi / 2, -la::pi / 2);	                                             //(6)
		linear_bezier(transform_matrix * la::Vec2D{ right_x + rx, lower_y }, transform_matrix * la::Vec2D{ right_x + rx, upper_y }); //(7)
		arc(transform_matrix, { right_x, upper_y }, rx, ry, 0, -la::pi / 2);														 //(8)
	}
}

void draw::circle(la::Transform_Matrix transform_matrix, std::string_view parameters, std::size_t resolution)
{
	transform_matrix = transform_matrix * read::get_transform_matrix(parameters);

	const double cx = read::to_scaled(read::get_attribute_data(parameters, { "cx=" }), 0.0);
	const double cy = read::to_scaled(read::get_attribute_data(parameters, { "cy=" }), 0.0);
	const double r  = read::to_scaled(read::get_attribute_data(parameters, { "r=" }), 0.0);

	save_go_to(transform_matrix * (la::Vec2D{ cx, cy } +la::Vec2D{ r, 0.0 }));	//intersection of positive x-axis and circle is starting point
	for (std::size_t step = 1; step <= resolution; step++) {
		const double angle = 2 * la::pi * (resolution - step) / static_cast<double>(resolution);
		save_draw_to(transform_matrix * la::Vec2D{ cx + std::cos(angle) * r, cy + std::sin(angle) * r });
	}
}

void draw::ellipse(la::Transform_Matrix transform_matrix, std::string_view parameters, std::size_t resolution)
{
	transform_matrix = transform_matrix * read::get_transform_matrix(parameters);

	const double cx = read::to_scaled(read::get_attribute_data(parameters, { "cx=" }), 0.0);
	const double cy = read::to_scaled(read::get_attribute_data(parameters, { "cy=" }), 0.0);
	const double rx = read::to_scaled(read::get_attribute_data(parameters, { "rx=" }), 0.0);
	const double ry = read::to_scaled(read::get_attribute_data(parameters, { "ry=" }), 0.0);

	save_go_to(transform_matrix * (la::Vec2D{ cx, cy } + la::Vec2D{ rx, 0.0 }));	//intersection of positive x-axis and ellipse is starting point
	for (std::size_t step = 1; step <= resolution; step++) {
		const double angle = 2 * la::pi * (resolution - step) / static_cast<double>(resolution);
		save_draw_to(transform_matrix * la::Vec2D{ cx + std::cos(angle) * rx, cy + std::sin(angle) * ry });
	}
}

void draw::polyline(la::Transform_Matrix transform_matrix, std::string_view parameters, std::size_t resolution)
{
	transform_matrix = transform_matrix * read::get_transform_matrix(parameters);

	const std::string_view points_view = get_attribute_data(parameters, { "points=" });
	const std::vector<double> points = from_csv(points_view);
	assert(points.size() % 2 == 0);

	la::Board_Vec start = transform_matrix * la::Vec2D{ points[0], points[1] };
	save_go_to(start);
	for (std::size_t i = 2; i < points.size(); i += 2) {
		const la::Board_Vec end = transform_matrix * la::Vec2D{ points[i], points[i + 1] };
		linear_bezier(start, end);
		start = end;
	}
}

void draw::polygon(la::Transform_Matrix transform_matrix, std::string_view parameters, std::size_t resolution)
{
	transform_matrix = transform_matrix * read::get_transform_matrix(parameters);

	const std::string_view points_view = get_attribute_data(parameters, { "points=" });
	const std::vector<double> points = from_csv(points_view);
	assert(points.size() % 2 == 0);

	la::Board_Vec start = transform_matrix * la::Vec2D{ points[0], points[1] };
	la::Board_Vec end(0, 0);
	save_go_to(start);
	for (std::size_t i = 2; i < points.size(); i += 2) {
		end = transform_matrix * la::Vec2D{ points[i], points[i + 1] };
		linear_bezier(start, end);
		start = end;
	}
	end = transform_matrix * la::Vec2D{ points[0], points[1] };	//polygon is closed -> last operation is to connect to first point
	linear_bezier(start, end);
}

void draw::path(la::Transform_Matrix transform_matrix, std::string_view parameters, std::size_t resolution)
{
	transform_matrix = transform_matrix * read::get_transform_matrix(parameters);

	std::string_view data_view = get_attribute_data(parameters, { "d=" });
	la::Vec2D current_point = { 0.0, 0.0 };	//as a path always continues from the last point, this is the point the last path element ended (this is not yet transformed)
	la::Vec2D current_subpath_begin = { 0.0, 0.0 };	//called initial point by w3
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
				const la::Vec2D next_point = next_elem.coords_type == Coords_Type::absolute ?
					la::Vec2D{ data[0], data[1] } :
					current_point + la::Vec2D{ data[0], data[1] };
				save_go_to(transform_matrix * next_point);
				current_point = next_point;
				current_subpath_begin = current_point;
			}
			for (std::size_t i = 2; i < data.size(); i += 2) {
				const la::Vec2D next_point = next_elem.coords_type == Coords_Type::absolute ?
					la::Vec2D{ data[i], data[i + 1] } :
					current_point + la::Vec2D{ data[i], data[i + 1] };
				draw::linear_bezier(transform_matrix * current_point, transform_matrix * next_point);
				current_point = next_point;
			}
			break;

		case Path_Elem::vertical_line:
			{
				//although that makes no sense, there can be multiple vertical lines stacked -> we directly draw to the end
				const la::Vec2D next_point = next_elem.coords_type == Coords_Type::absolute ?
					la::Vec2D{ current_point.x, data.back() } :
					la::Vec2D{ current_point.x, current_point.y + data.back() };
				draw::linear_bezier(transform_matrix * current_point, transform_matrix * next_point);
				current_point = next_point;
			}
			break;

		case Path_Elem::horizontal_line:
			{
				//although that makes no sense, there can be multiple horizontal lines stacked -> we directly draw to the end
				const la::Vec2D next_point = next_elem.coords_type == Coords_Type::absolute ?
					la::Vec2D{ data.back(), current_point.y } :
					la::Vec2D{ current_point.x + data.back(), current_point.y };
				draw::linear_bezier(transform_matrix * current_point, transform_matrix * next_point);
				current_point = next_point;
			}
			break;

		case Path_Elem::line:
			assert(data.size() % 2 == 0);
			{
				la::Vec2D next_point = { 0.0, 0.0 };
				for (std::size_t i = 0; i < data.size(); i += 2) {
					next_point = next_elem.coords_type == Coords_Type::absolute ?
						la::Vec2D{ data[i], data[i + 1] } :
						current_point + la::Vec2D{ data[i], data[i + 1] };
					draw::linear_bezier(transform_matrix * current_point, transform_matrix * next_point);
					current_point = next_point;
				}
			}
			break;

		case Path_Elem::arc: 
			current_point = process_arc(transform_matrix, next_elem, current_point);
			break;
		case Path_Elem::quadr_bezier: 
			current_point = process_quadr_bezier(transform_matrix, next_elem, current_point);
			break;
		case Path_Elem::cubic_bezier:
			current_point = process_cubic_bezier(transform_matrix, next_elem, current_point);
			break;
		case Path_Elem::closed:
			draw::linear_bezier(transform_matrix * current_point, transform_matrix * current_subpath_begin);
			current_point = current_subpath_begin;
			break;
		}
		next_elem = path::take_next_elem(data_view);
	}
}

void draw::arc(const la::Transform_Matrix& transform_matrix, la::Vec2D center, double rx, double ry, double start_angle, double delta_angle, std::size_t resolution)
{
	const double angle_per_step = delta_angle / resolution;

	for (std::size_t step = 1; step <= resolution; step++) {
		const double angle = start_angle + angle_per_step * step;
		save_draw_to(transform_matrix * la::Vec2D{ center.x + std::cos(angle) * rx, center.y + std::sin(angle) * ry });
	}
}

void draw::linear_bezier(la::Board_Vec start, la::Board_Vec end, std::size_t resolution)
{
	for (std::size_t step = 1; step <= resolution; step++) {
		//as given in wikipedia for linear bezier curves: https://en.wikipedia.org/wiki/B%C3%A9zier_curve#Linear_B%C3%A9zier_curves
		const double t = step / static_cast<double>(resolution);
		save_draw_to(start + t * (end - start));
	}
}

void draw::quadr_bezier(la::Board_Vec start, la::Board_Vec control, la::Board_Vec end, std::size_t resolution)
{
	for (std::size_t step = 1; step <= resolution; step++) {
		//formula taken from https://en.wikipedia.org/wiki/B%C3%A9zier_curve#Quadratic_B%C3%A9zier_curves
		const double t = step / static_cast<double>(resolution);
		const la::Board_Vec waypoint = (1 - t) * (1 - t) * start + 2 * (1 - t) * t * control + t * t * end;
		save_draw_to(waypoint);
	}
}

void draw::cubic_bezier(la::Board_Vec start, la::Board_Vec control_1, la::Board_Vec control_2, la::Board_Vec end, std::size_t resolution)
{
	for (std::size_t step = 1; step <= resolution; step++) {
		//formula taken from https://en.wikipedia.org/wiki/B%C3%A9zier_curve#Cubic_B%C3%A9zier_curves
		const double t = step / static_cast<double>(resolution);
		const double tpow2 = t * t;
		const double onet = (1 - t);
		const double onetpow2 = onet * onet;
		const la::Board_Vec waypoint = onetpow2 * (onet * start + 3 * t * control_1) + tpow2 * (3 * onet * control_2 + t * end);
		//const la::Vec2D waypoint = (1 - t) * (1 - t) * (1 - t) * start + 3 * (1 - t) * (1 - t) * t * control_1 + 3 * (1 - t) * t * t * control_2 + t * t * t * end; //<- equivalent function
		save_draw_to(waypoint);
	}
}
