#pragma once
#include <QWidget>
#include <QColor>
#include <memory>
#include <map>
#include <string>
#include <vector>

namespace Ui {
class DisplayConfigWidget;
}

namespace Display {
class DisplayManager;
}

class QToolButton;
class QLineEdit;
class QCheckBox;
class QPushButton;
class QSlider;
class QLabel;
class QTableWidget;
class QComboBox;
class QListWidget;
class QSpinBox;

class DisplayConfigWidget : public QWidget {
  Q_OBJECT

 public:
  explicit DisplayConfigWidget(QWidget *parent = nullptr);
  ~DisplayConfigWidget();

  void SetDisplayManager(Display::DisplayManager *manager);
  void SetChannelList(const std::vector<std::string> &channel_list);
  void LoadConfig();
  void SaveConfig();

 private slots:
  void OnToggleDisplay(const std::string &display_name, bool visible);
  void OnDisplayTopicChanged(const std::string &display_name, const QString &topic);
  void OnKeyValueChanged(const std::string &key, const QString &value);
  void OnAddKeyValue();
  void OnRemoveKeyValue(const std::string &key);
  void OnAddImageConfig();
  void OnRemoveImageConfig(int row);
  void OnImageConfigChanged(int row);
  void OnRobotShapePointChanged();
  void OnRobotShapeIsEllipseChanged(bool checked);
  void OnRobotShapeColorChanged();
  void OnRobotShapeOpacityChanged(int value);

 private:
  void InitUI();
  void InitDisplayConfigTab();
  void InitKeyValueTab();
  void InitImageConfigTab();
  void InitRobotShapeTab();
  void InitChannelConfigTab();
  void InitNodeGroupTab();
  void RefreshKeyValueTab();
  void RefreshNodeGroupList();
  void LoadGroupDetail(int index);
  void RefreshNodesList();
  void RefreshTopicsList();
  void OnNodeGroupSelected(int row);
  void OnAddNodeGroup();
  void OnDeleteNodeGroup();
  void OnSaveAndReloadNodeGroup();
  void OnAddExpectedNode();
  void OnAddHealthTopic();
  void UpdateDisplayVisibility(const std::string &display_name, bool visible);
  void AutoSaveConfig();
  std::string ExtractChannelType(const std::string &channel_path);

  std::unique_ptr<Ui::DisplayConfigWidget> ui_;
  Display::DisplayManager *display_manager_{nullptr};

  std::map<std::string, QToolButton*> display_toggle_buttons_;
  std::map<std::string, QLineEdit*> display_topic_edits_;
  std::map<std::string, QLineEdit*> key_value_edits_;

  QTableWidget *robot_points_table_{nullptr};
  QCheckBox *robot_is_ellipse_checkbox_{nullptr};
  QPushButton *robot_color_button_{nullptr};
  QSlider *robot_opacity_slider_{nullptr};
  QLabel *robot_opacity_label_{nullptr};
  QColor robot_color_;

  QComboBox *channel_type_combo_{nullptr};
  QLineEdit *rosbridge_ip_edit_{nullptr};
  QLineEdit *rosbridge_port_edit_{nullptr};
  bool is_loading_config_{false};
  std::vector<std::string> channel_list_;

  QListWidget *node_group_list_{nullptr};
  QLineEdit *group_name_edit_{nullptr};
  QCheckBox *group_critical_checkbox_{nullptr};
  QSpinBox *group_timeout_spinbox_{nullptr};
  int current_group_index_{-1};
};
