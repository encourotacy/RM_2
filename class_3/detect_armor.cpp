#include <cstdlib>
#include <iostream>
#include <string>

#include <opencv2/opencv.hpp>
#include "detector.hpp"

// 传统装甲板识别（对照 sp_vision_25 Detector::detect，不含数字分类）
// 灰度 → 二值 → 轮廓 → 灯条几何/颜色过滤 → 同色灯条配对
// 在 class_3/ 下运行: build/detect_armor
// 也可指定图片:     build/detect_armor imgs/blue_4.jpg

int main(int argc, char ** argv)
{
    const std::string img_path = (argc > 1) ? argv[1] : "imgs/blue_4.jpg";
    cv::Mat bgr_img = cv::imread(img_path);
    if (bgr_img.empty()) {
        std::cerr << "无法读取图片: " << img_path << "\n"
                  << "请在 class_3/ 目录下运行，例如: build/detect_armor imgs/red_3.jpg\n";
        return 1;
    }

    const DetectResult result = detect_armor(bgr_img);
    print_result(img_path, result);

    if (std::getenv("DISPLAY") == nullptr) {
        std::cout << "未检测到 DISPLAY，跳过窗口显示。\n";
        return 0;
    }

    cv::Mat detection = draw_result(bgr_img, result);
    cv::Mat binary_show, detection_show;
    cv::resize(result.binary_img, binary_show, {}, 0.5, 0.5);
    cv::resize(detection, detection_show, {}, 0.5, 0.5);
    cv::imshow("binary", binary_show);
    cv::imshow("detection", detection_show);
    cv::waitKey(0);
    return 0;
}
