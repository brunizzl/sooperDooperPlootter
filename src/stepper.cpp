#include <wiringPi.h>

#include <stdio.h>

#include <cmath>

#include <iostream>
#include <fstream>

#define steps_per_mm 40

#define width 840
#define width_s width * steps_per_mm

//using namespace std;

// Sequenz in der die Spulen des Schrittmotors angeschaltet werden müssen
const bool sequence[][4] = {
		{ LOW,  LOW,  LOW,  HIGH },
		{ LOW,  LOW,  HIGH, HIGH },
		{ LOW,  LOW,  HIGH, LOW },
		{ LOW,  HIGH, HIGH, LOW },
		{ LOW,  HIGH, LOW,  LOW },
		{ HIGH, HIGH, LOW,  LOW },
		{ HIGH, LOW,  LOW,  LOW },
		{ HIGH, LOW,  LOW,  HIGH }
};

struct Coord_mm
{
	double x;
	double y;
};

struct Coord_steps
{
	int x;
	int y;
};

// Schrittmotor Klasse
class stepper_motor {
	char pins[4];
	int current_step = 0;
	std::ofstream output;
	public:
		stepper_motor (char, char, char, char);
		void setup();
		void step(int, int);
		int get_current_step();
		void write_current_step(int);
		void write_mm(double);
		void save_cable_length();
		void step_to(int, int);
};

// initialisierung mit Anschluss-Pins
stepper_motor::stepper_motor(char IN1, char IN2, char IN3, char IN4) {
	pins[0] = IN1;
	pins[1] = IN2;
	pins[2] = IN3;
	pins[3] = IN4;
}

// setup der IOs
void stepper_motor::setup() {
	for(int i = 0; i < 4; i++) {
		pinMode(pins[i], OUTPUT);
	}
	std::ifstream configfile;
	configfile.open (std::to_string(pins[0]) + ".txt");
	configfile >> current_step;
	configfile.close();
}

// Mach schritte
void stepper_motor::step(int steps, int speed = 10) {
	std::cout << "gehe von " << current_step;
	if (steps > 0) {
		for (int s = steps + current_step; s > current_step; s--) {
			for(int p = 0; p < 4; p++) {
				digitalWrite(pins[p], sequence[s % 8][p]);
			}
			delay(speed);
		}
	} 
	else if (steps < 0) {
		for (int s = current_step; s < -steps + current_step; s++) {
			for(int p = 0; p < 4; p++) {
				digitalWrite(pins[p], sequence[s % 8][p]);
			}
			delay(speed);
		}
	}
	current_step += steps;
	std::cout << " zu " << current_step << " steps\n";
}

void stepper_motor::step_to(int step, int speed = 10){
	while (step < current_step){
		for(int p = 0; p < 4; p++) {
				digitalWrite(pins[p], sequence[current_step % 8][p]);
			}
		current_step--;
		delay(speed);
	}
	while (step > current_step){
		for(int p = 0; p < 4; p++) {
				digitalWrite(pins[p], sequence[current_step % 8][p]);
			}
		current_step++;
		delay(speed);
	}
}

int stepper_motor::get_current_step(){
	return current_step;
}

void stepper_motor::write_current_step(int lol){
	current_step = lol;
}

void stepper_motor::write_mm(double mm) {
	current_step = mm * steps_per_mm;
	std::string filename = std::to_string(pins[0]) + ".txt";
	output.open (filename.c_str());
	output << current_step;
	output.close();
	std::cout << filename << '\n';
}

void stepper_motor::save_cable_length(){
	std::string filename = std::to_string(pins[0]) + ".txt";
	output.open (filename.c_str());
	output << current_step;
	output.close();
	std::cout << "Kabellänge gespeichert" << filename << '\n';
}

stepper_motor right (19, 16, 26, 20);
stepper_motor left (17, 18, 22, 23);

int mm2steps(double mm){
	return mm * steps_per_mm;
}

double steps2mm(int steps){
	return steps / steps_per_mm;
}

void go_to(double x, double y){
	x = mm2steps(x);
	y = mm2steps(y);
	int cable_left = left.get_current_step();
	int cable_right = right.get_current_step();
	int steps_left = sqrt(x * x + y * y); //total cable step
	int steps_right = sqrt((mm2steps(width) - x) * (mm2steps(width) - x) + y * y);
	steps_left -= cable_left;
	steps_right -= cable_right;
	left.step(steps_left);
	right.step(steps_right);
}

Coord_mm get_position_mm(){
	double S1 = steps2mm(left.get_current_step());
	double S2 = steps2mm(right.get_current_step());
	double b = width;
	double x = (b*b + S1*S1 - S2*S2) / (2 * b);
	double y = sqrt(S1*S1 - x*x);

	std::cout << x << " x mm\n" << y << " y mm\n";
	return {x, y};
}

Coord_steps get_position_steps(){
	int S1 = left.get_current_step();
	int S2 = right.get_current_step();
	int b = width_s;
	int x = (b*b + S1*S1 - S2*S2) / (2 * b);
	int y = sqrt(S1*S1 - x*x);

	std::cout << x << " x steps\n" << y << " y steps\n";
	return {x, y};
}

void run_line(double x, double y){
	int x2 = mm2steps(x);
	int y2 = mm2steps(y);
	Coord_steps now = get_position_steps();
	int x1 = now.x;
	int y1 = now.y;
	int dx = x2 - x1;
	int dy = y2 - y1;
	double steigung = dy / dx;
	while(now.x != x2){
		if(dx < 0){
			left
		}
	}
}


int main() {
	wiringPiSetupGpio(); // Initalize Pi GPIO
	printf("Hello world!\n");

	//left.write_mm(963);
	//right.write_mm(900);

	left.setup();
	right.setup();

	std::cout << left.get_current_step() << " S1 Steps\n";
	std::cout << right.get_current_step() << " S2 Steps\n";
	std::cout << steps2mm(left.get_current_step()) << " S1 mm\n";
	std::cout << steps2mm(right.get_current_step()) << " S2 mm\n";
	//std::cout << right.get_current_step() << '\n';
	//right.step(-4000000, 4);
	//left.step(-40000000, 4);
	//go_to(400, 600);
	//std::cout << right.get_current_step() << '\n';

	Coord_mm pos = get_position_mm();

	Coord_steps pos_steps = get_position_steps();

	left.save_cable_length();
	right.save_cable_length();
}
