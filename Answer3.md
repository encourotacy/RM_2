# RoboMaster视觉组第三次培训：参考答案

对应讲义：[Class3.md](./Class3.md)

## 一、OpenCV 函数的运用

### 参考答案：`show_img_hw` / `main_hw`

`show_img_hw.cpp`：

```cpp
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
```

`main_hw.cpp`：

```cpp
// Task1
cv::cvtColor(bgr_img, gray_img, cv::COLOR_BGR2GRAY);
cv::resize(gray_img, gray_img, {}, 0.5, 0.5);
cv::imshow("gray", gray_img);
cv::resize(gray_img, gray_img, {}, 2, 2);

// Task2
cv::threshold(gray_img, binary_img, 130, 255, cv::THRESH_BINARY);
cv::resize(binary_img, binary_img, {}, 0.5, 0.5);
cv::imshow("binary", binary_img);
cv::resize(binary_img, binary_img, {}, 2, 2);

// Task3
cv::findContours(binary_img, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_NONE);

// Task4（写在已有的 for 循环后面）
cv::drawContours(drawcontours, contours, {}, {0, 0, 255}, 5);
cv::resize(drawcontours, drawcontours, {}, 0.5, 0.5);
cv::imshow("drawcontours", drawcontours);

// Task5
for (const auto & contour : contours) {          // 作业里已有
    auto rotated_rect = cv::minAreaRect(contour);
    rotated_rects.emplace_back(rotated_rect);
}

// Task6
for (const auto & rotated_rect : rotated_rects) { // 作业里已有
    std::vector<cv::Point2f> points(4);           // 作业里已有
    rotated_rect.points(points.data());
    tools::draw_points(drawrect, points);         // 作业里已有
}
cv::resize(drawrect, drawrect, {}, 0.5, 0.5);
cv::imshow("drawrect", drawrect);
```

## 二、装甲板识别

### 参考答案：`detect_armor_hw`

`detect_armor_hw.cpp`：

```cpp
// Task1
bool angle_ok = lightbar.angle_error < kMaxAngleErrorDeg * CV_PI / 180.0;
bool ratio_ok = lightbar.ratio > kMinLightbarRatio && lightbar.ratio < kMaxLightbarRatio;
bool length_ok = lightbar.length > kMinLightbarLength;

// Task2
if (!(angle_ok && ratio_ok && length_ok)) {
    continue;
}

// Task3
if (!get_color(bgr_img, contour, lightbar.color)) {
    continue;
}

// Task4
if (result.lightbars[i].color != result.lightbars[j].color) {
    continue;
}

// Task5
Armor armor(result.lightbars[i], result.lightbars[j]);

// Task6
bool ratio_ok = armor.ratio > kMinArmorRatio && armor.ratio < kMaxArmorRatio;
bool side_ok = armor.side_ratio < kMaxSideRatio;
bool rect_ok = armor.rectangular_error < kMaxRectangularErrorDeg * CV_PI / 180.0;
if (ratio_ok && side_ok && rect_ok) {
    result.armors.emplace_back(armor);
}
```

## 三、用 `solvePnP` 求距离

### 参考答案：课后作业 Task 01～03

```cpp
// Task 01
static const std::vector<cv::Point3f> object_points{
    {-ARMOR_WIDTH / 2, -LIGHTBAR_LENGTH / 2, 0}, 
    { ARMOR_WIDTH / 2, -LIGHTBAR_LENGTH / 2, 0}, 
    { ARMOR_WIDTH / 2,  LIGHTBAR_LENGTH / 2, 0}, 
    {-ARMOR_WIDTH / 2,  LIGHTBAR_LENGTH / 2, 0} 
};

// Task 02
std::vector<cv::Point2f> img_points{
    armor.left.top,
    armor.right.top,
    armor.right.bottom,
    armor.left.bottom
};

// Task 03
cv::Mat rvec, tvec;
cv::solvePnP(object_points, img_points, camera_matrix, dist_coeffs, rvec, tvec);
```

`ARMOR_WIDTH` / `LIGHTBAR_LENGTH` 用前面的米制尺寸；小装甲宽 0.135。四点顺序必须 Task 01 和 Task 02 对上：都是左上、右上、右下、左下。

