
#include "svgHandling.hpp"


#include <cmath>
#include <iostream>
#include <fstream>
#include <streambuf>
#include <cassert>


void draw_from_file(const char* const name)
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
	while (test.type != read::Elem_Type::unknown) {
		std::cout << name_of(test.type) << ":\n" << test.content << "\n\n";
		test = read::next_elem({ test.content.data() + test.content.length()});
	}
}

using namespace read;

std::string_view read::name_of(Elem_Type type)
{
	switch (type) {
	case Elem_Type::svg:			return { "svg" };
	case Elem_Type::svg_end:		return { "/svg" };
	case Elem_Type::line:			return { "line" };
	case Elem_Type::polyline:		return { "polyline" };
	case Elem_Type::polygon:		return { "polygon" };
	case Elem_Type::rect:			return { "rect" };
	case Elem_Type::ellypse:		return { "ellypse" };
	case Elem_Type::circle:			return { "circle" };
	case Elem_Type::path:			return { "path" };
	case Elem_Type::transform:		return { "transform" };
	case Elem_Type::g:				return { "g" };
	case Elem_Type::g_end:			return { "/g" };
	case Elem_Type::unknown:		return { "unknown" };
	}
	assert(false);	//if this assert is hit, you may update the switchcase above.
	return {};
}

std::size_t read::find_skip_quotations(std::string_view search_zone, std::string_view find)
{
	std::size_t next_quotation_start = search_zone.find_first_of('\"');
	std::size_t prev_quotation_end = 0;

	while (next_quotation_start != std::string::npos) {
		const std::string_view search_section = { search_zone.data() + prev_quotation_end, next_quotation_start - prev_quotation_end };
		const std::size_t found = search_section.find(find);
		if (found != std::string::npos) {
			return found;
		}
		prev_quotation_end = search_zone.find_first_of('\"', next_quotation_start + 1);
		if (prev_quotation_end == std::string::npos) {
			throw std::exception("function read::find_skip_quotations(): quotation (using \"\") was started, but not ended.");
		}
		next_quotation_start = search_zone.find_first_of('\"', prev_quotation_end + 1);
	}

	return search_zone.find(find, prev_quotation_end);
}

Elem_Data read::next_elem(std::string_view view)
{
	const std::size_t open_bracket = view.find_first_of('<');
	if (open_bracket == std::string::npos) {
		return { Elem_Type::unknown, "" };
	}
	view.remove_prefix(open_bracket + 1);	//if view started as "\n <circle cx=...", it now is "circle cx=..."

	for (Elem_Type type : all_elem_types) {
		const std::string_view type_name = name_of(type);
		if (view.compare(0, type_name.length(), type_name) == 0) {
			view.remove_prefix(type_name.length());		//"circle cx=..." becomes " cx=..."
			const std::size_t end_bracket = view.find_first_of('>');
			if (end_bracket == std::string::npos) {
				throw std::exception("function read::next_elem(): element type identified, but end sequence of element not found");
			}
			view.remove_suffix(view.length() - end_bracket);
			return { type, view };
		}
	}
	return { Elem_Type::unknown, "" };
}



std::vector<Coord_mm> draw::circle(Coord_mm center, double radius, std::size_t resolution)
{
	std::vector<Coord_mm> path;
	path.reserve(resolution + 1);	//startpoint is also endpoint -> has to be inserted in the end again

	for (std::size_t nr = 0; nr < resolution; nr++) {
		const double angle = 2 * pi * (resolution - nr);
		path.push_back({ center.x + std::cos(angle) * radius, center.y + std::sin(angle) * radius });
	}
	path.push_back(path.front());
	return path;
}