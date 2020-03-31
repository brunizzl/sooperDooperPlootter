# sooperDooperPlootter

This is a project for class "Betriebssysteme - eine Einführung" at Leuphana University Lüneburg. 

sooperDooperPlooter is currently split into two parts. The folder brunoSrc contains a program to convert SVG graphics into very easy to parse bbf (Bashey-Bruno-Format) data, witch is a textfile with numbers seperated by whitespaces. 
The second part is found in folder basheySrc. This second program is meant to be compiled and run on the raspberry pie of the plotting device. Here the bbf file is read in.

The syntax of a bff command bff is as follows: [flag] [x-coordinate] [y-coordinate], where [flag] is eighter '0' meaning "go to the specified coordinates" or '1' meaning "draw a straight line from the current position to the specified coordinates". A bbf file can have an abitrary ammount of commands, but is required to start with a goto.

The plotter itself is not an own design, but developed by Fredrik Stridsman, his project can be found here https://github.com/snebragd/stringent.

The library libBMP.h is taken from the course "Prozedurale Programmierung" at Hamburg University of Tecnology written by 
Kai Ohlhus and shared with permission from institute E19, TUHH.

# Setup

Compile all files in brunoSrc on any system as one single C++ project, C++17 or higher is required.
Compile the files in basheySrc for raspbian, C++11 or higher is required.
