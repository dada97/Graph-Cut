#pragma once

#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include "Utils.h"

using namespace std;
using namespace cv;

class ImageData {
private:
	Mat img;
	int width;
	int height;
};