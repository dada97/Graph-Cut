#pragma once

#include "maxflow.h"
#include "Overlap.h"
#include "ImagesController.h"
#include <filesystem>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/core/utils/filesystem.hpp>
#include <chrono>
#include "Utils.h"

using namespace std;
using namespace cv;
using namespace std::chrono;

using maxflow::Graph_III;

namespace fs = std::filesystem;

class GraphCut {
public:
	GraphCut() {};
	GraphCut(string inputpath,string outputpath);
	void startStitching();
private:
	string inputDir;
	string outputDir;
	string debugDir;

	ImagesController imagesController;
};