#include "overlap.h"

bool Overlap::isROI(int y, int x) {
	return sourceImg.at<Vec4b>(y, x)[3] && sinkImg.at<Vec4b>(y, x)[3];
}

void Overlap::extractROI(Mat source, Mat sink) {
	sourceImg = source;
	sinkImg = sink;

	int w = sourceImg.size().width;
	int h = sourceImg.size().height;

	overlapImg = Mat(h, w, CV_8UC1);
	overlapBoundary = Mat(h, w, CV_8UC1);

	for (int i = 0; i < h; i++) {
		for (int j = 0; j < w; j++) {
			int index = i * w + j;
			if (isROI(i, j)) {
				overlapImg.at < uchar >(i, j) = 255;
			}
		}
	}


	if (Utils::isDebug) {
		string overlappath = Utils::debugPath + "/" + to_string(Utils::sourceImgindex) + "_" + to_string(Utils::sinkImgindex) + "overlap.jpg";
		imwrite(overlappath, overlapImg);
	}

	vector<vector<Point> > contours;
	vector<Vec4i> hierarchy;
	findContours(overlapImg, contours, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE);

	double maxArea = 0;
	int maxAreaContourId = -1;
	for (int j = 0; j < contours.size(); j++) {
		double newArea = cv::contourArea(contours.at(j));
		if (newArea > maxArea) {
			maxArea = newArea;
			maxAreaContourId = j;
		}
	}

	Rect bbox = boundingRect(contours[maxAreaContourId]);
	overlapCenter = bbox.x + bbox.width / 2;
	Mat drawing = Mat(h, w, CV_8UC1);
	drawContours(drawing, contours, maxAreaContourId, 255, 2, LINE_8, hierarchy, 0);
	imwrite("./result/contours.jpg", drawing);

	for (size_t i = 0; i < contours.size(); i++)
	{
		drawContours(overlapBoundary, contours, (int)i, 255, 50, LINE_8, hierarchy, 0);
	}
}