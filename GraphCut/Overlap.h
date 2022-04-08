#pragma once
#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include "Utils.h"

using namespace std;
using namespace cv;

class Overlap {
public:
	void extractROI(Mat sourceImg, Mat sinkImg);
	void updateROI(Mat label);

	int overlapCenter;
	Mat overlapImg;
	Mat overlapBoundary;
private:
	bool isROI(int y, int x);
	Mat sourceImg;
	Mat sinkImg;
	Mat label;
};