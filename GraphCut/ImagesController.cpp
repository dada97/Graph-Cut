#include "ImagesController.h"

int ImagesController::getImagesNumber() {
	return images.size();
}

void ImagesController::readImages(string imgdir) {
	images.clear();
	for (const auto& entry : fs::directory_iterator(imgdir)) {
		if (entry.path().extension() == ".png") {
			Mat img = imread(entry.path().string(), CV_LOAD_IMAGE_UNCHANGED);
			images.push_back(img);
		}
	}
}

int ImagesController::edgeEnergy(Point2d s, Point2d t) {
	int w = sourceData.imgScaled.size().width;
	int h = sinkData.imgScaled.size().width;

	Vec4b& colorA_s = sourceData.imgScaled.at<Vec4b>(s.x, s.y);
	Vec4b& colorB_s = sinkData.imgScaled.at<Vec4b>(s.x, s.y);

	Vec4b& colorA_t = sourceData.imgScaled.at<Vec4b>(t.x, t.y);
	Vec4b& colorB_t = sinkData.imgScaled.at<Vec4b>(t.x, t.y);

	int gradientA_s = sourceData.imgScaled.at<Vec4b>(s.x, s.y)[3] ? sourceData.gradient.at<uchar>(s.x, s.y) : 0;
	int gradientB_s = sinkData.imgScaled.at<Vec4b>(s.x, s.y)[3] ? sinkData.gradient.at<uchar>(s.x, s.y) : 0;

	int gradientA_t = sourceData.imgScaled.at<Vec4b>(t.x, t.y)[3] ? sourceData.gradient.at<uchar>(t.x, t.y) : 0;
	int gradientB_t = sinkData.imgScaled.at<Vec4b>(t.x, t.y)[3] ? sinkData.gradient.at<uchar>(t.x, t.y) : 0;

	int cr = abs(colorA_s[0] - colorB_s[0]) + abs(colorA_t[0] - colorB_t[0]);
	int cg = abs(colorA_s[1] - colorB_s[1]) + abs(colorA_t[1] - colorB_t[1]);
	int cb = abs(colorA_s[2] - colorB_s[2]) + abs(colorA_t[2] - colorB_t[2]);

	int gradientcost = abs(gradientA_s - gradientB_s) + abs(gradientA_t - gradientB_t);
	int rgbcost = (cr + cg + cb) / 3;


	int energy = (0.5 * rgbcost + 0.5 * (gradientcost));
	if (currentFrameindex > 0) {

		int idx = (s.x) * w + (s.y);

		int idx2 = (t.x) * w + (t.y);

		auto previouskey = previousEnergy[imgindex].find(make_pair(idx, idx2));
		energy = (energy) * 0.2 + previouskey->second * 0.8;
	}


	return energy;
}

void ImagesController::buildGraph() {
	int w = sourceData.img.size().width;
	int h = sourceData.img.size().height;

	int rescale_w = sourceData.img.size().width / scalefactor;
	int rescale_h = sourceData.img.size().height / scalefactor;

	Mat overlapImg = overlap.overlapImg;
	Mat overlapBoundary = overlap.overlapBoundary;
	int overlapCenter = overlap.overlapCenter;
	// calculate node count
	int nodes_count = 0;
	for (int i = 0; i < rescale_h; i++) {
		for (int j = 0; j < rescale_w; j++) {
			int index = i * rescale_w + j;
			if (overlapImg.at <uchar>(i * scalefactor, j * scalefactor)) {

				pixelIndex2nodeIndex[index] = nodes_count++;
			}
		}
	}
	//calculate edge count
	int edge_count = 0;
	for (int i = 0; i < rescale_h; i++) {
		for (int j = 0; j < rescale_w; j++) {
			int index = i * rescale_w + j;
			if (pixelIndex2nodeIndex.find(index) != pixelIndex2nodeIndex.end()) {
				int leftIndex = i * rescale_w + (j - 1);
				if (pixelIndex2nodeIndex.find(leftIndex) != pixelIndex2nodeIndex.end()) {
					edge_count++;
				}
				int upIndex = (i - 1) * rescale_w + j;
				if (pixelIndex2nodeIndex.find(upIndex) != pixelIndex2nodeIndex.end()) {
					edge_count++;
				}
			}
		}
	}
	//fill out graph
	G = new Graph_III(nodes_count, edge_count);
	Mat dataterm = Mat(h, w, CV_8UC3);

	std::map<std::pair<int, int>, int> currentEdgeEnergy;


	for (int i = 0; i < rescale_h; i++) {
		for (int j = 0; j < rescale_w; j++) {
			int index = i * rescale_w + j;
			if (overlapImg.at <uchar>(i * scalefactor, j * scalefactor)) {
				G->add_node();
				Point2d p_node(i, j);
				int nodeIndex = pixelIndex2nodeIndex.find(index)->second;

				if (overlapImg.at<uchar>(i * scalefactor, j * scalefactor) == 255, overlapBoundary.at<uchar>(i * scalefactor, j * scalefactor) == 255) {


					if (j * scalefactor < overlapCenter) {
						G->add_tweights(nodeIndex, INT_MAX, 0);
						dataterm.at<Vec3b>(i * scalefactor, j * scalefactor) = Vec3b(255, 0, 0);
					}
					else {
						G->add_tweights(nodeIndex, 0, INT_MAX);
						dataterm.at<Vec3b>(i * scalefactor, j * scalefactor) = Vec3b(0, 255, 0);
					}
				}

				// left edge weight
				int leftIndex = i * rescale_w + (j - 1);
				Point2d p_left(i, (j - 1));
				auto leftNodeMap = pixelIndex2nodeIndex.find(leftIndex);

				if (leftNodeMap != pixelIndex2nodeIndex.end()) {
					int e = edgeEnergy(p_node, p_left);
					G->add_edge(nodeIndex, leftNodeMap->second, e, e);
					currentEdgeEnergy[std::make_pair(index, leftIndex)] = e;
				}

				// up edge weight
				int upIndex = (i - 1) * rescale_w + j;
				Point2d p_up((i - 1), j);
				auto upNodeMap = pixelIndex2nodeIndex.find(upIndex);
				if (upNodeMap != pixelIndex2nodeIndex.end()) {
					int e = edgeEnergy(p_node, p_up);
					G->add_edge(nodeIndex, upNodeMap->second, e, e);
					currentEdgeEnergy[std::make_pair(index, upIndex)] = e;
				}


			}
		}
	}

	if (currentFrameindex == 0) {
		previousEnergy.push_back(currentEdgeEnergy);
	}
	else {
		previousEnergy[imgindex] = currentEdgeEnergy;
	}
	currentEdgeEnergy.clear();
	string datatermpath = Utils::debugPath + "/" + to_string(Utils::sourceImgindex) + "_" + to_string(Utils::sinkImgindex) + "dataTerm.jpg";
	if (Utils::isDebug) {
		imwrite(datatermpath, dataterm);
	}
}

Mat ImagesController::textureMapping() {
	int w = sourceData.img.size().width;
	int h = sourceData.img.size().height;

	int rescale_w = sourceData.img.size().width / scalefactor;
	int rescale_h = sourceData.img.size().height / scalefactor;

	Mat label(h, w, CV_8UC4, Scalar(0, 0, 0));
	Mat image(h, w, CV_8UC4, Scalar(0, 0, 0, 0));

	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {

			int index = (y / scalefactor) * (w / scalefactor) + (x / scalefactor);
			auto nodeMap = pixelIndex2nodeIndex.find(index);

			bool check = true;

			int node_seg = G->what_segment(nodeMap->second);
			int cur_seg = -1;
			//check seam position

			for (int wx = 0; wx < 10; wx++) {
				if (x + wx > 0 && x + wx < w - 1) {
					int win_idx = (y / scalefactor) * (w / scalefactor) + (x + wx) / scalefactor;
					auto nodeMap2 = pixelIndex2nodeIndex.find(win_idx);
					if (nodeMap2 != pixelIndex2nodeIndex.end()) {
						if (cur_seg == -1) {
							cur_seg = G->what_segment(nodeMap2->second);
						}
						if (cur_seg != G->what_segment(nodeMap2->second)) {
							check = false;
						}
					}
				}
			}

			//draw label
			if (nodeMap != pixelIndex2nodeIndex.end()) {
				if (node_seg == Graph_III::SOURCE) {
					label.at < Vec4b >(y, x) = Vec4b(255, 0, 0, 255);
				}
				else if (node_seg == Graph_III::SINK) {
					label.at < Vec4b >(y, x) = Vec4b(0, 255, 0, 255);
				}
			}

			if (check == true) {
				if (nodeMap != pixelIndex2nodeIndex.end()) {
					if (node_seg == Graph_III::SOURCE) {
						image.at < Vec4b >(y, x) = sourceData.img.at < Vec4b >(y, x);
					}
					else if (node_seg == Graph_III::SINK) {
						image.at < Vec4b >(y, x) = sinkData.img.at < Vec4b >(y, x);
					}
				}
				else {
					if (sourceData.img.at < Vec4b >(y, x)[3] != 0) {
						image.at < Vec4b >(y, x) = sourceData.img.at < Vec4b >(y, x);
					}
					else if (sinkData.img.at < Vec4b >(y, x)[3] != 0) {
						image.at < Vec4b >(y, x) = sinkData.img.at < Vec4b >(y, x);
					}
				}
			}
			else {
				if (image.at < Vec4b >(y, x) == Vec4b(0, 0, 0, 0)) {
					for (int wx = 0; wx < 10; wx++) {
						if (x + wx > 0 && x + wx < w - 1) {

							int pos_y = y;
							int pos_x = x + wx;

							if (sourceData.img.at < Vec4b >(pos_y, pos_x)[3] == 0) {
								image.at < Vec4b >(pos_y, pos_x) = sinkData.img.at < Vec4b >(pos_y, pos_x);
							}
							else if (sinkData.img.at < Vec4b >(pos_y, pos_x)[3] == 0) {
								image.at < Vec4b >(pos_y, pos_x) = sourceData.img.at < Vec4b >(pos_y, pos_x);
							}
							else if (image.at < Vec4b >(pos_y, pos_x)[3] == 0) {
								float alpha = 1.0 - (1.0 / 10.0 * ((float)wx));

								float beta = 1 - alpha;

								image.at < Vec4b >(pos_y, pos_x) = sourceData.img.at < Vec4b >(pos_y, pos_x) * alpha + sinkData.img.at < Vec4b >(pos_y, pos_x) * beta;
							}
						}
					}
				}
			}
		}
	}

	if (Utils::isDebug) {
		string labelpath = Utils::debugPath + "/" + to_string(Utils::sourceImgindex) + "_" + to_string(Utils::sinkImgindex) + "label.png";
		imwrite(labelpath, label);
	}

	return image;
}

Mat ImagesController::stitchImage(Mat source, Mat sink) {

	//initial source sink img;
	int w = source.size().width;
	int h = source.size().height;

	sourceData.img = source;
	sinkData.img = sink;

	resize(source, sourceData.imgScaled, Size(w / scalefactor, h / scalefactor), 0, 0, INTER_LINEAR);
	resize(sink, sinkData.imgScaled, Size(w / scalefactor, h / scalefactor), 0, 0, INTER_LINEAR);

	Mat sourceGray, sinkGray;
	cvtColor(sourceData.imgScaled, sourceGray, COLOR_RGB2GRAY, 0);
	cvtColor(sinkData.imgScaled, sinkGray, COLOR_RGB2GRAY, 0);

	Laplacian(sourceGray, sourceData.gradient, CV_8U, 1, 1, 0, BORDER_DEFAULT);
	Laplacian(sinkGray, sinkData.gradient, CV_8U, 1, 1, 0, BORDER_DEFAULT);

	if (Utils::isDebug) {
		string srcgradientpath = Utils::debugPath + "/" + to_string(Utils::sourceImgindex) + "_" + to_string(Utils::sinkImgindex) + "sourceGradient.png";
		imwrite(srcgradientpath, sourceData.gradient);

		string sinkgradientpath = Utils::debugPath + "/" + to_string(Utils::sourceImgindex) + "_" + to_string(Utils::sinkImgindex) + "sinkGradient.png";
		imwrite(sinkgradientpath, sinkData.gradient);
	}

	if (w != sink.size().width || h != sink.size().height) {
		std::printf("Input Images Resolution Not Matched");
		return {};
	}

	overlap.extractROI(sourceData.img, sinkData.img);
	buildGraph();

	// solve min-cut
	int flow = G->maxflow();
	std::printf("Flow = %d\n", flow);

	// output Result label,image,(image with cut seam)
	Mat nodeType(h, w, CV_8UC4, Scalar(0, 0, 0));
	Mat result = textureMapping();


	pixelIndex2nodeIndex.clear();
	G->reset();
	delete G;

	return result;
}


Mat ImagesController::stitchingImages() {

	Mat source = images[0];
	Utils::sinkImgindex = 0;
	for (int i = 1; i < images.size(); i++) {
		imgindex = i - 1;
		int imgindex = i - 1;
		Utils::sinkImgindex = i;
		cout << "index :" << i << endl;

		Mat sink = images[i];
		stitchResult = stitchImage(source, sink);
		source = stitchResult;
		Utils::sourceImgindex = i;
	}

	return stitchResult;
}