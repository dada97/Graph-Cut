#include "overlap.h"

Overlap::Overlap(Mat sourceImg, Mat sinkImg, int srcindex, int sinkindex) {
	extractROI(sourceImg, sinkImg, srcindex, sinkindex);;
}

bool Overlap::isROI(int y, int x) {
	Vec4b source_color = sourceImg.at<Vec4b>(y, x)[0];
	Vec4b sink_color = sinkImg.at<Vec4b>(y, x)[0];

	if(source_color==sink_color&&source_color[0]==0&&source_color[1]==0&& source_color[2] == 0)
		return false;
	
	return (sourceImg.at<Vec4b>(y, x)[3] && sinkImg.at<Vec4b>(y, x)[3]);
}

void Overlap::extractROI(Mat source, Mat sink,int srcindex,int sinkindex) {
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

	Mat contourImg = Mat(h, w, CV_8UC1);

	drawContours(overlapBoundary, contours, maxAreaContourId, 255, 40, LINE_8, hierarchy, 0);


	if (Utils::isDebug) {
		string overlappath = Utils::debugPath + "/overlap" + to_string(srcindex) + "_" + to_string(sinkindex) + ".jpg";
		imwrite(overlappath, overlapImg);

		string contourpath = Utils::debugPath + "/contours" + to_string(srcindex) + "_" + to_string(sinkindex) + ".jpg";
		imwrite(contourpath, overlapBoundary);


	}

}

void Overlap::extractDataTerm() {

	int w = overlapImg.size().width;
	int h = overlapImg.size().height;
	dataTermMap = Mat(h, w, CV_8UC3);

	for (int i = 0; i < h; i++) {
		for (int j = 0; j < w; j++) {

			if (overlapImg.at <uchar>(i , j )) {
				
				if (overlapImg.at<uchar>(i , j ) == 255, overlapBoundary.at<uchar>(i , j ) == 255) {

					if (j  < overlapCenter) {
						dataTermMap.at<Vec3b>(i, j) = Vec3b(255, 0, 0);
					}		
					else {
						dataTermMap.at<Vec3b>(i , j ) = Vec3b(0, 255, 0);
					}
				}
			}
		}
	}
	
	if (Utils::isDebug) {
		string datatermpath = Utils::debugPath + "/dataTerm" + to_string(Utils::sourceImgindex) + "_" + to_string(Utils::sinkImgindex) + ".jpg";

		dataTermMap.copyTo(boundaryDataterm, overlapBoundary);
	}

}


void Overlap::updateROI(Mat label){


	int w = label.size().width;
	int h = label.size().height;


	Mat color = Mat(h, w, CV_8UC3);
	Mat mask = Mat(h, w, CV_8UC1);


	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			Vec3b color = label.at<Vec3b>(y, x);
			
			
			for (int wy = 0; wy < 2; wy++) {
				for (int wx = 0; wx < 2; wx++) {

					if (color[0] != 0 && color[1] != 0 && color[2] != 0 && color[3] != 0) {
						continue;
					}
					else if (x + wx < w && y + wy < h) {

						Vec3b othercolor = label.at<Vec3b>(y+wy, x+wx);
						if (color == Vec3b(255, 0, 0)&&othercolor== Vec3b(0, 255, 0)) {
							mask.at<uchar>(y, x) =255;
							break;
						}
						else if (color == Vec3b(0, 255, 0) && othercolor == Vec3b(255, 0, 0)) {
							mask.at<uchar>(y, x) = 255;
							break;
						}
					}
				}
			}
			

		}
	}

	Mat structureElement = getStructuringElement(MORPH_RECT, Size(25, 25), Point(-1, -1));
	dilate(mask, mask, structureElement, Point(-1, -1), 1);

	bitwise_and(mask, overlapImg, mask);
	label.copyTo(color, mask);


	Mat dataTerm = Mat(h, w, CV_8UC3);
	Mat notmask;
	bitwise_not(mask, notmask);
	label.copyTo(dataTerm, notmask);
	boundaryDataterm.copyTo(dataTerm,overlapBoundary);


	if (Utils::isDebug) {
		string seammaskpath = Utils::debugPath + "/seammask" + to_string(Utils::sourceImgindex) + "_" + to_string(Utils::sinkImgindex) + ".jpg";
		imwrite(seammaskpath, mask);
		string colorseammaskpath = Utils::debugPath + "/seamcolormask" + to_string(Utils::sourceImgindex) + "_" + to_string(Utils::sinkImgindex) + ".jpg";
		imwrite(colorseammaskpath, color);
	}
	dataTermMap = dataTerm;

}