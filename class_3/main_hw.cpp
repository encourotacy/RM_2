#include <opencv2/opencv.hpp>
#include <vector>
#include "img_tools.hpp"

int main()
{
    // 读取图片
    cv::Mat bgr_img;
    // TODO: 用 cv::imread 读取 "imgs/red_3.jpg"

    // 彩色图转灰度图
    cv::Mat gray_img;
    // TODO: 用 cv::cvtColor 把 bgr_img 转成灰度图 gray_img
    // 提示：转换码是 cv::COLOR_BGR2GRAY
    // 显示灰度图，调试用
    cv::resize(gray_img, gray_img, {}, 0.5, 0.5);
    cv::imshow("gray", gray_img);
    cv::resize(gray_img, gray_img, {}, 2, 2);

    // 进行二值化
    cv::Mat binary_img;
    // TODO: 用 cv::threshold 把 gray_img 二值化到 binary_img
    // 提示：阈值 120，最大值 255，类型 cv::THRESH_BINARY
    // 显示二值图，调试用
    cv::resize(binary_img, binary_img, {}, 0.5, 0.5);
    cv::imshow("binary", binary_img);
    cv::resize(binary_img, binary_img, {}, 2, 2);

    // 获取轮廓点
    std::vector<std::vector<cv::Point>> contours;
    // TODO: 用 cv::findContours 从 binary_img 提取轮廓到 contours
    // 提示：模式 cv::RETR_EXTERNAL，方法 cv::CHAIN_APPROX_NONE

    // 显示轮廓点，调试用
    cv::Mat drawcontours = bgr_img.clone();
    for (const auto & contour : contours) {
        tools::drawContour(drawcontours, contour);
    }
    // TODO: 用 cv::drawContours 把 contours 画到 drawcontours 上
    // 提示：颜色可用 {0, 0, 5}，线宽 5
    cv::resize(drawcontours, drawcontours, {}, 0.5, 0.5);
    cv::imshow("drawcontours", drawcontours);

    // 获取旋转矩形并显示，示范用
    std::vector<cv::RotatedRect> rotated_rects;
    for (const auto & contour : contours) {
        // TODO: 用 cv::minAreaRect 拟合当前 contour，结果放进 rotated_rects
    }
    cv::Mat drawrect = bgr_img.clone();
    for (const auto & rotated_rect : rotated_rects) {
        std::vector<cv::Point2f> points(4);
        // TODO: 用 rotated_rect.points(...) 取出 4 个角点
        tools::draw_points(drawrect, points);
    }
    cv::resize(drawrect, drawrect, {}, 0.5, 0.5);
    cv::imshow("drawrect", drawrect);

    cv::waitKey(0);
    return 0;
}
