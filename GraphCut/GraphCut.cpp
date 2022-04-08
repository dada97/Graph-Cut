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
	int w = source_rescale.size().width;
	int h = sink_rescale.size().height;

	

	Vec4b& colorA_s = source_rescale.at<Vec4b>(s.x , s.y );
	Vec4b& colorB_s = sink_rescale.at<Vec4b>(s.x , s.y );

	Vec4b& colorA_t = source_rescale.at<Vec4b>(t.x , t.y );
	Vec4b& colorB_t = sink_rescale.at<Vec4b>(t.x , t.y );

	
	int gradientA_s = source_rescale.at<Vec4b>(s.x , s.y )[3] ? source_gradient.at<uchar>(s.x, s.y) : 0;
	int gradientB_s = sink_rescale.at<Vec4b>(s.x , s.y )[3] ? sink_gradient.at<uchar>(s.x, s.y) : 0;

	int gradientA_t = source_rescale.at<Vec4b>(t.x , t.y )[3] ? source_gradient.at<uchar>(t.x , t.y ) : 0;
	int gradientB_t = sink_rescale.at<Vec4b>(t.x , t.y )[3] ? sink_gradient.at<uchar>(t.x , t.y ) : 0;


	int cr = abs(colorA_s[0] - colorB_s[0]) + abs(colorA_t[0] - colorB_t[0]);
	int cg = abs(colorA_s[1] - colorB_s[1]) + abs(colorA_t[1] - colorB_t[1]);
	int cb = abs(colorA_s[2] - colorB_s[2]) + abs(colorA_t[2] - colorB_t[2]);

	int gradientcost = abs(gradientA_s - gradientB_s) + abs(gradientA_t - gradientB_t);
	int rgbcost = (cr+cg+cb)/3 ;


	int energy = (0.5 * rgbcost + 0.5 * (gradientcost));
	if (currentFrameindex >0) {
		
		int idx = (s.x) * w + (s.y);

		int idx2 = (t.x) * w + (t.y);

		auto previouskey = previousEnergy[imgindex].find(make_pair(idx, idx2));
		energy = (energy)*0.2 + previouskey->second*0.8;
	}


	return energy;
}

void GraphCut::buildGraph() {

	int w = sourceImg.size().width;
	int h = sourceImg.size().height;

	int rescale_w = sourceImg.size().width / scalefactor;
	int rescale_h = sourceImg.size().height / scalefactor;

	Mat overlapImg = overlap.overlapImg;
	Mat overlapBoundary = overlap.overlapBoundary;
	int overlapCenter = overlap.overlapCenter;

	// calculate node count
	int nodes_count = 0;
	for (int i = 0; i < rescale_h; i++) {
		for (int j = 0; j < rescale_w; j++) {
			int index = i * rescale_w + j;
			if (overlapImg.at <uchar>(i* scalefactor, j* scalefactor)) {

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
			if (overlapImg.at <uchar>(i* scalefactor, j* scalefactor)) {
				G->add_node();
				Point2d p_node(i, j);
				int nodeIndex = pixelIndex2nodeIndex.find(index)->second;

				if (overlapImg.at<uchar>(i* scalefactor, j*scalefactor) == 255, overlapBoundary.at<uchar>(i* scalefactor, j* scalefactor) == 255) {


					if (j* scalefactor < overlapCenter) {
						G->add_tweights(nodeIndex, INT_MAX, 0);
						dataterm.at<Vec3b>(i* scalefactor, j* scalefactor) = Vec3b(255, 0, 0);
					}
					else {
						G->add_tweights(nodeIndex, 0, INT_MAX);
						dataterm.at<Vec3b>(i* scalefactor, j* scalefactor) = Vec3b(0, 255, 0);
					}
				}

				else {
					/*    int sourceWeight, sinkWeight = 0;
						dataterm(p_node, sourceWeight, sinkWeight);
						G.add_tweights(nodeIndex, sourceWeight, sinkWeight);*/
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

Mat GraphCut::textureMapping() {

	int w = sourceImg.size().width;
	int h = sourceImg.size().height;

	int rescale_w = sourceImg.size().width / scalefactor;
	int rescale_h = sourceImg.size().height / scalefactor;

	Mat label(h, w, CV_8UC4, Scalar(0, 0, 0));
	Mat image(h, w, CV_8UC4, Scalar(0, 0, 0, 0));

	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {

			int index = (y/ scalefactor) *(w/ scalefactor) + (x/ scalefactor);
			auto nodeMap = pixelIndex2nodeIndex.find(index);

			bool check = true;

			int node_seg = G->what_segment(nodeMap->second);

			//check seam position
			int cur_seg = -1;
			for (int wx = 0; wx < 10; wx++) {
				if (x + wx > 0 && x + wx < w - 1) {
					int win_idx = (y/ scalefactor)*(w/ scalefactor) + (x + wx)/ scalefactor;
					auto nodeMap2 = pixelIndex2nodeIndex.find(win_idx);
					if (nodeMap2 != pixelIndex2nodeIndex.end()) {
						if (cur_seg == -1) {
							cur_seg = G->what_segment(nodeMap->second);
						}
						if (cur_seg != G->what_segment(nodeMap->second)) {
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
						image.at < Vec4b >(y, x) = sourceImg.at < Vec4b >(y, x);
					}
					else if (node_seg == Graph_III::SINK) {
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

				if (image.at < Vec4b >(y, x) == Vec4b(0,0,0,0)) {
					for (int wx = 0; wx < 10; wx++) {

						if (x + wx > 0 && x + wx < w - 1) {

							int pos_y = y;
							int pos_x = x + wx;

							//int index2 = (pos_y / scalefactor) * (w / scalefactor) + (pos_x / scalefactor);
							//auto nodeMap2 = pixelIndex2nodeIndex.find(index2);
							//if (G->what_segment(nodeMap2->second) == Graph_III::SOURCE) {
							//	label.at < Vec4b >(pos_y, pos_x) = Vec4b(255, 0, 0, 255);
							//}
							//else if (G->what_segment(nodeMap2->second) == Graph_III::SINK) {
							//	label.at < Vec4b >(pos_y, pos_x) = Vec4b(0, 255, 0, 255);
							//}

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
				}

			}

		}
	}
	overlap.updateROI(label);
	
	
	if (Utils::isDebug) {
		string labelpath = Utils::debugPath + "/" + to_string(Utils::sourceImgindex) + "_" + to_string(Utils::sinkImgindex) + "label.png";
		imwrite(labelpath, label);
	}
	
	return image;
}

Mat GraphCut::stitchingImages() {

	auto start = high_resolution_clock::now();
	int w = sourceImg.size().width;
	int h = sourceImg.size().height;

	resize(sourceImg, source_rescale, Size(w / scalefactor, h / scalefactor), 0, 0, INTER_LINEAR);
	resize(sinkImg, sink_rescale, Size(w / scalefactor, h / scalefactor), 0, 0, INTER_LINEAR);


	Mat sourceGray, sinkGray;
	cvtColor(source_rescale, sourceGray,COLOR_RGB2GRAY, 0);
	cvtColor(sink_rescale, sinkGray, COLOR_RGB2GRAY, 0);
	
	Laplacian(sourceGray, source_gradient,CV_8U, 1, 1, 0, BORDER_DEFAULT);
	Laplacian(sinkGray, sink_gradient, CV_8U, 1, 1, 0, BORDER_DEFAULT);
	
	if (Utils::isDebug) {
		string srcgradientpath = Utils::debugPath + "/" + to_string(Utils::sourceImgindex) + "_" + to_string(Utils::sinkImgindex) + "sourceGradient.png";
		imwrite(srcgradientpath, source_gradient);

		string sinkgradientpath = Utils::debugPath + "/" + to_string(Utils::sourceImgindex) + "_" + to_string(Utils::sinkImgindex) + "sinkGradient.png";
		imwrite(sinkgradientpath, sink_gradient);
	}


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

			imgindex = i - 1;
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
		currentFrameindex++;
	}
}
