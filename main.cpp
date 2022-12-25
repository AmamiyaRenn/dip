#include <iostream>
#include <string>
#include <vector>

#include <opencv2/core/cvstd.hpp>
#include <opencv2/core/types.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/opencv.hpp>

#include "meter_reader.h"

void on_findPointer(int, void*);
void on_HoughCircles(int, void*);
void on_threshold(int, void*);

// Define Variables
cv::Mat                src_img, mid_img, gray_img, gray_img_copy, gray_origin_img, bin_img, dst_img, canny_img;
std::vector<cv::Vec3f> pcircles;
int                    tracker_max = 200;
int                    tracker_val = 100;

int gray_center_x = 0;
int gray_center_y = 0;

// Define File Path
std::string src_image_path = "../img/";

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        std::cerr << "lack of args" << std::endl;
        return -1;
    }
    src_image_path += std::string(argv[1]) + ".jpg";

    MeterReader meter_reader(true);
    meter_reader.readImage(src_image_path);
    meter_reader.findCenter();
    meter_reader.findCircle();
    meter_reader.findPointer();

    src_img = meter_reader.getGrayOpenImg();
    // cv::namedWindow("tracker", cv::WINDOW_NORMAL);
    // cv::createTrackbar("参数值", "tracker", &tracker_val, tracker_max, on_findPointer);
    // cv::createTrackbar("参数值", "tracker", &tracker_val, tracker_max, on_threshold);

    meter_reader.displayImage();

    return 0;
}

void on_findCenter()
{
    dst_img = src_img.clone();
    cv::HoughCircles(gray_img_copy, pcircles, cv::HOUGH_GRADIENT, 1, 10, 80, tracker_val, 2, 200);
    // cv::HoughCircles(gray_img, pcircles, cv::HOUGH_GRADIENT, 1, 10, 80, 40, 2, 45);
    // cv::HoughCircles(gray_img, pcircles, cv::HOUGH_GRADIENT, 1, 10, 80, 50, g_houghValue, g_houghValueMax);
    for (auto cc : pcircles)
    {
        cc[0] += gray_center_y;
        cc[1] += gray_center_x;
        // 画圆
        cv::circle(dst_img, cv::Point(cc[0], cc[1]), cc[2], cv::Scalar(0, 0, 255), 1, cv::LINE_AA);
        // 画圆心
        cv::circle(dst_img, cv::Point(cc[0], cc[1]), 2, cv::Scalar(127, 255, 127), 1, cv::LINE_AA);
    }
}

// 获得指针线
void on_findPointer(int, void*)
{
    cv::Mat show_img = src_img.clone();

    cv::Canny(src_img, canny_img, 100, 200, 3);
    // cv::imshow("canny", canny_img);

    std::vector<cv::Vec4i> lines;
    // 统计霍夫变换得到指针线
    cv::HoughLinesP(canny_img, lines, 1., CV_PI / 180, tracker_val, 100, 30);

    for (auto line : lines)
        cv::line(
            show_img, cv::Point(line[0], line[1]), cv::Point(line[2], line[3]), cv::Scalar(0, 0, 255), 2, cv::LINE_AA);

    if (lines.size() == 2)
    {
        cv::Point2f intersect = MeterReader::getCrossPoint(lines[0], lines[1]);

        cv::circle(show_img, intersect, 2, cv::Scalar(127, 255, 127), 2, cv::LINE_AA);
    }

    cv::imshow("tracker", show_img);
}

void on_threshold(int, void*)
{
    cv::Mat show_img = src_img.clone();
    cv::threshold(src_img, gray_img, tracker_val, 255, cv::THRESH_BINARY);
    // cv::imshow("tracker", gray_img);
}
