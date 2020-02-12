#include <wiringPi.h>

#include <stdio.h>

#include <iostream>
#include <fstream>

#define steps_per_mm 10

using namespace std;

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

struct Coord
{
	double x;
	double y;
};

// Schrittmotor Klasse
class stepper_motor {
	char pins[4];
	int current_step = 0;
	ofstream pos;
	public:
		stepper_motor (char, char, char, char);
		void setup();
		void step(int, int);
		int get_current_step();
		void write_current_step(int);
		void write_mm(double);
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
	ifstream configfile;
	configfile.open (to_string(pins[0]) + ".txt");
	double mm << configfile;
	current_step = mm * steps_per_mm;
	configfile.close;
}

// Mach schritte
void stepper_motor::step(int steps, int speed = 100) {
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
}

int stepper_motor::get_current_step(){
	return current_step;
}

void stepper_motor::write_current_step(int lol){
	current_step = lol;
}

void stepper_motor::write_mm(double mm) {
	current_step = mm;
}

stepper_motor left (19, 16, 26, 20);
stepper_motor right (17, 18, 22, 23);

std::ofstream left_store ("left.txt");

int main() {
	wiringPiSetupGpio(); // Initalize Pi GPIO
	printf("Hello world!\n");

	left.setup();
	right.setup();

	right.step(10000, 10);
}
