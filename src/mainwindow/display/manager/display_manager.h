/*
 * @Author: chengyang chengyangkj@outlook.com
 * @Date: 2023-03-29 14:21:31
 * @LastEditors: chengyangkj chengyangkj@qq.com
 * @LastEditTime: 2023-10-15 09:31:36
 * @FilePath:
 * ////include/display/display_manager.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置
 * 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#pragma once
#include "algorithm.h"

#include <Eigen/Dense>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QLabel>
#include <QPushButton>
#include <QtWidgets/QVBoxLayout>
#include <any>
#include <functional>
#include <map>
#include "config/config_manager.h"
#include "map/topology_map.h"
#include "display/manager/view_manager.h"
#include "display/display_cost_map.h"
#include "display/manager/display_factory.h"
#include "display/display_occ_map.h"
#include "display/display_path.h"
#include "display/laser_points.h"
#include "display/point_shape.h"
#include "display/robot_shape.h"
#include "widgets/set_pose_widget.h"
#include "core/framework/message_bus.h"
// group
#define GROUP_MAP "Group_Map"
namespace Display {

class SceneManager;
class DisplayManager : public QObject {
  Q_OBJECT

 public:
  DisplayManager();
  ~DisplayManager();

  // ── 显示图层访问 ──────────────────────────────────
  QGraphicsView *GetViewPtr() { return graphics_view_ptr_; }
  VirtualDisplay *GetDisplay(const std::string &name);
  bool SetDisplayConfig(const std::string &config_name, const std::any &data);
  void SetFocusOn(const std::string &display_type) {
    focus_display_ = display_type;
  }

  // ── 坐标系转换（world / scene / map 三套坐标互转）────
  QPointF wordPose2Scene(const QPointF &point);
  RobotPose wordPose2Scene(const RobotPose &point);
  QPointF wordPose2Map(const QPointF &pose);
  RobotPose wordPose2Map(const RobotPose &pose);
  RobotPose mapPose2Word(const RobotPose &pose);
  RobotPose scenePoseToWord(const RobotPose &pose);
  RobotPose scenePoseToMap(const RobotPose &pose);

  // ── 机器人位姿 ──────────────────────────────────
  void UpdateRobotPose(const RobotPose &pose);
  RobotPose GetRobotPose() { return robot_pose_; }

  // ── 地图数据 ────────────────────────────────────
  OccupancyMap &GetMap();

  // ── 交互模式切换 ─────────────────────────────────
  void SetRelocMode(bool is_move);
  void SetNavGoalMode(bool is_start);

 signals:
  // ── 鼠标 / 位姿发布 ─────────────────────────────
  void cursorPosMap(QPointF);
  void signalPub2DPose(const RobotPose &pose);   // 重定位位姿发布
  void signalPub2DGoal(const RobotPose &pose);   // 导航目标点发布

  // ── 地图相关信号 ─────────────────────────────────
  void signalPubMap(const OccupancyMap &map);
  void signalEditMapModeChanged(MapEditMode mode);
  void signalTopologyMapUpdate(const TopologyMap &map);
  void signalCurrentSelectPointChanged(const TopologyMap::PointInfo &);

 public slots:
  // ── 视图缩放 ────────────────────────────────────
  void updateScaled(double value);
  void SetScaleBig();
  void SetScaleSmall();
  void FocusDisplay(const std::string &display_type);

  // ── 重定位 ──────────────────────────────────────
  void StartReloc();
  void slotSetRobotPose(const RobotPose &pose);

  // ── 地图编辑 ────────────────────────────────────
  void SetEditMapMode(MapEditMode mode);
  void SetToolRange(double range);
  double GetEraserRange() const;
  double GetPenRange() const;

  // ── 导航点管理 ──────────────────────────────────
  void AddOneNavPoint();
  void AddPointAtRobotPosition();

  // ── 机器人位姿（场景层回调）─────────────────────
  void slotRobotScenePoseChanged(const RobotPose &pose);

  // ── 地图数据读写 ────────────────────────────────
  OccupancyMap GetOccupancyMap();
  void UpdateOCCMap(const OccupancyMap &map);
  TopologyMap GetTopologyMap();
  void UpdateTopologyMap(const TopologyMap &topology_map);

 private:
  // ── 内部初始化 ──────────────────────────────────
  void InitUi();
  std::vector<Point> transLaserPoint(const std::vector<Point> &point);

  // ── UI 组件 ────────────────────────────────────
  ViewManager *graphics_view_ptr_;
  SetPoseWidget *set_reloc_pose_widget_;
  SceneManager *scene_manager_ptr_;
  QPushButton *btn_move_focus_;

  // ── 状态标志 ────────────────────────────────────
  bool init_flag_{false};
  bool is_reloc_mode_{false};
  double global_scal_value_ = 1;
  std::string focus_display_;

  // ── 核心数据 ────────────────────────────────────
  std::map<std::string, std::any> display_map_;   // 图层名 -> 图层实例
  Framework::MessageBus::CallbackId topology_map_sub_id_{0};
  Framework::MessageBus::CallbackId occupancy_map_sub_id_{0};
  Framework::MessageBus::CallbackId robot_pose_sub_id_{0};
  Framework::MessageBus::CallbackId laser_scan_sub_id_{0};
  RobotPose robot_pose_{0, 0, 0};
  RobotPose robot_pose_goal_{0, 0, 0};
  OccupancyMap map_data_;
  RobotPose local_cost_world_pose_;
  OccupancyMap local_cost_map_;
};

}  // namespace Display
