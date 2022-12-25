#include "meter_reader.h"
#include <iostream>
#include <string>
#include <vector>

#include <opencv2/core/cvstd.hpp>
#include <opencv2/core/types.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/opencv.hpp>

void on_HoughCircles(int, void*);
void lineFit();

// Define Variables
cv::Mat                src_img, mid_img, gray_img, gray_img_copy, gray_origin_img, bin_img, dst_img, canny_img;
std::vector<cv::Vec3f> pcircles;
int                    g_houghValue    = 40;
int                    g_houghValueMax = 100;

int gray_center_x = 0;
int gray_center_y = 0;

// Define File Path
std::string src_image_path = "../img/8.jpg";

int main()
{
    MeterReader meter_reader(true);
    meter_reader.readImage(src_image_path);
    meter_reader.findCenter();
    meter_reader.findPointer();
    meter_reader.stopDisplay();

    return 0;
}

void process()
{

    // Load the image from the given file name
    src_img = cv::imread(src_image_path, 1);

    // Check if image is loaded fine
    if (src_img.empty())
    {
        printf("Error opening image\n");
        return;
    }

    cv::namedWindow("Dst", cv::WINDOW_NORMAL);

    // copy the src image
    mid_img = src_img.clone();
    dst_img = src_img.clone();

    // gray
    cv::cvtColor(mid_img, gray_img, cv::COLOR_BGR2GRAY);

    cv::medianBlur(gray_img, gray_img, 3);

    cv::threshold(gray_img, bin_img, 130, 255, cv::THRESH_BINARY);

    cv::threshold(gray_img, gray_img, 130, 255, cv::THRESH_TOZERO);
    gray_origin_img = gray_img.clone();
    gray_center_x   = gray_img.size().height * 0.4;
    gray_center_y   = gray_img.size().width * 0.4;
    gray_img        = gray_img(cv::Range(gray_center_x, gray_img.size().height * 0.7),
                        cv::Range(gray_center_y, gray_img.size().width * 0.7));
    gray_img_copy   = gray_img;

    cv::createTrackbar("参数值", "Dst", &g_houghValue, g_houghValueMax, on_HoughCircles);

    // cv::HoughCircles(gray_img, pcircles, cv::HOUGH_GRADIENT, 1, 50, 80, 50, g_houghValue, g_houghValueMax);
    cv::HoughCircles(gray_img_copy, pcircles, cv::HOUGH_GRADIENT, 1, 10, 80, g_houghValue, 2, 200);
    // cv::HoughCircles(gray_img, pcircles, cv::HOUGH_GRADIENT, 1, 50, 80, 80, 50, 200);
    for (auto cc : pcircles)
    {
        // 画圆
        cv::circle(dst_img, cv::Point(cc[0], cc[1]), cc[2], cv::Scalar(0, 0, 255), 1, cv::LINE_AA);
        // 画圆心
        cv::circle(dst_img, cv::Point(cc[0], cc[1]), 2, cv::Scalar(127, 255, 127), 1, cv::LINE_AA);
    }

    cv::imshow("Dst", dst_img);

    cv::namedWindow("Gray", cv::WINDOW_NORMAL);
    cv::imshow("Gray", gray_img);

    // stop
    cv::waitKey(0);
    cv::destroyAllWindows();
}

void on_HoughCircles(int, void*)
{
    dst_img = src_img.clone();
    cv::HoughCircles(gray_img_copy, pcircles, cv::HOUGH_GRADIENT, 1, 10, 80, g_houghValue, 2, 200);
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