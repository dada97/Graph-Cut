#pragma once

#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include "Utils.h"
#include "ImageData.h"

using namespace std;
using namespace cv;

class ImagesController {
public:
	ImagesController(string imagesfolder);

private:
	vector<ImageData> image;
};