#include "GraphCut.h"


void GraphCut::init(string inputpath, string outputpath) {
	inputDir = inputpath;
	outputDir = outputpath;
}

void GraphCut::loadImagesPathFromFolder(std::string path, vector<string>& imgsPath) {
	for (const auto& entry : fs::directory_iterator(path)) {
		if (entry.path().extension() == ".png") {
			imgsPath.push_back(entry.path().string());
		}
	}
}

int GraphCut::edgeEnergy(Point2d s, Point2d t) {

	Vec4b& colorA_s = sourceImg.at<Vec4b>(s.x, s.y);
	Vec4b& colorB_s = sinkImg.at<Vec4b>(s.x, s.y);

	Vec4b& colorA_t = sourceImg.at<Vec4b>(t.x, t.y);
	Vec4b& colorB_t = sinkImg.at<Vec4b>(t.x, t.y);


	int cr = abs(colorA_s[0] - colorB_s[0]) + abs(colorA_t[0] - colorB_t[0]);
	int cg = abs(colorA_s[1] - colorB_s[1]) + abs(colorA_t[1] - colorB_t[1]);
	int cb = abs(colorA_s[2] - colorB_s[2]) + abs(colorA_t[2] - colorB_t[2]);

	int energy = (cr + cg + cb) / 3;

	return energy;
}

void GraphCut::buildGraph() {

	int w = sourceImg.size().width;
	int h = sourceImg.size().height;

	Mat overlapImg = overlap.overlapImg;
	Mat overlapBoundary = overlap.overlapBoundary;
	int overlapCenter = overlap.overlapCenter;

	// calculate node count
	int nodes_count = 0;
	for (int i = 0; i < h; i++) {
		for (int j = 0; j < w; j++) {
			int index = i * w + j;
			if (overlapImg.at <uchar>(i, j)) {

				pixelIndex2nodeIndex[index] = nodes_count++;
			}
		}
	}

	//calculate edge count
	int edge_count = 0;
	for (int i = 0; i < h; i++) {
		for (int j = 0; j < w; j++) {
			int index = i * w + j;
			if (pixelIndex2nodeIndex.find(index) != pixelIndex2nodeIndex.end()) {
				int leftIndex = i * w + (j - 1);
				if (pixelIndex2nodeIndex.find(leftIndex) != pixelIndex2nodeIndex.end()) {
					edge_count++;
				}
				int upIndex = (i - 1) * w + j;
				if (pixelIndex2nodeIndex.find(upIndex) != pixelIndex2nodeIndex.end()) {
					edge_count++;
				}
			}
		}
	}

	//fill out graph
	G = new Graph_III(nodes_count, edge_count);
	Mat dataterm = Mat(h, w, CV_8UC3);
	for (int i = 0; i < h; i++) {
		for (int j = 0; j < w; j++) {
			int index = i * w + j;
			if (overlapImg.at <uchar>(i, j)) {
				G->add_node();
				Point2d p_node(i, j);
				int nodeIndex = pixelIndex2nodeIndex.find(index)->second;

				if (overlapImg.at<uchar>(i, j) == 255, overlapBoundary.at<uchar>(i, j) == 255) {


					if (j < overlapCenter) {
						G->add_tweights(nodeIndex, INT_MAX, 0);
						dataterm.at<Vec3b>(i, j) = Vec3b(255, 0, 0);
					}
					else {
						G->add_tweights(nodeIndex, 0, INT_MAX);
						dataterm.at<Vec3b>(i, j) = Vec3b(0, 255, 0);
					}
				}

				else {
					/*    int sourceWeight, sinkWeight = 0;
						dataterm(p_node, sourceWeight, sinkWeight);
						G.add_tweights(nodeIndex, sourceWeight, sinkWeight);*/
				}

				// left edge weight
				int leftIndex = i * w + (j - 1);
				Point2d p_left(i, j - 1);
				auto leftNodeMap = pixelIndex2nodeIndex.find(leftIndex);
				if (leftNodeMap != pixelIndex2nodeIndex.end()) {
					int e = edgeEnergy(p_node, p_left);
					G->add_edge(nodeIndex, leftNodeMap->second, e, e);
				}

				// up edge weight
				int upIndex = (i - 1) * w + j;
				Point2d p_up(i - 1, j);
				auto upNodeMap = pixelIndex2nodeIndex.find(upIndex);
				if (upNodeMap != pixelIndex2nodeIndex.end()) {
					int e = edgeEnergy(p_node, p_up);
					G->add_edge(nodeIndex, upNodeMap->second, e, e);
				}


			}
		}
	}
	string datatermpath = Utils::debugPath + "/" + to_string(Utils::sourceImgindex) + "_" + to_string(Utils::sinkImgindex) + "dataTerm.jpg";
	if (Utils::isDebug) {
		imwrite(datatermpath, dataterm);
	}

}

Mat GraphCut::textureMapping() {

	int w = sourceImg.size().width;
	int h = sourceImg.size().height;

	Mat label(h, w, CV_8UC4, Scalar(0, 0, 0));
	Mat image(h, w, CV_8UC4, Scalar(0, 0, 0, 0));

	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {

			int index = y * w + x;
			auto nodeMap = pixelIndex2nodeIndex.find(index);

			bool check = true;
			int cur_seg = -1;


			for (int wx = 0; wx < 10; wx++) {
				if (x + wx > 0 && x + wx < w - 1) {
					int win_idx = (y)*w + (x + wx);
					auto nodeMap = pixelIndex2nodeIndex.find(win_idx);
					if (nodeMap != pixelIndex2nodeIndex.end()) {
						if (cur_seg == -1) {
							cur_seg = G->what_segment(nodeMap->second);
						}
						if (cur_seg != G->what_segment(nodeMap->second)) {
							check = false;
						}

					}

				}
			}




			if (check == true) {
				if (nodeMap != pixelIndex2nodeIndex.end()) {
					if (G->what_segment(nodeMap->second) == Graph_III::SOURCE) {
						label.at < Vec4b >(y, x) = Vec4b(255, 0, 0, 127);
						image.at < Vec4b >(y, x) = sourceImg.at < Vec4b >(y, x);
					}
					else if (G->what_segment(nodeMap->second) == Graph_III::SINK) {
						label.at < Vec4b >(y, x) = Vec4b(0, 255, 0, 127);
						image.at < Vec4b >(y, x) = sinkImg.at < Vec4b >(y, x);
					}
				}
				else {
					if (sourceImg.at < Vec4b >(y, x)[3] != 0) {
						image.at < Vec4b >(y, x) = sourceImg.at < Vec4b >(y, x);
					}
					else if (sinkImg.at < Vec4b >(y, x)[3] != 0) {
						image.at < Vec4b >(y, x) = sinkImg.at < Vec4b >(y, x);
					}
				}


			}
			else {
				//for (int wy = -10; wy < 10; wy++) {
				for (int wx = 0; wx < 10; wx++) {

					if (x + wx > 0 && x + wx < w - 1) {


						int pos_y = y;
						int pos_x = x + wx;

						if (sourceImg.at < Vec4b >(pos_y, pos_x)[3] == 0) {
							image.at < Vec4b >(pos_y, pos_x) = sinkImg.at < Vec4b >(pos_y, pos_x);
						}
						else if (sinkImg.at < Vec4b >(pos_y, pos_x)[3] == 0) {
							image.at < Vec4b >(pos_y, pos_x) = sourceImg.at < Vec4b >(pos_y, pos_x);
						}
						else if (image.at < Vec4b >(pos_y, pos_x)[3] == 0) {
							float alpha = 1.0 - (1.0 / 10.0 * ((float)wx));
							//cout << alpha << endl;
							float beta = 1 - alpha;

							image.at < Vec4b >(pos_y, pos_x) = sourceImg.at < Vec4b >(pos_y, pos_x) * alpha + sinkImg.at < Vec4b >(pos_y, pos_x) * beta;
						}


					}

				}
				x += 9;
				// }

			}

		}
	}
	return image;
}

Mat GraphCut::stitchingImages() {

	auto start = high_resolution_clock::now();
	int w = sourceImg.size().width;
	int h = sourceImg.size().height;

	if (w != sinkImg.size().width || h != sinkImg.size().height) {
		std::printf("Input Images Resolution Not Matched");
		return {};
	}
	overlap.extractROI(sourceImg, sinkImg);
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

	auto stop = high_resolution_clock::now();
	auto duration = duration_cast<milliseconds>(stop - start);
	auto secs = std::chrono::duration_cast<std::chrono::duration<float>>(duration);
	cout << "Execution time: " << secs.count() << "s" << endl;

	return result;
}

void GraphCut::startStitching() {


	//create result folder
	cv::utils::fs::createDirectory(outputDir);

	string panoramaDir = cv::utils::fs::join(outputDir, "panorama");
	cv::utils::fs::createDirectory(panoramaDir);
	
	if (Utils::isDebug) {
		debugDir = cv::utils::fs::join(outputDir, "debug");
		cv::utils::fs::createDirectory(debugDir);
	}

	cout << inputDir << endl;

	//list all frame name
	for (const auto& entry : fs::directory_iterator(inputDir)) {
		string frame_name = entry.path().filename().string();
		string imagesDir = entry.path().string();

		if (Utils::isDebug) {
			string debugFrameDir = cv::utils::fs::join(debugDir, frame_name);
			cv::utils::fs::createDirectory(debugFrameDir);
			Utils::debugPath = debugFrameDir;
		}

		vector<string> imagesPath;

		//load all images path in current frame
		loadImagesPathFromFolder(imagesDir, imagesPath);
		cout << "\nTotal images: " << imagesPath.size() << endl;

		auto start = high_resolution_clock::now();

		sourceImg = imread(imagesPath[0], CV_LOAD_IMAGE_UNCHANGED);
		Utils::sourceImgindex = 0;
		Mat result;
		//Start stitching image
		for (int i = 1; i < imagesPath.size(); i++) {
			Utils::sinkImgindex = i;
			cout << imagesPath[i] << endl;

			sinkImg = imread(imagesPath[i], CV_LOAD_IMAGE_UNCHANGED);
			result = stitchingImages();

			sourceImg = result;
			Utils::sourceImgindex = i;
		}

		//write panorama result
		string resultPath = cv::utils::fs::join(panoramaDir, frame_name);
		imwrite(resultPath + ".png", result);

		auto stop = high_resolution_clock::now();
		auto duration = duration_cast<milliseconds>(stop - start);
		auto secs = std::chrono::duration_cast<std::chrono::duration<float>>(duration);
		cout << "Total Execution Time: " << secs.count() << "s"<<endl << endl;
	}
}
