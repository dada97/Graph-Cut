#pragma once

#include "maxflow.h"
#include "Overlap.h"
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
	void init(string inputpath, string outputpath);
	void startStitching();
private:
	void loadImagesPathFromFolder(string path, vector<std::string>& imgs);

	int edgeEnergy(Point2d s, Point2d t);
	void buildGraph();
	Mat textureMapping();
	Mat stitchingImages();

	Mat sourceImg;
	Mat sinkImg;

	Overlap overlap;
	Graph_III* G;

	std::map<int, int> pixelIndex2nodeIndex;

	string inputDir;
	string outputDir;
	string debugDir;
	vector<string> imagesDir;

};