#include <iostream>
#include <sstream>
#include <charconv>

#include "svgHandling.hpp"
#include "test.hpp"

int real_main(int argc, char* argv[])
{
	if (argc == 1) {
		std::cout << "Error: Expected SVG file name, width in mm and height in mm as console parameter.\n";
	}
	else if (argc == 4) {
			const std::string svg_name = argv[1] + std::string(".svg");
			const std::string bbf_name = argv[1] + std::string(".bbf");
			const std::string bmp_name = argv[1] + std::string(".bmp");
			const double width = std::strtod(argv[2], nullptr);
			const double height = std::strtod(argv[3], nullptr);
			test::svg_to_bmp(svg_name.c_str(), bmp_name.c_str(), width * 2, height * 2);
			test::svg_to_bbf(svg_name.c_str(), bbf_name.c_str(), width, height);
	}
	else {
		std::cout << "Error: wrong numer of parameters.\n";
	}
	std::cout << "Press [Enter] to close program\n";
	std::cin.get();
	return 0;
}

int main(int argc, char* argv[])
{
	return real_main(argc, argv);
	const double width = 350;
	const double height = 350;
	//test::svg_to_bmp("samples/rect01.svg", "samples/bmps/rect01.bmp", width, height);
	//test::svg_to_bmp("samples/rect02.svg", "samples/bmps/rect02.bmp", width, height);
	//test::svg_to_bmp("samples/circle01.svg", "samples/bmps/circle01.bmp", width, height);
	//test::svg_to_bmp("samples/ellipse01.svg", "samples/bmps/ellipse01.bmp", width, height);
	//test::svg_to_bmp("samples/line01.svg", "samples/bmps/line01.bmp", width, height);
	//test::svg_to_bmp("samples/polygon01.svg", "samples/bmps/polygon01.bmp", width, height);
	//test::svg_to_bmp("samples/polyline01.svg", "samples/bmps/polyline01.bmp", width, height);
	//test::svg_to_bmp("samples/triangle01.svg", "samples/bmps/triangle01.bmp", width, height);
	//test::svg_to_bmp("samples/quad01.svg", "samples/bmps/quad01.bmp", width, height);
	//test::svg_to_bmp("samples/arcs01.svg", "samples/bmps/arcs01.bmp", width, height);
	//test::svg_to_bmp("samples/cubic01.svg", "samples/bmps/cubic01.bmp", width, height);
	//test::svg_to_bmp("samples/test1.svg", "samples/bmps/test1.bmp", width, height);
	//test::svg_to_bmp("samples/test2.svg", "samples/bmps/test2.bmp", width, height);
	//test::svg_to_bmp("samples/test3.svg", "samples/bmps/test3.bmp", width, height);
	//test::svg_to_bmp("samples/test4.svg", "samples/bmps/test4.bmp", width, height);
	//test::svg_to_bmp("samples/test5.svg", "samples/bmps/test5.bmp", width, height);
	//test::svg_to_bmp("samples/test6.svg", "samples/bmps/test6.bmp", width, height);
	//test::svg_to_bmp("samples/test7.svg", "samples/bmps/test7.bmp", width, height);
	//test::svg_to_bmp("samples/test8.svg", "samples/bmps/test8.bmp", width, height);
	//test::svg_to_bmp("samples/test9.svg", "samples/bmps/test9.bmp", width, height);
	//test::svg_to_bmp("samples/test10.svg", "samples/bmps/test10.bmp", width, height);
	//test::svg_to_bmp("samples/test11.svg", "samples/bmps/test11.bmp", width, height);
	//test::svg_to_bmp("samples/test12.svg", "samples/bmps/test12.bmp", width, height);
	//test::svg_to_bmp("samples/test13.svg", "samples/bmps/test13.bmp", width, height);
	//test::svg_to_bbf("samples/test13.svg", "samples/bbfs/test13.bbf", width, height);
	//test::svg_to_bmp("samples/test14.svg", "samples/bmps/test14.bmp", width, height);
	//test::svg_to_bmp("samples/test15.svg", "samples/bmps/test15.bmp", width, height);
	//test::svg_to_bbf("samples/test15.svg", "samples/bbfs/test15.bbf", width, height);
	//test::svg_to_bmp("samples/test16.svg", "samples/bmps/test16.bmp", width, height);
	//test::svg_to_bmp("samples/test17.svg", "samples/bmps/test17.bmp", width, height);

	//test::svg_to_bbf("samples/circle02.svg", "samples/bbfs/circle02.bbf", width, height);
	//test::svg_to_bbf("samples/arcs01.svg", "samples/bbfs/arcs01.bbf", width, height);
	//test::svg_to_bbf("samples/car.svg", "samples/bbfs/car.bbf", width, height);
	//test::svg_to_bmp("samples/car.svg", "samples/bmps/car.bmp", width, height);

	//test::svg_to_bmp("samples/selmatec.svg", "samples/bmps/selmatec.bmp", width, height);
	//test::svg_to_bbf("samples/selmatec.svg", "samples/bbfs/selmatec.bbf", width, height);
	//test::svg_to_bmp("samples/Rick_and_Morty.svg", "samples/bmps/Rick_and_Morty.bmp", width, height);
	//test::svg_to_bbf("samples/Rick_and_Morty.svg", "samples/bbfs/Rick_and_Morty.bbf", width, height);
	//test::svg_to_bmp("samples/BoJack_Horseman_Logo.svg", "samples/bmps/BoJack_Horseman_Logo.bmp", width, height);
	//test::svg_to_bbf("samples/BoJack_Horseman_Logo.svg", "samples/bbfs/BoJack_Horseman_Logo.bbf", width, height);
	//test::svg_to_bmp("samples/firefox.svg", "samples/bmps/firefox.bmp", width, height);
	//test::svg_to_bbf("samples/firefox.svg", "samples/bbfs/firefox.bbf", width, height);
	//test::svg_to_bmp("samples/wwf.svg", "samples/bmps/wwf.bmp", width, height);
	//test::svg_to_bbf("samples/wwf.svg", "samples/bbfs/wwf.bbf", width, height);
	//test::svg_to_bmp("samples/homer-simpson.svg", "samples/bmps/homer-simpson.bmp", width, height);
	//test::svg_to_bbf("samples/homer-simpson.svg", "samples/bbfs/homer-simpson.bbf", width, height);

	//std::cin.get();
}

