#pragma once

#include <iostream>

#include <math.h>
#include <opencv2/core/hal/interface.h>
#include <opencv2/core/mat.hpp>
#include <opencv2/core/types.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

class MeterReader
{
public:
    explicit MeterReader(bool display = false) : display(display) {}

    // 读入图片，并进行预处理
    bool readImage(std::string path)
    {
        if (path.find(".jpg") != -1)
        { // 读jpg的情况
            src_img = cv::imread(path, 1);
            if (src_img.empty())
            {
                std::cerr << "Error reading image from" << path << std::endl;
                return false;
            }
            show_img = src_img;
            // get gray image
            cv::cvtColor(src_img, gray_img, cv::COLOR_BGR2GRAY);
            // median blur
            cv::medianBlur(gray_img, gray_img, 3);
            // cv::GaussianBlur(gray_img, gray_img, cv::Size(3, 3), 0);

            int     kernel_size = 3;
            cv::Mat kernel      = getStructuringElement(cv::MORPH_RECT, cv::Size(kernel_size, kernel_size));

            // erode and dilate(开运算)
            cv::morphologyEx(gray_img, gray_open_img, cv::MORPH_OPEN, kernel);

            cv::threshold(gray_open_img, bin_img, 115, 255, cv::THRESH_BINARY);
            cv::medianBlur(bin_img, bin_img, 3);
            if (display)
                cv::imshow("open", gray_open_img);

            // get cutted gray image
            gray_cut_img = gray_img(cv::Range(center.y = gray_img.size().height * 0.38, gray_img.size().height * 0.63),
                                    cv::Range(center.x = gray_img.size().width * 0.38, gray_img.size().width * 0.63));
        } // TODO: read folder

        return true;
    }

    // 找到表盘中心，通过获得拟合中心圆的方法
    bool findCenter()
    {
        cv::Canny(gray_cut_img, canny_img, 100, 200, 3);
        // if (display)
        // {
        // cv::imshow("gray_cut_img", gray_cut_img);
        // }

        for (int param2 = 60; param2 >= 20; param2--)
        {
            std::vector<cv::Vec3f> pcircles;
            cv::HoughCircles(canny_img, pcircles, cv::HOUGH_GRADIENT, 1, 10, 80, param2, 2, 200);
            for (auto cc : pcircles)
            {
                center.x += cc[0];
                center.y += cc[1];
                center_radius = cc[2];

                if (display)
                {
                    // 画圆
                    cv::circle(show_img, center, center_radius, cv::Scalar(0, 0, 255), 1, cv::LINE_AA);
                    // 画圆心
                    cv::circle(show_img, center, 2, cv::Scalar(255, 0, 0), 2, cv::LINE_AA);
                    cv::imshow("output", show_img);
                }

                return true;
            }
        }
        return false;
    }

    // 找到表盘外围圆
    bool findCircle()
    {
        cv::Canny(gray_open_img, canny_img, 100, 200, 3);
        for (int param2 = 80; param2 >= 20; param2--)
        {
            std::vector<cv::Vec3f> pcircles;
            cv::HoughCircles(canny_img, pcircles, cv::HOUGH_GRADIENT, 1, 10, 80, param2, 100, 200);
            for (auto cc : pcircles)
            {
                cv::Point ccenter = cv::Point(cc[0], cc[1]);
                circle_radius     = cc[2];

                // 把圆外的像素设为255
                for (int i = 0; i < bin_img.rows; i++)
                {
                    uchar* p_src_data = bin_img.ptr<uchar>(i);
                    for (int j = 0; j < bin_img.cols; j++)
                        if (pow(i - ccenter.y, 2) + pow(j - ccenter.x, 2) >= pow(cc[2], 2))
                            p_src_data[j] = 255;
                }
                if (display)
                    cv::imshow("binary", bin_img);

                if (display)
                {
                    // 画圆
                    cv::circle(show_img, ccenter, circle_radius, cv::Scalar(0, 0, 255), 1, cv::LINE_AA);
                    // 画圆心
                    cv::circle(show_img, ccenter, 2, cv::Scalar(255, 0, 0), 2, cv::LINE_AA);
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
        cv::Point intersect = center;
        for (int threshold2 = 90; threshold2 >= 45; threshold2--)
        {
            cv::Canny(bin_img, canny_img, 100, 200, 3);
            if (display)
                cv::imshow("canny", canny_img);

            std::vector<cv::Vec4i> lines;
            // 统计霍夫变换得到指针线
            cv::HoughLinesP(canny_img, lines, 1., CV_PI / 180, threshold2, 70, 30);

            static bool flag = false;
            if (lines.size() == 1 && !flag)
            {
                auto      line = lines[0];
                cv::Point p1(line[0], line[1]);
                cv::Point p2(line[2], line[3]);
                intersect = l2length(p1, center) > l2length(p2, center) ? p1 : p2;

                flag = true;
            }
            if (lines.size() == 2)
            {
                cv::Point cross_point = getCrossPoint(lines[0], lines[1]);
                float     l2len       = l2length(cross_point, center);
                if (l2len > 2 * center_radius && l2len < circle_radius)
                    intersect = l2length(intersect, center) < l2length(cross_point, center) ? intersect : cross_point;

                if (display)
                {
                    // 画霍夫线
                    for (auto line : lines)
                        cv::line(show_img,
                                 cv::Point(line[0], line[1]),
                                 cv::Point(line[2], line[3]),
                                 cv::Scalar(0, 0, 255),
                                 1,
                                 cv::LINE_AA);
                    // 画交点
                    cv::circle(show_img, intersect, 2, cv::Scalar(255, 0, 0), 2, cv::LINE_AA);

                    // 画指针线
                    cv::line(show_img, intersect, center, cv::Scalar(0, 255, 0), 2, cv::LINE_AA);
                    cv::imshow("output", show_img);
                }
                return true;
            }
        }

        if (display)
        {
            // 画交点
            cv::circle(show_img, intersect, 2, cv::Scalar(255, 0, 0), 2, cv::LINE_AA);

            // 画指针线
            cv::line(show_img, intersect, center, cv::Scalar(0, 255, 0), 2, cv::LINE_AA);
            cv::imshow("output", show_img);
        }

        return false;
    }

    void displayImage()
    {
        if (display)
        {
            while (static_cast<char>(cv::waitKey(1)) != 'q')
                ;
            cv::destroyAllWindows();
        }
    }

    cv::Mat getSrcImg() const { return src_img; }
    cv::Mat getGrayOpenImg() const { return gray_open_img; }

    // 求两条直线交点
    // http://t.csdn.cn/aGB2z
    static cv::Point2f getCrossPoint(cv::Vec4i LineA, cv::Vec4i LineB)
    {
        double ka, kb;
        ka = static_cast<double>(LineA[3] - LineA[1]) / static_cast<double>(LineA[2] - LineA[0]); //求出LineA斜率
        kb = static_cast<double>(LineB[3] - LineB[1]) / static_cast<double>(LineB[2] - LineB[0]); //求出LineB斜率

        cv::Point2f cross_point;
        cross_point.x = (ka * LineA[0] - LineA[1] - kb * LineB[0] + LineB[1]) / (ka - kb);
        cross_point.y = (ka * kb * (LineA[0] - LineB[0]) + ka * LineB[1] - kb * LineA[1]) / (ka - kb);
        return cross_point;
    }

    static float l2length(cv::Point p1, cv::Point p2) { return sqrt(pow(p1.x - p2.x, 2) + pow(p1.y - p2.y, 2)); }

    /***** 点到直线的距离:P到AB的距离*****/
    // P为线外一点，AB为线段两个端点
    static float getDistP2L(cv::Point pointP, cv::Point pointA, cv::Point pointB)
    {
        // 求直线方程
        int a = 0, b = 0, c = 0;
        a = pointA.y - pointB.y;
        b = pointB.x - pointA.x;
        c = pointA.x * pointB.y - pointA.y * pointB.x;
        // 代入点到直线距离公式
        return (static_cast<float>(abs(a * pointP.x + b * pointP.y + c))) / (sqrt(a * a + b * b));
    }

private:
    bool display; // 是否展示

    cv::Mat src_img;       // 原始图像
    cv::Mat gray_img;      // 灰度图像
    cv::Mat gray_cut_img;  // 灰度图像
    cv::Mat bin_img;       // 灰度二值化后的图像
    cv::Mat gray_open_img; // 开操作后的灰度图
    cv::Mat canny_img;     // 经过canny边缘检测后的图像
    cv::Mat show_img;      // 用于展示的图像

    cv::Point center; // 表盘中心
    int       center_radius;
    int       circle_radius;
};
