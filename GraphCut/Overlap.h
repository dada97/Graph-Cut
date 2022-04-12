#pragma once
#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include "Utils.h"

using namespace std;
using namespace cv;

class Overlap {
public:
	Overlap() {};
	Overlap(Mat sourceImg, Mat sinkImg, int sourceindex, int sinkindex);
	void extractROI(Mat sourceImg, Mat sinkImg,int sourceindex,int sinkindex);
	void updateROI(Mat label);
	void extractDataTerm();

	Mat overlapImg;
	Mat overlapBoundary;
	int overlapCenter;
	Mat dataTermMap;
private:
	bool isROI(int y, int x);
	Mat sourceImg;
	Mat sinkImg;
	Mat label;
	Mat boundaryDataterm;
};