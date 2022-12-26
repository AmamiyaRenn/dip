#pragma once

#include <cassert>
#include <iostream>

#include <math.h>
#include <opencv2/core/hal/interface.h>
#include <opencv2/core/mat.hpp>
#include <opencv2/core/types.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <string>

class MeterReader
{
public:
    explicit MeterReader(float bias, float range) : bias(bias), range(range) {}

    // 读单张图
    void readMeter(std::string path)
    {
        this->display = true;

        readImage(path);
        findCenter();
        findCircle();
        findPointer();
        calAngle();
    }

    // 批处理数据
    void readMeterBatch(std::string path, int batch)
    {
        display = false, this->batch = batch;

        for (int i = 1; i <= batch; i++)
        {
            std::string name      = std::to_string(i);
            std::string full_path = path + name + ".jpg";
            readImage(full_path);
            assert(findCenter());
            assert(findCircle());
            // assert(findPointer());
            findPointer();
            calAngle();
            writeImage(path + "result/", name);
            resetImage();
        }
    }

    void resetImage()
    {
        src_img       = cv::Mat(); // 原始图像
        gray_img      = cv::Mat(); // 灰度图像
        gray_cut_img  = cv::Mat(); // 灰度图像
        bin_img       = cv::Mat(); // 灰度二值化后的图像
        gray_open_img = cv::Mat(); // 开操作后的灰度图
        canny_img     = cv::Mat(); // 经过canny边缘检测后的图像
        show_img      = cv::Mat(); // 用于=cv::Mat();展示的图像

        center        = cv::Point(); // 表盘中心
        intersect     = cv::Point();
        center_radius = 0;
        circle_radius = 0;
    }

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
            show_img = src_img.clone();
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
        }

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
            cv::HoughCircles(canny_img, pcircles, cv::HOUGH_GRADIENT, 1, 10, 80, param2, 2, 500);
            for (auto cc : pcircles)
            {
                center.x += cc[0];
                center.y += cc[1];
                center_radius = cc[2];

                // 画圆
                cv::circle(show_img, center, center_radius, cv::Scalar(0, 0, 255), 1, cv::LINE_AA);
                // 画圆心
                cv::circle(show_img, center, 2, cv::Scalar(255, 0, 0), 2, cv::LINE_AA);
                if (display)
                {
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
            cv::HoughCircles(canny_img, pcircles, cv::HOUGH_GRADIENT, 1, 10, 80, param2, 100, 500);
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

                // 画圆
                cv::circle(show_img, ccenter, circle_radius, cv::Scalar(0, 0, 255), 1, cv::LINE_AA);
                // 画圆心
                cv::circle(show_img, ccenter, 2, cv::Scalar(255, 0, 0), 2, cv::LINE_AA);
                if (display)
                {
                    cv::imshow("binary", bin_img);
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
        intersect        = center;
        static bool flag = false;
        for (int threshold2 = 200; threshold2 >= 45; threshold2--) // 90 45
        {
            cv::Canny(bin_img, canny_img, 100, 200, 3);
            if (display)
                cv::imshow("canny", canny_img);

            std::vector<cv::Vec4i> lines;
            // 统计霍夫变换得到指针线
            cv::HoughLinesP(canny_img, lines, 1., CV_PI / 180, threshold2, 70, 30);

            // std::cout << "lines.size: " << lines.size() << "\n";
            if (lines.size() == 1 && !flag)
            {
                auto      line = lines[0];
                cv::Point p1(line[0], line[1]);
                cv::Point p2(line[2], line[3]);
                intersect = l2length(p1, center) > l2length(p2, center) ? p1 : p2;

                // std::cout << "1:" << intersect << "\n";
                flag = true;
            }
            if (lines.size() == 2)
            {
                cv::Point cross_point = getCrossPoint(lines[0], lines[1]);
                float     l2len       = l2length(cross_point, center);
                if (l2len > 2 * center_radius && l2len < 1.2 * circle_radius)
                {
                    intersect = intersect == center                                         ? cross_point :
                                l2length(intersect, center) < l2length(cross_point, center) ? intersect :
                                                                                              cross_point;
                }
                // std::cout << cross_point << "\n";
                if (intersect == center)
                    continue;

                // 画交点
                cv::circle(show_img, intersect, 2, cv::Scalar(255, 0, 0), 2, cv::LINE_AA);

                // 画指针线
                cv::line(show_img, intersect, center, cv::Scalar(0, 255, 0), 2, cv::LINE_AA);
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
                    cv::imshow("output", show_img);
                }
                flag = false;
                return true;
            }
        }

        // 画交点
        cv::circle(show_img, intersect, 2, cv::Scalar(255, 0, 0), 2, cv::LINE_AA);
        // 画指针线
        cv::line(show_img, intersect, center, cv::Scalar(0, 255, 0), 2, cv::LINE_AA);
        if (display)
        {
            cv::imshow("output", show_img);
        }
        if (!flag)
            std::cout << "failed:" << intersect << "\n";
        flag = false;

        return false;
    }

    // 计算结果并显示到左上角
    void calAngle() const
    {
        cv::Vec4d line1(center.x, center.y, intersect.x, intersect.y);
        cv::Vec4d line2(center.x, center.y, center.x, center.y + 10);
        auto      lines_intersect = linesIntersection(line1, line2);
        float     angle           = intersect.x < center.x ? lines_intersect[3] : 360 - lines_intersect[3];
        float     result          = (angle - bias) / 360.f * range;
        cv::putText(show_img,
                    "result: " + std::to_string(result),
                    cv ::Point(10, 20),
                    cv::FONT_HERSHEY_PLAIN,
                    1,
                    cv::Scalar(0, 0, 0));
        if (display)
            cv::imshow("output", show_img);
        std::cout << line1 << " " << angle << " " << result << "\n";
    }

    void displayImage() const
    {
        if (display)
        {
            while (static_cast<char>(cv::waitKey(1)) != 'q')
                ;
            cv::destroyAllWindows();
        }
    }

    void writeImage(std::string path, std::string name)
    {
        std::string filename = path + name + ".png";
        std::cout << filename << "\n";
        cv::imwrite(filename, show_img);
    }

    cv::Mat getSrcImg() const { return src_img; }
    cv::Mat getGrayOpenImg() const { return gray_open_img; }
    cv::Mat getBinImg() const { return bin_img; }

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

    // 判断两条线是否相交，若相交则求出交点和夹角(deg)
    static cv::Vec4d linesIntersection(const cv::Vec4d l1, const cv::Vec4d l2)
    {
        double x1 = l1[0], y1 = l1[1], x2 = l1[2], y2 = l1[3];
        double a1 = -(y2 - y1), b1 = x2 - x1, c1 = (y2 - y1) * x1 - (x2 - x1) * y1; // 一般式：a1x+b1y1+c1=0
        double x3 = l2[0], y3 = l2[1], x4 = l2[2], y4 = l2[3];
        double a2 = -(y4 - y3), b2 = x4 - x3, c2 = (y4 - y3) * x3 - (x4 - x3) * y3; // 一般式：a2x+b2y1+c2=0
        bool   r  = false;                                                          // 判断结果
        double x0 = 0, y0 = 0;                                                      // 交点
        double angle = 0;                                                           // 夹角
        // 判断相交
        if (b1 == 0 && b2 != 0) // l1垂直于x轴，l2倾斜于x轴
            r = true;
        else if (b1 != 0 && b2 == 0) // l1倾斜于x轴，l2垂直于x轴
            r = true;
        else if (b1 != 0 && b2 != 0 && a1 / b1 != a2 / b2)
            r = true;
        if (r)
        {
            //计算交点
            x0 = (b1 * c2 - b2 * c1) / (a1 * b2 - a2 * b1);
            y0 = (a1 * c2 - a2 * c1) / (a2 * b1 - a1 * b2);
            // 计算夹角
            double a = sqrt(pow(x4 - x2, 2) + pow(y4 - y2, 2));
            double b = sqrt(pow(x4 - x0, 2) + pow(y4 - y0, 2));
            double c = sqrt(pow(x2 - x0, 2) + pow(y2 - y0, 2));
            angle    = acos((b * b + c * c - a * a) / (2 * b * c)) * 180 / CV_PI;
        }
        return cv::Vec4d(r, x0, y0, angle);
    }

private:
    bool display; // 是否展示
    int  batch;   // 进行批处理，0代表不进行批处理，大于0的话就从1.jpg处理到n.jpg

    cv::Mat src_img;       // 原始图像
    cv::Mat gray_img;      // 灰度图像
    cv::Mat gray_cut_img;  // 灰度图像
    cv::Mat bin_img;       // 灰度二值化后的图像
    cv::Mat gray_open_img; // 开操作后的灰度图
    cv::Mat canny_img;     // 经过canny边缘检测后的图像
    cv::Mat show_img;      // 用于展示的图像

    cv::Point center; // 表盘中心
    cv::Point intersect;
    int       center_radius;
    int       circle_radius;

    float bias;  // 0刻度偏置角度（deg）
    float range; // 量程
};
