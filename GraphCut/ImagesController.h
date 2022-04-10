#pragma once

#include "maxflow.h"
#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include "Utils.h"
#include "Overlap.h"
#include <filesystem>

using namespace std;
using namespace cv;

namespace fs = std::filesystem;
using maxflow::Graph_III;

struct ImgData
{
	Mat img;
	Mat imgScaled;
	Mat gradient;
};

class ImagesController {
public:
	ImagesController() {};
	Mat stitchingImages(string imgdir);

	int currentFrameindex = 0;
private:

	void readImages(string imgdir);
	void findOverlapRegion();
	void buildGraph();
	int edgeEnergy(Point2d s, Point2d t);
	Mat stitchImage(Mat source, Mat sink);
	Mat textureMapping();

	vector<ImgData> images;
	vector<ImgData> previousImg;
	vector<Overlap> overlapROI;

	Mat stitchResult;

	ImgData sourceData;
	ImgData sinkData;

	int sourceIndex;
	int sinkIndex;

	Overlap overlap;
	Mat label;
	Graph_III* G;

	std::map<int, int> pixelIndex2nodeIndex;
	vector<map<pair<int, int>, int>>previousEnergy;

	int imgindex = 0;
	int scalefactor = 3;
};