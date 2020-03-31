#include <iostream>
#include <sstream>
#include <charconv>

#include "svgHandling.hpp"
#include "test.hpp"

int real_main(int argc, char* argv[])
{
	if (argc >= 3 && argc <= 5) {
		const std::string svg_name = argv[1] + std::string(".svg");
		const std::string bbf_name = argv[1] + std::string(".bbf");
		const std::string bmp_name = argv[1] + std::string(".bmp");
		double width = 100;
		double height = 100;
		uint16_t mesh_size = 100;
		switch (argc) {
		case 5:
			mesh_size = std::atoi(argv[4]);
		case 4:
			height = std::strtod(argv[3], nullptr);
			width = std::strtod(argv[2], nullptr);
			break;
		case 3:
			height = std::strtod(argv[2], nullptr);
			width = std::strtod(argv[2], nullptr);
			break;
		}
		std::cout << "\nreading in " << svg_name << " ..." << std::endl;
		std::string content_str = read::string_from_file(svg_name.c_str());
		read::preprocess_str(content_str);
		test::svg_to_bmp(content_str, bmp_name.c_str(), width, height, mesh_size, 2);
		test::svg_to_bbf(content_str, bbf_name.c_str(), width, height);
	}
	else {
		std::cout << "Error: wrong numer of parameters.\n";
		std::cout << "valid call options are:                                                examples:\n\n";
		std::cout << "sooperDooperPlooter <SVG_name> <wall_width_&_height>                   sooperDooperPlooter examplePicture 350\n";
		std::cout << "sooperDooperPlooter <SVG_name> <wall_width> <wall_height>              sooperDooperPlooter examplePicture 350 100\n";
		std::cout << "sooperDooperPlooter <SVG_name> <wall_width> <wall_height> <mesh_size>  sooperDooperPlooter examplePicture 350 100 10\n";
		std::cout << "all units are to be provided in mm.\n";
	}
	return 0;
}

int main(int argc, char* argv[])
{
	//return real_main(argc, argv);

	const double width = 350;
	const double height = 350;
	//test::read_string_to_all("1", width, height);
	//test::read_string_to_all("w3test1", width, height);
	//test::read_string_to_all("homer-simpson", width, height);
	//test::read_string_to_all("bojack", width, height);
	test::read_string_to_all("wwf", width, height);
}

