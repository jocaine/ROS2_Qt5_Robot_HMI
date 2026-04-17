#ifndef MAINWINDOW_H
#define MAINWINDOW_H

// ── Qt 核心 ──
#include <QMainWindow>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QWidgetAction>

// ── Qt 扩展（头文件中实际用到的类型）──
#include <QThread>
#include <memory>
#include <map>
#include <string>
#include <any>

// ── 第三方：Dock 框架 ──
#include "DockAreaWidget.h"
#include "DockManager.h"
#include "DockWidget.h"

// ── OpenCV ──
#include <opencv2/imgproc/imgproc.hpp>

// ── 项目模块 ──
#include "channel_manager.h"
#include "config/config_manager.h"
#include "display/manager/display_manager.h"
#include "point_type.h"
#include "widgets/dashboard.h"
#include "widgets/nav_goal_table_view.h"
#include "widgets/speed_ctrl.h"
#include "widgets/ratio_layouted_frame.h"
#include "widgets/map_toolbar_widget.h"
#include "widgets/edit_toolbar_widget.h"
#include "widgets/task_list_panel_widget.h"
#include "core/framework/framework.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

/**
 * @brief 主窗口类
 *
 * 职责：
 *   - UI 布局初始化（工具栏、Dock 面板、信号槽连接）
 *   - 通信通道管理（打开/关闭/注册消息订阅）
 *   - 地图加载与保存
 *   - 窗口状态持久化
 */
class MainWindow : public QMainWindow {
  Q_OBJECT

 public:
  MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

 // ── 槽函数 / 数据接收 ──────────────────────────────────────
 public slots:
  void signalCursorPose(QPointF pos);
  void RecvChannelMsg(const MsgId &id, const std::any &data);
  void updateOdomInfo(RobotState state);
  void RestoreState();
  void SlotSetBatteryStatus(double percent, double voltage);
  void SlotRecvImage(const std::string &location, std::shared_ptr<cv::Mat> data);

 // ── 事件处理 ───────────────────────────────────────────────
 protected:
  void closeEvent(QCloseEvent *event) override;

 // ── 信号 ───────────────────────────────────────────────────
 signals:
  void OnRecvChannelData(const MsgId &id, const std::any &data);

 // ── UI 初始化 ──────────────────────────────────────────────
 private:
  void setupUi();
  void initGlobalStyle();
  struct MapToolBarButtons;
  struct EditToolBarWidgets;
  MapToolBarButtons createMapToolBar(QVBoxLayout* center_layout);
  EditToolBarWidgets createEditToolBar(QHBoxLayout* center_h_layout);
  void createDockPanels(QVBoxLayout* center_layout, QHBoxLayout* center_h_layout);
  void connectSignals(const MapToolBarButtons& tb, const EditToolBarWidgets& et);

 // ── 通信通道管理 ───────────────────────────────────────────
 private:
  bool openChannel();
  bool openChannel(const std::string &channel_name);
  void closeChannel();
  void registerChannel();

 // ── 窗口状态持久化 / 地图操作 ──────────────────────────────
 private:
  void SaveState();
  bool LoadMap(const std::string& file_path);

 // ── 成员变量 ───────────────────────────────────────────────
 private:
  Ui::MainWindow *ui;

  // Dock 框架
  ads::CDockManager *dock_manager_;
  ads::CDockAreaWidget *center_docker_area_;

  // 通信
  ChannelManager channel_manager_;
  Framework::MessageBus::CallbackId odom_pose_sub_id_{0};
  Framework::MessageBus::CallbackId robot_pose_sub_id_{0};
  Framework::MessageBus::CallbackId battery_state_sub_id_{0};
  Framework::MessageBus::CallbackId image_sub_id_{0};

  // 显示
  Display::DisplayManager *display_manager_{nullptr};
  DashBoard *speed_dash_board_{nullptr};
  SpeedCtrlWidget *speed_ctrl_widget_{nullptr};
  NavGoalTableView *nav_goal_table_view_{nullptr};
  std::map<std::string, RatioLayoutedFrame *> image_frame_map_;

  // 静态UI包装控件
  MapToolBarWidget *map_toolbar_widget_{nullptr};
  EditToolBarWidget *edit_toolbar_widget_{nullptr};
  TaskListPanelWidget *task_list_panel_widget_{nullptr};

  // 状态栏控件
  QProgressBar *battery_bar_;
  QLabel *label_power_;

  // 地图
  std::string map_path_{"./map"};
};

#endif  // MAINWINDOW_H
