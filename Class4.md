# RoboMaster视觉组第四次培训：PnP解算


## 一、课程概述

上一节课我们让相机"看见"了装甲板——得到了四个角点的像素坐标、颜色、兵种、板型。但自瞄要打的不是"图像里的装甲板"，而是现实中三米开外、正在移动的装甲板。**PnP（Perspective-n-Point）解算**就是把这"四个像素点"变成"目标在三维空间中的位置和朝向"的关键一步。本节课我们将从原理讲到工程实现，最后大家在真实工程代码上动手跑通整条解算链路。

## 二、PnP 是什么：从 2D 到 3D

### 1. 为什么有了相机内参还不够？

回忆相机标定：内参矩阵 \(K\) 告诉我们"像素 ↔ 三维射线"的对应关系。但一条射线上的所有点都投影到同一个像素——

- 像素 \((320, 240)\) 既可能是 1 米处的小装甲板，也可能是 10 米处的大装甲板
- 内参只能给出"方向"，给不出"距离"

PnP 的作用就是结合**三个信息**反推目标位姿：

1. 相机内参 \(K\) 和畸变系数（来自标定）
2. 目标的**真实 3D 尺寸**（装甲板宽 13.5cm / 23cm，灯条长 5.6cm）
3. 目标在图像上的 **2D 投影**（上一节课检测到的四个角点）

### 2. 问题的数学描述

**输入（已知）：**

- 内参矩阵 \(K\)、畸变系数
- \(n\) 个 3D 点在物体坐标系下的坐标 \(P_i^w = (X_i^w, Y_i^w, Z_i^w)\)
- 对应的 \(n\) 个 2D 像素坐标 \(p_i = (u_i, v_i)\)

**输出（求解）：**

- 外参 \([R \mid t]\)，即目标相对于相机的旋转和平移

**投影约束**（每个点对都满足）：

\[
s_i \begin{bmatrix} u_i \\ v_i \\ 1 \end{bmatrix}
= K \, [R \mid t] \, \begin{bmatrix} X_i^w \\ Y_i^w \\ Z_i^w \\ 1 \end{bmatrix}
\]

理论上 \(n \geq 3\) 即可求解，装甲板有 4 个角点，属于超定问题，精度更高。

### 3. 坐标变换与外参矩阵

同一个点在不同坐标系下数值不同。PnP 求出的 \([R \mid t]\) 就是"物体坐标系 → 相机坐标系"的变换：

\[
P^c = R \cdot P^w + t
\]

- \(R\) 是 \(3\times3\) 旋转矩阵，对齐坐标轴方向
- \(t = (t_x, t_y, t_z)^T\) 是平移向量，调整原点位置

用齐次坐标合并成 \(4\times4\) 外参矩阵：

\[
\begin{bmatrix} P^c \\ 1 \end{bmatrix}
= \begin{bmatrix} R & t \\ 0 & 1 \end{bmatrix}
\begin{bmatrix} P^w \\ 1 \end{bmatrix}
\]

### 4. PnP 能给我们什么？

解出 \([R \mid t]\) 后：

- **距离**：\(d = \|t\| = \sqrt{t_x^2 + t_y^2 + t_z^2}\)
- **朝向**：从 \(R\) 可算出 yaw / pitch / roll 三个欧拉角
- **完整 6 自由度位姿**：3 平移 + 3 旋转，正是自瞄打击所需

## 三、OpenCV 中的 PnP 求解

### 1. `cv::solvePnP` 函数签名

```cpp
bool cv::solvePnP(
    InputArray  objectPoints,   // 3D 模型点（物体坐标系）
    InputArray  imagePoints,    // 对应 2D 像素点
    InputArray  cameraMatrix,    // 内参 K
    InputArray  distCoeffs,      // 畸变系数
    OutputArray rvec,            // 输出：旋转向量（3×1）
    OutputArray tvec,            // 输出：平移向量（3×1）
    bool        useExtrinsicGuess = false,
    int         flags = SOLVEPNP_ITERATIVE);
```

> ⚠️ `rvec` 是**旋转向量**而非旋转矩阵，方向为旋转轴、模长为旋转角，二者通过 `cv::Rodrigues` 互转。

### 2. 求解方法对比

| 方法 | 点数 | 特点 | 适用场景 |
|------|------|------|----------|
| `SOLVEPNP_ITERATIVE` | ≥4 | LM 迭代优化，精度高 | 通用，对精度要求高 |
| `SOLVEPNP_EPNP` | ≥4 | 线性时间，速度快 | 多点初始估计 |
| `SOLVEPNP_P3P` | =3 | 最多 4 组解，需第 4 点验证 | 点数受限 |
| `SOLVEPNP_AP3P` | =3 | 代数法，数值稳定性更好 | 点数受限 |
| **`SOLVEPNP_IPPE`** | ≥4 | **专为平面物体设计**，快且准 | **装甲板（平面）推荐** |

装甲板是平面目标，工程里统一用 `SOLVEPNP_IPPE`。

### 3. 3D 模型点的建立

装甲板四角点在"以装甲板中心为原点"的物体坐标系下取值，单位是**米**。本工程的定义如下：

```11:25:Visual Identity/sp_vision_25-main/tasks/auto_aim/solver.cpp
constexpr double LIGHTBAR_LENGTH = 56e-3;     // m
constexpr double BIG_ARMOR_WIDTH = 230e-3;    // m
constexpr double SMALL_ARMOR_WIDTH = 135e-3;  // m

const std::vector<cv::Point3f> BIG_ARMOR_POINTS{
  {0, BIG_ARMOR_WIDTH / 2, LIGHTBAR_LENGTH / 2},
  {0, -BIG_ARMOR_WIDTH / 2, LIGHTBAR_LENGTH / 2},
  {0, -BIG_ARMOR_WIDTH / 2, -LIGHTBAR_LENGTH / 2},
  {0, BIG_ARMOR_WIDTH / 2, -LIGHTBAR_LENGTH / 2}};
const std::vector<cv::Point3f> SMALL_ARMOR_POINTS{
  {0, SMALL_ARMOR_WIDTH / 2, LIGHTBAR_LENGTH / 2},
  {0, -SMALL_ARMOR_WIDTH / 2, LIGHTBAR_LENGTH / 2},
  {0, -SMALL_ARMOR_WIDTH / 2, -LIGHTBAR_LENGTH / 2},
  {0, SMALL_ARMOR_WIDTH / 2, -LIGHTBAR_LENGTH / 2}};
```

> 📝 注意四点顺序必须和 `armor.points` 里的 2D 点**一一对应**，否则解出来的是错的位姿。本工程 2D 点顺序是 `左上、右上、右下、左下`，3D 点也按相同顺序排列。

### 4. 课堂演示：最小 PnP 示例

课堂上我们用一张已知尺寸的装甲板图片，现场跑下面这段代码，把"装甲板离相机多远"算出来：

```cpp
#include <opencv2/opencv.hpp>
#include <vector>
#include <iostream>

int main() {
  // 1. 相机内参（替换为你的标定值）
  cv::Mat K = (cv::Mat_<double>(3, 3) << 1785.49, 0, 672.48,
                                          0, 1785.03, 559.90,
                                          0, 0, 1);
  cv::Mat dist = (cv::Mat_<double>(5, 1) << -0.076, 0.112, 0.0005, -0.0028, 0);

  // 2. 3D 模型点（小装甲板，单位米）
  std::vector<cv::Point3f> obj = {
    {0,  0.135/2,  0.056/2},
    {0, -0.135/2,  0.056/2},
    {0, -0.135/2, -0.056/2},
    {0,  0.135/2, -0.056/2}};

  // 3. 2D 像素点（来自上一节课的检测器，这里用假数据演示）
  std::vector<cv::Point2f> img = {
    {700, 400}, {800, 400}, {800, 450}, {700, 450}};

  // 4. 求解
  cv::Vec3d rvec, tvec;
  cv::solvePnP(obj, img, K, dist, rvec, tvec, false, cv::SOLVEPNP_IPPE);

  // 5. 输出距离
  double dist_m = cv::norm(tvec);
  std::cout << "距离 = " << dist_m << " m" << std::endl;
  std::cout << "tvec = " << tvec << std::endl;
}
```

> 💡 **课堂互动**：把 2D 点的横坐标整体扩大 1.5 倍（模拟装甲板变近），观察 `tvec[2]`（z 方向距离）怎么变；把内参 `fx` 改大，观察距离怎么变——理解内参对距离解算的影响。

## 四、工程实战：装甲板位姿解算

> 本节对应 `Visual Identity/sp_vision_25-main/tasks/auto_aim/solver.cpp` 的 `Solver::solve`，请边读边在编辑器里打开对照。

### 1. 整体流程

```
上一节课的 Armor（含 2D 四点、板型）
        │
        ▼
 cv::solvePnP（3D 模型点 + 2D 点 + 内参）
        │  → rvec, tvec（相机坐标系下）
        ▼
 Rodrigues(rvec) → R_armor2camera
        │
        ▼
 坐标系链：camera → gimbal → world
        │  → xyz_in_gimbal, xyz_in_world
        │  → ypr_in_gimbal, ypr_in_world
        ▼
 yaw 优化（重投影误差最小化）
        │
        ▼
 写回 Armor（供后续跟踪/瞄准使用）
```

### 2. 调用 solvePnP

核心求解只有几行，按板型选择对应的 3D 模型点：

```57:67:Visual Identity/sp_vision_25-main/tasks/auto_aim/solver.cpp
//solvePnP（获得姿态）
void Solver::solve(Armor & armor) const
{
  const auto & object_points =
    (armor.type == ArmorType::big) ? BIG_ARMOR_POINTS : SMALL_ARMOR_POINTS;

  cv::Vec3d rvec, tvec;
  cv::solvePnP(
    object_points, armor.points, camera_matrix_, distort_coeffs_, rvec, tvec, false,
    cv::SOLVEPNP_IPPE);

  Eigen::Vector3d xyz_in_camera;
  cv::cv2eigen(tvec, xyz_in_camera);
  armor.xyz_in_gimbal = R_camera2gimbal_ * xyz_in_camera + t_camera2gimbal_;
  armor.xyz_in_world = R_gimbal2world_ * armor.xyz_in_gimbal_;
```

> 📝 `armor.points` 就是上一节课 `Armor` 结构体里存的四个 2D 角点，这里直接喂给 `solvePnP`，实现了第三课和第四课的衔接。

### 3. 旋转向量 → 旋转矩阵

`solvePnP` 输出的 `rvec` 是旋转向量，要参与坐标系变换需先转成旋转矩阵：

```68:74:Visual Identity/sp_vision_25-main/tasks/auto_aim/solver.cpp
  cv::Mat rmat;
  cv::Rodrigues(rvec, rmat);
  Eigen::Matrix3d R_armor2camera;
  cv::cv2eigen(rmat, R_armor2camera);
  Eigen::Matrix3d R_armor2gimbal = R_camera2gimbal_ * R_armor2camera;
  Eigen::Matrix3d R_armor2world = R_gimbal2world_ * R_armor2gimbal;
  armor.ypr_in_gimbal = tools::eulers(R_armor2gimbal, 2, 1, 0);
  armor.ypr_in_world = tools::eulers(R_armor2world, 2, 1, 0);
```

`cv::Rodrigues` 把 \(3\times1\) 旋转向量转成 \(3\times3\) 旋转矩阵；`tools::eulers(R, 2, 1, 0)` 按 ZYX 顺序分解出 yaw/pitch/roll。

### 4. 坐标系链：camera → gimbal → world

PnP 解出的是"装甲板在相机坐标系下的位姿"，但自瞄要的是"装甲板在云台/世界坐标系下的位姿"。工程里通过两组外参串联：

\[
P^{gimbal} = R_{cam2gim} \cdot P^{cam} + t_{cam2gim}
\]
\[
P^{world} = R_{gim2world} \cdot P^{gimbal}
\]

- \(R_{cam2gim}, t_{cam2gim}\)：相机相对云台的安装关系（机械标定，写死在 yaml）
- \(R_{gim2world}\)：云台相对世界的关系（由 IMU 四元数实时更新）

`set_R_gimbal2world` 接收 IMU 四元数，把"IMU 体坐标系"的旋转转成"云台坐标系"的旋转：

```49:54:Visual Identity/sp_vision_25-main/tasks/auto_aim/solver.cpp
void Solver::set_R_gimbal2world(const Eigen::Quaterniond & q)
{
  Eigen::Matrix3d R_imubody2imuabs = q.toRotationMatrix();
  R_gimbal2world_ = R_gimbal2imubody_.transpose() * R_imubody2imuabs * R_gimbal2imubody_;
}
```

> 💡 **课堂互动**：把 `R_camera2gimbal` 设成单位阵、`t_camera2gimbal` 设成零，相当于"相机就装在云台中心"，观察解算结果是否退化回纯相机坐标系——理解外参的作用。

### 5. 相机标定参数（yaml）

相机内参、畸变、相机到云台的外参都写在配置文件里，构造函数一次性读入：

```31:44:Visual Identity/sp_vision_25-main/tasks/auto_aim/solver.cpp
Solver::Solver(const std::string & config_path) : R_gimbal2world_(Eigen::Matrix3d::Identity())
{
  auto yaml = YAML::LoadFile(config_path);

  auto R_gimbal2imubody_data = yaml["R_gimbal2imubody"].as<std::vector<double>>();
  auto R_camera2gimbal_data = yaml["R_camera2gimbal"].as<std::vector<double>>();
  auto t_camera2gimbal_data = yaml["t_camera2gimbal"].as<std::vector<double>>();
  R_gimbal2imubody_ = Eigen::Matrix<double, 3, 3, Eigen::RowMajor>(R_gimbal2imubody_data.data());
  R_camera2gimbal_ = Eigen::Matrix<double, 3, 3, Eigen::RowMajor>(R_camera2gimbal_data.data());
  t_camera2gimbal_ = Eigen::Matrix<double, 3, 1>(t_camera2gimbal_data.data());

  auto camera_matrix_data = yaml["camera_matrix"].as<std::vector<double>>();
  auto distort_coeffs_data = yaml["distort_coeffs"].as<std::vector<double>>();
  Eigen::Matrix<double, 3, 3, Eigen::RowMajor> camera_matrix(camera_matrix_data.data());
  Eigen::Matrix<double, 1, 5> distort_coeffs(distort_coeffs_data.data());
  cv::eigen2cv(camera_matrix, camera_matrix_);
  cv::eigen2cv(distort_coeffs, distort_coeffs_);
}
```

以 `configs/standard3.yaml` 为例，真实标定值长这样：

```yaml
camera_matrix: [1785.488, 0, 672.481, 0, 1785.026, 559.896, 0, 0, 1]
distort_coeffs: [-0.076, 0.112, 0.0005, -0.0028, 0]
R_camera2gimbal: [-0.027, -0.126, 0.992, -0.999, 0.020, -0.025, -0.017, -0.992, -0.127]
t_camera2gimbal: [0.132, 0.104, 0.025]
```

> ⚠️ `camera_matrix` 是 9 个数按行展开的 \(3\times3\) 矩阵：`[fx, 0, cx, 0, fy, cy, 0, 0, 1]`。`fx/fy` 是焦距（像素），`cx/cy` 是主点。不同相机/镜头标定值不同，**不能直接抄**。

### 6. 重投影误差与 yaw 优化

PnP 直接解出的 yaw 在装甲板接近正对时误差较大（四个角点对 yaw 不敏感）。工程里用"重投影误差最小化"二次优化：把装甲板按不同 yaw 重投影回图像，找误差最小的那个 yaw。

`reproject_armor` 负责把世界坐标系下的位姿重投影成像素点：

```90:124:Visual Identity/sp_vision_25-main/tasks/auto_aim/solver.cpp
std::vector<cv::Point2f> Solver::reproject_armor(
  const Eigen::Vector3d & xyz_in_world, double yaw, ArmorType type, ArmorName name) const
{
  auto sin_yaw = std::sin(yaw);
  auto cos_yaw = std::cos(yaw);

  auto pitch = (name == ArmorName::outpost) ? -15.0 * CV_PI / 180.0 : 15.0 * CV_PI / 180.0;
  auto sin_pitch = std::sin(pitch);
  auto cos_pitch = std::cos(pitch);

  // clang-format off
  const Eigen::Matrix3d R_armor2world {
    {cos_yaw * cos_pitch, -sin_yaw, cos_yaw * sin_pitch},
    {sin_yaw * cos_pitch,  cos_yaw, sin_yaw * sin_pitch},
    {         -sin_pitch,        0,           cos_pitch}
  };
  // clang-format on

  // get R_armor2camera t_armor2camera
  const Eigen::Vector3d & t_armor2world = xyz_in_world;
  Eigen::Matrix3d R_armor2camera =
    R_camera2gimbal_.transpose() * R_gimbal2world_.transpose() * R_armor2world;
  Eigen::Vector3d t_armor2camera =
    R_camera2gimbal_.transpose() * (R_gimbal2world_.transpose() * t_armor2world - t_camera2gimbal_);

  // get rvec tvec
  cv::Vec3d rvec;
  cv::Mat R_armor2camera_cv;
  cv::eigen2cv(R_armor2camera, R_armor2camera_cv);
  cv::Rodrigues(R_armor2camera_cv, rvec);
  cv::Vec3d tvec(t_armor2camera[0], t_armor2camera[1], t_armor2camera[2]);

  // reproject
  std::vector<cv::Point2f> image_points;
  const auto & object_points = (type == ArmorType::big) ? BIG_ARMOR_POINTS : SMALL_ARMOR_POINTS;
  cv::projectPoints(object_points, rvec, tvec, camera_matrix_, distort_coeffs_, image_points);
  return image_points;
}
```

`optimize_yaw` 在 ±70° 范围内每度搜索一次，取重投影误差最小的 yaw：

```198:216:Visual Identity/sp_vision_25-main/tasks/auto_aim/solver.cpp
void Solver::optimize_yaw(Armor & armor) const
{
  Eigen::Vector3d gimbal_ypr = tools::eulers(R_gimbal2world_, 2, 1, 0);

  constexpr double SEARCH_RANGE = 140;  // degree
  auto yaw0 = tools::limit_rad(gimbal_ypr[0] - SEARCH_RANGE / 2 * CV_PI / 180.0);

  auto min_error = 1e10;
  auto best_yaw = armor.ypr_in_world[0];

  for (int i = 0; i < SEARCH_RANGE; i++) {
    double yaw = tools::limit_rad(yaw0 + i * CV_PI / 180.0);
    auto error = armor_reprojection_error(armor, yaw, (i - SEARCH_RANGE / 2) * CV_PI / 180.0);

    if (error < min_error) {
      min_error = error;
      best_yaw = yaw;
    }
  }

  armor.yaw_raw = armor.ypr_in_world[0];
  armor.ypr_in_world[0] = best_yaw;
}
```

> 📝 平衡步兵（big + 3/4/5 号）不做 yaw 优化，因为其 pitch 假设不成立，`solve` 末尾会 `return` 跳过。

> 💡 **课堂互动**：把 `SEARCH_RANGE` 从 140 改到 20，观察远距离斜着对着的装甲板 yaw 是否还能解准——理解搜索范围与精度的权衡。

## 五、鲁棒性处理：多解与剪枝

> 本节对应 `Visual Identity/rmcs_auto_aim_v2-main/src/utility/math/solve_pnp/pnp_solution.cpp` 的 `RobustPnpSolution::solve`，展示了工程级 PnP 的健壮写法。

### 1. `solvePnPGeneric` 取多组解

`SOLVEPNP_IPPE` 对平面物体可能返回**两组解**（装甲板正反两面都满足投影约束）。用 `cv::solvePnPGeneric` 一次拿到所有解和对应重投影误差：

```106:114:Visual Identity/rmcs_auto_aim_v2-main/src/utility/math/solve_pnp/pnp_solution.cpp
    const auto success =
        cv::solvePnPGeneric(shape_points, armor_points, camera_matrix, distort_coeff, rota_vecs,
            tran_vecs, false, cv::SOLVEPNP_IPPE, cv::noArray(), cv::noArray(), errors);

    if (!success || rota_vecs.empty() || rota_vecs.size() != tran_vecs.size()) return false;
```

### 2. 剪枝一：去除背对相机的装甲板

两组解里往往有一组是"装甲板背面对着你"。用装甲板法向量和平移向量的点积判断朝向，背面的直接丢掉：

```124:148:Visual Identity/rmcs_auto_aim_v2-main/src/utility/math/solve_pnp/pnp_solution.cpp
    for (std::size_t index = 0; index < rota_vecs.size(); ++index) {
        if (tran_vecs[index][2] <= 0.0) continue;          // z 必须在相机前方

        auto rotation_opencv = cv::Mat { };
        cv::Rodrigues(rota_vecs[index], rotation_opencv);
        // ... 转成 Eigen ...

        // [剪枝] 去除背对着的装甲板
        const auto back_direction  = q_camera_armor * Eigen::Vector3d::UnitX();
        const auto camera_to_armor = t_camera_armor.normalized();
        if (back_direction.dot(camera_to_armor) <= 0.0) continue;
```

### 3. 剪枝二：pitch 合理性

装甲板的 pitch 在物理上有合理范围（前哨站 -15°，其他 +15°）。解出来的 pitch 偏离过多说明是错解：

```149:155:Visual Identity/rmcs_auto_aim_v2-main/src/utility/math/solve_pnp/pnp_solution.cpp
        // [剪枝] 去除 Odom 系下，Pitch 不合理的装甲板
        const auto expected = (armor2d.genre == ArmorGenre::OUTPOST) //
            ? kPredictedOutpostArmorPitch
            : kPredictedOtherArmorPitch;
        const auto pitch    = eulers(q_odom_armor, 2, 1, 0)[1];
        if (std::abs(pitch - expected) > util::deg2rad(40.0)) continue;
```

### 4. 选最优解

通过剪枝后，按重投影误差最小的原则选出最终解：

```156:166:Visual Identity/rmcs_auto_aim_v2-main/src/utility/math/solve_pnp/pnp_solution.cpp
        const auto error = index < errors.size() ? errors[index] : 0.0;
        if (error >= best_error) continue;

        auto armor = Armor3d { };

        armor.translation = t_odom_armor;
        armor.orientation = q_odom_armor;

        best_armor = armor;
        best_error = error;
    }
```

> 💡 **课堂互动**：把"背对剪枝"那行注释掉，对着装甲板背面拍一张，看是否会解出"穿过去的"位姿——理解多解筛选的必要性。

## 六、实战操作：把解算链路跑起来

### 1. 课堂练习清单

1. **跑通最小示例**：把第三节第 4 小节的 PnP 示例代码编译运行，确认能输出距离值。
2. **接入真实检测点**：把上一节课 `armor_detect` 输出的四个 2D 角点（从日志里抄一组）填进示例的 `img`，对比解算距离和实际距离。
3. **换内参**：用 `configs/standard3.yaml` 和 `configs/sentry.yaml` 两套内参分别解算同一组点，观察距离差异——理解标定的重要性。
4. **画坐标轴**：调用下面的 `draw_pose_axes` 在装甲板上画 XYZ 轴，直观看位姿是否正确：

```cpp
void draw_pose_axes(cv::Mat & image, const cv::Mat & K, const cv::Mat & dist,
                    const cv::Vec3d & rvec, const cv::Vec3d & tvec, float length = 0.1) {
  std::vector<cv::Point3f> axis = {
    {0, 0, 0}, {length, 0, 0}, {0, length, 0}, {0, 0, length}};
  std::vector<cv::Point2f> proj;
  cv::projectPoints(axis, rvec, tvec, K, dist, proj);
  cv::line(image, proj[0], proj[1], cv::Scalar(0, 0, 255), 3);   // X-红
  cv::line(image, proj[0], proj[2], cv::Scalar(0, 255, 0), 3);   // Y-绿
  cv::line(image, proj[0], proj[3], cv::Scalar(255, 0, 0), 3);   // Z-蓝
}
```

5. **重投影验证**：把解出的 `rvec/tvec` 用 `cv::projectPoints` 投回图像，和原始 2D 点对比，算平均像素误差——这是判断 PnP 是否解准的最直接方法。

### 2. 数据流回顾

```
第三课输出：Armor（2D 四点、板型）
        │
        ▼
 Solver::solve
        │  ├─ cv::solvePnP → rvec, tvec（相机系）
        │  ├─ Rodrigues → R
        │  ├─ camera → gimbal → world（外参链）
        │  └─ optimize_yaw（重投影优化）
        ▼
 Armor（xyz_in_world, ypr_in_world）→ 交给后续跟踪/瞄准
```

### 3. 课后任务

1. **基础任务**：完成相机标定，获取自己摄像头（或虚拟机内摄像头）的内参和畸变系数。
2. **核心任务**：用给定的相机内参和一张装甲板图片，结合上一节课的检测器输出四个角点，给出装甲板的旋转矩阵和平移向量，并算出距离。
3. **进阶任务**：实现重投影误差计算，当误差 > 5 像素时在图像上标红警告，作为 PnP 解算质量的自检机制。
4. **思考题**：为什么装甲板斜着对着相机时，直接 PnP 解出的 yaw 误差大？yaw 优化为什么能缓解？（提示：四个角点在 yaw 方向的投影位移不敏感。）

**下一节预告**：我们将学习目标跟踪算法，包括卡尔曼滤波和各种跟踪器的原理与应用，让自瞄系统能稳定跟踪移动目标。
