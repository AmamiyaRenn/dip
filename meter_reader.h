#pragma once

#include <iostream>

#include <opencv2/core/mat.hpp>
#include <opencv2/core/types.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

class MeterReader
{
public:
    explicit MeterReader(bool display = false) : display(display)
    {
        if (display)
        {
            cv::namedWindow("output", cv::WINDOW_NORMAL);
            cv::namedWindow("canny", cv::WINDOW_NORMAL);
        }
    }

    bool readImage(std::string path)
    {
        if (path.find(".jpg") != -1)
        { // 读jpg的情况
            src_img = cv::imread(path, 1);
            if (src_img.empty())
            {
                std::cerr << "Error reading image " << path << std::endl;
                return false;
            }
            show_img = src_img;
            // get gray
            cv::cvtColor(src_img, gray_img, cv::COLOR_BGR2GRAY);
            // median blur
            cv::medianBlur(gray_img, gray_img, 3);

            // erode and dilate
            int     kernel_size = 5;
            cv::Mat kernel      = getStructuringElement(cv::MORPH_RECT, cv::Size(kernel_size, kernel_size));
            cv::erode(gray_img, canny_img, kernel);
            cv::dilate(canny_img, canny_img, kernel);
            cv::imshow("erode and dilate", canny_img);

        } // TODO: read folder

        return true;
    }

    bool findCenter()
    {
        cv::Canny(gray_img, canny_img, 100, 200, 3);
        for (int param2 = 80; param2 >= 40; param2--)
        {
            std::vector<cv::Vec3f> pcircles;
            cv::HoughCircles(canny_img, pcircles, cv::HOUGH_GRADIENT, 1, 10, 80, param2, 50, 200);
            for (auto cc : pcircles)
            {
                center.x = cc[0];
                center.y = cc[1];

                if (display)
                {
                    // 画圆
                    cv::circle(show_img, center, cc[2], cv::Scalar(0, 0, 255), 1, cv::LINE_AA);
                    // 画圆心
                    cv::circle(show_img, center, 2, cv::Scalar(127, 255, 127), 1, cv::LINE_AA);
                    cv::imshow("output", show_img);
                }

                return true;
            }
        }
        return false;
    }

    // 获得指针线
    bool findPointer()
    {
        cv::Canny(gray_img, canny_img, 100, 200, 3);
        if (display)
        {
            cv::imshow("canny", canny_img);
        }
        std::vector<cv::Vec4i> lines;
        // 统计霍夫变换得到指针线
        cv::HoughLinesP(canny_img, lines, 1., CV_PI / 180, 100, 100, 10);

        if (lines.empty())
            return false;
        if (display)
        {
            for (auto line : lines)
                cv::line(show_img,
                         cv::Point(line[0], line[1]),
                         cv::Point(line[2], line[3]),
                         cv::Scalar(0, 0, 255),
                         2,
                         cv::LINE_AA);
            cv::imshow("output", show_img);
        }

        return true;
    }

    void stopDisplay()
    {
        while (static_cast<char>(cv::waitKey(1)) != 'q')
            ;
        cv::destroyAllWindows();
    }

private:
    bool      display;   // 是否展示
    cv::Mat   src_img;   // 原始图像
    cv::Mat   show_img;  // 用于展示的图像
    cv::Mat   gray_img;  // 灰度图像
    cv::Mat   canny_img; // 经过canny边缘检测后的图像
    cv::Point center;
};
