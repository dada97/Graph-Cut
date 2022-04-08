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

struct StitchImgData
{
	Mat img;
	Mat imgScaled;
	Mat gradient;
};


class ImagesController {
public:
	ImagesController() {};
	void readImages(string imgdir);
	int getImagesNumber();

	Mat stitchingImages();

	int currentFrameindex = 0;
private:
	int edgeEnergy(Point2d s, Point2d t);
	void buildGraph();
	Mat textureMapping();

	Mat stitchImage(Mat source, Mat sink);

	vector<Mat> images;
	Mat stitchResult;

	StitchImgData sourceData;
	StitchImgData sinkData;

	Overlap overlap;
	Graph_III* G;

	std::map<int, int> pixelIndex2nodeIndex;
	vector<map<pair<int, int>, int>>previousEnergy;

	int imgindex = 0;
	int scalefactor = 3;
};