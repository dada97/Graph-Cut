
#include <iostream>
#include "GraphCut.h"

using namespace std;


/*
    I/O struture
    --- input
    |---data1
    | |---0
    | | |---0.png
    | | |---1.png
    | |---1
    | | |---0.png
    | | |---1.png
    --- result(create by program)
    |---data1
    | |---panorama
    | | |---0.png
    | | |---1.png
    | | |---2.png

    */

void main(int argc, char** argv)
{
    cout << "Debug mode : " << (Utils::isDebug ? "True": "False") << endl;
    string filename = argv[1];
    string input_dir = "./input/" + filename;
    string output_dir = "./result/" + filename;

    GraphCut graphcut(input_dir, output_dir) ;
    graphcut.startStitching();
}

