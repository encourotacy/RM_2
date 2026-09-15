# RoboMaster视觉组第三次培训：装甲板的识别

## 目录

- [一、课程概述](#sec-overview)
- [二、本课会用到的 OpenCV 函数](#sec-opencv)
  - [1. 读图、缩放、显示](#sec-imread)
  - [2. 色彩空间与转换码](#sec-cvtcolor)
  - [3. 通道分离 `cv::split`](#sec-split)
  - [4. 轮廓 `cv::findContours` / `cv::drawContours`](#sec-contours)
  - [5. 旋转矩形 `cv::minAreaRect`](#sec-minarearect)
- [三、课堂演示](#sec-demo)
- [四、参考答案](#sec-answers)
- [五、和后续装甲板识别的衔接](#sec-next)
- [六、从各种图到装甲板识别](#sec-detect)
  - [1. 整条流水线看一眼](#sec-detect-pipeline)
  - [2. 灰度 → 二值 → 轮廓 → 旋转矩形](#sec-detect-front)
  - [3. 几何过滤](#sec-detect-geom)
  - [4. 灯条配对](#sec-detect-pair)
  - [5. 数字分类](#sec-detect-digit)
  - [6. 画在原图上核对](#sec-detect-draw)

<a id="sec-overview"></a>
## 一、课程概述

对视觉组来说，自瞄、打前哨站、打基地，都建立在「先把装甲板从图像里找出来」之上。今天不直接上完整工程，先把本课会用到的 OpenCV 函数练熟。

## <img src="./media/Class3/armor_2.jpg" alt="armor_2" style="zoom:15%;" />

```
读图 imread
   │
   ├─ show_img：split 拆 B/G/R，看三个通道
   │
   └─ main：BGR → 灰度 → 二值 → 轮廓 → 旋转矩形
```

| 文件 | 作用 |
|------|------|
| `show_img.cpp` / `main.cpp` | 完整示例 |
| `show_img_hw.cpp` / `main_hw.cpp` | 课堂演示，按 Task 现场填写 |
| `include/img_tools.hpp` | 画点、画轮廓的小工具 |
| `imgs/red_3.jpg` | 默认输入图（红方装甲板） |

编译、运行必须在 `class_3/` 下，因为图片路径是相对路径：

```bash
cd class_3
cmake -B build
make -C build
build/show_img
build/main
```

安装 OpenCV：`sudo apt install libopencv-dev`。头文件统一写：

```cpp
#include <opencv2/opencv.hpp>
#include <vector>
#include "img_tools.hpp"   // 画轮廓 / 旋转矩形时才需要
```

---

<a id="sec-opencv"></a>
## 二、本课会用到的 OpenCV 函数

<a id="sec-imread"></a>
### 1. 读图、缩放、显示

```cpp
cv::Mat img = cv::imread("imgs/red_3.jpg");   // 读出来是 BGR
cv::resize(img, img, {}, 0.5, 0.5);           // 宽高都变成 0.5 倍；{} 表示不指定绝对尺寸
cv::imshow("gray", img);                      // 窗口名自己起
cv::waitKey(0);                               // 等到按任意键再结束
```

演示里先缩小再 `imshow`、再 `resize(..., 2, 2)` 放大回去，只是为了窗口别太大，后面的二值化和找轮廓仍用原尺寸。

<a id="sec-cvtcolor"></a>
### 2. 色彩空间与转换码

OpenCV 默认通道顺序是 **BGR**，不是 RGB。`channels.at(0)` 是蓝，`at(2)` 是红。

| 空间 | 通道 | 本课用途 |
|------|------|----------|
| **BGR** | B、G、R | `imread` 读出来就是它 |
| **Gray** | 单通道亮度 | 二值化、找轮廓的前置步骤 |
| **Binary** | 0 / 255 | `findContours` 的输入 |
| **HSV** | H（色相）、S（饱和度）、V（明度） | 按颜色分割（后续识别红/蓝会用） |

颜色转换：

```cpp
cv::cvtColor(src, dst, code);   // src：输入图；dst：输出图
```

OpenCV 里很多函数都这样写：`src` 是原来的图（source），`dst` 是处理完得到的新图（destination）。写代码时换成自己的变量名即可，例如 `cvtColor(bgr_img, gray_img, ...)`。

| 转换码 | 含义 |
|--------|------|
| `cv::COLOR_BGR2GRAY` | 彩色 → 灰度 |
| `cv::COLOR_BGR2HSV` | 彩色 → HSV |
| `cv::COLOR_BGR2RGB` | BGR → RGB（一般显示才用） |

灰度再变成二值：

```cpp
cv::threshold(src, dst, thresh, maxval, type);   // src：灰度图；dst：二值图
```

| 参数 | 本课取值 | 含义 |
|------|----------|------|
| `thresh` | `120` | 阈值 |
| `maxval` | `255` | 超过阈值时写成这个值 |
| `type` | `cv::THRESH_BINARY` | ≥ 阈值 → `maxval`，否则 → 0 |

合在一起：

```cpp
cv::cvtColor(bgr, gray, cv::COLOR_BGR2GRAY);
cv::threshold(gray, binary, 120, 255, cv::THRESH_BINARY);
```

后续按颜色抠灯条时会用到 HSV：

```cpp
cv::cvtColor(bgr, hsv, cv::COLOR_BGR2HSV);
cv::inRange(hsv, cv::Scalar(100, 80, 80), cv::Scalar(124, 255, 255), mask);
```

`inRange` 的意思是：像素的 H、S、V 都落在上下限之间，就写成 255（白），否则写成 0（黑），结果放进 `mask`。

| 参数 | 本课取值 | 含义 |
|------|----------|------|
| 第 1 个 | `hsv` | 输入图（HSV） |
| 第 2 个 | `Scalar(100, 80, 80)` | 下限：H≥100，S≥80，V≥80 |
| 第 3 个 | `Scalar(124, 255, 255)` | 上限：H≤124，S≤255，V≤255 |
| 第 4 个 | `mask` | 输出的黑白掩码图 |

OpenCV 里 H 的范围是 **0～180**（不是 0～360）。上面这组大约是蓝色；红色会绕过 0 和 180，通常要写两段再拼起来。

### <img src="./media/Class3/HSV_1-1789442641878-3.jpeg" alt="HSV_1" style="zoom:100%;" />               <img src="./media/Class3/HSV_2-1789442747971-5.jpeg" alt="HSV_2" style="zoom:67%;" />

<a id="sec-split"></a>
### 3. 通道分离 `cv::split`

```cpp
std::vector<cv::Mat> channels;
cv::split(bgr_img, channels);     // 拆成 3 张单通道图
cv::Mat blue  = channels.at(0);   // B
cv::Mat green = channels.at(1);   // G
cv::Mat red   = channels.at(2);   // R
```

红方灯条在 **R 通道**往往更亮，蓝方则在 **B 通道**更亮。

<a id="sec-contours"></a>
### 4. 轮廓 `cv::findContours` / `cv::drawContours`

```cpp
cv::findContours(image, contours, mode, method);
```

| 参数 | 本课取值 | 含义 |
|------|----------|------|
| `image` | 二值图 `binary_img` | 白区域会被描边 |
| `contours` | `std::vector<std::vector<cv::Point>>` | 外层：有几条轮廓；内层：这条轮廓上的点 |
| `mode` | `cv::RETR_EXTERNAL` | 只取最外层轮廓 |
| `method` | `cv::CHAIN_APPROX_NONE` | 保留边界上每一个点 |

画轮廓：

```cpp
cv::drawContours(image, contours, contourIdx, color, thickness);
```

| 参数 | 本课取值 | 含义 |
|------|----------|------|
| `contourIdx` | `{}` 或 `-1` | 画全部轮廓 |
| `color` | `{0, 0, 255}` | BGR 颜色，亮红 |
| `thickness` | `5` | 线宽 |

本课 `tools::drawContour` 会把轮廓上每个点画出来，方便看 `findContours` 到底找到了什么。

<a id="sec-minarearect"></a>
### 5. 旋转矩形 `cv::minAreaRect`

```cpp
cv::RotatedRect rr = cv::minAreaRect(contour);   // 用面积最小的旋转矩形包住轮廓
std::vector<cv::Point2f> points(4);
rr.points(points.data());                        // 取出 4 个角点
```

轮廓数量事先不知道，用 `std::vector` 存：

```cpp
std::vector<cv::RotatedRect> rotated_rects;
rotated_rects.emplace_back(rotated_rect);   // 往末尾追加
```

画矩形四个角用 `tools::draw_points`。在副本上画图时先 `clone()`，避免把原图涂花：

```cpp
cv::Mat drawcontours = bgr_img.clone();
```

---

<a id="sec-demo"></a>
## 三、课堂演示

对照 `show_img.cpp` / `main.cpp`，在 `*_hw.cpp` 里按 Task 把语句填上。

```bash
make -C build show_img_hw main_hw
build/show_img_hw    # 应弹出 blue / green / red
build/main_hw        # 应弹出 gray / binary / drawcontours / drawrect
```

`show_img_hw.cpp` 要写：`imread`、`split`、取 B/G/R、`resize`、`imshow`。

`main_hw.cpp` 要写：

| Task | 函数 | 窗口 |
|------|------|------|
| 1 | `cvtColor`，转换码 `COLOR_BGR2GRAY` | `gray` |
| 2 | `threshold`，`120 / 255 / THRESH_BINARY` | `binary` |
| 3 | `findContours`，`RETR_EXTERNAL` + `CHAIN_APPROX_NONE` | （不弹窗） |
| 4 | `drawContours`，颜色 `{0, 0, 255}`，线宽 5 | `drawcontours` |
| 5 | `minAreaRect` + `emplace_back` | （不弹窗） |
| 6 | `rotated_rect.points(...)` | `drawrect` |

灰度、二值显示完后记得 `resize(..., 2, 2)` 恢复原大小，否则后面找轮廓会和原图对不上。

> 把路径改成 `imgs/blue_4.jpg`，看 blue / red 谁更亮。把阈值 `120` 改成 `80` 和 `200`，看 `binary` 和轮廓数量怎么变。

---

<a id="sec-answers"></a>
## 四、参考答案

`show_img_hw.cpp`：

```cpp
bgr_img = cv::imread("imgs/red_3.jpg");
cv::split(bgr_img, channels);
blue = channels.at(0);
green = channels.at(1);
red = channels.at(2);
cv::resize(blue, blue, {}, 0.5, 0.5);
cv::resize(green, green, {}, 0.5, 0.5);
cv::resize(red, red, {}, 0.5, 0.5);
cv::imshow("blue", blue);
cv::imshow("green", green);
cv::imshow("red", red);
```

`main_hw.cpp`：

```cpp
cv::cvtColor(bgr_img, gray_img, cv::COLOR_BGR2GRAY);
cv::resize(gray_img, gray_img, {}, 0.5, 0.5);
cv::imshow("gray", gray_img);
cv::resize(gray_img, gray_img, {}, 2, 2);

cv::threshold(gray_img, binary_img, 120, 255, cv::THRESH_BINARY);
cv::resize(binary_img, binary_img, {}, 0.5, 0.5);
cv::imshow("binary", binary_img);
cv::resize(binary_img, binary_img, {}, 2, 2);

cv::findContours(binary_img, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_NONE);
cv::drawContours(drawcontours, contours, {}, {0, 0, 255}, 5);
cv::resize(drawcontours, drawcontours, {}, 0.5, 0.5);
cv::imshow("drawcontours", drawcontours);

auto rotated_rect = cv::minAreaRect(contour);
rotated_rects.emplace_back(rotated_rect);
rotated_rect.points(points.data());
cv::resize(drawrect, drawrect, {}, 0.5, 0.5);
cv::imshow("drawrect", drawrect);
```

---

<a id="sec-next"></a>
## 五、和后续装甲板识别的衔接

| 本课算子 | 后面怎么用 |
|----------|------------|
| `split` / BGR | 比较轮廓上蓝、红通道，判断敌我颜色 |
| `cvtColor` BGR→Gray | 灯条检测前先转灰度 |
| `threshold` | 把亮灯条变成白色连通域 |
| `findContours` | 抠出灯条轮廓 |
| `minAreaRect` | 拟合灯条，再算长宽比、角度 |

完整自瞄还会做几何过滤、灯条配对、数字分类等，留给后续课程。今天先保证：通道能拆开，灰度 → 二值 → 轮廓 → 旋转矩形能跑通。

<a id="sec-detect"></a>
## 六、从各种图到装甲板识别

课堂练习停在「旋转矩形」。完整识别还要回答：哪些矩形是灯条、哪两根构成一块板、板上写的是几。入口是 `detect_armor.cpp`，流水线在 `include/detector.hpp`，灯条和装甲板的几何定义在 `include/armor.hpp`。当前代码做到配对为止，数字分类先讲原理、先不写进程序。

<a id="sec-detect-pipeline"></a>
### 1. 整条流水线看一眼

本课弹出来的窗口，对应识别里的每一步；加粗的三步是这一节要讲清楚的。

```
BGR 原图
  │
  ├─ gray          灰度图
  │     │
  │     └─ binary  二值图（灯条变成白色连通域）
  │           │
  │           └─ contours / drawcontours  轮廓
  │                 │
  │                 └─ drawrect  旋转矩形（全部候选）
  │                       │
  │                       ├─ 【几何过滤】形状 + B/R 比色     → 灯条
  │                       │
  │                       ├─ 【灯条配对】同色两两配 + 再过滤 + 去重 → 装甲板候选
  │                       │
  │                       └─ 【数字分类】透视变换抠数字 + 网络     → 兵种 / 丢掉假板
  │
  └─ detection     在原图上画出灯条（红/蓝）和装甲板（绿框）
```

| 窗口 / 图 | 你在看什么 | 对应算子 |
|-----------|------------|----------|
| 原图 BGR | `imread` 读进来的彩色图 | — |
| `gray` | 亮度，灯条最亮 | `cvtColor` `BGR2GRAY` |
| `binary` | 亮的变成白、暗的变成黑 | `threshold` |
| `drawcontours` | 白色区域的边界 | `findContours` |
| `drawrect` | 用最小旋转矩形包住每条轮廓 | `minAreaRect` |
| `detection` | 滤完、配完之后的灯条和装甲板 | `detect_armor` |

跑起来看当前终态（还没有数字）：

```bash
cd class_3
make -C build detect_armor
build/detect_armor                 # 默认 imgs/armor2.jpg
build/detect_armor imgs/red_3.jpg
```

会弹出 `binary` 和 `detection`。终端会打印灯条数、装甲板数、颜色、中心和四个角点。

<a id="sec-detect-front"></a>
### 2. 灰度 → 二值 → 轮廓 → 旋转矩形

和 `main.cpp` 完全一样：灯条是画面里最亮的东西，灰度后用阈值切开，白色连通域变成轮廓，再用旋转矩形包住。

```cpp
cv::cvtColor(bgr_img, gray_img, cv::COLOR_BGR2GRAY);
cv::threshold(gray_img, binary_img, 120, 255, cv::THRESH_BINARY);
cv::findContours(binary_img, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_NONE);
Lightbar lightbar(cv::minAreaRect(contour), id);
```

阈值仍是课堂的 `120`。太低会把反光、数字也切进来；太高会把远处细灯条切没。

`minAreaRect` 只给出四个角。构造 `Lightbar` 时把四个角按 **y 从小到大**排序：上面两个角取中点当 `top`，下面两个当 `bottom`。后面过滤、配对都用这些量，不再直接画旋转矩形的四个角。

| 量 | 怎么来的 | 后面谁用 |
|----|----------|----------|
| `length` | `top` 到 `bottom` | 几何过滤（太短就扔） |
| `width` | 上端两个角的距离 | 算长宽比 |
| `ratio` | `length / width` | 几何过滤（不像细条就扔） |
| `angle` / `angle_error` | 相对竖直偏了多少 | 几何过滤；配对时还要看是否和底板垂直 |
| `{top, bottom}` | 灯条两端 | 配对时拼出装甲板四角 |

<a id="sec-detect-geom"></a>
### 3. 几何过滤

二值图里什么亮都会出轮廓：反光、白色数字、地面高光、横着的灯带。几何过滤要做的事只有一句：**单看这一根，它长得像不像灯条。**

```
像灯条                不像灯条
  ██                  ████████   横条：偏角太大
  ██                  ██         太短：噪声 / 太远
  ██                  ████       太胖：数字、反光块
  ██
```

```cpp
if (!check_lightbar(lightbar)) continue;
```

三项必须同时成立：

| 条件 | 阈值 | 为什么这样设 | 挡掉什么 |
|------|------|--------------|----------|
| 偏角 `< 45°` | `kMaxAngleErrorDeg` | 车上的灯条接近竖直；车侧倾、斜着看时会歪一点，但很少超过 45° | 地平线、横灯带 |
| 长宽比 `1.5～20` | `kMin/MaxLightbarRatio` | 灯条是细长矩形。接近正方形（比值 ~1）不是灯条；细得像一条线（>20）多半是噪点 | 数字块、极细的亮线 |
| 长度 `> 15` 像素 | `kMinLightbarLength` | 太短的亮点打不准，也容易是噪声 | 碎反光 |

形状过了，再用颜色挡一次。课堂 `split` 已经看过：红方灯条在 **R 通道**更亮，蓝方在 **B 通道**更亮。这里不用整图 `split`，只把**这条轮廓上每个点**的 B、R 加起来比：

```cpp
blue_sum += bgr_img.at<cv::Vec3b>(point)[0];   // B
red_sum  += bgr_img.at<cv::Vec3b>(point)[2];   // R

if (red_sum  > blue_sum * 1.2) color = red;
if (blue_sum > red_sum  * 1.2) color = blue;
```

两边差不多（白色数字、反光）就丢掉。`1.2` 是为了别把「差不多亮」误判成红或蓝。HSV + `inRange` 也能按颜色抠，工程里两种都会见到；这份代码走通道求和，和本课 `split` 直接对应。

过完这一步，列表里剩下的才叫灯条：有形状、有红/蓝。假灯条会在这里被扔掉，后面配对才不会被噪声带偏。

<a id="sec-detect-pair"></a>
### 4. 灯条配对

一块装甲板不是一根灯条，而是左右两根**同色**灯条夹着一块数字板：

```
  左上 ●————————● 右上
       |   3    |
       |        |
  左下 ●————————● 右下
```

配对要做的事：**从已经滤好的灯条里，找出「这两根属于同一块板」的组合。**

先按灯条中心的 x 从左到右排好，这样每一对里左边那根一定是 `left`、右边是 `right`。再两两组合：红配红、蓝配蓝，不同色直接跳过（同一辆车的灯条同色，敌我也不会配到一起）。

```
按 x 排序后的灯条:   A        B        C

可能的配对:  (A,B)  (A,C)  (B,C)
           同色才往下走，再看像不像一块板。
           (A,C) 往往隔太远，间距比会超上限。
```

```cpp
Armor armor(left_bar, right_bar);   // 已保证 left.center.x < right.center.x
if (check_armor(armor)) armors.emplace_back(armor);
```

构造时四个角取两根灯条的端点：`左上、右上、右下、左下`（顺序后面 PnP 还要用）。中心是两根灯条中心的中点。然后用三个量判断「这两根像不像一块板」——这是配对之后的第二轮几何过滤，滤的是**一对灯条的相对关系**，不是单根。

| 量 | 怎么算 | 合理范围 | 物理含义 | 挡掉什么 |
|----|--------|----------|----------|----------|
| `ratio` | 两灯条中心距 / 较长那根 | `1～5` | 小装甲大约 `135 / 56 ≈ 2.4`，大装甲大约 `230 / 56 ≈ 4.1` | 挨太近（同一根被拆成两半）或隔太远（左边配到另一块板的右边） |
| `side_ratio` | 长灯条 / 短灯条 | `< 1.5` | 同一块板上两根灯条应当差不多长 | 一根完整、一根只露出一截；或配到了无关的亮条 |
| `rectangular_error` | 灯条方向和「左右中心连线」相对垂直的偏差 | `< 25°` | 真装甲板接近矩形：整车转了，灯条和上下边一起转，仍然接近垂直 | 两根平行但错位的灯条、明显的平行四边形 |

`rectangular_error` 可以记成：先算左右中心连线的倾角 `roll`，再看每根灯条的 `angle` 是否大约等于 `roll + 90°`，取左右偏差里较大的那个。车斜着停的时候，真板仍然能过；胡乱配对往往过不了。

配对是「所有同色灯条两两试」，不是只配相邻的。所以三根并排时会配出两块「装甲板」，中间那根被共享：

```
A —— B —— C      配出 (A,B) 和 (B,C)，B 用了两次
```

去重：共享灯条时，留下 `ratio` 更接近 **2.5** 的那块（更像常见的小装甲），另一块丢掉。这样步兵车侧三根灯条并排时，不会把「左板的右灯条 + 右板的左灯条」当成第三块板。

这一步结束，得到的是装甲板**候选**：颜色、中心、四个角点都有了。几何上像板，不等于板上真有数字——那是下一步的事。

<a id="sec-detect-digit"></a>
### 5. 数字分类

几何过滤和配对只看灯条形状。场上两根碰巧平行的灯、广告牌、误配的灯条，仍可能画出一个很像装甲板的绿框。自瞄还要知道**这是谁**：1 号英雄、2 号工程、3/4/5 步兵、哨兵、前哨站、基地，或者根本不是板。

分类要做的事：**把两灯条中间那块图案抠成一张正视图，交给网络认数字。**

```
原图上的斜四边形                    固定大小的正视图
  ●——————●                            ┌────────┐
   \  3  /     warpPerspective        │   3    │  → 网络 → "3"（步兵，小装甲）
    ●——●                              └────────┘
```

四点顺序必须和配对时一致：`左上、右上、右下、左下`。透视变换把任意姿态的数字拉成同一尺寸，远近、倾斜造成的变形就先被消掉，网络才比较好认。

```cpp
// 原理示意，当前 detect_armor 还没有这一步
std::vector<cv::Point2f> src = armor.points;
std::vector<cv::Point2f> dst = {{0, 0}, {W - 1, 0}, {W - 1, H - 1}, {0, H - 1}};
cv::Mat M = cv::getPerspectiveTransform(src, dst);
cv::warpPerspective(gray_img, pattern, M, {W, H});
// pattern 送进分类网络
```

网络（工程里常见是小 CNN，ONNX 推理）输出的是类别，不是「画出来的那个字」本身：

| 输出 | 含义 | 板型（给下一课 PnP 用） |
|------|------|-------------------------|
| `1` | 英雄 | 大装甲（约 23 cm） |
| `2` | 工程 | 小装甲（约 13.5 cm） |
| `3` / `4` / `5` | 步兵 | 小装甲 |
| `sentry` | 哨兵 | 小装甲 |
| `outpost` | 前哨站 | 大装甲 |
| `base` | 基地 | 大装甲 |
| `not_armor` | 不是装甲板 | 丢掉这个候选 |

置信度太低或判成 `not_armor`，这块候选就删掉——这是几何过滤之后的最后一道闸。判对之后，装甲板才同时具备：**颜色、四个角点、兵种、大小板型**。下一课 PnP 要用板型选 3D 模型点（小 13.5 cm / 大 23 cm）；打谁、打不打工程，也靠这个名字。

当前 `detect_armor` **没有**这一步，所以 `detection` 窗口里只标颜色和 `armor0`，不标数字。假板如果几何上像，也会被画出来。分类网络需要单独训练和模型文件，这一节只把位置和输入输出讲清，代码先不动。

<a id="sec-detect-draw"></a>
### 6. 画在原图上核对

`draw_result` 在原图副本上画：

- 灯条：红 / 蓝折线，旁边标 `red` / `blue`
- 装甲板：绿色四边形，标 `armor0 red` 这类字

对照两个窗口即可：`binary` 里该亮的地方有没有亮；`detection` 里灯条颜色对不对、绿框有没有套住整块板。有数字分类之后，绿框旁边还应出现 `3`、`sentry` 这样的名字。

把路径换成 `imgs/blue_4.jpg`，看颜色会不会判成 `blue`。把阈值从 `120` 改成 `80` 和 `200`，看灯条数和装甲板数怎么变——和课堂改 `binary` 是同一件事，只是后面多了过滤和配对。

PnP 位姿还不做，那是下一课：用这里得到的四个角点和板型，把「图像里的板」变成「空间里的位置」。
