
#include <iostream>
#include "maxflow.h"
#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <chrono>


using namespace std;
using namespace cv;
using namespace std::chrono;
using maxflow::Graph_III; 

enum Direction {
    NONE = 0,
    LEFT,
    RIGHT,
    UP,
    BOTTOM,
};

//find node index
int nodeIndex(int x,int y,int width,int height, enum Direction dir) {

    if (dir == Direction::NONE) {
        return x * height + y;
    }
    else if (dir == Direction::LEFT) {
        if (x - 1 < 0)
            return (x - 1) * height + y;
        else
            return -1;
    }
    else if(dir == Direction::RIGHT){
        if (x + 1 < width)
            return (x + 1) * height + y;
        else
            return -1;
    }  
    else if (dir == Direction::UP) {
        if (y - 1 < 0) 
            return x * height + y - 1;
        else
            return -1;       
    }
    else if (dir == Direction::BOTTOM) {
        if (y + 1 < height)
            return x * height + y + 1;
        else
            return -1;
    }
    return -1;
}

//compute energy term (Basic Color Difference)
int colorCost(Point2d s, Point2d t,Mat img1,Mat img2) {
 
    Vec4b& colorA_s = img1.at<Vec4b>(s.y, s.x);
    Vec4b& colorB_s = img2.at<Vec4b>(s.y, s.x);

    Vec4b& colorA_t = img1.at<Vec4b>(t.y, t.x);
    Vec4b& colorB_t = img2.at<Vec4b>(t.y, t.x);

    int cr = abs(colorA_s[0] - colorB_s[0]) + abs(colorA_t[0] - colorB_t[0]);
    int cg = abs(colorA_s[1] - colorB_s[1]) + abs(colorA_t[1] - colorB_t[1]);
    int cb = abs(colorA_s[2] - colorB_s[2]) + abs(colorA_t[2] - colorB_t[2]);
    int ca = abs(colorA_s[3] - colorB_s[3]) + abs(colorA_t[3] - colorB_t[3]);

    int energy = (cr + cg + cb + ca)/4;
    return energy;
}


int main()
{
    auto start = high_resolution_clock::now();
    //img1 img2 position
    Point2f pos_img1(0, 6);
    Point2f pos_img2(1098, 0);

    //read img
    Mat imgL = imread("./input/0.png", IMREAD_UNCHANGED);
    Mat imgR = imread("./input/1.png", IMREAD_UNCHANGED);

    cout << "img1 path: ./0.jpg" << endl;
    cout << "img2 path: ./1.jpg" << endl;
    cout << "img1 position offset: 0 6"<<endl;
    cout << "img2 position offset: 1098 0" << endl;

    int panorama_width = max(imgL.cols + pos_img1.x, imgR.cols + pos_img2.x);
    int panorama_height = max(imgL.rows + pos_img1.y, imgR.rows + pos_img2.y);
    cout << "panorama size: " << panorama_width << " " << panorama_height << endl;

    int minx = pos_img2.x;
    int overlap_width = imgL.cols + pos_img1.x - minx;

    int miny = 0;
    int overlap_height = panorama_height;
    cout << "overlap_size : " << overlap_width << " " << overlap_height << endl;

    Mat imgL_panorama(panorama_height, panorama_width, CV_8UC4, Scalar(0, 0, 0));
    Mat imgR_panorama(panorama_height, panorama_width, CV_8UC4, Scalar(0, 0, 0));

    //translate image
    imgL(Rect(0, 0, imgL.cols, imgL.rows)).copyTo(imgL_panorama(Rect(pos_img1.x, pos_img1.y, imgL.cols, imgL.rows)));
    imgR(Rect(0, 0, imgR.cols, imgR.rows)).copyTo(imgR_panorama(Rect(pos_img2.x, pos_img2.y, imgR.cols, imgR.rows)));

    //crop overlap region
    Mat imgL_overlap, imgR_overlap;
    Rect myROI(minx, 0, overlap_width, overlap_height);
    imgL_overlap = imgL_panorama(myROI);
    imgR_overlap = imgR_panorama(myROI);
  
    int nodes_num = overlap_width * overlap_height;
    int edges_num = (overlap_height - 1) * overlap_width + (overlap_width - 1) * overlap_height;
  
    cout << "Graph nodes: " << nodes_num << endl;
    cout << "Graph edges: " << edges_num << endl;

    //Graphcut
    Graph_III G = Graph_III(nodes_num, edges_num);
    /*
            1——4——7——10
          / |  |  |  | \
     src  ——2——5——8——11——sink
          \ |  |  |  | /
            3——6——9——12
    */
    G.add_node(nodes_num);
   
    for (int x = 0; x < overlap_width; x++) {
        for (int y = 0; y < overlap_height; y++) {

            int node = nodeIndex(x, y, overlap_width, overlap_height,Direction::NONE);

            //if pixel at left or right side,set weight to infinity
            if (x == 0) {
                G.add_tweights(node, INT_MAX, 0 );
            }
            else if (x == overlap_width - 1) {
                G.add_tweights(node, 0, INT_MAX);
            }   

            Point2d p_node(x, y); //node coordinate
            Point2d p_right(x+1, y); //right side node coordinate
            Point2d p_bottom(x, y+1); //bottom side node coordinate

            int rightnode = nodeIndex(x, y, overlap_width, overlap_height, Direction::RIGHT);
            int bottomnode = nodeIndex(x, y, overlap_width, overlap_height, Direction::BOTTOM);
                      
            if (rightnode != -1) {
                //compute Cost add weight
                float cost = colorCost(p_node,p_right, imgL_overlap, imgR_overlap);
                G.add_edge(node, rightnode, cost, cost);
            }
          
            if (bottomnode != -1) {
                //compute Cost add weight
                float cost = colorCost(p_node, p_bottom, imgL_overlap, imgR_overlap);
                G.add_edge(node, bottomnode, cost, cost);
            }
        }
    }

    //Calculate maxflow
    G.maxflow();
    Mat overlap_result(overlap_height, overlap_width, CV_8UC4, Scalar(0, 0, 0));
    Mat overlap_result_seam(overlap_height, overlap_width, CV_8UC4, Scalar(0, 0, 0));

    vector<vector<bool>> seam_flag(overlap_height, vector<bool>(overlap_width));
    vector<vector<bool>> fill_flag(overlap_height, vector<bool>(overlap_width));

    for (int x = 0; x < overlap_width; x++) {
        for (int y = 0; y < overlap_height; y++) {
            seam_flag[y][x] = false;
            fill_flag[y][x] = false;
        }
    }

    for (int x = 0; x < overlap_width; x++) {
        for (int y = 0; y < overlap_height; y++) {
            int node = nodeIndex(x, y, overlap_width, overlap_height, Direction::NONE);

            if (G.what_segment(node) == Graph_III::SOURCE) {

                int rightnode = nodeIndex(x, y, overlap_width, overlap_height, Direction::RIGHT);
                int bottomnode = nodeIndex(x, y, overlap_width, overlap_height, Direction::BOTTOM);

                if (rightnode != -1 || bottomnode != 1) {
                    if (G.what_segment(rightnode) == Graph_III::SINK || G.what_segment(bottomnode) == Graph_III::SINK) {
                        seam_flag[y][x] = true;
                    }
                }
            }
            else {

                int upnode = nodeIndex(x, y, overlap_width, overlap_height, Direction::UP);
                int leftnode = nodeIndex(x, y, overlap_width, overlap_height, Direction::LEFT);

                if (G.what_segment(upnode) == Graph_III::SOURCE || G.what_segment(leftnode) == Graph_III::SOURCE) {
                    seam_flag[y][x] = true;                  
                }
            }
        }
    }   

    int blend_width = 40;
    for (int x = 0; x < overlap_width; x++) {
        for (int y = 0; y < overlap_height; y++) {
            int node = nodeIndex(x, y, overlap_width, overlap_height, Direction::NONE);
         
            if (G.what_segment(node) == Graph_III::SOURCE ) {

                overlap_result.at<Vec4b>(y, x) = imgL_overlap.at<Vec4b>(y, x);
                overlap_result_seam.at<Vec4b>(y, x) = imgL_overlap.at<Vec4b>(y, x);
                fill_flag[y][x] = true;

                if (seam_flag[y][x]==true) {

                    Vec4b colorL = imgL_overlap.at<Vec4b>(y, x);
                    Vec4b colorR = imgR_overlap.at<Vec4b>(y, x);

                    overlap_result.at<Vec4b>(y, x)[0] = colorL[0] * 0.5 + colorR[0] * 0.5;
                    overlap_result.at<Vec4b>(y, x)[1] = colorL[1] * 0.5 + colorR[1] * 0.5;
                    overlap_result.at<Vec4b>(y, x)[2] = colorL[2] * 0.5 + colorR[2] * 0.5;       
                    overlap_result.at<Vec4b>(y, x)[3] = 255;

                    overlap_result_seam.at<Vec4b>(y, x) = Vec4b(0, 0, 255,255);
              
                    //Linear Blending
                    for (int i = 0; i < blend_width; i++) {
                      
                       int pos_x = x + 1 - blend_width/2 + i;
                       if (pos_x <0 || pos_x >=overlap_width||seam_flag[y][pos_x]==true) {
                           continue;
                       }
                       else {

                           float alpha = 1-(i / (float)(blend_width));
                           float beta = 1 - alpha;    

                           colorL = imgL_overlap.at<Vec4b>(y, pos_x);
                           colorR = imgR_overlap.at<Vec4b>(y, pos_x);
                           
                           overlap_result.at<Vec4b>(y, pos_x)[0] = colorL[0] * alpha + colorR[0] * beta;
                           overlap_result.at<Vec4b>(y, pos_x)[1] = colorL[1] * alpha + colorR[1] * beta;
                           overlap_result.at<Vec4b>(y, pos_x)[2] = colorL[2] * alpha + colorR[2] * beta;
                           overlap_result.at<Vec4b>(y, pos_x)[3] = 255;

                           overlap_result_seam.at<Vec4b>(y, pos_x)[0] = colorL[0] * alpha + colorR[0] * beta;
                           overlap_result_seam.at<Vec4b>(y, pos_x)[1] = colorL[1] * alpha + colorR[1] * beta;
                           overlap_result_seam.at<Vec4b>(y, pos_x)[2] = colorL[2] * alpha + colorR[2] * beta;
                           overlap_result_seam.at<Vec4b>(y, pos_x)[3] = 255;
                       
                           fill_flag[y][pos_x] = true;
                       }
                   
                    }

                    //result with seam 
                    overlap_result_seam.at<Vec4b>(y, x + 1) = Vec4b(0, 0, 255, 255);
                    overlap_result_seam.at<Vec4b>(y, x - 1) = Vec4b(0, 0, 255, 255);
                }
            }
            else {
             
                if (fill_flag[y][x] == false) {
                    overlap_result.at<Vec4b>(y, x) = imgR_overlap.at<Vec4b>(y, x);
                    overlap_result_seam.at<Vec4b>(y, x) = imgR_overlap.at<Vec4b>(y, x);
                }                           
            }
        }
    }

    //graph cut result image
    Mat panorama (panorama_height, panorama_width, CV_8UC4, Scalar(0, 0, 0));
    Mat panorama_seam (panorama_height, panorama_width, CV_8UC4, Scalar(0, 0, 0));

    //Panorama
    imgL(Rect(0, 0, imgL.cols - overlap_width, imgL.rows)).copyTo(panorama(Rect(pos_img1.x, pos_img1.y, imgL.cols - overlap_width, imgL.rows)));
    imgR(Rect(overlap_width, 0, imgR.cols - overlap_width, imgR.rows)).copyTo(panorama(Rect(pos_img2.x + overlap_width, pos_img2.y, imgR.cols - overlap_width, imgR.rows)));
    overlap_result(Rect(0, 0, overlap_width, overlap_height)).copyTo(panorama(Rect(imgL.cols - overlap_width, 0, overlap_width, overlap_height)));

    //Panorama with seam
    imgL(Rect(0, 0, imgL.cols - overlap_width, imgL.rows)).copyTo(panorama_seam(Rect(pos_img1.x, pos_img1.y, imgL.cols - overlap_width, imgL.rows)));
    imgR(Rect(overlap_width, 0, imgR.cols - overlap_width, imgR.rows)).copyTo(panorama_seam(Rect(pos_img2.x + overlap_width, pos_img2.y, imgR.cols - overlap_width, imgR.rows)));
    overlap_result_seam(Rect(0, 0, overlap_width, overlap_height)).copyTo(panorama_seam(Rect(imgL.cols - overlap_width, 0, overlap_width, overlap_height)));

    imwrite("./result/panorama.png",panorama);
    imwrite("./result/panorama_seam.png", panorama_seam);

    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(stop - start);

    typedef std::chrono::duration<float> float_seconds;
    auto secs = std::chrono::duration_cast<float_seconds>(duration);
    cout <<"\nexecution time: " <<secs.count() << "s" << endl;
 
    return 0;
}

