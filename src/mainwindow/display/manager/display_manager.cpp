// 坐标系说明：
// 1. scenePose  — QGraphicsScene 全局坐标系（图元坐标系）
// 2. 图层坐标系 — 每个 QGraphicsItem 的局部坐标系
// 3. occPose    — 栅格地图像素坐标系（行列）
// 4. wordPose   — 机器人世界坐标系（米，ROS map frame）
#ifdef constant
#undef constant
#endif
#include "display/manager/display_manager.h"
#include "display/point_shape.h"
#include "display/laser_points.h"
#include <Eigen/Eigen>
#include <QOpenGLWidget>
#include "algorithm.h"
#include "display/manager/scene_manager.h"
#include "logger/logger.h"
#include "core/framework/framework.h"
namespace Display {

// ══════════════════════════════════════════════════════════════
//  构造 / 析构
// ══════════════════════════════════════════════════════════════

DisplayManager::DisplayManager() {
  graphics_view_ptr_ = new ViewManager();
  scene_manager_ptr_ = new SceneManager();
  scene_manager_ptr_->Init(graphics_view_ptr_, this);
  graphics_view_ptr_->SetDisplayManagerPtr(this);
  // 设置绘制区域
  FactoryDisplay::Instance()->Init(graphics_view_ptr_, scene_manager_ptr_);
  connect(scene_manager_ptr_,
          SIGNAL(signalTopologyMapUpdate(const TopologyMap &)), this,
          SIGNAL(signalTopologyMapUpdate(const TopologyMap &)));
  connect(
      scene_manager_ptr_,
      SIGNAL(signalCurrentSelectPointChanged(const TopologyMap::PointInfo &)),
      this,
      SIGNAL(signalCurrentSelectPointChanged(const TopologyMap::PointInfo &)));
  connect(scene_manager_ptr_,
          SIGNAL(signalEditMapModeChanged(MapEditMode)), this,
          SIGNAL(signalEditMapModeChanged(MapEditMode)));

  // ── 注册显示图层 ──────────────────────────────────
  (new DisplayOccMap(DISPLAY_MAP, 1));
  (new DisplayCostMap(DISPLAY_GLOBAL_COST_MAP, 2, DISPLAY_MAP));
  (new DisplayCostMap(DISPLAY_LOCAL_COST_MAP, 3, DISPLAY_MAP));
  (new PointShape(PointShape::ePointType::kRobot, DISPLAY_ROBOT, DISPLAY_ROBOT,
                  9, DISPLAY_MAP))
      ->SetRotateEnable(true);
  new LaserPoints(DISPLAY_LASER, 2, DISPLAY_MAP);
  new DisplayPath(DISPLAY_GLOBAL_PATH, 6, DISPLAY_MAP);
  new DisplayPath(DISPLAY_LOCAL_PATH, 6, DISPLAY_MAP);
  new RobotShape(DISPLAY_ROBOT_FOOTPRINT, 8, DISPLAY_MAP);

  // ── 默认图层配置 ──────────────────────────────────
  SetDisplayConfig(DISPLAY_GLOBAL_PATH + "/Color", Color(0, 0, 255));
  SetDisplayConfig(DISPLAY_LOCAL_PATH + "/Color", Color(0, 255, 0));

  // ── 信号连接 ──────────────────────────────────────
  connect(GetDisplay(DISPLAY_ROBOT),
          SIGNAL(signalPoseUpdate(const RobotPose &)), this,
          SLOT(slotRobotScenePoseChanged(const RobotPose &)));
  // 设置默认地图图层响应鼠标事件
  FactoryDisplay::Instance()->SetMoveEnable(DISPLAY_MAP);
  InitUi();

  // ── 消息订阅 ──────────────────────────────────────
  topology_map_sub_id_ = SUBSCRIBE(MSG_ID_TOPOLOGY_MAP, [this](const TopologyMap& data) {
    scene_manager_ptr_->UpdateTopologyMap(data);
  });

  occupancy_map_sub_id_ = SUBSCRIBE(MSG_ID_OCCUPANCY_MAP, [this](const OccupancyMap& data) {
    map_data_ = data;
  });

  robot_pose_sub_id_ = SUBSCRIBE(MSG_ID_ROBOT_POSE, [this](const RobotPose& data) {
    if (!is_reloc_mode_) {
      UpdateRobotPose(data);
    }
  });

  laser_scan_sub_id_ = SUBSCRIBE(MSG_ID_LASER_SCAN, [this](const LaserScan& data) {
    auto* laser_display = dynamic_cast<LaserPoints*>(GetDisplay(DISPLAY_LASER));
    if (laser_display) {
      std::vector<Point> transformed_points = transLaserPoint(data.data);
      laser_display->UpdateLaserData(data.id, transformed_points);
    }
  });
}

DisplayManager::~DisplayManager() {
  if (topology_map_sub_id_ != 0) {
    UNSUBSCRIBE(MSG_ID_TOPOLOGY_MAP, topology_map_sub_id_);
    topology_map_sub_id_ = 0;
  }
  if (occupancy_map_sub_id_ != 0) {
    UNSUBSCRIBE(MSG_ID_OCCUPANCY_MAP, occupancy_map_sub_id_);
    occupancy_map_sub_id_ = 0;
  }
  if (robot_pose_sub_id_ != 0) {
    UNSUBSCRIBE(MSG_ID_ROBOT_POSE, robot_pose_sub_id_);
    robot_pose_sub_id_ = 0;
  }
  if (laser_scan_sub_id_ != 0) {
    UNSUBSCRIBE(MSG_ID_LASER_SCAN, laser_scan_sub_id_);
    laser_scan_sub_id_ = 0;
  }
}

// ══════════════════════════════════════════════════════════════
//  显示图层访问
// ══════════════════════════════════════════════════════════════

VirtualDisplay *DisplayManager::GetDisplay(const std::string &name) {
  return FactoryDisplay::Instance()->GetDisplay(name);
}

bool DisplayManager::SetDisplayConfig(const std::string &config_name,
                               const std::any &data) {
  QString q_config_name = QString::fromStdString(config_name);
  auto config_list = q_config_name.split("/");
  if (config_list.empty() || config_list.size() != 2) {
    return false;
  }
  VirtualDisplay *display = GetDisplay(config_list[0].toStdString());
  if (!display) {
    std::cout << "error current display not fi csxnd:"
              << config_list[0].toStdString() << " config_name:" << config_name
              << std::endl;
    return false;
  }
  // 设置图层是否响应鼠标事件
  if (config_list[1] == "MouseEvent") {
    bool is_response;
    GetAnyData(bool, data, is_response);
    display->SetMoveEnable(is_response);
    std::cout << "config:" << config_name << " res:" << is_response
              << std::endl;
  }
  return display->SetDisplayConfig(config_list[1].toStdString(), data);
}

// ══════════════════════════════════════════════════════════════
//  坐标系转换（world / scene / map 三套坐标互转）
// ══════════════════════════════════════════════════════════════

// world → scene（全局图元坐标）
RobotPose DisplayManager::wordPose2Scene(const RobotPose &point) {
  double x, y;
  map_data_.xy2ScenePose(point.x, point.y, x, y);
  QPointF pose = FactoryDisplay::Instance()
                   ->GetDisplay(DISPLAY_MAP)
                     ->PoseToScene(QPointF(x, y));
  RobotPose res;
  res.x = pose.x();
  res.y = pose.y();
  res.theta = point.theta;
  return res;
}

QPointF DisplayManager::wordPose2Scene(const QPointF &point) {
  double x, y;
  map_data_.xy2ScenePose(point.x(), point.y(), x, y);
  return FactoryDisplay::Instance()
      ->GetDisplay(DISPLAY_MAP)
      ->PoseToScene(QPointF(x, y));
}

// world → map（栅格像素坐标）
RobotPose DisplayManager::wordPose2Map(const RobotPose &pose) {
  RobotPose ret = pose;
  double x, y;
  map_data_.xy2ScenePose(pose.x, pose.y, x, y);
  ret.x = x;
  ret.y = y;
  return ret;
}

QPointF DisplayManager::wordPose2Map(const QPointF &pose) {
  QPointF ret;
  double x, y;
  map_data_.xy2ScenePose(pose.x(), pose.y(), x, y);
  ret.setX(x);
  ret.setY(y);
  return ret;
}

// map → world
RobotPose DisplayManager::mapPose2Word(const RobotPose &pose) {
  RobotPose ret = pose;
  double x, y;
  map_data_.ScenePose2xy(pose.x, pose.y, x, y);
  ret.x = x;
  ret.y = y;
  return ret;
}

// scene → world
RobotPose DisplayManager::scenePoseToWord(const RobotPose &pose) {
  QPointF pose_map = FactoryDisplay::Instance()
                         ->GetDisplay(DISPLAY_MAP)
                         ->mapFromScene(QPointF(pose.x, pose.y));
  return mapPose2Word(RobotPose(pose_map.x(), pose_map.y(), pose.theta));
}

// scene → map
RobotPose DisplayManager::scenePoseToMap(const RobotPose &pose) {
  QPointF pose_map = FactoryDisplay::Instance()
                         ->GetDisplay(DISPLAY_MAP)
                         ->mapFromScene(QPointF(pose.x, pose.y));
  return RobotPose(pose_map.x(), pose_map.y(), pose.theta);
}

// ══════════════════════════════════════════════════════════════
//  机器人位姿
// ══════════════════════════════════════════════════════════════

void DisplayManager::UpdateRobotPose(const RobotPose &pose) {
  robot_pose_ = pose;
  auto* robot_display = dynamic_cast<PointShape*>(GetDisplay(DISPLAY_ROBOT));
  if (robot_display) {
    robot_display->UpdateData(wordPose2Map(pose));
    robot_display->update();
  }
}

// ══════════════════════════════════════════════════════════════
//  地图数据
// ══════════════════════════════════════════════════════════════

OccupancyMap &DisplayManager::GetMap() { return map_data_; }

// ══════════════════════════════════════════════════════════════
//  交互模式切换
// ══════════════════════════════════════════════════════════════

void DisplayManager::SetRelocMode(bool is_start) {
  is_reloc_mode_ = is_start;
  if (is_start) {
    FocusDisplay("");
    set_reloc_pose_widget_->SetPose(
        RobotPose(robot_pose_.x, robot_pose_.y, robot_pose_.theta));
    auto current_scene = GetDisplay(DISPLAY_ROBOT)->scenePos();
    QPointF view_pos = graphics_view_ptr_->mapFromScene(current_scene);
    set_reloc_pose_widget_->move(QPoint(view_pos.x()+10, view_pos.y()+10));
    set_reloc_pose_widget_->show();
  } else {
    set_reloc_pose_widget_->hide();
  }
  FactoryDisplay::Instance()->SetMoveEnable(DISPLAY_ROBOT, is_start);
}

void DisplayManager::SetNavGoalMode(bool is_start) {
  // TODO: 导航目标模式实现
}

// ══════════════════════════════════════════════════════════════
//  Slots — 视图缩放
// ══════════════════════════════════════════════════════════════

void DisplayManager::updateScaled(double value) {
  FactoryDisplay::Instance()->SetDisplayScaled(DISPLAY_LASER, value);
}

void DisplayManager::SetScaleBig() {
  FactoryDisplay::Instance()
      ->GetDisplay(DISPLAY_MAP)
      ->SetScaled(
          FactoryDisplay::Instance()->GetDisplay(DISPLAY_MAP)->GetScaleValue() *
          1.3);
}

void DisplayManager::SetScaleSmall() {
  FactoryDisplay::Instance()
      ->GetDisplay(DISPLAY_MAP)
      ->SetScaled(
          FactoryDisplay::Instance()->GetDisplay(DISPLAY_MAP)->GetScaleValue() *
          0.7);
}

void DisplayManager::FocusDisplay(const std::string &display_name) {
  FactoryDisplay::Instance()->SetFocusDisplay(display_name);
}

// ══════════════════════════════════════════════════════════════
//  Slots — 重定位
// ══════════════════════════════════════════════════════════════

void DisplayManager::StartReloc() {
  if (!set_reloc_pose_widget_->isVisible()) {
    SetRelocMode(true);
  }
}

void DisplayManager::slotSetRobotPose(const RobotPose &pose) {
  FactoryDisplay::Instance()->SetMoveEnable(DISPLAY_ROBOT, false);
  UpdateRobotPose(pose);
  // 300ms 后恢复拖拽，避免 widget 回调和拖拽冲突
  QTimer::singleShot(300, [this]() {
    FactoryDisplay::Instance()->SetMoveEnable(DISPLAY_ROBOT, true);
  });
}

// ══════════════════════════════════════════════════════════════
//  Slots — 地图编辑
// ══════════════════════════════════════════════════════════════

void DisplayManager::SetEditMapMode(MapEditMode mode) { scene_manager_ptr_->SetEditMapMode(mode); }
void DisplayManager::SetToolRange(double range) { scene_manager_ptr_->SetToolRange(range); }
double DisplayManager::GetEraserRange() const { return scene_manager_ptr_->GetEraserRange(); }
double DisplayManager::GetPenRange() const { return scene_manager_ptr_->GetPenRange(); }

// ══════════════════════════════════════════════════════════════
//  Slots — 导航点管理
// ══════════════════════════════════════════════════════════════

void DisplayManager::AddOneNavPoint() { scene_manager_ptr_->AddOneNavPoint(); }
void DisplayManager::AddPointAtRobotPosition() { scene_manager_ptr_->AddPointAtRobotPosition(); }

// ══════════════════════════════════════════════════════════════
//  Slots — 机器人位姿（场景层回调）
// ══════════════════════════════════════════════════════════════

// 重定位模式下，用户拖拽机器人图标时实时更新坐标和 widget 位置
void DisplayManager::slotRobotScenePoseChanged(const RobotPose &pose) {
  if (is_reloc_mode_) {
    QPointF occ_pose =
        GetDisplay(DISPLAY_MAP)->mapFromScene(QPointF(pose.x, pose.y));
    double x, y;
    map_data_.ScenePose2xy(occ_pose.x(), occ_pose.y(), x, y);
    robot_pose_.x = x;
    robot_pose_.y = y;
    robot_pose_.theta = pose.theta;
    QPointF view_pos =
        graphics_view_ptr_->mapFromScene(QPointF(pose.x, pose.y));
    set_reloc_pose_widget_->move(QPoint(view_pos.x()+10, view_pos.y()+10));
    set_reloc_pose_widget_->SetPose(
        RobotPose(robot_pose_.x, robot_pose_.y, robot_pose_.theta));
  }
}

// ══════════════════════════════════════════════════════════════
//  Slots — 地图数据读写
// ══════════════════════════════════════════════════════════════

OccupancyMap DisplayManager::GetOccupancyMap() {
  auto display_map_ = static_cast<DisplayOccMap *>(FactoryDisplay::Instance()->GetDisplay(DISPLAY_MAP));
  if (display_map_ != nullptr) {
    return display_map_->GetOccupancyMap();
  }
  return map_data_;
}

void DisplayManager::UpdateOCCMap(const OccupancyMap &map) {
  PUBLISH(MSG_ID_OCCUPANCY_MAP, map);
}

TopologyMap DisplayManager::GetTopologyMap() {
  return scene_manager_ptr_->GetTopologyMap();
}

void DisplayManager::UpdateTopologyMap(const TopologyMap &topology_map) {
  PUBLISH(MSG_ID_TOPOLOGY_MAP, topology_map);
  QTimer::singleShot(500, [this]() {
    SetScaleBig();
  });
}

// ══════════════════════════════════════════════════════════════
//  Private — 内部初始化 / 工具函数
// ══════════════════════════════════════════════════════════════

void DisplayManager::InitUi() {
  set_reloc_pose_widget_ = new SetPoseWidget(graphics_view_ptr_);
  set_reloc_pose_widget_->hide();
  connect(set_reloc_pose_widget_, &SetPoseWidget::SignalHandleOver,
          [this](const bool &is_submit, const RobotPose &pose) {
            SetRelocMode(false);
            if (is_submit) {
              emit signalPub2DPose(pose);
            }
          });
  connect(set_reloc_pose_widget_, SIGNAL(SignalPoseChanged(const RobotPose &)),
          this, SLOT(slotSetRobotPose(const RobotPose &)));
}

// 将车身坐标系下的激光点转换为栅格地图像素坐标
std::vector<Point>
DisplayManager::transLaserPoint(const std::vector<Point> &point) {
  std::vector<Point> res;
  for (auto one_point : point) {
    // 车身坐标 → 世界坐标
    basic::RobotPose map_pose = basic::absoluteSum(
        basic::RobotPose(robot_pose_.x, robot_pose_.y, robot_pose_.theta),
        basic::RobotPose(one_point.x, one_point.y, 0));
    // 世界坐标 → 栅格像素坐标
    double x, y;
    map_data_.xy2ScenePose(map_pose.x, map_pose.y, x, y);
    res.push_back(Point(x, y));
  }
  return res;
}

}  // namespace Display
