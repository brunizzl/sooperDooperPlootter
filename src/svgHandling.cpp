
#include "svgHandling.hpp"


//all needed for nanosvg
#include <stdio.h>
#include <string.h>
#include <math.h>
#define NANOSVG_IMPLEMENTATION
#include "nanosvg.h"


#include <cmath>
#include <iostream>

std::vector<Coord_mm> to_path::circle(Coord_mm center, double radius, std::size_t resolution)
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

void test::nanosvg()
{
	NSVGimage* image = nsvgParseFromFile("test.svg", "px", 96);
	printf("size: %f x %f\n", image->width, image->height);
	// Use...
	for (NSVGshape* shape = image->shapes; shape != NULL; shape = shape->next) {
		for (NSVGpath* path = shape->paths; path != NULL; path = path->next) {
			for (int i = 0; i < path->npts - 1; i += 3) {
				std::cout << "        punkte " << i << " bis " << i + 2 << ": " << path->pts[i] << ", " << path->pts[i + 1] << ", " << path->pts[i + 2] << '\n';

			}
			std::cout << "    naechster pfad...\n";
		}
		std::cout << "naechste shape...\n";
	}
	// Delete
	nsvgDelete(image);
}
