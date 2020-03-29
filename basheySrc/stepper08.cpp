#include <wiringPi.h>

#include <stdio.h>

#include <cmath>

#include <iostream>
#include <fstream>

#include <signal.h>

#include <chrono>

#define steps_per_mm 40

//#define width 577
//#define width_s width * steps_per_mm

#define servo_pin 13
#define pd 15 // pen up val
#define pu 165 // pen down val

//using namespace std;

// Sequenz in der die Spulen des Schrittmotors angeschaltet werden müssen
const bool sequence[][4] =
{
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

int mm2steps(double mm)
{
    return std::round(mm * steps_per_mm);
}

double steps2mm(int steps)
{
    return double(steps) / double(steps_per_mm);
}

// Schrittmotor Klasse
class stepper_motor
{
public:
    stepper_motor (char, char, char, char);
    void setup();
    void step(int, int);
    unsigned int current_step = 0;
    char pins[4];
};



// initialisierung mit Anschluss-Pins
stepper_motor::stepper_motor(char IN1, char IN2, char IN3, char IN4)
{
    pins[0] = IN1;
    pins[1] = IN2;
    pins[2] = IN3;
    pins[3] = IN4;
}

// setup der IOs
void stepper_motor::setup()
{
    for(int i = 0; i < 4; i++)
    {
        pinMode(pins[i], OUTPUT);
    }
}

// Mach schritte
void stepper_motor::step(int direction, int speed = 6)
{
    if (direction > 0)
    {
        this->current_step++;
        int seq = this->current_step % 8;
        for(int p = 0; p < 4; p++)
        {
            digitalWrite(this->pins[p], sequence[seq][p]);
        }
        delay(speed);
    }
    else if (direction < 0)
    {
        this->current_step--;
        int seq = this->current_step % 8;
        for(int p = 0; p < 4; p++)
        {
            digitalWrite(this->pins[p], sequence[seq][p]);
        }
        delay(speed);
    }
}


// Plotter Class---------------------------------------------------------
class plotter
{
    stepper_motor &l;
    stepper_motor &r;
    int mode = 1;
public:
    plotter (stepper_motor &, stepper_motor &);
    void setPen(int);
    void go2(double, double, int);
    Coord_mm get_position_mm();
    void draw_bbf(std::string, double, double, int);
    double width;
    void setup();
    void end();
};

plotter::plotter(stepper_motor &A, stepper_motor &B)
    : l(A), r(B)
{
}

void plotter::setup()
{
    this->l.setup();
    this->r.setup();
    char k;
    double W, L, R;
    std::ifstream configfile("config.txt");
    configfile >> k >> W >> k >> L >> k >> R;
    configfile.close();
    this->l.current_step = mm2steps(L);
    this->r.current_step = mm2steps(R);
    this->width = W;
    std::cout << W << "mm width " << L << "mm L " << R << "mm R\n";
}

void plotter::end()
{
    std::ofstream output("config.txt");
    output << "W " << width
           << " L " << steps2mm(this->l.current_step)
           << " R " << steps2mm(this->r.current_step);
    output.close();
    for(int p = 0; p < 4; p++)
    {
        digitalWrite(this->l.pins[p], LOW);
        digitalWrite(this->r.pins[p], LOW);
    }
    this->setPen(0);
    digitalWrite(servo_pin, LOW);
    std::cout << "Config Gespeichert\n";
}

void plotter::setPen(int penMode)
{
    if(penMode == 0)
    {
        //stift hoch
        pwmWrite(servo_pin, pu);
        if (this->mode != penMode) delay(500);
    }
    else
    {
        //stift runter
        pwmWrite(servo_pin, pd);
        if (this->mode != penMode) delay(500);
    }
    this->mode = penMode;
}

void plotter::go2(double x, double y, int penMode = 0)
{
    std::cout << "PenMode " << penMode << " to x: " << x << " y: " << y << "\n";
    this->setPen(penMode);

    int steps_left = mm2steps(sqrt(x * x + y * y)); //total cable step
    int steps_right = mm2steps(sqrt((this->width - x) * (this->width - x) + y * y));
    int delta_l = steps_left - this->l.current_step;
    int delta_r = steps_right - this->r.current_step;

    //double ratio = std::abs(double(delta_l)) / std::abs(double(delta_r));

    if(std::abs(delta_l) > std::abs(delta_r))
    {
        double ratio = std::abs(double(delta_l)) / std::abs(double(delta_r));
        double counter = 0;
        while(this->l.current_step != steps_left)
        {
            this->l.step(delta_l);
            counter++;

            if (counter > ratio)
            {
                r.step(delta_r, 0);
                counter -= ratio;
            }
        }
    }
    else
    {
        double ratio = std::abs(double(delta_r)) / std::abs(double(delta_l));
        double counter = 0;
        while(this->r.current_step != steps_right)
        {
            r.step(delta_r);
            counter++;

            if (counter > ratio)
            {
                l.step(delta_l, 0);
                counter -= ratio;
            }
        }
    }
}

Coord_mm plotter::get_position_mm()
{
    double S1 = steps2mm(this->l.current_step);
    double S2 = steps2mm(this->r.current_step);
    double b = this->width;
    double x = (b * b + S1 * S1 - S2 * S2) / (2 * b);
    double y = sqrt(S1 * S1 - x * x);

    //std::cout << x << " x mm\n" << y << " y mm\n";
    return {x, y};
}


void plotter::draw_bbf(std::string path, double offset_x = 0, double offset_y = 0, int start_line = 0)
{
    auto start = std::chrono::steady_clock::now();
    std::cout << "draw " << path << "\n"
              << "x offset: " << offset_x << "\n"
              << "y offset: " << offset_y << "\n"
              << "starting at Line " << start_line << "\n";

    std::ifstream bbf(path); // bbf Bruno Bashi Format
    int g;
    double x, y;
    unsigned int current_line = 0;
    bool run = true;

    // check if bbf and offset conf is in boundaries
    std::cout << "checking for boundarie issues\n";
    while (bbf >> g >> x >> y)
    {
        x += offset_x;
        y += offset_y;
        if(x > this->width - 5 | x < 5)
        {
            std::cout << "Error: Objekt überschreitet X Grenzen\n";
            run = false;
            break;
        }
        if(y < width / 4)
        {
            std::cout << "Error: Objekt zu weit oben\n";
            run = false;
            break;
        }
        current_line++;
    }

    bbf.clear();
    bbf.seekg(0);
    current_line = 0;
    if(run)
    {
        while (bbf >> g >> x >> y)
        {
            if(current_line >= start_line)
            {
                x += offset_x;
                y += offset_y;

                std::cout << "line: " << current_line << " ";
                if(current_line == start_line) g = 0;
                this->go2(x, y, g);
            }
            current_line++;
        }

        auto end = std::chrono::steady_clock::now();

        std::cout << "Elapsed time in seconds : "
                  << std::chrono::duration_cast<std::chrono::seconds>(end - start).count() << " sec\n";
    }
    else{
        std::cout << "Error: mission abborted due to boundary issues\n";
    }
    bbf.close();
}


stepper_motor right (19, 16, 26, 20);
stepper_motor left (17, 18, 22, 23);
plotter pltr (left, right);


void sighandler(int sig)
{
    std::cout << "\nProgram abgebrochen vom Nutzer" << std::endl;

    pltr.end();
    pwmWrite(servo_pin, 165);

    exit(3);
}


int main(int argc, char *argv[])
{
    signal(SIGABRT, &sighandler);
    signal(SIGTERM, &sighandler);
    signal(SIGINT, &sighandler);

    wiringPiSetupGpio(); // Initalize Pi GPIO
    printf("Hello world!\n");

    pltr.setup();

    pinMode (servo_pin, PWM_OUTPUT) ;
    pwmSetMode (PWM_MODE_MS);
    pwmSetRange (2000);
    pwmSetClock (192);

    std::cout << steps2mm(left.current_step) << " S1 mm\n";
    std::cout << steps2mm(right.current_step) << " S2 mm\n";

    Coord_mm pos = pltr.get_position_mm();
    std::cout << "x: " << pos.x << "\n";
    std::cout << "y: " << pos.y << "\n";


    if (argc == 1)
    {
        std::cout << "Error: Keine Datei ausgewählt\n";
        exit(3);
    }
    else if (argc == 2)
    {
        const std::string path = argv[1] + std::string(".bbf");
        pltr.draw_bbf(path);
    }
    else if (argc == 3)
    {
        const std::string path = argv[1] + std::string(".bbf");
        double x_offset = std::strtod(argv[2], nullptr);
        pltr.draw_bbf(path, x_offset);
    }
    else if (argc == 4)
    {
        const std::string path = argv[1] + std::string(".bbf");
        double x_offset = std::strtod(argv[2], nullptr);
        double y_offset = std::strtod(argv[3], nullptr);
        pltr.draw_bbf(path, x_offset, y_offset);
    }
    else if (argc == 5)
    {
        const std::string path = argv[1] + std::string(".bbf");
        double x_offset = std::strtod(argv[2], nullptr);
        double y_offset = std::strtod(argv[3], nullptr);
        unsigned int start_line = std::atoi(argv[4]);
        pltr.draw_bbf(path, x_offset, y_offset, start_line);
    }
    else if (argc > 5)
    {
        std::cout << "Error: What do all your parameters even mean?\n";
        exit(3);
    }


    pltr.end();
}
