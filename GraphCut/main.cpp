
#include <iostream>
#include <opencv2/highgui/highgui.hpp>  // Video write
#include <filesystem>
#include <chrono>
#include "GraphCut.h"

using namespace std;

namespace fs = std::filesystem;

void loadImageFromFolder(std::string input, std::vector<std::string>& imgs) {

    for (const auto& entry : fs::directory_iterator(input)) {
        if (entry.path().extension() == ".png") {
            imgs.push_back(entry.path().string());
        }
    }
}

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

    GraphCut graphcut;
    graphcut.init(input_dir, output_dir);
    graphcut.startStitching();
}

