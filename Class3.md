# RoboMaster视觉组第三次培训：装甲板的识别

## 一、课程概述

本节课将带大家深入了解赛场上随处可见的结构装甲板，对于视觉组的成员来说，在赛前调试自瞄，在赛场对敌方机器人、前哨站、基地装甲板实现精准打击，是职责所在，技术点所在，同时也是难点所在，而装甲板识别，正是突破这些困难的重要一环。话不多说，让我们来进入今天的课程！

## <img src="./media/Class3/armor_2.jpg" alt="armor_2" style="zoom:15%;" />

## 二、OpenCV概述

## <img src="./media/Class3/OpenCV_logo-1789441510325-2.png" alt="OpenCV_logo" style="zoom:25%;" />

### 1.安装OpenCV

OpenCV 是一个开源的跨平台计算机视觉库，提供了图像处理、几何变换、特征提取、相机标定、机器学习等上千个 API，是 RoboMaster 视觉组日常开发的基础工具。

[OpenCV官网](https://opencv.org/)

Ubuntu 下通过 apt 安装：

```bash
sudo apt update
sudo apt install libopencv-dev             # OpenCV系统包
apt show libopencv-dev                     # 查看版本
```



CMake 工程里链接 OpenCV 只需：

[OpenCV4.5.4](https://docs.opencv.org/4.5.4/df/d65/tutorial_table_of_content_introduction.html)

```cmake
cmake_minimum_required(VERSION 2.8)
project( DisplayImage )
find_package( OpenCV REQUIRED )
include_directories( ${OpenCV_INCLUDE_DIRS} )
add_executable( DisplayImage DisplayImage.cpp )
target_link_libraries( DisplayImage ${OpenCV_LIBS} )
```

对于OpenCV的报错：

```bash
mkdir -p ~/.config/clangd
nano ~/.config/clangd/config.yaml
```

```bash
CompileFlags:
  Compiler: /usr/bin/g++
  Add:
    - -std=c++17
    - -I/usr/include/opencv4
    - -isystem
    - /usr/include/c++/11
    - -isystem
    - /usr/include/x86_64-linux-gnu/c++/11

InlayHints:
  Enabled: No
```

> 💡 **课堂验证**：装完后写一个 5 行的"Hello OpenCV"确认环境通畅：
>
> ```cpp
> #include <opencv2/opencv.hpp>
> int main() {
>   cv::Mat img(200, 200, CV_8UC3, cv::Scalar(0, 0, 255));   // 红色画布
>   cv::imshow("hello", img);
>   cv::waitKey(0);
> }
> ```
> 编译运行 `g++ hello.cpp -o hello `pkg-config --cflags --libs opencv4` && ./hello`，弹出红窗即成功。

### 2.色彩空间（Color Space）

色彩空间是像素值的"解释方式"，同一个像素在不同空间里含义完全不同。装甲板识别里我们至少要熟悉下面几种：

| 空间 | 通道 | 典型用途 |
|------|------|----------|
| **BGR** | B、G、R（OpenCV 默认顺序！） | 读写文件、`imshow` 显示 |
| **RGB** | R、G、B | 与大多数深度学习模型对接（注意顺序） |
| **Gray** | 单通道亮度 | 二值化、边缘检测、轮廓查找的前置步骤 |
| **HSV** | H（色相）、S（饱和度）、V（亮度） | 按颜色分割，对光照变化更鲁棒 |
| **Binary** | 0 / 255 | 形态学、轮廓提取 |

> ⚠️ OpenCV 默认通道顺序是 **BGR** 而非 RGB，这是初学者最常踩的坑。`imread` 读出来就是 BGR，喂给 YOLO 等模型前通常要转成 RGB（本工程的 OpenVINO 预处理里就做了这一步）。

色彩空间之间用 `cv::cvtColor` 转换：

```cpp
cv::Mat bgr = cv::imread("a.jpg");
cv::Mat gray, hsv, binary;
cv::cvtColor(bgr, gray, cv::COLOR_BGR2GRAY);
cv::cvtColor(bgr, hsv,  cv::COLOR_BGR2HSV);
cv::threshold(gray, binary, 150, 255, cv::THRESH_BINARY);   // 灰度 → 二值
```

HSV 空间按颜色取阈值（以蓝色装甲板为例）：

```cpp
// 蓝色 H 大致在 100~124，S/V 拉宽以容忍光照变化
cv::Mat hsv, mask;
cv::cvtColor(bgr, hsv, cv::COLOR_BGR2HSV);
cv::inRange(hsv, cv::Scalar(100,  80,  80),
                  cv::Scalar(124, 255, 255), mask);
// mask 中白色像素即为"蓝色"区域
```

> 💡 **课堂互动**：把上面 `inRange` 的 H 区间改成红色对应的 `0~10` 和 `160~180`（红色横跨 0°），看能否把红方装甲板单独抠出来。

### <img src="./media/Class3/HSV_1-1789442641878-3.jpeg" alt="HSV_1" style="zoom:100%;" />               <img src="./media/Class3/HSV_2-1789442747971-5.jpeg" alt="HSV_2" style="zoom:67%;" />

### 3.核心数据结构：`cv::Mat`

`cv::Mat` OpenCV 中最核心的图像容器，封装了像素矩阵、尺寸、通道数、深度和内存管理。

```cpp
// 创建空图像
cv::Mat img1;
// 创建指定大小、3 通道、8 位无符号类型（BGR）的图像
cv::Mat img2(480, 640, CV_8UC3, Scalar(0,0,255));  // 红色全画布
// 从文件读取
cv::Mat img = imread("path/to/image.jpg", IMREAD_COLOR);

```

- 访问像素

  ```cpp
  Vec3b pixel = img.at<Vec3b>(row, col);
  pixel[0] = 255;  // B 通道
  img.at<Vec3b>(row, col) = pixel;
  ```

  `at<>` 方便但慢，遍历全图时推荐用行指针：

  ```cpp
  for (int r = 0; r < img.rows; ++r) {
    auto * ptr = img.ptr<Vec3b>(r);          // 第 r 行首地址
    for (int c = 0; c < img.cols; ++c) {
      ptr[c][0] = 255;                       // B 通道
    }
  }
  ```

- ROI 截取（Region of Interest）

  `cv::Mat` 的 `operator()` 截取子矩阵，**共享内存**，不复制像素，是装甲板识别里抠图案最常用的操作：

  ```cpp
  cv::Rect roi(100, 50, 200, 150);
  cv::Mat patch = img(roi);          // 浅拷贝，修改 patch 会改 img
  cv::Mat patch2 = img(roi).clone(); // 深拷贝，独立一份
  ```

  本工程 `Detector::get_pattern` 就是用 `bgr_img(roi)` 抠出数字图案交给分类器的。

- 浅拷贝 vs 深拷贝

  ```cpp
  Mat a = imread("a.jpg");
  Mat b = a;            // 浅拷贝：共享数据，改 b 即改 a
  Mat c = a.clone();    // 深拷贝：独立内存
  a.copyTo(d);          // 深拷贝到已存在的 d
  ```

- 通道分离与合并

  ```cpp
  vector<Mat> chs;
  split(bgr, chs);          // chs[0]=B, chs[1]=G, chs[2]=R
  Mat blue = chs[0];       // 只取蓝色通道
  merge(chs, bgr2);         // 重新合并
  ```

- 常用类型宏

  - `CV_8U` / `CV_8UC3`（8 位无符号，1/3 通道）——最常见，普通图像
  - `CV_32F` / `CV_64F`（浮点型）——滤波系数、PCA、神经网络输入
  - 类型 + 通道可拼成 `CV_8UC1`、`CV_32FC3` 等组合

### 4.常用模块与函数

1. 图像I/O与显示

   ```cpp
   Mat img = imread("lenna.png", IMREAD_GRAYSCALE);
   if(img.empty()) { /* 读取失败 */ }
   imshow("灰度图", img);
   waitKey(0);  // 等待按键
   
   ```

   - `imread`:读取图像；
   - `imwrite`：保存图像；
   - `imshow`：显示窗口；
   - `waitKey(int ms)`：等待指定毫秒或按键。

2. 图像处理

   - 阈值化与二值化（装甲板识别的关键前置步骤）

     ```cpp
     Mat gray, binary;
     cvtColor(bgr, gray, COLOR_BGR2GRAY);
     threshold(gray, binary, 150, 255, THRESH_BINARY);          // 固定阈值
     // adaptiveThreshold(gray, binary, 255, ADAPTIVE_THRESH_GAUSSIAN_C,
     //                   THRESH_BINARY, 11, 2);                // 自适应阈值，光照不均时用
     ```

     `threshold` 的两个阈值参数（150、255）正是 `configs/armor_detect.yaml` 里 `threshold: 150` 的来源。

   - 滤波与去噪

     ```cpp
     Mat blur_img, gauss_img, median_img;
     blur(bgr, blur_img, Size(5,5));                            // 均值滤波
     GaussianBlur(bgr, gauss_img, Size(5,5), 1.5);             // 高斯滤波，最常用
     medianBlur(bgr, median_img, 5);                           // 中值滤波，去椒盐噪声
     bilateralFilter(bgr, bi_img, 5, 50, 50);                   // 保边滤波
     ```

   - 边缘检测

     ```cpp
     Mat edges;
     Canny(gray, edges, 50, 150);
     ```

     

   - 形态学操作

     ```cpp
     Mat bin, dilated;
     threshold(gray, bin, 100, 255, THRESH_BINARY);
     Mat kernel = getStructuringElement(MORPH_RECT, Size(3,3));
     dilate(bin, dilated, kernel);
     ```

   - 轮廓查找与几何拟合（灯条提取的核心）

     ```cpp
     std::vector<std::vector<cv::Point>> contours;
     findContours(binary, contours, RETR_EXTERNAL, CHAIN_APPROX_NONE);

     for (const auto & c : contours) {
       double area = contourArea(c);
       if (area < 10) continue;                       // 过滤小噪点

       cv::RotatedRect rr = minAreaRect(c);           // 最小外接旋转矩形
       cv::Rect bbox = boundingRect(c);              // 正外接矩形
       double len = arcLength(c, true);              // 周长
       // approxPolyDP(c, approx, 3, true);          // 多边形逼近
     }
     ```

     `minAreaRect` 返回的 `RotatedRect` 含 `center`、`size`、`angle`，本工程正是用它拟合灯条并算出长宽比和角度。

   - 绘图函数（可视化调试必备）

     ```cpp
     rectangle(img, bbox, Scalar(0,255,0), 2);              // 画矩形框
     circle(img, Point(100,100), 5, Scalar(0,0,255), -1);  // 画实心圆
     line(img, p1, p2, Scalar(255,0,0), 2);                 // 画线
     putText(img, "armor", Point(10,30), FONT_HERSHEY_SIMPLEX,
             0.6, Scalar(255,255,255), 1);                  // 写文字
     std::vector<Point> poly = {...};
     polylines(img, poly, true, Scalar(0,255,255), 2);      // 画多边形
     drawContours(img, contours, -1, Scalar(0,255,0), 2);   // 画所有轮廓
     ```

3. 几何变换

   ```cpp
   // 缩放
   Mat resized;
   resize(img, resized, Size(), 0.5, 0.5);
   
   // 旋转
   Point2f center(img.cols/2.0f, img.rows/2.0f);
   Mat M = getRotationMatrix2D(center, 45, 1.0);
   Mat rotated;
   warpAffine(img, rotated, M, img.size());

   // 透视变换（把斜着的装甲板"摆正"，分类前常用）
   Point2f src[4] = { /* 四个角点 */ };
   Point2f dst[4] = { Point2f(0,0), Point2f(100,0), Point2f(100,30), Point2f(0,30) };
   Mat H = getPerspectiveTransform(src, dst);
   warpPerspective(img, warped, H, Size(100,30));
   ```

   

4. 特征检测与匹配

   ```cpp
   Ptr<ORB> orb = ORB::create();
   vector<KeyPoint> kp;
   Mat desc;
   orb->detectAndCompute(img, noArray(), kp, desc);
   
   BFMatcher matcher(NORM_HAMMING);
   vector<DMatch> matches;
   matcher.match(desc1, desc2, matches);
   ```

   

5. 视频处理

   ```cpp
   VideoCapture cap(0);  // 打开默认摄像头
   if(!cap.isOpened()) { return -1; }
   Mat frame;
   while(true) {
       cap >> frame;
       if(frame.empty()) break;
       imshow("摄像头", frame);
       if(waitKey(30) == 27) break;  // 按 ESC 退出
   }
   cap.release();
   ```

   保存视频用 `VideoWriter`（本工程 `--save-dir` 是按帧存图，这里给出存视频的写法）：

   ```cpp
   VideoWriter writer("out.mp4",
       VideoWriter::fourcc('m','p','4','v'),    // 编码
       30, Size(cap.get(CAP_PROP_FRAME_WIDTH), cap.get(CAP_PROP_FRAME_HEIGHT)));
   while (true) {
     cap >> frame;
     if (frame.empty()) break;
     writer.write(frame);
   }
   writer.release();
   ```

### 5.本节小结与后续衔接

OpenCV 的算子像积木，装甲板识别就是把这些积木按特定顺序拼起来：

| 本节算子 | 在装甲板识别里的作用 |
|----------|----------------------|
| `cvtColor` BGR→Gray | 灯条检测前先转灰度 |
| `threshold` | 把灰度图二值化，分离亮灯条 |
| `findContours` | 从二值图里抠出灯条轮廓 |
| `minAreaRect` | 拟合灯条，算长宽比和角度 |
| `inRange` (HSV) | 按颜色区分红/蓝方 |
| `warpPerspective` | 把装甲板图案"摆正"再分类 |
| `rectangle` / `putText` | 画框、写标签做可视化 |

下一节我们就把这些算子串成完整的"传统装甲板识别"流水线。

## 三、传统视觉识别

> 本节内容对应 `Visual Identity/Training/tasks/auto_aim/detector.cpp`，请边读边在编辑器里打开该文件对照。

### 1. 整体思路

传统方法不依赖任何神经网络，只用 OpenCV 的图像处理算子，把"亮亮的灯条"从画面里抠出来，再两两配对成装甲板。整条流水线如下：

```
BGR 彩色图
   │  ① cvtColor     → 灰度图
   │  ② threshold    → 二值图
   │  ③ findContours → 若干轮廓
   │  ④ minAreaRect  → 候选灯条（带几何过滤）
   │  ⑤ 颜色判定（红 / 蓝）
   │  ⑥ 灯条从左到右排序 + 两两配对
   │  ⑦ 装甲板几何校验
   │  ⑧ 抠出数字图案 → 分类器判兵种
   │  ⑨ 去重（处理共用灯条）
   ▼
Armor 列表（颜色、兵种、板型、四点、置信度）
```

### 2. 主流程代码

`Detector::detect` 把上面的流水线一次性串起来：

```40:88:Visual Identity/Training/tasks/auto_aim/detector.cpp
std::list<Armor> Detector::detect(const cv::Mat & bgr_img, int frame_count)
{
  // 彩色图转灰度图
  cv::Mat gray_img;
  cv::cvtColor(bgr_img, gray_img, cv::COLOR_BGR2GRAY);

  // 进行二值化
  cv::Mat binary_img;
  cv::threshold(gray_img, binary_img, threshold_, 255, cv::THRESH_BINARY);
  cv::imshow("binary_img", binary_img);

  // 获取轮廓点
  std::vector<std::vector<cv::Point>> contours;
  cv::findContours(binary_img, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_NONE);

  // 获取灯条
  std::size_t lightbar_id = 0;
  std::list<Lightbar> lightbars;
  for (const auto & contour : contours) {
    auto rotated_rect = cv::minAreaRect(contour);
    auto lightbar = Lightbar(rotated_rect, lightbar_id);

    if (!check_geometry(lightbar)) continue;

    lightbar.color = get_color(bgr_img, contour);
    lightbars.emplace_back(lightbar);
    lightbar_id += 1;
  }

  // 将灯条从左到右排序
  lightbars.sort([](const Lightbar & a, const Lightbar & b) { return a.center.x < b.center.x; });

  // 获取装甲板
  std::list<Armor> armors;
  for (auto left = lightbars.begin(); left != lightbars.end(); left++) {
    for (auto right = std::next(left); right != lightbars.end(); right++) {
      if (left->color != right->color) continue;

      auto armor = Armor(*left, *right);
      if (!check_geometry(armor)) continue;

      armor.pattern = get_pattern(bgr_img, armor);
      classifier_.classify(armor);
      if (!check_name(armor)) continue;

      armor.type = get_type(armor);
      if (!check_type(armor)) continue;

      armor.center_norm = get_center_norm(bgr_img, armor.center);
      armors.emplace_back(armor);
    }
  }
```

### 3. 课堂演示：二值化与轮廓

课堂上我们会用一张装甲板近景图，现场跑下面这段最小可运行代码，让大家直观看到"灰度 → 二值 → 轮廓"每一步的画面变化：

```cpp
#include <opencv2/opencv.hpp>

int main() {
  cv::Mat bgr = cv::imread("imgs/armor_sample.jpg");
  cv::Mat gray, binary;
  cv::cvtColor(bgr, gray, cv::COLOR_BGR2GRAY);
  cv::threshold(gray, binary, 150, 255, cv::THRESH_BINARY);  // 阈值 150 来自 yaml

  std::vector<std::vector<cv::Point>> contours;
  cv::findContours(binary, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_NONE);

  cv::Mat vis = bgr.clone();
  cv::drawContours(vis, contours, -1, {0, 255, 0}, 2);
  cv::imshow("binary", binary);
  cv::imshow("contours", vis);
  cv::waitKey(0);
}
```

> 💡 **课堂互动**：把 `threshold` 的第二个参数从 150 改成 80、200，观察轮廓数量怎么变。这就是 `configs/armor_detect.yaml` 里 `threshold: 150` 的含义。

### 4. 灯条拟合与几何过滤

每个轮廓用 `cv::minAreaRect` 拟合成一个旋转矩形，再由 `Lightbar` 构造函数算出长度、宽度、角度等几何量：

```10:31:Visual Identity/Training/tasks/auto_aim/armor.cpp
Lightbar::Lightbar(const cv::RotatedRect & rotated_rect, std::size_t id)
: id(id), rotated_rect(rotated_rect)
{
  std::vector<cv::Point2f> corners(4);
  rotated_rect.points(&corners[0]);
  std::sort(corners.begin(), corners.end(), [](const cv::Point2f & a, const cv::Point2f & b) {
    return a.y < b.y;
  });

  center = rotated_rect.center;
  top = (corners[0] + corners[1]) / 2;
  bottom = (corners[2] + corners[3]) / 2;
  top2bottom = bottom - top;

  points.emplace_back(top);
  points.emplace_back(bottom);

  width = cv::norm(corners[0] - corners[1]);
  angle = std::atan2(top2bottom.y, top2bottom.x);
  angle_error = std::abs(angle - CV_PI / 2);
  length = cv::norm(top2bottom);
  ratio = length / width;
}
```

随后 `check_geometry` 用三个条件把噪声滤掉——灯条必须**接近竖直、长宽比合理、足够长**：

```233:242:Visual Identity/Training/tasks/auto_aim/detector.cpp
bool Detector::check_geometry(const Lightbar & lightbar) const
{
  auto angle_ok = lightbar.angle_error < max_angle_error_;
  auto ratio_ok = lightbar.ratio > min_lightbar_ratio_ && lightbar.ratio < max_lightbar_ratio_;
  auto length_ok = lightbar.length > min_lightbar_length_;
  return angle_ok && ratio_ok && length_ok;
}
```

对应配置文件 `configs/armor_detect.yaml`：

```yaml
threshold: 150
max_angle_error: 45        # 灯条与竖直方向最大夹角（度）
min_lightbar_ratio: 1.5   # 灯条长宽比下限
max_lightbar_ratio: 20    # 灯条长宽比上限
min_lightbar_length: 8     # 灯条最短长度（像素）
```

> 💡 **课堂互动**：把 `max_angle_error` 调到 90，看是否会误检到地面的反光条；调到 10，看侧身时的灯条是否会被丢弃。

### 5. 颜色判定：红还是蓝？

灯条颜色靠"轮廓内 B/R 通道累加和"比较得到，简单但有效：

```278:288:Visual Identity/Training/tasks/auto_aim/detector.cpp
Color Detector::get_color(const cv::Mat & bgr_img, const std::vector<cv::Point> & contour) const
{
  int red_sum = 0, blue_sum = 0;

  for (const auto & point : contour) {
    red_sum += bgr_img.at<cv::Vec3b>(point)[2];
    blue_sum += bgr_img.at<cv::Vec3b>(point)[0];
  }

  return blue_sum > red_sum ? Color::blue : Color::red;
}
```

> ⚠️ 注意 OpenCV 默认通道顺序是 **BGR**，所以 `[0]` 是蓝、`[2]` 是红。yaml 中的 `enemy_color` 决定哪一方是敌人。

### 6. 灯条配对与装甲板几何校验

灯条按 `center.x` 排序后，两两组合。只有**颜色相同**的灯条才有资格配对：

```67:82:Visual Identity/Training/tasks/auto_aim/detector.cpp
  for (auto left = lightbars.begin(); left != lightbars.end(); left++) {
    for (auto right = std::next(left); right != lightbars.end(); right++) {
      if (left->color != right->color) continue;

      auto armor = Armor(*left, *right);
      if (!check_geometry(armor)) continue;

      armor.pattern = get_pattern(bgr_img, armor);
      classifier_.classify(armor);
      if (!check_name(armor)) continue;

      armor.type = get_type(armor);
      if (!check_type(armor)) continue;

      armor.center_norm = get_center_norm(bgr_img, armor.center);
      armors.emplace_back(armor);
    }
  }
```

`Armor` 构造函数会算出三个装甲板几何量：

```37:54:Visual Identity/Training/tasks/auto_aim/armor.cpp
Armor::Armor(const Lightbar & left, const Lightbar & right)
: left(left), right(right), duplicated(false)
{
  color = left.color;
  center = (left.center + right.center) / 2;

  points.emplace_back(left.top);
  points.emplace_back(right.top);
  points.emplace_back(right.bottom);
  points.emplace_back(left.bottom);

  auto left2right = right.center - left.center;
  auto width = cv::norm(left2right);
  auto max_lightbar_length = std::max(left.length, right.length);
  auto min_lightbar_length = std::min(left.length, right.length);
  ratio = width / max_lightbar_length;
  side_ratio = max_lightbar_length / min_lightbar_length;
  // ... rectangular_error
}
```

- `ratio`：两灯条中心距 / 灯条长度，即装甲板长宽比
- `side_ratio`：左右灯条长度之比，越接近 1 越好
- `rectangular_error`：灯条与水平连线夹角和 π/2 的差，衡量"是否矩形"

`check_geometry(Armor)` 同时约束这三项：

```244:248:Visual Identity/Training/tasks/auto_aim/detector.cpp
bool Detector::check_geometry(const Armor & armor) const
{
  auto ratio_ok = armor.ratio > min_armor_ratio_ && armor.ratio < max_armor_ratio_;
  auto side_ratio_ok = armor.side_ratio < max_side_ratio_;
  auto rectangular_error_ok = armor.rectangular_error < max_rectangular_error_;
  return ratio_ok && side_ratio_ok && rectangular_error_ok;
}
```

对应 yaml：

```yaml
min_armor_ratio: 1
max_armor_ratio: 5
max_side_ratio: 1.5
max_rectangular_error: 25   # 度
```

> 💡 **课堂互动**：故意拿一张"灯条歪斜"的图，把 `max_rectangular_error` 从 25 调到 5，观察识别框是否消失——这就是几何约束在起作用。

### 7. 图案提取与兵种分类

通过 `get_pattern` 沿灯条方向外扩 ROI，把装甲板中间的数字图案抠出来交给分类器：

```290:311:Visual Identity/Training/tasks/auto_aim/detector.cpp
cv::Mat Detector::get_pattern(const cv::Mat & bgr_img, const Armor & armor) const
{
  const auto expand_ratio = pattern_expand_ratio_for(armor.name);

  // 延长灯条获得装甲板角点
  auto tl = armor.left.center - armor.left.top2bottom * expand_ratio;
  auto bl = armor.left.center + armor.left.top2bottom * expand_ratio;
  auto tr = armor.right.center - armor.right.top2bottom * expand_ratio;
  auto br = armor.right.center + armor.right.top2bottom * expand_ratio;

  auto roi_left = std::max<int>(std::min(tl.x, bl.x), 0);
  auto roi_top = std::max<int>(std::min(tl.y, tr.y), 0);
  auto roi_right = std::min<int>(std::max(tr.x, br.x), bgr_img.cols);
  auto roi_bottom = std::min<int>(std::max(bl.y, br.y), bgr_img.rows);
  // ... 返回 bgr_img(roi)
}
```

分类器（`classifier.cpp`，基于 ONNX 的 `tiny_resnet`）输出 `ArmorName`：`one/two/three/four/five/sentry/outpost/base`，再由 `get_type` 根据比例和兵种推断大/小装甲板：

```317:344:Visual Identity/Training/tasks/auto_aim/detector.cpp
ArmorType Detector::get_type(const Armor & armor)
{
  if (is_base_outpost_name(armor.name)) {
    return ArmorType::base_outpost;
  }

  if (armor.ratio > 3.0) {
    return ArmorType::big;
  }

  if (armor.ratio < 2.5) {
    return ArmorType::small;
  }

  // 英雄只能是大装甲板
  if (armor.name == ArmorName::one) {
    return ArmorType::big;
  }

  // 其他所有（工程、哨兵、步兵）默认都是小装甲板
  return ArmorType::small;
}
```

### 8. 去重：处理共用灯条

当三块装甲板紧挨在一起时，中间灯条会被左右两块装甲板同时使用。`detect` 末尾的双重循环负责去重：**重叠时保留 ROI 小的，相连时保留置信度高的**。

```88:117:Visual Identity/Training/tasks/auto_aim/detector.cpp
  for (auto armor1 = armors.begin(); armor1 != armors.end(); armor1++) {
    for (auto armor2 = std::next(armor1); armor2 != armors.end(); armor2++) {
      if (
        armor1->left.id != armor2->left.id && armor1->left.id != armor2->right.id &&
        armor1->right.id != armor2->left.id && armor1->right.id != armor2->right.id) {
        continue;
      }

      // 装甲板重叠, 保留roi小的
      if (armor1->left.id == armor2->left.id || armor1->right.id == armor2->right.id) {
        auto area1 = armor1->pattern.cols * armor1->pattern.rows;
        auto area2 = armor2->pattern.cols * armor2->pattern.rows;
        if (area1 < area2)
          armor2->duplicated = true;
        else
          armor1->duplicated = true;
      }

      // 装甲板相连，保留置信度大的
      if (armor1->left.id == armor2->right.id || armor1->right.id == armor2->left.id) {
        if (armor1->confidence < armor2->confidence)
          armor1->duplicated = true;
        else
          armor2->duplicated = true;
      }
    }
  }

  armors.remove_if([&](const Armor & a) { return a.duplicated; });
```

### 9. 传统方法小结

| 优点 | 缺点 |
|------|------|
| 不需要训练数据，纯几何即可工作 | 强光、反光、模糊时灯条易断/易连 |
| 可解释性强，每个过滤条件都看得见 | 远距离小灯条容易被几何阈值滤掉 |
| CPU 即可跑，部署成本低 | 装甲板数字必须靠分类器，端到端效果有限 |

> 📝 **课后小任务**：在 `configs/armor_detect.yaml` 里把 `threshold` 从 150 调到 100 和 200，分别跑 `./build/armor_detect --tradition=true imgs/`，记录识别成功数和误检数，写一段你的分析。

## 四、基于YOLO模型的装甲板识别

> 本节内容对应 `Visual Identity/Training/tasks/auto_aim/yolo.cpp` 与 `yolos/yolov5.cpp` 等文件。

### 1. 为什么要用 YOLO？

传统方法靠"灯条 → 配对"的几何规则，遇到反光、残缺、远距离就会失效。YOLO 直接从图像端到端回归出**装甲板的四个角点 + 颜色 + 兵种**，鲁棒性大幅提升。工程里同时支持三套模型，靠 yaml 一键切换：

```10:33:Visual Identity/Training/tasks/auto_aim/yolo.cpp
YOLO::YOLO(const std::string & config_path, bool debug, const std::string & device_key)
{
  auto yaml = YAML::LoadFile(config_path);
  auto yolo_name = yaml["yolo_name"].as<std::string>();

  if (yolo_name == "yolov8") {
    yolo_ = std::make_unique<YOLOV8>(config_path, debug, device_key);
  }

  else if (yolo_name == "yolo11") {
    yolo_ = std::make_unique<YOLO11>(config_path, debug, device_key);
  }

  else if (yolo_name == "yolov5") {
    yolo_ = std::make_unique<YOLOV5>(config_path, debug, device_key);
  }

  else {
    throw std::runtime_error("Unknown yolo name: " + yolo_name + "!");
  }
}
```

对应 yaml 配置：

```yaml
yolo_name: yolov5                # 可选 yolov5 / yolov8 / yolo11
yolov5_model_path: assets/yolov5_0526.xml
yolov8_model_path: assets/yolov8.xml
yolo11_model_path: assets/yolo11.xml
device: CPU                      # 没有核显时用 CPU，有核显可改 GPU
min_confidence: 0.8
use_traditional: true            # YOLO 检出后再用灯条精修角点
```

> 💡 **课堂互动**：把 `yolo_name` 在 `yolov5` / `yolov8` / `yolo11` 之间切换，对同一视频跑一遍，对比识别框的稳定性和 FPS。

### 2. OpenVINO 推理流程

工程使用 Intel OpenVINO 作为推理后端。以 YOLOv5 为例，构造函数里通过 `PrePostProcessor` 把"NHWC BGR u8"输入自动转成模型所需的"NCHW RGB f32"：

```46:62:Visual Identity/Training/tasks/auto_aim/yolos/yolov5.cpp
  auto model = core_.read_model(model_path_);
  ov::preprocess::PrePostProcessor ppp(model);
  auto & input = ppp.input();

  input.tensor()
    .set_element_type(ov::element::u8)
    .set_shape({1, 640, 640, 3})
    .set_layout("NHWC")
    .set_color_format(ov::preprocess::ColorFormat::BGR);

  input.model().set_layout("NCHW");

  input.preprocess()
    .convert_element_type(ov::element::f32)
    .convert_color(ov::preprocess::ColorFormat::RGB)
    .scale(255.0);

  model = ppp.build();
  compiled_model_ = core_.compile_model(
    model, device_, ov::hint::performance_mode(ov::hint::PerformanceMode::LATENCY));
```

推理时把原图按短边缩放到 640×640、补黑边，然后一次 `infer()` 同步推理：

```68:103:Visual Identity/Training/tasks/auto_aim/yolos/yolov5.cpp
std::list<Armor> YOLOV5::detect(const cv::Mat & raw_img, int frame_count)
{
  if (raw_img.empty()) {
    tools::logger()->warn("Empty img!, camera drop!");
    return std::list<Armor>();
  }

  cv::Mat bgr_img;
  if (use_roi_) {
    // ... ROI 裁切
  } else {
    bgr_img = raw_img;
  }

  auto x_scale = static_cast<double>(640) / bgr_img.rows;
  auto y_scale = static_cast<double>(640) / bgr_img.cols;
  auto scale = std::min(x_scale, y_scale);
  auto h = static_cast<int>(bgr_img.rows * scale);
  auto w = static_cast<int>(bgr_img.cols * scale);

  // preproces
  auto input = cv::Mat(640, 640, CV_8UC3, cv::Scalar(0, 0, 0));
  auto roi = cv::Rect(0, 0, w, h);
  cv::resize(bgr_img, input(roi), {w, h});
  ov::Tensor input_tensor(ov::element::u8, {1, 640, 640, 3}, input.data);

  // infer
  auto infer_request = compiled_model_.create_infer_request();
  infer_request.set_input_tensor(input_tensor);
  infer_request.infer();

  // postprocess
  auto output_tensor = infer_request.get_output_tensor();
  auto output_shape = output_tensor.get_shape();
  cv::Mat output(output_shape[1], output_shape[2], CV_32F, output_tensor.data());

  return parse(scale, output, raw_img, frame_count);
}
```

> ⚠️ `scale` 同时用于把模型输出的归一化坐标映射回原图，所以预处理和后处理的 `scale` 必须一致。

### 3. 后处理：解析四点 + 颜色 + 兵种

YOLOv5 的输出每行是 `8 个角点坐标 + obj 分数 + 4 维颜色独热 + 9 维兵种独热`。`parse` 负责把它们拆开，并用 `sigmoid` 还原置信度：

```110:165:Visual Identity/Training/tasks/auto_aim/yolos/yolov5.cpp
std::list<Armor> YOLOV5::parse(
  double scale, cv::Mat & output, const cv::Mat & bgr_img, int frame_count)
{
  // for each row: xywh + classess
  std::vector<int> color_ids, num_ids;
  std::vector<float> confidences;
  std::vector<cv::Rect> boxes;
  std::vector<std::vector<cv::Point2f>> armors_key_points;
  for (int r = 0; r < output.rows; r++) {
    double score = output.at<float>(r, 8);
    score = sigmoid(score);

    if (score < score_threshold_) continue;

    std::vector<cv::Point2f> armor_key_points;

    //颜色和类别独热向量
    cv::Mat color_scores = output.row(r).colRange(9, 13);     //color
    cv::Mat classes_scores = output.row(r).colRange(13, 22);  //num
    cv::Point class_id, color_id;
    int _class_id, _color_id;
    double score_color, score_num;
    cv::minMaxLoc(classes_scores, NULL, &score_num, NULL, &class_id);
    cv::minMaxLoc(color_scores, NULL, &score_color, NULL, &color_id);
    _class_id = class_id.x;
    _color_id = color_id.x;

    armor_key_points.push_back(
      cv::Point2f(output.at<float>(r, 0) / scale, output.at<float>(r, 1) / scale));
    armor_key_points.push_back(
      cv::Point2f(output.at<float>(r, 6) / scale, output.at<float>(r, 7) / scale));
    armor_key_points.push_back(
      cv::Point2f(output.at<float>(r, 4) / scale, output.at<float>(r, 5) / scale));
    armor_key_points.push_back(
      cv::Point2f(output.at<float>(r, 2) / scale, output.at<float>(r, 3) / scale));
    // ... 收集 boxes / confidences / key_points
  }
```

> 📝 注意四点顺序是 `tl, br, tr, bl`（按输出张量里 0..7 的排布），构造 `Armor` 时会自动重排成 `tl, tr, br, bl`。

### 4. NMS 与角点精修

用 `cv::dnn::NMSBoxes` 去掉重叠框，然后对每个保留的装甲板做名称/板型校验。当 `use_traditional: true` 时，会调用 `Detector::detect(Armor&, ...)` 在 YOLO 框出的 ROI 里**重新跑一遍灯条检测**，把角点贴合到真实灯条边缘：

```169:194:Visual Identity/Training/tasks/auto_aim/yolos/yolov5.cpp
  std::list<Armor> armors;
  for (const auto & i : indices) {
    if (use_roi_) {
      armors.emplace_back(
        color_ids[i], num_ids[i], confidences[i], boxes[i], armors_key_points[i], offset_);
    } else {
      armors.emplace_back(color_ids[i], num_ids[i], confidences[i], boxes[i], armors_key_points[i]);
    }
  }

  tmp_img_ = bgr_img;
  for (auto it = armors.begin(); it != armors.end();) {
    if (!check_name(*it)) {
      it = armors.erase(it);
      continue;
    }

    if (!check_type(*it)) {
      it = armors.erase(it);
      continue;
    }
    // 使用传统方法二次矫正角点
    if (use_traditional_) detector_.detect(*it, bgr_img);

    it->center_norm = get_center_norm(bgr_img, it->center);
    ++it;
  }
```

> 💡 **课堂互动**：把 `use_traditional` 在 `true` / `false` 间切换，观察 YOLO 给出的绿色四点是否更"贴边"。这就是"神经网络粗定位 + 传统方法精修"的混合策略。

### 5. YOLO 方法小结

| 优点 | 缺点 |
|------|------|
| 端到端，反光/残缺/远距离下仍能识别 | 需要标注数据和训练流程 |
| 直接输出四点，PnP 解算更稳 | 推理依赖 OpenVINO，部署环境较重 |
| 可叠加传统精修进一步提升角点精度 | 模型选择与超参（置信度、NMS）需调优 |

## 五、实战操作：把工程跑起来

> 本节对应 `Visual Identity/Training/` 整个工程，课堂上大家一起动手。

### 1. 编译

在 `Training` 目录下：

```bash
cmake -B build
cmake --build build --config Release -j
```

可执行文件出现在 `build/armor_detect`。**请在 Training 根目录运行**，以便找到 `configs/` 和 `assets/`。

### 2. 运行闭环

```bash
# 默认打开摄像头 0
./build/armor_detect

# 单张图片（按任意键下一张，q 退出）
./build/armor_detect path/to/image.jpg

# 图片文件夹
./build/armor_detect path/to/images/

# 视频
./build/armor_detect path/to/video.mp4

# 传统灯条方法
./build/armor_detect --tradition=true path/to/video.mp4

# 保存可视化到目录、无窗口（适合无显示器环境）
./build/armor_detect --save-dir=output --no-show=true path/to/images/

# 异步推理（视频或摄像头，提升吞吐）
./build/armor_detect_mt --camera=0
```

窗口标题 `armor_detect`，按 `q` 退出，`Ctrl+C` 同样退出。

### 3. 课堂练习清单

1. **跑通传统方法**：`./build/armor_detect --tradition=true imgs/`，观察 `binary_img` 窗口和 `detection` 窗口的关系。
2. **跑通 YOLO 方法**：`./build/armor_detect imgs/`，对比识别框数量与稳定性。
3. **切换模型**：编辑 `configs/armor_detect.yaml` 的 `yolo_name`，分别跑 yolov5 / yolov8 / yolo11。
4. **调阈值**：把 `min_confidence` 从 0.8 调到 0.5 和 0.95，观察误检和漏检的变化。
5. **开关精修**：`use_traditional` 设 true/false，对比四点是否更贴边。
6. **看日志**：终端会打印 `frame=... detect=...fps armors=...`，以及每个装甲板的颜色、兵种、板型、置信度、bbox。学会用日志定位问题。

### 4. 数据流回顾

```
摄像头 / 视频 / 图片
        │
        ▼
 YOLO 或 Detector::detect
        │
        ▼
  Armor 列表（颜色、兵种、板型、四点、置信度）
        │
        ▼
  画框 + 日志 + 可选存图
```

识别输出停在装甲板列表，后续的跟踪、PnP、瞄准、开火留给后续课程。

### 5. 课后任务

- 在 `imgs/` 放入你自己在赛场上录的视频帧，跑一遍两种方法，写一份对比报告（识别率、误检、FPS）。
- 选一个误检案例，分析是哪一步过滤失效（阈值？几何？分类器？），提出你的改进思路。
- 阅读 `tools/logger.hpp` 和 `tools/img_tools.hpp`，尝试在 `armor_detect.cpp` 的 `draw_armors` 里加一个"显示 FPS 折线图"的小功能。
