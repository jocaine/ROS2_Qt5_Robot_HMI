#include "display_config_widget.h"
#include "display/manager/display_factory.h"
#include "display/manager/display_manager.h"
#include "display/virtual_display.h"
#include "config/config_manager.h"
#include "config/config_define.h"
#include "msg/msg_info.h"
#include "logger/logger.h"
#include "ui_display_config_widget.h"
#include <QDebug>
#include <QFrame>
#include <QMessageBox>
#include <QAbstractItemView>
#include <QInputDialog>
#include <QTimer>

DisplayConfigWidget::DisplayConfigWidget(QWidget *parent)
    : QWidget(parent), ui_(std::make_unique<Ui::DisplayConfigWidget>()), robot_color_(QColor(0, 0, 255)) {
  ui_->setupUi(this);
  InitUI();
  QTimer::singleShot(3000, this, &DisplayConfigWidget::LoadConfig);
}

DisplayConfigWidget::~DisplayConfigWidget() {}

void DisplayConfigWidget::InitUI() {
  main_layout_ = qobject_cast<QVBoxLayout *>(layout());
  tab_widget_ = ui_->tabWidget;

  InitDisplayConfigTab();
  InitChannelConfigTab();
  InitKeyValueTab();
  InitImageConfigTab();
  InitRobotShapeTab();
}

void DisplayConfigWidget::InitDisplayConfigTab() {
  display_tab_ = ui_->displayTab;
  display_scroll_area_ = ui_->displayScrollArea;
  display_scroll_content_ = ui_->displayScrollContent;
  auto *scroll_layout = qobject_cast<QVBoxLayout *>(display_scroll_content_->layout());
  
  std::vector<std::pair<std::string, std::string>> display_types = {
    {DISPLAY_MAP, ":/images/classes/Map.png"},
    {DISPLAY_ROBOT, ":/images/classes/RobotModel.png"},
    {DISPLAY_LASER, ":/images/classes/LaserScan.png"},
    {DISPLAY_GLOBAL_PATH, ":/images/classes/Path.png"},
    {DISPLAY_LOCAL_PATH, ":/images/classes/Path.png"},
    {DISPLAY_GLOBAL_COST_MAP, ":/images/classes/Grid.png"},
    {DISPLAY_LOCAL_COST_MAP, ":/images/classes/Grid.png"},
    {DISPLAY_ROBOT_FOOTPRINT, ":/images/classes/RobotLink.png"},
    {DISPLAY_GOAL, ":/images/classes/SetGoal.png"},
  };
  
  for (const auto &[display_name, icon_path] : display_types) {
    QGroupBox *group_box = new QGroupBox(QString::fromStdString(display_name), display_scroll_content_);
    group_box->setStyleSheet(R"(
      QGroupBox {
        border: 1px solid #d0d0d0;
        border-radius: 4px;
        margin-top: 10px;
        padding-top: 10px;
        background-color: #f9f9f9;
      }
      QGroupBox::title {
        subcontrol-origin: margin;
        left: 10px;
        padding: 0 5px;
      }
    )");
    
    QVBoxLayout *group_layout = new QVBoxLayout(group_box);
    group_layout->setContentsMargins(10, 15, 10, 10);
    group_layout->setSpacing(8);
    
    QHBoxLayout *topic_layout = new QHBoxLayout();
    QLabel *topic_label = new QLabel("话题:", group_box);
    topic_label->setFixedWidth(60);
    QLineEdit *topic_edit = new QLineEdit(group_box);
    topic_edit->setPlaceholderText("输入话题名称");
    topic_edit->setStyleSheet(R"(
      QLineEdit {
        border: 1px solid #d0d0d0;
        border-radius: 4px;
        padding: 4px;
        background-color: #ffffff;
      }
      QLineEdit:focus {
        border-color: #1976d2;
      }
    )");
    display_topic_edits_[display_name] = topic_edit;
    connect(topic_edit, &QLineEdit::editingFinished, [this, display_name, topic_edit]() {
      OnDisplayTopicChanged(display_name, topic_edit->text());
    });
    topic_layout->addWidget(topic_label);
    topic_layout->addWidget(topic_edit);
    group_layout->addLayout(topic_layout);
    
    QHBoxLayout *control_layout = new QHBoxLayout();
    QLabel *visible_label = new QLabel("可见:", group_box);
    visible_label->setFixedWidth(60);
    QToolButton *toggle_btn = new QToolButton(group_box);
    toggle_btn->setCheckable(true);
    toggle_btn->setChecked(true);
    toggle_btn->setText("✓");
    toggle_btn->setFixedSize(30, 24);
    toggle_btn->setStyleSheet(R"(
      QToolButton {
        border: 1px solid #d0d0d0;
        border-radius: 4px;
        background-color: #ffffff;
        color: #333333;
        font-weight: bold;
      }
      QToolButton:checked {
        background-color: #4caf50;
        border-color: #4caf50;
        color: #ffffff;
      }
      QToolButton:hover {
        border-color: #1976d2;
      }
    )");
    display_toggle_buttons_[display_name] = toggle_btn;
    connect(toggle_btn, &QToolButton::toggled, [this, display_name, toggle_btn](bool checked) {
      toggle_btn->setText(checked ? "✓" : "");
      OnToggleDisplay(display_name, checked);
    });
    
    control_layout->addWidget(visible_label);
    control_layout->addWidget(toggle_btn);
    control_layout->addItem(new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Minimum));
    group_layout->addLayout(control_layout);
    
    scroll_layout->addWidget(group_box);
  }
  
  scroll_layout->addItem(new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding));
}

void DisplayConfigWidget::InitKeyValueTab() {
  key_value_tab_ = ui_->keyValueTab;
  key_value_scroll_area_ = ui_->keyValueScrollArea;
  key_value_scroll_content_ = ui_->keyValueScrollContent;

  connect(ui_->addKeyValueBtn, &QPushButton::clicked, this, &DisplayConfigWidget::OnAddKeyValue);

  RefreshKeyValueTab();
}

void DisplayConfigWidget::RefreshKeyValueTab() {
  QLayoutItem* item;
  while ((item = key_value_scroll_content_->layout()->takeAt(0)) != nullptr) {
    if (item->widget()) {
      item->widget()->deleteLater();
    }
    delete item;
  }
  key_value_edits_.clear();
  
  auto &config = Config::ConfigManager::Instance()->GetRootConfig();
  
  for (const auto &[key, value] : config.key_value) {
    QWidget *item_widget = new QWidget(key_value_scroll_content_);
    item_widget->setStyleSheet(R"(
      QWidget {
        background-color: #f9f9f9;
        border-radius: 4px;
        padding: 5px;
      }
      QWidget:hover {
        background-color: #f0f0f0;
      }
    )");
    
    QHBoxLayout *item_layout = new QHBoxLayout(item_widget);
    item_layout->setContentsMargins(8, 5, 8, 5);
    item_layout->setSpacing(10);
    
    QLabel *key_label = new QLabel(QString::fromStdString(key) + ":", item_widget);
    key_label->setFixedWidth(150);
    key_label->setStyleSheet(R"(
      QLabel {
        font-size: 12px;
        color: #333333;
        font-weight: bold;
      }
    )");
    item_layout->addWidget(key_label);
    
    QLineEdit *value_edit = new QLineEdit(QString::fromStdString(value), item_widget);
    value_edit->setPlaceholderText("输入值");
    value_edit->setStyleSheet(R"(
      QLineEdit {
        border: 1px solid #d0d0d0;
        border-radius: 4px;
        padding: 4px;
        background-color: #ffffff;
      }
      QLineEdit:focus {
        border-color: #1976d2;
      }
    )");
    key_value_edits_[key] = value_edit;
    connect(value_edit, &QLineEdit::editingFinished, [this, key, value_edit]() {
      OnKeyValueChanged(key, value_edit->text());
    });
    item_layout->addWidget(value_edit);
    
    QPushButton *remove_btn = new QPushButton("删除", item_widget);
    remove_btn->setFixedWidth(60);
    remove_btn->setStyleSheet(R"(
      QPushButton {
        border: 1px solid #f44336;
        border-radius: 4px;
        padding: 4px;
        background-color: #f44336;
        color: #ffffff;
      }
      QPushButton:hover {
        background-color: #da190b;
      }
    )");
    connect(remove_btn, &QPushButton::clicked, [this, key]() {
      OnRemoveKeyValue(key);
    });
    item_layout->addWidget(remove_btn);
    
    key_value_scroll_content_->layout()->addWidget(item_widget);
  }
  
  key_value_scroll_content_->layout()->addItem(new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding));
}

void DisplayConfigWidget::InitImageConfigTab() {
  image_tab_ = ui_->imageTab;
  image_table_ = ui_->imageTable;
  image_table_->horizontalHeader()->setStretchLastSection(true);

  connect(image_table_, &QTableWidget::cellChanged, this, &DisplayConfigWidget::OnImageConfigChanged);
  connect(ui_->addImageBtn, &QPushButton::clicked, this, &DisplayConfigWidget::OnAddImageConfig);
}

void DisplayConfigWidget::InitRobotShapeTab() {
  robot_shape_tab_ = ui_->robotShapeTab;
  robot_points_table_ = ui_->robotPointsTable;
  robot_points_table_->horizontalHeader()->setStretchLastSection(true);
  connect(robot_points_table_, &QTableWidget::cellChanged, this, &DisplayConfigWidget::OnRobotShapePointChanged);

  connect(ui_->addRobotPointBtn, &QPushButton::clicked, [this]() {
    int row = robot_points_table_->rowCount();
    robot_points_table_->insertRow(row);
    robot_points_table_->setItem(row, 0, new QTableWidgetItem("0.0"));
    robot_points_table_->setItem(row, 1, new QTableWidgetItem("0.0"));
    OnRobotShapePointChanged();
  });

  connect(ui_->removeRobotPointBtn, &QPushButton::clicked, [this]() {
    int row = robot_points_table_->currentRow();
    if (row >= 0) {
      robot_points_table_->removeRow(row);
      OnRobotShapePointChanged();
    }
  });

  robot_is_ellipse_checkbox_ = ui_->robotIsEllipseCheckbox;
  connect(robot_is_ellipse_checkbox_, &QCheckBox::toggled, this, &DisplayConfigWidget::OnRobotShapeIsEllipseChanged);

  robot_color_button_ = ui_->robotColorButton;
  connect(robot_color_button_, &QPushButton::clicked, this, &DisplayConfigWidget::OnRobotShapeColorChanged);

  robot_opacity_slider_ = ui_->robotOpacitySlider;
  robot_opacity_label_ = ui_->robotOpacityLabel;
  connect(robot_opacity_slider_, &QSlider::valueChanged, [this](int value) {
    robot_opacity_label_->setText(QString::number(value) + "%");
    OnRobotShapeOpacityChanged(value);
  });
}

void DisplayConfigWidget::SetChannelList(const std::vector<std::string> &channel_list) {
  channel_list_ = channel_list;
  
  channel_type_combo_->blockSignals(true);
  channel_type_combo_->clear();
  
  channel_type_combo_->addItem("auto", "auto");
  for (const auto &channel : channel_list_) {
    std::string channel_type = ExtractChannelType(channel);
    channel_type_combo_->addItem(channel_type.c_str(), channel_type.c_str());
  }
  
  auto &config = Config::ConfigManager::Instance()->GetRootConfig();
  std::string channel_type = config.channel_config.channel_type.empty() ? "auto" : config.channel_config.channel_type;
  int index = channel_type_combo_->findData(QString::fromStdString(channel_type));
  if (index >= 0) {
    channel_type_combo_->setCurrentIndex(index);
  } else {
    channel_type_combo_->setCurrentIndex(0);
  }
  
  channel_type_combo_->blockSignals(false);
}

std::string DisplayConfigWidget::ExtractChannelType(const std::string &channel_path) {
  size_t last_slash = channel_path.find_last_of("/\\");
  std::string filename = (last_slash == std::string::npos) ? channel_path : channel_path.substr(last_slash + 1);
  
  size_t last_dot = filename.find_last_of(".");
  if (last_dot != std::string::npos) {
    filename = filename.substr(0, last_dot);
  }
  
  const std::string prefix = "libchannel_";
  if (filename.find(prefix) == 0) {
    return filename.substr(prefix.length());
  }
  
  return filename;
}

void DisplayConfigWidget::InitChannelConfigTab() {
  channel_config_tab_ = ui_->channelConfigTab;
  channel_type_combo_ = ui_->channelTypeCombo;

  connect(channel_type_combo_, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int index) {
    // 如果正在加载配置，不显示提示
    if (is_loading_config_) {
      return;
    }
    
    auto &config = Config::ConfigManager::Instance()->GetRootConfig();
    QString channel_type = channel_type_combo_->itemData(index).toString();
    QString old_channel_type = QString::fromStdString(config.channel_config.channel_type);
    
    // 更新通道配置
    config.channel_config.channel_type = channel_type.toStdString();
    AutoSaveConfig();
    
    // 根据选择的类型显示/隐藏 ROSBridge 配置
    bool show_rosbridge = (channel_type == "rosbridge");
    rosbridge_ip_edit_->setEnabled(show_rosbridge);
    rosbridge_port_edit_->setEnabled(show_rosbridge);
    
    // 如果通道类型发生变化，提示用户需要重启
    if (channel_type != old_channel_type) {
      QMessageBox::information(this, "通道配置已更改", 
                                "通道类型已更改为: " + channel_type + "\n\n"
                                "请重启应用程序以使配置生效。",
                                QMessageBox::Ok);
    }
  });

  rosbridge_ip_edit_ = ui_->rosbridgeIpEdit;
  connect(rosbridge_ip_edit_, &QLineEdit::editingFinished, [this]() {
    // 如果正在加载配置，不显示提示
    if (is_loading_config_) {
      return;
    }
    
    auto &config = Config::ConfigManager::Instance()->GetRootConfig();
    QString new_ip = rosbridge_ip_edit_->text();
    QString old_ip = QString::fromStdString(config.channel_config.rosbridge_config.ip);
    
    // 更新 ROSBridge IP 配置
    config.channel_config.rosbridge_config.ip = new_ip.toStdString();
    AutoSaveConfig();
    
    // 如果 IP 地址发生变化，提示用户需要重启
    if (new_ip != old_ip && !new_ip.isEmpty()) {
      QMessageBox::information(this, "ROSBridge 配置已更改", 
                                "ROSBridge IP 地址已更改为: " + new_ip + "\n\n"
                                "请重启应用程序以使配置生效。",
                                QMessageBox::Ok);
    }
  });
  rosbridge_port_edit_ = ui_->rosbridgePortEdit;
  connect(rosbridge_port_edit_, &QLineEdit::editingFinished, [this]() {
    // 如果正在加载配置，不显示提示
    if (is_loading_config_) {
      return;
    }
    
    auto &config = Config::ConfigManager::Instance()->GetRootConfig();
    QString new_port = rosbridge_port_edit_->text();
    QString old_port = QString::fromStdString(config.channel_config.rosbridge_config.port);
    
    // 更新 ROSBridge 端口配置
    config.channel_config.rosbridge_config.port = new_port.toStdString();
    AutoSaveConfig();
    
    // 如果端口发生变化，提示用户需要重启
    if (new_port != old_port && !new_port.isEmpty()) {
      QMessageBox::information(this, "ROSBridge 配置已更改", 
                                "ROSBridge 端口已更改为: " + new_port + "\n\n"
                                "请重启应用程序以使配置生效。",
                                QMessageBox::Ok);
    }
  });
  reconnect_channel_btn_ = ui_->reconnectChannelBtn;
  connect(reconnect_channel_btn_, &QPushButton::clicked, [this]() {
    QMessageBox::information(this, "提示", 
                              "请重启应用程序以使通道配置生效。\n"
                              "当前配置已保存。",
                              QMessageBox::Ok);
  });
}

void DisplayConfigWidget::SetDisplayManager(Display::DisplayManager *manager) {
  display_manager_ = manager;
  if (display_manager_) {
  LoadConfig();
  }
}

void DisplayConfigWidget::OnToggleDisplay(const std::string &display_name, bool visible) {
  UpdateDisplayVisibility(display_name, visible);
  AutoSaveConfig();
}

void DisplayConfigWidget::OnDisplayTopicChanged(const std::string &display_name, const QString &topic) {
  auto &config = Config::ConfigManager::Instance()->GetRootConfig();
  auto it = std::find_if(config.display_config.begin(), config.display_config.end(),
                        [&display_name](const auto &item) {
                          return item.display_name == display_name;
                        });
  if (it != config.display_config.end()) {
    it->topic = topic.toStdString();
  } else {
    config.display_config.push_back(Config::DisplayConfig(display_name, topic.toStdString(), true));
  }
  AutoSaveConfig();
}

void DisplayConfigWidget::OnKeyValueChanged(const std::string &key, const QString &value) {
  SET_KEY_VALUE(key, value.toStdString())
}

void DisplayConfigWidget::OnAddKeyValue() {
  bool ok;
  QString key = QInputDialog::getText(this, "添加配置项", "请输入键名:", QLineEdit::Normal, "", &ok);
  if (ok && !key.isEmpty()) {
    QString value = QInputDialog::getText(this, "添加配置项", "请输入值:", QLineEdit::Normal, "", &ok);
    if (ok) {
      SET_KEY_VALUE(key.toStdString(), value.toStdString())
      RefreshKeyValueTab();
    }
  }
}

void DisplayConfigWidget::OnRemoveKeyValue(const std::string &key) {
  auto &config = Config::ConfigManager::Instance()->GetRootConfig();
  config.key_value.erase(key);
  AutoSaveConfig();
  RefreshKeyValueTab();
}

void DisplayConfigWidget::OnAddImageConfig() {
  int row = image_table_->rowCount();
  image_table_->insertRow(row);
  
  QTableWidgetItem *location_item = new QTableWidgetItem("");
  QTableWidgetItem *topic_item = new QTableWidgetItem("");
  QTableWidgetItem *enable_item = new QTableWidgetItem("true");
  enable_item->setFlags(enable_item->flags() & ~Qt::ItemIsEditable);
  
  QCheckBox *enable_checkbox = new QCheckBox();
  enable_checkbox->setChecked(true);
  enable_checkbox->setStyleSheet(R"(
    QCheckBox {
      spacing: 5px;
    }
    QCheckBox::indicator {
      width: 18px;
      height: 18px;
      border: 1px solid #d0d0d0;
      border-radius: 3px;
      background-color: #ffffff;
    }
    QCheckBox::indicator:checked {
      background-color: #4caf50;
      border-color: #4caf50;
    }
  )");
  connect(enable_checkbox, &QCheckBox::toggled, [this, row](bool checked) {
    image_table_->item(row, 2)->setText(checked ? "true" : "false");
    OnImageConfigChanged(row);
  });
  
  QPushButton *remove_btn = new QPushButton("删除");
  remove_btn->setStyleSheet(R"(
    QPushButton {
      border: 1px solid #f44336;
      border-radius: 4px;
      padding: 2px 8px;
      background-color: #f44336;
      color: #ffffff;
    }
    QPushButton:hover {
      background-color: #da190b;
    }
  )");
  connect(remove_btn, &QPushButton::clicked, [this, row]() {
    OnRemoveImageConfig(row);
  });
  
  image_table_->setItem(row, 0, location_item);
  image_table_->setItem(row, 1, topic_item);
  image_table_->setItem(row, 2, enable_item);
  image_table_->setCellWidget(row, 2, enable_checkbox);
  image_table_->setCellWidget(row, 3, remove_btn);
  
  OnImageConfigChanged(row);
}

void DisplayConfigWidget::OnRemoveImageConfig(int row) {
  image_table_->removeRow(row);
  
  auto &config = Config::ConfigManager::Instance()->GetRootConfig();
  if (row < static_cast<int>(config.images.size())) {
    config.images.erase(config.images.begin() + row);
    AutoSaveConfig();
  }
  
  for (int i = row; i < image_table_->rowCount(); i++) {
    QPushButton *btn = qobject_cast<QPushButton*>(image_table_->cellWidget(i, 3));
    if (btn) {
      btn->disconnect();
      connect(btn, &QPushButton::clicked, [this, i]() {
        OnRemoveImageConfig(i);
      });
    }
    QCheckBox *checkbox = qobject_cast<QCheckBox*>(image_table_->cellWidget(i, 2));
    if (checkbox) {
      checkbox->disconnect();
      connect(checkbox, &QCheckBox::toggled, [this, i](bool checked) {
        image_table_->item(i, 2)->setText(checked ? "true" : "false");
        OnImageConfigChanged(i);
      });
    }
  }
}

void DisplayConfigWidget::OnImageConfigChanged(int row) {
  if (row < 0 || row >= image_table_->rowCount()) {
    return;
  }
  
  auto &config = Config::ConfigManager::Instance()->GetRootConfig();
  
  QTableWidgetItem *location_item = image_table_->item(row, 0);
  QTableWidgetItem *topic_item = image_table_->item(row, 1);
  QCheckBox *enable_checkbox = qobject_cast<QCheckBox*>(image_table_->cellWidget(row, 2));
  
  if (!location_item || !topic_item || !enable_checkbox) {
    return;
  }
  
  Config::ImageDisplayConfig image_config;
  image_config.location = location_item->text().toStdString();
  image_config.topic = topic_item->text().toStdString();
  image_config.enable = enable_checkbox->isChecked();
  
  if (row < static_cast<int>(config.images.size())) {
    config.images[row] = image_config;
  } else {
    config.images.push_back(image_config);
  }
  
  AutoSaveConfig();
}

void DisplayConfigWidget::OnRobotShapePointChanged() {
  auto &config = Config::ConfigManager::Instance()->GetRootConfig();
  config.robot_shape_config.shaped_points.clear();
  
  for (int row = 0; row < robot_points_table_->rowCount(); row++) {
    QTableWidgetItem *x_item = robot_points_table_->item(row, 0);
    QTableWidgetItem *y_item = robot_points_table_->item(row, 1);
    
    if (x_item && y_item) {
      bool x_ok, y_ok;
      double x = x_item->text().toDouble(&x_ok);
      double y = y_item->text().toDouble(&y_ok);
      
      if (x_ok && y_ok) {
        config.robot_shape_config.shaped_points.push_back({x, y});
      }
    }
  }
  
  AutoSaveConfig();
}

void DisplayConfigWidget::OnRobotShapeIsEllipseChanged(bool checked) {
  auto &config = Config::ConfigManager::Instance()->GetRootConfig();
  config.robot_shape_config.is_ellipse = checked;
  AutoSaveConfig();
}

void DisplayConfigWidget::OnRobotShapeColorChanged() {
  QColor color = QColorDialog::getColor(robot_color_, this, "选择颜色");
  if (color.isValid()) {
    robot_color_ = color;
    QString color_style = QString("background-color: %1;").arg(color.name());
    robot_color_button_->setStyleSheet(R"(
      QPushButton {
        border: 1px solid #d0d0d0;
        border-radius: 4px;
        padding: 4px;
      }
      QPushButton:hover {
        background-color: #f0f0f0;
        border-color: #1976d2;
      }
    )" + color_style);
    
    auto &config = Config::ConfigManager::Instance()->GetRootConfig();
    QString color_str = QString("0x%1").arg(color.rgb(), 8, 16, QChar('0')).toUpper();
    config.robot_shape_config.color = color_str.toStdString();
    AutoSaveConfig();
  }
}

void DisplayConfigWidget::OnRobotShapeOpacityChanged(int value) {
  auto &config = Config::ConfigManager::Instance()->GetRootConfig();
  config.robot_shape_config.opacity = value / 100.0f;
  AutoSaveConfig();
}

void DisplayConfigWidget::UpdateDisplayVisibility(const std::string &display_name, bool visible) {
  auto display = Display::FactoryDisplay::Instance()->GetDisplay(display_name);
  if (display) {
    display->setVisible(visible);
    LOG_INFO("Display " << display_name << " visibility set to " << (visible ? "visible" : "hidden"));
  }
}

void DisplayConfigWidget::AutoSaveConfig() {
  Config::ConfigManager::Instance()->StoreConfig();
}

void DisplayConfigWidget::LoadConfig() {
  auto &config = Config::ConfigManager::Instance()->GetRootConfig();
  
  for (auto &display_config : config.display_config) {
    auto toggle_it = display_toggle_buttons_.find(display_config.display_name);
    if (toggle_it != display_toggle_buttons_.end()) {
      toggle_it->second->blockSignals(true);
      toggle_it->second->setChecked(display_config.visible);
      toggle_it->second->setText(display_config.visible ? "✓" : "");
      toggle_it->second->blockSignals(false);
      UpdateDisplayVisibility(display_config.display_name, display_config.visible);
    }
    
    auto topic_it = display_topic_edits_.find(display_config.display_name);
    if (topic_it != display_topic_edits_.end()) {
      topic_it->second->blockSignals(true);
      topic_it->second->setText(QString::fromStdString(display_config.topic));
      topic_it->second->blockSignals(false);
    }
  }
  
  RefreshKeyValueTab();
  
  image_table_->blockSignals(true);
  image_table_->setRowCount(0);
  for (size_t i = 0; i < config.images.size(); i++) {
    const auto &image_config = config.images[i];
    int row = image_table_->rowCount();
    image_table_->insertRow(row);
    
    QTableWidgetItem *location_item = new QTableWidgetItem(QString::fromStdString(image_config.location));
    QTableWidgetItem *topic_item = new QTableWidgetItem(QString::fromStdString(image_config.topic));
    QTableWidgetItem *enable_item = new QTableWidgetItem(image_config.enable ? "true" : "false");
    enable_item->setFlags(enable_item->flags() & ~Qt::ItemIsEditable);
    
    QCheckBox *enable_checkbox = new QCheckBox();
    enable_checkbox->setChecked(image_config.enable);
    enable_checkbox->setStyleSheet(R"(
      QCheckBox {
        spacing: 5px;
      }
      QCheckBox::indicator {
        width: 18px;
        height: 18px;
        border: 1px solid #d0d0d0;
        border-radius: 3px;
        background-color: #ffffff;
      }
      QCheckBox::indicator:checked {
        background-color: #4caf50;
        border-color: #4caf50;
      }
    )");
    connect(enable_checkbox, &QCheckBox::toggled, [this, row](bool checked) {
      image_table_->item(row, 2)->setText(checked ? "true" : "false");
      OnImageConfigChanged(row);
    });
    
    QPushButton *remove_btn = new QPushButton("删除");
    remove_btn->setStyleSheet(R"(
      QPushButton {
        border: 1px solid #f44336;
        border-radius: 4px;
        padding: 2px 8px;
        background-color: #f44336;
        color: #ffffff;
      }
      QPushButton:hover {
        background-color: #da190b;
      }
    )");
    connect(remove_btn, &QPushButton::clicked, [this, row]() {
      OnRemoveImageConfig(row);
    });
    
    image_table_->setItem(row, 0, location_item);
    image_table_->setItem(row, 1, topic_item);
    image_table_->setItem(row, 2, enable_item);
    image_table_->setCellWidget(row, 2, enable_checkbox);
    image_table_->setCellWidget(row, 3, remove_btn);
  }
  image_table_->blockSignals(false);
  
  robot_points_table_->blockSignals(true);
  robot_points_table_->setRowCount(0);
  for (const auto &point : config.robot_shape_config.shaped_points) {
    int row = robot_points_table_->rowCount();
    robot_points_table_->insertRow(row);
    robot_points_table_->setItem(row, 0, new QTableWidgetItem(QString::number(point.x)));
    robot_points_table_->setItem(row, 1, new QTableWidgetItem(QString::number(point.y)));
  }
  robot_points_table_->blockSignals(false);
  
  robot_is_ellipse_checkbox_->blockSignals(true);
  robot_is_ellipse_checkbox_->setChecked(config.robot_shape_config.is_ellipse);
  robot_is_ellipse_checkbox_->blockSignals(false);
  
  QString color_str = QString::fromStdString(config.robot_shape_config.color);
  if (color_str.startsWith("0x")) {
    bool ok;
    uint rgb = color_str.mid(2).toUInt(&ok, 16);
    if (ok) {
      robot_color_ = QColor::fromRgb(rgb);
      QString color_style = QString("background-color: %1;").arg(robot_color_.name());
      robot_color_button_->setStyleSheet(R"(
        QPushButton {
          border: 1px solid #d0d0d0;
          border-radius: 4px;
          padding: 4px;
        }
        QPushButton:hover {
          background-color: #f0f0f0;
          border-color: #1976d2;
        }
      )" + color_style);
    }
  }
  
  robot_opacity_slider_->blockSignals(true);
  robot_opacity_slider_->setValue(static_cast<int>(config.robot_shape_config.opacity * 100));
  robot_opacity_label_->setText(QString::number(robot_opacity_slider_->value()) + "%");
  robot_opacity_slider_->blockSignals(false);
  
  // Load channel config
  is_loading_config_ = true;
  
  // 加载通道类型
  std::string channel_type = config.channel_config.channel_type.empty() ? "auto" : config.channel_config.channel_type;
  channel_type_combo_->blockSignals(true);
  int index = channel_type_combo_->findData(QString::fromStdString(channel_type));
  if (index >= 0) {
    channel_type_combo_->setCurrentIndex(index);
  } else {
    channel_type_combo_->setCurrentIndex(0); // default to "auto"
  }
  channel_type_combo_->blockSignals(false);
  
  // 加载 ROSBridge IP
  std::string rosbridge_ip = config.channel_config.rosbridge_config.ip.empty() ? "127.0.0.1" : config.channel_config.rosbridge_config.ip;
  rosbridge_ip_edit_->blockSignals(true);
  rosbridge_ip_edit_->setText(QString::fromStdString(rosbridge_ip));
  rosbridge_ip_edit_->blockSignals(false);
  
  // 加载 ROSBridge 端口
  std::string rosbridge_port = config.channel_config.rosbridge_config.port.empty() ? "9090" : config.channel_config.rosbridge_config.port;
  rosbridge_port_edit_->blockSignals(true);
  rosbridge_port_edit_->setText(QString::fromStdString(rosbridge_port));
  rosbridge_port_edit_->blockSignals(false);
  
  // Enable/disable ROSBridge config based on channel type
  bool show_rosbridge = (channel_type == "rosbridge");
  rosbridge_ip_edit_->setEnabled(show_rosbridge);
  rosbridge_port_edit_->setEnabled(show_rosbridge);
  
  is_loading_config_ = false;
}

void DisplayConfigWidget::SaveConfig() {
  AutoSaveConfig();
}

