#include <opencv2/opencv.hpp>

int main()
{
    cv::Mat bgr_img;
    // TODO: 用 cv::imread 读取 "imgs/red_3.jpg"，赋给 bgr_img

    // 分离三个通道并显示，调试用
    std::vector<cv::Mat> channels;
    // TODO: 用 cv::split 把 bgr_img 拆进 channels
    cv::Mat blue;
    cv::Mat green;
    cv::Mat red;
    // TODO: 从 channels 里取出 B、G、R 三个单通道图
    // 提示：OpenCV 默认是 BGR，channels.at(0) 是蓝

    // TODO: 把 blue / green / red 都缩小到 0.5 倍，方便显示
    // 提示：cv::resize(src, dst, {}, 0.5, 0.5);

    // TODO: 用 cv::imshow 分别显示名为 "blue" "green" "red" 的窗口

    cv::waitKey(0);
    return 0;
}
