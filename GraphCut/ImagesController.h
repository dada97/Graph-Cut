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
	ImagesController(string inputpath,string outputpath);
	
	void readImages(string imgdir);
	void stitchingImages();
	void buildGraph();
	Mat textureMapping();

	int getImagesNumber();



	Mat result;
private:
	int edgeEnergy(Point2d s, Point2d t);
	Mat stitchImage(Mat source,Mat sink);

	vector<Mat> images;

	string inputDir;
	string outputDir;

	Overlap overlap;
	Graph_III* G;

	StitchImgData sourceData;
	StitchImgData sinkData;

	std::map<int, int> pixelIndex2nodeIndex;
	vector<map<pair<int, int>, int>>previousEnergy;

	int currentFrameindex = 0;
	int imgindex = 0;

	int scalefactor = 3;
};