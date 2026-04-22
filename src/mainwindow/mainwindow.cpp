/**
 * @file mainwindow.cpp
 * @brief MainWindow 主窗口实现
 *
 * 职责：
 *   - UI 布局初始化（工具栏、Dock 面板、信号槽连接）
 *   - 通信通道管理（打开/关闭/注册消息订阅）
 *   - 地图加载与保存
 *   - 窗口状态持久化
 */

#include "mainwindow.h"
#include <QDebug>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <QFileInfo>
#include <QFile>
#include "AutoHideDockContainer.h"
#include "DockAreaTabBar.h"
#include "DockAreaTitleBar.h"
#include "DockAreaWidget.h"
#include "DockComponentsFactory.h"
#include "Eigen/Dense"
#include "FloatingDockContainer.h"
#include "algorithm.h"
#include "logger/logger.h"
#include "ui_mainwindow.h"
#include <QButtonGroup>
#include <QMessageBox>
#include <QApplication>

namespace {
QString loadAppStyleSheet() {
  QFile file(":/qss/app.qss");
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    return {};
  }
  return QString::fromUtf8(file.readAll());
}
}  // namespace

#include "widgets/speed_ctrl.h"
#include "widgets/display_config_widget.h"
#include "display/manager/view_manager.h"
#include <QTimer>
using namespace ads;


////////////////////////////////////////////////////////////////////////////
/// 构造 / 析构
////////////////////////////////////////////////////////////////////////////

// 初始化主窗口：注册 Qt 元类型 → 构建 UI → 打开通信通道 → 恢复窗口状态 → 加载上次地图
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
  Q_INIT_RESOURCE(images);
  Q_INIT_RESOURCE(media);
  Q_INIT_RESOURCE(styles);
  LOG_INFO(" MainWindow init thread id" << QThread::currentThreadId());
  qRegisterMetaType<std::string>("std::string");
  qRegisterMetaType<RobotPose>("RobotPose");
  qRegisterMetaType<RobotSpeed>("RobotSpeed");
  qRegisterMetaType<RobotState>("RobotState");
  qRegisterMetaType<OccupancyMap>("OccupancyMap");
  qRegisterMetaType<OccupancyMap>("OccupancyMap");
  qRegisterMetaType<LaserScan>("LaserScan");
  qRegisterMetaType<RobotPath>("RobotPath");
  qRegisterMetaType<MsgId>("MsgId");
  qRegisterMetaType<std::any>("std::any");
  qRegisterMetaType<TopologyMap>("TopologyMap");
  qRegisterMetaType<TopologyMap::PointInfo>("TopologyMap::PointInfo");
  setupUi();
  openChannel();
  QTimer::singleShot(50, [=]() { 
    RestoreState();
    std::string map_path = Config::ConfigManager::Instance()->GetRootConfig().map_config.path;
    if (!map_path.empty()) {
      std::string yaml_path = map_path;
      if (yaml_path.find(".yaml") == std::string::npos && yaml_path.find(".yml") == std::string::npos) {
        yaml_path += ".yaml";
      }
      if (QFile::exists(QString::fromStdString(yaml_path))) {
        LoadMap(yaml_path);
      }
    }
  });
}

MainWindow::~MainWindow() {
  if (odom_pose_sub_id_ != 0) {
    UNSUBSCRIBE(MSG_ID_ODOM_POSE, odom_pose_sub_id_);
    odom_pose_sub_id_ = 0;
  }
  if (robot_pose_sub_id_ != 0) {
    UNSUBSCRIBE(MSG_ID_ROBOT_POSE, robot_pose_sub_id_);
    robot_pose_sub_id_ = 0;
  }
  if (battery_state_sub_id_ != 0) {
    UNSUBSCRIBE(MSG_ID_BATTERY_STATE, battery_state_sub_id_);
    battery_state_sub_id_ = 0;
  }
  if (image_sub_id_ != 0) {
    UNSUBSCRIBE(MSG_ID_IMAGE, image_sub_id_);
    image_sub_id_ = 0;
  }
  delete ui;
}

// 工具栏按钮集合（用于跨方法传递局部控件指针）
struct MainWindow::MapToolBarButtons {
  QToolButton *reloc = nullptr;
  QToolButton *edit_map = nullptr;
  QToolButton *open_map = nullptr;
  QToolButton *save_map = nullptr;
  QToolButton *re_save_map = nullptr;
};

struct MainWindow::EditToolBarWidgets {
  QWidget *container = nullptr;
  QToolButton *normal_cursor = nullptr;
  QToolButton *add_point = nullptr;
  QToolButton *add_topology_path = nullptr;
  QToolButton *add_region = nullptr;
  QToolButton *erase = nullptr;
  QToolButton *draw_pen = nullptr;
  QToolButton *draw_line = nullptr;
};


////////////////////////////////////////////////////////////////////////////
/// UI 初始化
////////////////////////////////////////////////////////////////////////////

// UI 初始化入口：调度各子方法完成完整布局
void MainWindow::setupUi() {
  ui->setupUi(this);
  initGlobalStyle();
  auto toolbar_btns = createMapToolBar(ui->verticalLayout);
  auto edit_widgets = createEditToolBar(ui->centerHLayout);
  createDockPanels(ui->verticalLayout, ui->centerHLayout);
  connectSignals(toolbar_btns, edit_widgets);
}

// 全局样式 + DockManager 配置
void MainWindow::initGlobalStyle() {
  qApp->setStyleSheet(loadAppStyleSheet());

  CDockManager::setConfigFlag(CDockManager::OpaqueSplitterResize, true);
  CDockManager::setConfigFlag(CDockManager::XmlCompressionEnabled, false);
  CDockManager::setConfigFlag(CDockManager::FocusHighlighting, true);
  CDockManager::setConfigFlag(CDockManager::DockAreaHasUndockButton, false);
  CDockManager::setConfigFlag(CDockManager::DockAreaHasTabsMenuButton, false);
  CDockManager::setConfigFlag(CDockManager::MiddleMouseButtonClosesTab, true);
  CDockManager::setConfigFlag(CDockManager::EqualSplitOnInsertion, true);
  CDockManager::setConfigFlag(CDockManager::ShowTabTextOnlyForActiveTab, true);
  // X11 native title bar hands drag events to the WM, breaking ADS drop overlays
  CDockManager::setConfigFlag(CDockManager::FloatingContainerForceQWidgetTitleBar, true);
  // X11 compositing fails to repaint behind frameless windows during drag; use window-frame preview
  CDockManager::setConfigFlag(CDockManager::DragPreviewShowsContentPixmap, true);
  CDockManager::setConfigFlag(CDockManager::DragPreviewHasWindowFrame, true);
  CDockManager::setAutoHideConfigFlags(CDockManager::DefaultAutoHideConfig);
  dock_manager_ = new CDockManager(this);
  ui->verticalLayout->setContentsMargins(0, 0, 0, 5);
  ui->verticalLayout->setSpacing(5);
}

// 创建顶部地图工具栏（视图菜单、重定位、编辑、打开/保存地图、电池）
MainWindow::MapToolBarButtons MainWindow::createMapToolBar(QVBoxLayout* center_layout) {
  map_toolbar_widget_ = new MapToolBarWidget(this);
  center_layout->insertWidget(0, map_toolbar_widget_);

  QToolButton *view_menu_btn = map_toolbar_widget_->viewMenuButton();
  QToolButton *reloc_btn = map_toolbar_widget_->relocButton();
  QToolButton *edit_map_btn = map_toolbar_widget_->editMapButton();
  QToolButton *open_map_btn = map_toolbar_widget_->openMapButton();
  QToolButton *save_map_btn = map_toolbar_widget_->saveMapButton();
  QToolButton *re_save_map_btn = map_toolbar_widget_->saveAsButton();

  view_menu_btn->setMenu(ui->menuView);
  menuBar()->setVisible(false);

  battery_bar_ = map_toolbar_widget_->batteryBar();
  label_power_ = map_toolbar_widget_->powerLabel();
  health_indicator_ = map_toolbar_widget_->healthIndicator();
  SlotSetBatteryStatus(0, 0);

  return {reloc_btn, edit_map_btn, open_map_btn, save_map_btn, re_save_map_btn};
}

// 创建左侧地图编辑工具栏（鼠标、点位、拓扑连接、橡皮擦、画笔、线段）
MainWindow::EditToolBarWidgets MainWindow::createEditToolBar(QHBoxLayout* center_h_layout) {
  edit_toolbar_widget_ = new EditToolBarWidget();

  QToolButton *normal_cursor_btn = edit_toolbar_widget_->normalCursorButton();
  QToolButton *add_point_btn = edit_toolbar_widget_->addPointButton();
  QToolButton *add_topology_path_btn = edit_toolbar_widget_->addTopologyPathButton();
  QToolButton *add_region_btn = edit_toolbar_widget_->addRegionButton();
  QToolButton *erase_btn = edit_toolbar_widget_->eraseButton();
  QToolButton *draw_pen_btn = edit_toolbar_widget_->drawPenButton();
  QToolButton *draw_line_btn = edit_toolbar_widget_->drawLineButton();

  QButtonGroup *edit_map_button_group = new QButtonGroup(this);
  edit_map_button_group->addButton(normal_cursor_btn);
  edit_map_button_group->addButton(add_point_btn);
  edit_map_button_group->addButton(add_topology_path_btn);
  edit_map_button_group->addButton(add_region_btn);
  edit_map_button_group->addButton(erase_btn);
  edit_map_button_group->addButton(draw_pen_btn);
  edit_map_button_group->addButton(draw_line_btn);

  normal_cursor_btn->setChecked(true);

  edit_toolbar_widget_->hide();
  center_h_layout->addWidget(edit_toolbar_widget_);

  return {edit_toolbar_widget_, normal_cursor_btn, add_point_btn,
          add_topology_path_btn, add_region_btn, erase_btn, draw_pen_btn, draw_line_btn};
}

// 创建中心区域 + 所有 Dock 面板（仪表盘、速度控制、图层配置、导航任务、图像）
void MainWindow::createDockPanels(QVBoxLayout* center_layout, QHBoxLayout* center_h_layout) {
  // ── 中心地图显示区域 ──────────────────────────────────────
  display_manager_ = new Display::DisplayManager();
  center_h_layout->addWidget(display_manager_->GetViewPtr());
  center_layout->addLayout(center_h_layout);

  // 减小下方边距
  center_layout->setContentsMargins(0, 0, 0, 5);
  center_layout->setSpacing(5);

  // ── 中心主窗体（Dock 容器）────────────────────────────────
  // setLayout first to reparent center_layout away from centralwidget,
  // then delete centralwidget so it no longer owns the layout.
  QWidget *center_widget = new QWidget();
  center_widget->setObjectName("centralWidgetHost");
  center_widget->setLayout(center_layout);
  delete ui->centralwidget;
  CDockWidget *CentralDockWidget = new CDockWidget("CentralWidget");
  CentralDockWidget->setWidget(center_widget);
  center_docker_area_ = dock_manager_->setCentralWidget(CentralDockWidget);
  center_docker_area_->setAllowedAreas(DockWidgetArea::OuterDockAreas);

  // ── 速度仪表盘 Dock ───────────────────────────────────────
  ads::CDockWidget *DashBoardDockWidget = new ads::CDockWidget("DashBoard");
  QWidget *speed_dashboard_widget = new QWidget();
  DashBoardDockWidget->setWidget(speed_dashboard_widget);
  speed_dash_board_ = new DashBoard(speed_dashboard_widget);
  auto dashboard_area =
      dock_manager_->addDockWidget(ads::DockWidgetArea::RightDockWidgetArea,
                                   DashBoardDockWidget, center_docker_area_);
  ui->menuView->addAction(DashBoardDockWidget->toggleViewAction());

  // ── 速度控制 Dock ─────────────────────────────────────────
  speed_ctrl_widget_ = new SpeedCtrlWidget();
  connect(speed_ctrl_widget_, &SpeedCtrlWidget::signalControlSpeed,
          [this](const RobotSpeed &speed) {
            PUBLISH(MSG_ID_SET_ROBOT_SPEED, speed);
          });
  ads::CDockWidget *SpeedCtrlDockWidget = new ads::CDockWidget("SpeedCtrl");
  SpeedCtrlDockWidget->setWidget(speed_ctrl_widget_);
  auto speed_ctrl_area =
      dock_manager_->addDockWidget(ads::DockWidgetArea::BottomDockWidgetArea,
                                   SpeedCtrlDockWidget, dashboard_area);
  ui->menuView->addAction(SpeedCtrlDockWidget->toggleViewAction());

  // ── 图层配置管理 Dock ─────────────────────────────────────
  DisplayConfigWidget *display_config_widget_ = new DisplayConfigWidget();
  display_config_widget_->SetDisplayManager(display_manager_);
  display_config_widget_->SetChannelList(channel_manager_.DiscoveryAllChannel());
  ads::CDockWidget *DisplayConfigDockWidget = new ads::CDockWidget("ConfigManager");
  DisplayConfigDockWidget->setWidget(display_config_widget_);
  DisplayConfigDockWidget->setMinimumSizeHintMode(ads::CDockWidget::MinimumSizeHintFromDockWidget);
  DisplayConfigDockWidget->setMinimumSize(250, 200);
  auto display_config_area =
      dock_manager_->addDockWidget(ads::DockWidgetArea::LeftDockWidgetArea,
                                   DisplayConfigDockWidget, center_docker_area_);
  DisplayConfigDockWidget->toggleView(true);
  ui->menuView->addAction(DisplayConfigDockWidget->toggleViewAction());

  // ── 导航任务列表 Dock ─────────────────────────────────────
  task_list_panel_widget_ = new TaskListPanelWidget();
  nav_goal_table_view_ = new NavGoalTableView();
  task_list_panel_widget_->tableContainerLayout()->addWidget(nav_goal_table_view_);
  ads::CDockWidget *nav_goal_list_dock_widget = new ads::CDockWidget("Task");

  QPushButton *btn_add_one_goal = task_list_panel_widget_->addGoalButton();
  QPushButton *btn_start_task_chain = task_list_panel_widget_->startTaskChainButton();
  QCheckBox *loop_task_checkbox = task_list_panel_widget_->loopTaskCheckBox();
  QPushButton *btn_load_task_chain = task_list_panel_widget_->loadTaskChainButton();
  QPushButton *btn_save_task_chain = task_list_panel_widget_->saveTaskChainButton();

  nav_goal_list_dock_widget->setWidget(task_list_panel_widget_);
  nav_goal_list_dock_widget->setMinimumSizeHintMode(
      CDockWidget::MinimumSizeHintFromDockWidget);
  nav_goal_list_dock_widget->setMinimumSize(200, 150);
  dock_manager_->addDockWidget(ads::DockWidgetArea::RightDockWidgetArea,
                               nav_goal_list_dock_widget, center_docker_area_);
  nav_goal_list_dock_widget->toggleView(false);
  connect(nav_goal_table_view_, &NavGoalTableView::signalSendNavGoal,
          [this](const RobotPose &pose) {
            PUBLISH(MSG_ID_SET_NAV_GOAL_POSE, pose);
          });
  connect(btn_load_task_chain, &QPushButton::clicked, [this]() {
    QString fileName = QFileDialog::getOpenFileName(nullptr, "Open JSON file",
                                                    "", "JSON files (*.json)",
                                                    nullptr, QFileDialog::DontUseNativeDialog);

    // 如果用户选择了文件，则输出文件名
    if (!fileName.isEmpty()) {
      qDebug() << "Selected file:" << fileName;
      nav_goal_table_view_->LoadTaskChain(fileName.toStdString());
    }
  });
  connect(btn_save_task_chain, &QPushButton::clicked, [this]() {
    QString fileName = QFileDialog::getSaveFileName(nullptr, "Save JSON file",
                                                    "", "JSON files (*.json)",
                                                    nullptr, QFileDialog::DontUseNativeDialog);

    // 如果用户选择了文件，则输出文件名
    if (!fileName.isEmpty()) {
      qDebug() << "Selected file:" << fileName;
      if (!fileName.endsWith(".json")) {
        fileName += ".json";
      }
      nav_goal_table_view_->SaveTaskChain(fileName.toStdString());
      
      // 显示保存成功对话框
      QMessageBox::information(this, "保存成功", 
                              "任务链文件已成功保存到:\n" + fileName,
                              QMessageBox::Ok);
    }
  });

  // nav_goal_list_dock_widget->toggleView(false);
  ui->menuView->addAction(nav_goal_list_dock_widget->toggleViewAction());
  connect(
      btn_add_one_goal, &QPushButton::clicked,
      [this, nav_goal_list_dock_widget]() { nav_goal_table_view_->AddItem(); });
  connect(btn_start_task_chain, &QPushButton::clicked,
          [this, btn_start_task_chain, loop_task_checkbox]() {
            if (btn_start_task_chain->text() == "Start Task Chain") {
              btn_start_task_chain->setText("Stop Task Chain");
              nav_goal_table_view_->StartTaskChain(loop_task_checkbox->isChecked());
            } else {
              btn_start_task_chain->setText("Start Task Chain");
              nav_goal_table_view_->StopTaskChain();
            }
          });
  connect(nav_goal_table_view_, &NavGoalTableView::signalTaskFinish,
          [this, btn_start_task_chain]() {
            LOG_INFO("task finish!");
            btn_start_task_chain->setText("Start Task Chain");
          });
  connect(display_manager_,
          SIGNAL(signalTopologyMapUpdate(const TopologyMap &)),
          nav_goal_table_view_, SLOT(UpdateTopologyMap(const TopologyMap &)));
  connect(
      display_manager_,
      SIGNAL(signalCurrentSelectPointChanged(const TopologyMap::PointInfo &)),
      nav_goal_table_view_,
      SLOT(UpdateSelectPoint(const TopologyMap::PointInfo &)));

  // ── 图像显示窗口（根据配置动态创建）─────────────────────

  for (auto one_image : Config::ConfigManager::Instance()->GetRootConfig().images) {
    LOG_INFO("init image window location:" << one_image.location << " topic:" << one_image.topic);
    image_frame_map_[one_image.location] = new RatioLayoutedFrame();
    ads::CDockWidget *dock_widget = new ads::CDockWidget(std::string("image/" + one_image.location).c_str());
    dock_widget->setWidget(image_frame_map_[one_image.location]);

    dock_manager_->addDockWidget(ads::DockWidgetArea::RightDockWidgetArea,
                                 dock_widget, center_docker_area_);
    dock_widget->toggleView(false);
    ui->menuView->addAction(dock_widget->toggleViewAction());
  }

  // ── 节点健康状态面板 ──────────────────────────────────────
  node_health_widget_ = new NodeHealthWidget();
  ads::CDockWidget *node_health_dock = new ads::CDockWidget("NodeHealth");
  node_health_dock->setWidget(node_health_widget_);
  node_health_dock->setMinimumSizeHintMode(CDockWidget::MinimumSizeHintFromDockWidget);
  node_health_dock->setMinimumSize(260, 200);
  dock_manager_->addDockWidget(ads::DockWidgetArea::BottomDockWidgetArea,
                               node_health_dock, center_docker_area_);
  node_health_dock->toggleView(false);
  ui->menuView->addAction(node_health_dock->toggleViewAction());

}

// 连接所有信号槽
void MainWindow::connectSignals(const MapToolBarButtons& tb, const EditToolBarWidgets& et) {
  // 从结构体解包按钮指针（保持原代码最小改动）
  auto* reloc_btn = tb.reloc;
  auto* edit_map_btn = tb.edit_map;
  auto* open_map_btn = tb.open_map;
  auto* save_map_btn = tb.save_map;
  auto* re_save_map_btn = tb.re_save_map;
  auto* tools_edit_map_widget = et.container;
  auto* normal_cursor_btn = et.normal_cursor;
  auto* add_point_btn = et.add_point;
  auto* add_topology_path_btn = et.add_topology_path;
  auto* add_region_btn = et.add_region;
  auto* erase_btn = et.erase;
  auto* draw_pen_btn = et.draw_pen;
  auto* draw_line_btn = et.draw_line;

  // ── 信号槽连接 ────────────────────────────────────────────

  connect(this, SIGNAL(OnRecvChannelData(const MsgId &, const std::any &)),
          this, SLOT(RecvChannelMsg(const MsgId &, const std::any &)), Qt::BlockingQueuedConnection);
  connect(display_manager_, &Display::DisplayManager::signalPub2DPose,
          [this](const RobotPose &pose) {
            PUBLISH(MSG_ID_SET_RELOC_POSE, pose);
          });
  connect(display_manager_, &Display::DisplayManager::signalPub2DGoal,
          [this](const RobotPose &pose) {
            PUBLISH(MSG_ID_SET_NAV_GOAL_POSE, pose);
          });
  // ui相关
  connect(reloc_btn, &QToolButton::clicked,
          [this]() { display_manager_->StartReloc(); });

  connect(re_save_map_btn, &QToolButton::clicked, [this]() {
    QString fileName = QFileDialog::getSaveFileName(nullptr, "Save Map files",
                                                    "", "Map files (*.yaml,*.pgm,*.pgm.json)",
                                                    nullptr, QFileDialog::DontUseNativeDialog);
    if (!fileName.isEmpty()) {
      // 用户选择了文件夹，可以在这里进行相应的操作
      LOG_INFO("用户选择的保存地图路径：" << fileName.toStdString());
      
      // 保存占用栅格地图
      auto occ_map = display_manager_->GetOccupancyMap();
      occ_map.Save(fileName.toStdString());
      
      // 保存拓扑地图
      auto topology_map = display_manager_->GetTopologyMap();

      std::string topology_path = fileName.toStdString();
      // 替换扩展名为.topology
      size_t last_dot = topology_path.find_last_of(".");
      if (last_dot != std::string::npos) {
        topology_path = topology_path.substr(0, last_dot) + ".topology";
      } else {
        topology_path += ".topology";
      }
      Config::ConfigManager::Instance()->WriteTopologyMap(topology_path, topology_map);
      
      // 显示保存成功对话框
      QMessageBox::information(this, "保存成功", 
                              "地图文件已成功保存到:\n" + fileName,
                              QMessageBox::Ok);
    } else {
      // 用户取消了选择
      LOG_INFO("取消保存地图");
    }
  });

  connect(save_map_btn, &QToolButton::clicked, [this]() {
    
    // 保存占用栅格地图
    auto occ_map = display_manager_->GetOccupancyMap();
    occ_map.Save(map_path_);
    
    // 保存拓扑地图
    auto topology_map = display_manager_->GetTopologyMap();


    std::string topology_path = map_path_ + ".topology";
    Config::ConfigManager::Instance()->WriteTopologyMap(topology_path, topology_map);
    
    //发送到ROS
    PUBLISH(MSG_ID_TOPOLOGY_MAP_UPDATE, topology_map);

    // 显示保存成功对话框
    QMessageBox::information(this, "保存成功", 
                            "地图文件已成功保存到:\n" + QString::fromStdString(map_path_),
                            QMessageBox::Ok);
  });

  connect(open_map_btn, &QToolButton::clicked, [this]() {
    QStringList filters;
    filters
        << "地图(*.yaml)"
        << "拓扑地图(*.topology)";

    QString fileName = QFileDialog::getOpenFileName(nullptr, "OPen Map files",
                                                    "", filters.join(";;"),
                                                    nullptr, QFileDialog::DontUseNativeDialog);
    if (!fileName.isEmpty()) {
      LOG_INFO("用户选择的打开地图路径：" << fileName.toStdString());
      LoadMap(fileName.toStdString());
    } else {
      LOG_INFO("取消打开地图");
    }
  });

  
  connect(edit_map_btn, &QToolButton::clicked, [this, tools_edit_map_widget, edit_map_btn]() {
    if (edit_map_btn->text() == "编辑地图") {
      display_manager_->SetEditMapMode(Display::MapEditMode::kMoveCursor);
      edit_map_btn->setText("结束编辑");
      tools_edit_map_widget->show();
    } else {
      display_manager_->SetEditMapMode(Display::MapEditMode::kStopEdit);
      edit_map_btn->setText("编辑地图");
      tools_edit_map_widget->hide();
      // 隐藏添加机器人位置按钮
      Display::ViewManager* view_manager = dynamic_cast<Display::ViewManager*>(display_manager_->GetViewPtr());
      if (view_manager) {
        view_manager->ShowAddRobotPosButton(false);
      }
    }
  });
  connect(add_point_btn, &QToolButton::clicked, [this]() {
    display_manager_->SetEditMapMode(Display::MapEditMode::kAddPoint);
    // 显示添加机器人位置按钮
    Display::ViewManager* view_manager = dynamic_cast<Display::ViewManager*>(display_manager_->GetViewPtr());
    if (view_manager) {
      view_manager->ShowAddRobotPosButton(true);
      // 连接按钮点击事件（只在进入模式时连接一次）
      QToolButton* add_robot_pos_btn = view_manager->GetAddRobotPosButton();
      if (add_robot_pos_btn) {
        // 先断开之前的连接（如果有）
        add_robot_pos_btn->disconnect();
        connect(add_robot_pos_btn, &QToolButton::clicked, [this]() {
          display_manager_->AddPointAtRobotPosition();
        });
      }
    }
  });
  // 当退出 kAddPoint 模式时，隐藏添加机器人位置按钮
  auto hideAddRobotPosButton = [this]() {
    Display::ViewManager* view_manager = dynamic_cast<Display::ViewManager*>(display_manager_->GetViewPtr());
    if (view_manager) {
      view_manager->ShowAddRobotPosButton(false);
    }
  };
  
  connect(normal_cursor_btn, &QToolButton::clicked, [this, hideAddRobotPosButton]() { 
    display_manager_->SetEditMapMode(Display::MapEditMode::kMoveCursor);
    hideAddRobotPosButton();
  });
  connect(erase_btn, &QToolButton::clicked, [this, hideAddRobotPosButton]() { 
    display_manager_->SetEditMapMode(Display::MapEditMode::kErase);
    hideAddRobotPosButton();
    // 更新滑动条显示为红色
    Display::ViewManager* view_manager = dynamic_cast<Display::ViewManager*>(display_manager_->GetViewPtr());
    if (view_manager) {
      view_manager->UpdateToolSizeSlider(display_manager_->GetEraserRange());
    }
  });
  connect(draw_line_btn, &QToolButton::clicked, [this, hideAddRobotPosButton]() { 
    display_manager_->SetEditMapMode(Display::MapEditMode::kDrawLine);
    hideAddRobotPosButton();
  });
  connect(add_region_btn, &QToolButton::clicked, [this, hideAddRobotPosButton]() { 
    display_manager_->SetEditMapMode(Display::MapEditMode::kRegion);
    hideAddRobotPosButton();
  });
  connect(draw_pen_btn, &QToolButton::clicked, [this, hideAddRobotPosButton]() { 
    display_manager_->SetEditMapMode(Display::MapEditMode::kDrawWithPen);
    hideAddRobotPosButton();
    // 更新滑动条显示为蓝色
    Display::ViewManager* view_manager = dynamic_cast<Display::ViewManager*>(display_manager_->GetViewPtr());
    if (view_manager) {
      view_manager->UpdateToolSizeSlider(display_manager_->GetPenRange());
    }
  });
  connect(add_topology_path_btn, &QToolButton::clicked, [this, hideAddRobotPosButton]() { 
    display_manager_->SetEditMapMode(Display::MapEditMode::kLinkTopology);
    hideAddRobotPosButton();
  });
  
  connect(display_manager_->GetDisplay(DISPLAY_MAP),
          SIGNAL(signalCursorPose(QPointF)), this,
          SLOT(signalCursorPose(QPointF)));
}


////////////////////////////////////////////////////////////////////////////
/// 通信通道管理
////////////////////////////////////////////////////////////////////////////

// 自动检测并打开通信通道，连接成功后延迟 6s 检查连接状态
bool MainWindow::openChannel() {
  if (channel_manager_.OpenChannelAuto()) {
    registerChannel();
    
    // 延迟检查连接状态（连接超时是5秒）
    auto* channel = channel_manager_.GetChannel();
    if (channel) {
      QTimer::singleShot(6000, this, [this, channel]() {
        if (channel->IsConnectionFailed()) {
          std::string error_msg = channel->GetConnectionError();
          std::string channel_name = channel->Name();
          if (!error_msg.empty()) {
            QMessageBox::critical(this, QString::fromStdString(channel_name) + " 连接失败", 
                                  QString::fromStdString(error_msg),
                                  QMessageBox::Ok);
          } else {
            QMessageBox::critical(this, QString::fromStdString(channel_name) + " 连接失败", 
                                  "无法连接到 " + QString::fromStdString(channel_name) + " 服务器。\n\n请检查：\n"
                                  "1. 服务器是否正在运行\n"
                                  "2. 配置是否正确\n"
                                  "3. 网络连接是否正常",
                                  QMessageBox::Ok);
          }
        }
      });
    }
    
    return true;
  }
  return false;
}

// 按指定名称打开通信通道
bool MainWindow::openChannel(const std::string &channel_name) {
  if (channel_manager_.OpenChannel(channel_name)) {
    registerChannel();
    return true;
  }
  return false;
}

// 关闭当前通信通道
void MainWindow::closeChannel() { channel_manager_.CloseChannel(); }

// 注册消息总线订阅：里程计、机器人位姿、电池状态、图像数据
void MainWindow::registerChannel() {
  if (odom_pose_sub_id_ != 0) {
    UNSUBSCRIBE(MSG_ID_ODOM_POSE, odom_pose_sub_id_);
  }
  if (robot_pose_sub_id_ != 0) {
    UNSUBSCRIBE(MSG_ID_ROBOT_POSE, robot_pose_sub_id_);
  }
  if (battery_state_sub_id_ != 0) {
    UNSUBSCRIBE(MSG_ID_BATTERY_STATE, battery_state_sub_id_);
  }
  if (image_sub_id_ != 0) {
    UNSUBSCRIBE(MSG_ID_IMAGE, image_sub_id_);
  }
  if (node_health_sub_id_ != 0) {
    UNSUBSCRIBE(MSG_ID_NODE_HEALTH, node_health_sub_id_);
  }

  odom_pose_sub_id_ = SUBSCRIBE(MSG_ID_ODOM_POSE, [this](const RobotState& data) {
    if (!speed_dash_board_) return;
    updateOdomInfo(data);
  });

  robot_pose_sub_id_ = SUBSCRIBE(MSG_ID_ROBOT_POSE, [this](const RobotPose& p) {
      OnRobotPoseUpdate(p);
  });

  battery_state_sub_id_ = SUBSCRIBE(MSG_ID_BATTERY_STATE, [this](const std::map<std::string, std::string>& map) {
    if (!battery_bar_ || !label_power_) return;
    this->SlotSetBatteryStatus(std::stod(map.at("percent")),
                               std::stod(map.at("voltage")));
  });

  image_sub_id_ = SUBSCRIBE(MSG_ID_IMAGE, [this](const std::pair<std::string, std::shared_ptr<cv::Mat>>& location_to_mat) {
      this->SlotRecvImage(location_to_mat.first, location_to_mat.second);
  });

  node_health_sub_id_ = SUBSCRIBE(MSG_ID_NODE_HEALTH, [this](const SystemHealthStatus& status) {
    QMetaObject::invokeMethod(this, [this, status]() {
      if (node_health_widget_) node_health_widget_->update(status);
      if (health_indicator_) {
        switch (status.overall_level) {
          case HealthLevel::Normal:
            health_indicator_->setText("系统: 正常");
            health_indicator_->setStyleSheet("font-weight:bold; border-radius:4px; background:#2d7a2d; color:white;");
            break;
          case HealthLevel::Degraded:
            health_indicator_->setText("系统: 降级");
            health_indicator_->setStyleSheet("font-weight:bold; border-radius:4px; background:#b8860b; color:white;");
            break;
          case HealthLevel::Fault:
            health_indicator_->setText("系统: 故障");
            health_indicator_->setStyleSheet("font-weight:bold; border-radius:4px; background:#a02020; color:white;");
            break;
        }
      }
    }, Qt::QueuedConnection);
  });
}


////////////////////////////////////////////////////////////////////////////
/// 窗口状态持久化
////////////////////////////////////////////////////////////////////////////

// 保存窗口几何信息和 Dock 布局到 state.ini
void MainWindow::SaveState() {
  QSettings settings("state.ini", QSettings::IniFormat);
  settings.setValue("mainWindow/Geometry", this->saveGeometry());
  settings.setValue("mainWindow/State", this->saveState());
  dock_manager_->addPerspective("history");
  dock_manager_->savePerspectives(settings);
}

// 从 state.ini 恢复窗口几何信息和 Dock 布局

//============================================================================
void MainWindow::RestoreState() {
  QSettings settings("state.ini", QSettings::IniFormat);
  this->restoreGeometry(settings.value("mainWindow/Geometry").toByteArray());
  this->restoreState(settings.value("mainWindow/State").toByteArray());
  dock_manager_->loadPerspectives(settings);
  dock_manager_->openPerspective("history");
}


////////////////////////////////////////////////////////////////////////////
/// 地图操作
////////////////////////////////////////////////////////////////////////////

// 加载地图文件（支持 .yaml 栅格地图和 .topology 拓扑地图）
// 加载 yaml 时会自动查找同名 .topology 文件一并加载

bool MainWindow::LoadMap(const std::string& file_path) {
  if (file_path.empty()) {
    return false;
  }

  std::string extension = QFileInfo(QString::fromStdString(file_path)).suffix().toStdString();
  
  if (extension == "yaml") {
    map_path_ = file_path;
    size_t last_dot = map_path_.find_last_of(".");
    if (last_dot != std::string::npos) {
      map_path_ = map_path_.substr(0, last_dot);
    }

    Config::ConfigManager::Instance()->GetRootConfig().map_config.path = map_path_;
    Config::ConfigManager::Instance()->StoreConfig();

    OccupancyMap map;
    if (map.Load(file_path)) {
      display_manager_->UpdateOCCMap(map);
      
      std::string topology_path = file_path;
      size_t last_dot = topology_path.find_last_of(".");
      if (last_dot != std::string::npos) {
        topology_path = topology_path.substr(0, last_dot) + ".topology";
      } else {
        topology_path += ".topology";
      }
      
      if (QFile::exists(QString::fromStdString(topology_path))) {
        TopologyMap topology_map;
        if (Config::ConfigManager::Instance()->ReadTopologyMap(topology_path, topology_map)) {
          display_manager_->UpdateTopologyMap(topology_map);
        }
      }
      return true;
    } else {
      QMessageBox::warning(this, "打开失败", "无法打开地图文件: " + QString::fromStdString(file_path));
      return false;
    }
  } else if (extension == "topology") {
    TopologyMap topology_map;
    if (Config::ConfigManager::Instance()->ReadTopologyMap(file_path, topology_map)) {
      display_manager_->UpdateTopologyMap(topology_map);
      return true;
    } else {
      QMessageBox::warning(this, "打开失败", "无法打开拓扑地图文件: " + QString::fromStdString(file_path));
      return false;
    }
  }
  
  return false;
}


////////////////////////////////////////////////////////////////////////////
/// 槽函数 / 数据接收
////////////////////////////////////////////////////////////////////////////

// [已废弃] 保留以兼容旧代码，数据现通过 message_bus 订阅


void MainWindow::RecvChannelMsg(const MsgId &id, const std::any &data) {
  // 保留此方法以兼容现有代码，但不再使用
  // 数据现在通过 message_bus 订阅接收
}

// 接收图像数据并更新对应位置的图像显示控件


void MainWindow::SlotRecvImage(const std::string &location, std::shared_ptr<cv::Mat> data) {
  if (image_frame_map_.count(location)) {
    QImage image(data->data, data->cols, data->rows, data->step[0], QImage::Format_RGB888);
    image_frame_map_[location]->setImage(image);
  }
}

// 根据里程计数据更新速度仪表盘：前进(D档)、后退(R档)、静止(N档)
void MainWindow::updateOdomInfo(RobotState state) {
  speed_dash_board_->set_speed(abs(state.vx * 100));
  if (state.vx > 0.001) {
    speed_dash_board_->set_gear(DashBoard::kGear_D);
  } else if (state.vx < -0.001) {
    speed_dash_board_->set_gear(DashBoard::kGear_R);
  } else {
    speed_dash_board_->set_gear(DashBoard::kGear_N);
  }
}

// 更新电池进度条和电压标签
void MainWindow::SlotSetBatteryStatus(double percent, double voltage) {
  battery_bar_->setValue(percent);
  label_power_->setText(QString::number(voltage, 'f', 2) + "V");
}

// 鼠标在地图上移动时，更新状态栏的地图坐标和场景坐标

void MainWindow::signalCursorPose(QPointF pos) {
  basic::Point mapPos =
      display_manager_->mapPose2Word(basic::Point(pos.x(), pos.y()));
  Display::ViewManager* view_manager = dynamic_cast<Display::ViewManager*>(display_manager_->GetViewPtr());
  if (view_manager) {
    view_manager->UpdateMapPos("Map: (" + QString::number(mapPos.x, 'f', 2) +
                               ", " + QString::number(mapPos.y, 'f', 2) + ")");
    view_manager->UpdateScenePos("Scene: (" + QString::number(pos.x(), 'f', 2) +
                                 ", " + QString::number(pos.y(), 'f', 2) + ")");
  }
}


////////////////////////////////////////////////////////////////////////////
/// 事件处理
////////////////////////////////////////////////////////////////////////////

// 窗口关闭事件：断开信号 → 关闭通道 → 保存状态 → 销毁 DockManager

//============================================================================
void MainWindow::closeEvent(QCloseEvent *event) {
  // Delete dock manager here to delete all floating widgets. This ensures
  // that all top level windows of the dock manager are properly closed
  // write state

  disconnect(this, SIGNAL(OnRecvChannelData(const MsgId &, const std::any &)),
             this, SLOT(RecvChannelMsg(const MsgId &, const std::any &)));
  closeChannel();
  display_manager_ = nullptr;
  SaveState();
  dock_manager_->deleteLater();
  QMainWindow::closeEvent(event);
  LOG_INFO("ros qt5 gui app close!");
}

void MainWindow::OnRobotPoseUpdate(const RobotPose& robot_pose) {
  if (!nav_goal_table_view_ || !display_manager_) return;
  nav_goal_table_view_->UpdateRobotPose(robot_pose);
  Display::ViewManager* view_manager = dynamic_cast<Display::ViewManager*>(display_manager_->GetViewPtr());
  if (view_manager) {
    view_manager->UpdateRobotPos("Robot: (" + QString::number(robot_pose.x, 'f', 2) + ", " +
                                 QString::number(robot_pose.y, 'f', 2) + ", " +
                                 QString::number(robot_pose.theta, 'f', 2) + ")");
  }
}
