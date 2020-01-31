#pragma once

#include <vector>


struct Coord
{
	double x;
	double y;
};


//defines the behavior of one cable for the time one line segment is drawn
//with beginning of line segment at motorsteps == 0 and timesteps == 0
//speed is assumed to stay constant while drawing the current line segment
class Cable
{
public:
	int new_steps(int timestep);
	int all_steps() const;

	Cable(double speed_);

private:
	double speed; //speed the cable should move at, given in [motorsteps/timestep]
	int motorsteps; //motorsteps since we started to draw the current line
};

void test_cable(double speed, int total_timesteps);


//draws straight line from start to end
void draw_line(Coord start, Coord end);

//draws staight lines from start to waypoints[1], 
void draw_path(Coord start, const std::vector<Coord>& waypoints);

void draw_circle(Coord center, double radius);

Coord get_position();
