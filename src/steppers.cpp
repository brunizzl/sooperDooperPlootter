#include "steppers.hpp"

#include <iostream>

Coord_mm get_position()
{
	return { 0, 0 };
}

Cable::Cable(double speed_)
	:speed(speed_), motorsteps(0)
{
}

int Cable::new_steps(int timestep)
{
	//complete steps to take from start of line segment to now with given speed
	const int discrete_steps = this->speed * timestep;    		//computed in double, then floored to next lower int

	//difference of steps needed to be taken and steps actually taken
	const int delta_steps = discrete_steps - this->motorsteps;  

	this->motorsteps += delta_steps; //the result of this function is used to turn the motor by delta_steps. that is recorded here
	return delta_steps;
}

int Cable::all_steps() const
{
	return this->motorsteps;
}

void test_cable(double speed, int total_timesteps)
{
	std::cout << "speed: " << speed << '\n';
	Cable cable(speed);
	for (int timestep = 1; timestep <= total_timesteps; timestep++) {
		if (timestep != 1) {
			std::cout << ", ";
		}
		std::cout << cable.new_steps(timestep);
	}
	std::cout << "\ntotal motor steps done over " << total_timesteps << " timesteps: " << cable.all_steps() << std::endl;
}
