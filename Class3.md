# RoboMaster视觉组第三次培训：装甲板的识别

## 一、课程概述

本节课将带大家深入了解赛场上随处可见的结构装甲板。对视觉组来说，赛前调试自瞄、赛场打击敌方机器人、前哨站、基地装甲板，都建立在「先把装甲板从图像里找出来」之上。

今天不直接上完整自瞄工程，而是先把 OpenCV 的基本语法练熟。课堂完整示例在 `class_3/show_img.cpp` 和 `class_3/main.cpp`；课后请在挖空文件 `show_img_hw.cpp`、`main_hw.cpp` 里自己填写对应语句。

## <img src="./media/Class3/armor_2.jpg" alt="armor_2" style="zoom:15%;" />

这两份程序合在一起，就是传统装甲板识别最前面的几块积木：

```
读图 imread
   │
   ├─ show_img：split 拆 B/G/R，看三个通道
   │
   └─ main：BGR → 灰度 → 二值 → 轮廓 → 旋转矩形
```

后面的灯条过滤、红蓝判定、两两配对，都建立在今天这几步之上。

## 二、OpenCV 与本课工程

### 1. 安装 OpenCV

OpenCV 是开源跨平台计算机视觉库，提供图像读写、颜色转换、阈值、轮廓、几何拟合等 API，是视觉组日常开发的基础工具。

[OpenCV官网](https://opencv.org/)  
[OpenCV 4.5.4 入门教程](https://docs.opencv.org/4.5.4/df/d65/tutorial_table_of_content_introduction.html)

Ubuntu 下通过 apt 安装：

```bash
sudo apt update
sudo apt install libopencv-dev             # OpenCV 系统包
apt show libopencv-dev                     # 查看版本
```

编辑器里 OpenCV 头文件报红时，可配置 clangd（只影响红线，不影响 `make` 编译）：

```bash
mkdir -p ~/.config/clangd
nano ~/.config/clangd/config.yaml
```

```yaml
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

本课工程 `class_3/` 已用 CMake 导出 `compile_commands.json`，并配有 `.clangd`，工程内的 `include/img_tools.hpp` 也可被索引到。

> 💡 **课堂验证**：先确认环境能弹出窗口。
>
> ```cpp
> #include <opencv2/opencv.hpp>
> int main() {
>   cv::Mat img(200, 200, CV_8UC3, cv::Scalar(0, 0, 255));   // 红色画布
>   cv::imshow("hello", img);
>   cv::waitKey(0);
> }
> ```
>
> `g++ hello.cpp -o hello $(pkg-config --cflags --libs opencv4) && ./hello`，弹出红窗即成功。

### 2. 本课 CMake 工程

示例与作业都在 `class_3/`。CMakeLists 核心是：找到 OpenCV、把头文件目录加上、为每个 cpp 生成可执行文件。

```cmake
cmake_minimum_required(VERSION 3.16.3)
project(class_3)
set(CMAKE_CXX_STANDARD 17)
find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})
include_directories(include)

add_executable(main main.cpp)
add_executable(show_img show_img.cpp)
add_executable(main_hw main_hw.cpp)
add_executable(show_img_hw show_img_hw.cpp)

target_link_libraries(main ${OpenCV_LIBS})
target_link_libraries(show_img ${OpenCV_LIBS})
target_link_libraries(main_hw ${OpenCV_LIBS})
target_link_libraries(show_img_hw ${OpenCV_LIBS})
```

在 `class_3` 目录下编译、运行（**必须在 `class_3/` 下运行**，因为图片路径是相对路径 `imgs/red_3.jpg`）：

```bash
cd class_3
cmake -B build
make -C build
build/show_img          # 课堂完整示例：通道分离
build/main              # 课堂完整示例：灰度 / 二值 / 轮廓 / 旋转矩形
```

作业填完后同样：

```bash
make -C build
build/show_img_hw
build/main_hw
```

| 文件 | 作用 |
|------|------|
| `show_img.cpp` / `main.cpp` | 课堂完整示例，**不要改** |
| `show_img_hw.cpp` / `main_hw.cpp` | 挖空作业，按 `TODO` 填写 |
| `include/img_tools.hpp` | 画点、画轮廓的小工具，作业里直接调用 |
| `imgs/red_3.jpg` | 本课默认输入图（红方装甲板） |

### 3. 程序开头要写的头文件

```cpp
#include <opencv2/opencv.hpp>   // OpenCV 几乎所有常用接口
#include <vector>               // std::vector，长度可变的数组
#include "img_tools.hpp"        // 本课提供的画图小工具（main 才需要）
```

函数都写在 `cv::` 命名空间里，例如 `cv::Mat`、`cv::imread`。`std::vector` 在 `main.cpp` 里用来存「很多条轮廓」「很多个旋转矩形」。

## 三、OpenCV 基本语法（本课会用到的）

### 1. 色彩空间

色彩空间是像素值的「解释方式」。同一张图，按 BGR 看是彩色，按 Gray 看是亮度，按 Binary 看只剩黑白。

| 空间 | 通道 | 本课用途 |
|------|------|----------|
| **BGR** | B、G、R | `imread` 读出来就是它；`imshow` 也按它显示 |
| **Gray** | 单通道亮度 | 二值化、找轮廓的前置步骤 |
| **Binary** | 0 / 255 | `findContours` 的输入 |
| **HSV** | H、S、V | 按颜色分割（本课作业暂不用，后续识别红/蓝会用） |

> ⚠️ OpenCV 默认通道顺序是 **BGR** 而不是 RGB。`channels.at(0)` 是蓝，`at(2)` 是红。

色彩空间用 `cv::cvtColor` 转换，灰度再用 `cv::threshold` 变成二值：

```cpp
cv::Mat bgr = cv::imread("imgs/red_3.jpg");
cv::Mat gray, binary;
cv::cvtColor(bgr, gray, cv::COLOR_BGR2GRAY);
cv::threshold(gray, binary, 120, 255, cv::THRESH_BINARY);
```

这三行就是 `main.cpp` 前半段的核心。

后续按颜色抠灯条时会用到 HSV（本课先混个眼熟）：

```cpp
cv::Mat hsv, mask;
cv::cvtColor(bgr, hsv, cv::COLOR_BGR2HSV);
cv::inRange(hsv, cv::Scalar(100, 80, 80), cv::Scalar(124, 255, 255), mask);
```

### <img src="./media/Class3/HSV_1-1789442641878-3.jpeg" alt="HSV_1" style="zoom:100%;" />               <img src="./media/Class3/HSV_2-1789442747971-5.jpeg" alt="HSV_2" style="zoom:67%;" />

### 2. 核心数据结构：`cv::Mat`

`cv::Mat` 是图像容器，里面是像素矩阵，还带着宽高、通道数、数据类型。

```cpp
cv::Mat img1;                                              // 空图
cv::Mat img2(480, 640, CV_8UC3, cv::Scalar(0, 0, 255));   // 红画布
cv::Mat img = cv::imread("imgs/red_3.jpg");               // 从文件读
```

- 读图失败时 `img.empty()` 为真，后面的 `cvtColor` / `imshow` 会出问题。作业里路径写错时先检查这一条。
- `clone()` 是深拷贝。`main.cpp` 里 `bgr_img.clone()` 是为了在副本上画轮廓，不把原图涂花。

```cpp
cv::Mat drawcontours = bgr_img.clone();   // 独立一份，再往上画
```

- 通道分离（`show_img.cpp` 的核心）：

```cpp
std::vector<cv::Mat> channels;
cv::split(bgr_img, channels);     // channels[0]=B, [1]=G, [2]=R
cv::Mat blue = channels.at(0);
cv::Mat green = channels.at(1);
cv::Mat red = channels.at(2);
```

### 3. 读图、缩放、显示

```cpp
cv::Mat img = cv::imread("imgs/red_3.jpg");
if (img.empty()) { /* 读取失败，检查路径和当前工作目录 */ }

cv::resize(img, img, {}, 0.5, 0.5);   // 宽高都变成 0.5 倍；{} 表示不指定绝对尺寸
cv::imshow("gray", img);              // 窗口名可以自己起
cv::waitKey(0);                       // 等到按任意键再结束
```

`main.cpp` 里先缩小再 `imshow`、再放大回去，只是为了窗口别太大，**二值化和找轮廓仍用原尺寸**，所以灰度 / 二值会 `resize` 两次。

### 4. 灰度、二值、轮廓、旋转矩形

装甲板灯条又亮又细，传统做法是：灰度 → 二值 → 轮廓 → 最小外接旋转矩形。

```cpp
std::vector<std::vector<cv::Point>> contours;
cv::findContours(binary, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_NONE);
// 外层 vector：有几条轮廓；内层 vector：这条轮廓上的点

for (const auto & c : contours) {
    cv::RotatedRect rr = cv::minAreaRect(c);   // 拟合灯条常用
    std::vector<cv::Point2f> points(4);
    rr.points(points.data());                  // 取出 4 个角点
}
```

| 函数 | 作用 |
|------|------|
| `cvtColor(..., COLOR_BGR2GRAY)` | 彩色变单通道亮度 |
| `threshold(..., 120, 255, THRESH_BINARY)` | 像素 ≥120 变 255，否则变 0 |
| `findContours` | 从白区域描出边界 |
| `RETR_EXTERNAL` | 只取最外层轮廓 |
| `CHAIN_APPROX_NONE` | 保留边界上每一个点 |
| `minAreaRect` | 用面积最小的旋转矩形包住轮廓 |
| `RotatedRect::points` | 得到矩形四个角 |

画轮廓可以用 OpenCV 自带的 `drawContours`，也可以用本课 `tools::drawContour` 把每个点画出来，便于看清 `findContours` 到底找到了什么。

### 5. `std::vector` 在本课里干什么

`<vector>` 提供可变长数组。轮廓数量、每条轮廓的点数事先都不知道，所以必须用它。

```cpp
std::vector<std::vector<cv::Point>> contours;   // 许多条轮廓
std::vector<cv::RotatedRect> rotated_rects;     // 许多个旋转矩形
rotated_rects.emplace_back(rotated_rect);       // 往末尾追加一个
```

## 四、课堂示例精读

先运行完整示例，对照窗口看每一步，再去填作业。

```bash
cd class_3
build/show_img
build/main
```

### 1. `show_img.cpp`：看懂 BGR 三个通道

完整程序只做四件事：读图 → 拆通道 → 缩小 → 显示。

```cpp
#include <opencv2/opencv.hpp>

int main()
{
    cv::Mat bgr_img;
    bgr_img = cv::imread("imgs/red_3.jpg");

    std::vector<cv::Mat> channels;
    cv::split(bgr_img, channels);
    cv::Mat blue = channels.at(0);
    cv::Mat green = channels.at(1);
    cv::Mat red = channels.at(2);

    cv::resize(blue, blue, {}, 0.5, 0.5);
    cv::resize(green, green, {}, 0.5, 0.5);
    cv::resize(red, red, {}, 0.5, 0.5);

    cv::imshow("blue", blue);
    cv::imshow("green", green);
    cv::imshow("red", red);

    cv::waitKey(0);
    return 0;
}
```

红方装甲板的灯条在 **R 通道**往往更亮，蓝方则在 **B 通道**更亮。这就是后面「比较轮廓上 B/R 累加和来判断红蓝」的直觉来源。

> 💡 **课堂互动**：把路径改成 `imgs/blue_4.jpg`，看 blue / red 两个窗口谁更亮。

### 2. `main.cpp`：灰度 → 二值 → 轮廓 → 旋转矩形

流水线与窗口对应关系：

| 步骤 | 函数 | 窗口名 |
|------|------|--------|
| 读图 | `imread` | （原图不单独显示） |
| 转灰度 | `cvtColor` | `gray` |
| 二值化 | `threshold` | `binary` |
| 找轮廓 | `findContours` + `drawContours` | `drawcontours` |
| 旋转矩形 | `minAreaRect` + `points` | `drawrect` |

关键片段：

```cpp
cv::cvtColor(bgr_img, gray_img, cv::COLOR_BGR2GRAY);
cv::threshold(gray_img, binary_img, 120, 255, cv::THRESH_BINARY);

std::vector<std::vector<cv::Point>> contours;
cv::findContours(binary_img, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_NONE);

for (const auto & contour : contours) {
    auto rotated_rect = cv::minAreaRect(contour);
    rotated_rects.emplace_back(rotated_rect);
}
```

画图时用了 `include/img_tools.hpp`：

- `tools::drawContour`：把一条轮廓上的每个点画出来
- `tools::draw_points`：把旋转矩形的四个角连成框

> 💡 **课堂互动**：把 `threshold` 的 `120` 改成 `80` 和 `200`（只改完整示例做实验即可），看 `binary` 和轮廓数量怎么变。阈值就是在「灯条够亮」和「别把反光也当成灯条」之间做取舍。

## 五、课后作业

**不要修改** `main.cpp` 和 `show_img.cpp`。在下面两个挖空文件里，按注释把 OpenCV 语句补全，效果应与课堂示例一致。

| 作业文件 | 对照完整示例 | 要填写的语法 |
|----------|----------------|--------------|
| `class_3/show_img_hw.cpp` | `show_img.cpp` | `imread`、`split`、取通道、`resize`、`imshow` |
| `class_3/main_hw.cpp` | `main.cpp` | `imread`、`cvtColor`、`threshold`、`findContours`、`drawContours`、`minAreaRect`、`points` |

### 1. 作业一：填写 `show_img_hw.cpp`

打开文件，搜索 `TODO`，补全后编译运行：

```bash
cd class_3
cmake -B build
make -C build show_img_hw
build/show_img_hw
```

验收：弹出 `blue`、`green`、`red` 三个窗口，和 `build/show_img` 观感一致。

### 2. 作业二：填写 `main_hw.cpp`

同样搜索 `TODO`。循环、`imshow`、`tools::` 画图已经写好，你需要补上真正的 OpenCV 处理。

```bash
make -C build main_hw
build/main_hw
```

验收：依次（或同时）出现 `gray`、`binary`、`drawcontours`、`drawrect`，轮廓和旋转矩形能框住亮的灯条区域。

### 3. 填写提示（想不起来再看）

`show_img_hw.cpp`：

```cpp
bgr_img = cv::imread("imgs/red_3.jpg");
cv::split(bgr_img, channels);
blue = channels.at(0);
green = channels.at(1);
red = channels.at(2);
cv::resize(blue, blue, {}, 0.5, 0.5);
cv::imshow("blue", blue);
```

`main_hw.cpp`：

```cpp
bgr_img = cv::imread("imgs/red_3.jpg");
cv::cvtColor(bgr_img, gray_img, cv::COLOR_BGR2GRAY);
cv::threshold(gray_img, binary_img, 120, 255, cv::THRESH_BINARY);
cv::findContours(binary_img, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_NONE);
cv::drawContours(drawcontours, contours, {}, {0, 0, 5}, 5);

auto rotated_rect = cv::minAreaRect(contour);
rotated_rects.emplace_back(rotated_rect);
rotated_rect.points(points.data());
```

提示里的代码是「该写什么」，请自己敲进作业文件，不要整份复制 `main.cpp`。

### 4. 选做

- 把图片换成 `imgs/blue_4.jpg` 或 `imgs/armor2.jpg`，观察通道亮度和轮廓变化。
- 只改作业里的阈值，记录 80 / 120 / 200 时轮廓大概有多少条。
- 阅读 `include/img_tools.hpp`，看 `drawContour` 如何用 `cv::circle` 把点画出来。

## 六、和后续装甲板识别的衔接

今天的算子后面会这样用：

| 本课算子 | 在装甲板识别里的作用 |
|----------|----------------------|
| `split` / BGR | 比较轮廓上蓝、红通道，判断敌我颜色 |
| `cvtColor` BGR→Gray | 灯条检测前先转灰度 |
| `threshold` | 把亮灯条变成白色连通域 |
| `findContours` | 抠出灯条轮廓 |
| `minAreaRect` | 拟合灯条，再算长宽比、角度 |
| `clone` + 画图 | 调试时看每一步有没有找对 |

完整自瞄还会做：几何过滤（太斜、太短的丢掉）、灯条左右配对、数字分类、YOLO 检测等，留给后续课程。今天先保证：通道能拆开、灰度二值轮廓矩形能跑通。
