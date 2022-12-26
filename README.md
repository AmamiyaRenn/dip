#! https://zhuanlan.zhihu.com/p/594519915
![Image](https://w.wallhaven.cc/full/8o/wallhaven-8ogvq1.jpg)

# Opencv 实现刻度盘读数（C++版本）

这是武汉大学自动化专业图像处理专选课的结课作业，任务是实现一个软件能自动批量识别 25 张 2.5 级和另外两张 0.16 与 0.25 级精度指针式仪表的读数，并将识别结果数值用程序写在图片的左上角区域

工程代码在[此处](https://github.com/AmamiyaRenn/dip)

整体采用暴力思路，调库+调参，没什么特别的

## 环境配置

### 运行环境

使用 Manjaro21 作为操作系统，主机为[联想拯救者 Y7000p-2020](https://item.lenovo.com.cn/product/1007685.html)，主体代码为 C++17，使用 CMake 与 Makefile 进行构建，编译器为 gcc12，使用 VSCode 作为编辑器，shell 为 zsh

关于 Manjaro 的安装，不在讨论的范围内；当然，也可以用 windows 或 mac。

### C++环境配置

本次使用 C++作为编程语言，C++具有多范式编程、运行速度快、支持嵌入式开发等等特性，非常适合用在对运行时性能有要求的环境中

同时 C++拥有众多的库，本次就会使用 Opencv 来进行机器视觉的操作
本次运行环境配置仅需要参考[这篇文章](http://t.csdn.cn/UauA3)

在 code 文件夹下加入 src 文件夹，并加入 main.cpp，向其中写入

```c++
#include <iostream>
int main()
{
    std::cout << "Hello world!" << std::endl;
    return 0;
}
```

运行程序，可以看到终端输出 Hello world!

### Opencv 配置

要在 linux 系统中安装 Opencv，只参考官方教程就行了，具体来说就是[这个网站](https://docs.opencv.org/4.x/d7/d9f/tutorial_linux_install.html)

我下载的是 [4.6.0 版](https://sourceforge.net/projects/opencvlibrary/files/4.6.0/)

在安装前，一定要先安装依赖项，不然会导致程序闪退报错等等问题；关于依赖项的安装，可以参考这篇文章: [在 ubuntu20.04 下安装 opencv4(C++版)](http://t.csdn.cn/m6dgn)

### 一个简单的测试

一切准备就绪后，测试一下环境吧，具体来说，可以试一下 [tutorial](https://docs.opencv.org/4.x/db/df5/tutorial_linux_gcc_cmake.html) 中提供的例子，如果没什么问题的话，应该会显示你提供的图片

## 方法与代码

### 思路

先分析一下需要被识别的数据集的信息，可以发现这些图片的特征就是仪表盘几乎占满或溢出图片，所以可以通过霍夫圆消除仪表盘外数据的干扰

![原图](https://pic4.zhimg.com/80/v2-ef6d659eaae4e87eb6106af868e2554f.jpg)

图片的另一个特征就是表盘并不是正着摆放的，基本上都会有所倾斜，并且倾斜的角度基本相同，所以可以提前测得 0 刻度偏置角度（deg）bias，然后量程 range 也可以提前获得

既然能获得这些信息，那么只需要获得指针角度就能计算出结果了，公式为`result  = (angle - bias) / 360.f * range;`

接下来的问题就是指针角度如何获得。指针由两点——中心点与指针针尖——连线构成，其中中心可以通过检测表盘外侧霍夫圆得到，不过这个实践证明精度不够，因为光线明暗方向的不同导致最后 canny 边缘检测得到的图像已经歪了所以我走了另一条道路——霍夫圆识别指针中心的小圆，实践证明这个方法是正确的

随后就是指针针尖如何获得的问题了，第一个想法就是角点检测，然后通过指针长度来筛选角点，但实践证明很困难，因为表盘刻度也是角点，而且几乎和指针针尖重合
网上主流的想法是统计霍夫线，我也是采用这个想法，通过调参即可直接得到线，难点是预处理；我采用识别两条霍夫线（指针两边）取交点来得到比较准确的针尖点
两点获得后即可获得指针线，再与中心点向下的一条基准线算个角度，然后套上文的 result 公式即可得到答案

### 预处理

工作主要集中在灰度图上，所以线对原图 src_img 进行灰度转换
cv::cvtColor(src_img, gray_img, cv::COLOR_BGR2GRAY);
即可得到灰度图 gray_img

![灰度图](https://pic4.zhimg.com/80/v2-750bc86c97b93ccd4b7d121b20036b3d.png)

一般原灰度图是不好做各种图像操作的，要先经过滤波，我这里采用了中值滤波，范围为 3

```c++
cv::medianBlur(gray_img, gray_img, 3);
```

![中值滤波后的灰度图](https://pic4.zhimg.com/80/v2-777220b6c40c832d46a7082643dca2e8.png)

接下来对处理过的图像做开运算，即先腐蚀后膨胀的运算，开运算会增强灰度图对比度，这为后面识别指针带来方便

```c++
int kernel_size = 3;
cv::Mat kernel = getStructuringElement(cv::MORPH_RECT, cv::Size(kernel_size, kernel_size));
cv::morphologyEx(gray_img, gray_open_img, cv::MORPH_OPEN, kernel);
```

![开运算后的灰度图](https://pic4.zhimg.com/80/v2-e974b31e28df0e4754d09c85b9429ed9.png)

开运算后就进行二值化，此时二值化已经能得到一个比较漂亮的结果了

```c++
cv::threshold(gray_open_img, bin_img, 115, 255, cv::THRESH_BINARY);
```

![二值化后的开运算图](https://pic4.zhimg.com/80/v2-6fc9e98d164970ca70ec5ea330b4b6e3.png)

不过二值化后还得滤波一下才能作处理（边缘需要平滑），此处简单起见还是中值滤波

![滤波后的二值化图](https://pic4.zhimg.com/80/v2-faffc6a91c058818fc3b66d692223e4f.png)

额外的，为了检测中心小圆，还需要对灰度图进行一下裁剪，因为数据集中小圆位置相对稳定，所以可以直接裁剪中间

```c++
gray_cut_img=
gray_img(cv::Range(center.y=gray_img.size().height*0.38,gray_img.size().height*0.63),cv::Range(center.x=gray_img.size().width*0.38, gray_img.size().width * 0.63));
```

![裁剪后的灰度图](https://pic4.zhimg.com/80/v2-6b0275388dc15d3bdac5e537458d7c1a.png)

下面给出对应函数代码

```c++
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
```

### 寻找表盘中心

主体算法是使用霍夫圆，关于霍夫圆，可以参考[这篇文章](http://t.csdn.cn/OQ9Jz)

要进行霍夫变换，首先要进行边缘检测，此处使用 canny 算法，关于 canny 算法，可以参考[这篇文章](http://t.csdn.cn/XGSNM)

```c++
cv::Canny(gray_cut_img, canny_img, 100, 200, 3);
```

![Canny 边缘检测](https://pic4.zhimg.com/80/v2-029fe67c44aec5682fbb5915610a4287.png)

霍夫变换的核心就是这一句，不过我对 param2 也就是边缘检测阈值以外的两个阈值进行调整，此处我给的范围是 60->20，从高阈值到低阈值，选择第一个圆即可

```c++
std::vector<cv::Vec3f> pcircles;
cv::HoughCircles(canny_img, pcircles, cv::HOUGH_GRADIENT, 1, 10, 80, param2, 2, 500);
```

![霍夫圆检测](https://pic4.zhimg.com/80/v2-73f9a60f3edd02c2a372a164700f6506.png)

对应函数代码

```c++
// 找到表盘中心，通过获得拟合中心圆的方法
bool findCenter()
{
    cv::Canny(gray_cut_img, canny_img, 100, 200, 3);
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
                cv::imshow("output", show_img);

            return true;
        }
    }
    return false;
}
```

### 寻找表盘外围圆

这里思路与上一小节的思路基本一致，不过为了后面霍夫线识别指针线方便，顺便把大圆外的像素都归一了（改成了 255）

此处的细节是我使用了开运算图像作为输入以获得更好的边缘检测结果

![Canny 边缘检测](https://pic4.zhimg.com/80/v2-9e81dfa5da9d2bbdf837bd6910a2687e.png)

随后也是霍夫变换，不过最小圆半径 minRadius 改为了 100 以适应新情况，还有就是 param2 范围改为了 80-20，因为大圆特征更明显

![霍夫圆检测的对比](https://pic4.zhimg.com/80/v2-27b7782c8b3ec0ea9cc8597871be06da.png)

从上图可以明显发现如果直接简单的选择外围圆的效果其实不如选中间的圆（但更大的原因是没怎么调参吧）

![归一化圆外像素后的二值图](https://pic4.zhimg.com/80/v2-9a6030c7189de5b52c600db771489314.png)

对应函数代码

```c++
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
```

### 获得指针线

还是霍夫变换，不过这次使用概率霍夫线，关于概率霍夫线，可以参考[这篇文章]9 http://t.csdn.cn/OYXNt)

```c++
std::vector<cv::Vec4i> lines;
cv::HoughLinesP(canny*img, lines, 1., CV_PI / 180, threshold2, 70, 30);
```

注意最小线长 minLineLength 不能太长，因为图片分辨率太低

此处我依然是调节了阈值 threshold，选用的参数为 200->45(其实只需要 90->45 即可，不过对于分辨率更高更清晰的仪表就需要 200 了)

从 200 到 45 的过程会出现越来越多的线，我的算法会把重点放在处理两条线的情况，取两条线交点 cross_point 为指针针尖，但如果指针长度 l2len 太短或太长——这些都是有问题的情况，前者是有线几乎和指针边缘线垂直了，后者是错乱了，则不更新针尖点 intersect；如果是正常的线，则越长越好，越长说明拟合的点越靠近表盘边缘，越可能是指针点

```c++
cv::Point cross_point = getCrossPoint(lines[0], lines[1]);
float     l2len       = l2length(cross_point, center);
if (l2len > 2 * center_radius && l2len < 1.2 * circle_radius)
{
    intersect = intersect == center                                         ? cross_point :
                l2length(intersect, center) < l2length(cross_point, center) ? intersect :
                                                                              cross_point;
}
if (intersect == center)
    continue;
```

其中选择线的远端（远离中心的点）作为 intersect 即可
霍夫变换前要边缘检测，这里我输入的是之前二值化并经过滤波的图片

![边缘检测二值化并归一化多余像素后的图](https://pic4.zhimg.com/80/v2-59262f165e53bcdc604d30e381d9ae25.png)

从上图可以看出，前面做的预处理还是非常有用的

![指针线](https://pic4.zhimg.com/80/v2-dad29199f9d664ddf1cb65a6fc209655.png)

对应代码

```c++
// 获得指针线
bool findPointer()
{
    intersect        = center;
    static bool flag = false;
    cv::Canny(bin_img, canny_img, 100, 200, 3);

    for (int threshold2 = 200; threshold2 >= 45; threshold2--) // 90 45
    {
        if (display)
            cv::imshow("canny", canny_img);

        std::vector<cv::Vec4i> lines;
        // 统计霍夫变换得到指针线
        cv::HoughLinesP(canny_img, lines, 1., CV_PI / 180, threshold2, 70, 30);

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
            if (l2len > 2 * center_radius && l2len < 1.2 * circle_radius)
            {
                intersect = intersect == center                                         ? cross_point :
                            l2length(intersect, center) < l2length(cross_point, center) ? intersect :
                                                                                          cross_point;
            }
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
        cv::imshow("output", show_img);
    if (!flag)
        std::cout << "failed:" << intersect << "\n";
    flag = false;

    return false;
}
```

### 计算结果并写入结果到图片

在思路那节介绍过此处的线夹角思路了，此处不多赘述，直接给出代码

```c++
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
```

![最终结果](https://pic4.zhimg.com/80/v2-1a0f5de2589910029dbe83e924d76217.png)

### 关于调参

调参当然是用 trackerbar，关于 trackerbar，可以参考[这篇文章](http://t.csdn.cn/q28EZ)，下面我给出我代码的中使用的实例

```c++
int tracker_max = 200;
int tracker_val = 200;

cv::createTrackbar("参数值", "tracker", &tracker_val, tracker_max, on_threshold);

void on_threshold(int, void*)
{
    cv::Mat show_img = src_img.clone();
    cv::threshold(src_img, gray_img, tracker_val, 255, cv::THRESH_BINARY);
    cv::imshow("tracker", gray_img);
}
```
