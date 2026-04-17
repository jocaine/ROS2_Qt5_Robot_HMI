#include "widgets/topology_route_widget.h"

#include <QKeySequence>

#include "ui_topology_route_widget.h"

TopologyRouteWidget::TopologyRouteWidget(QWidget *parent)
    : FloatingPanelWidget(parent),
      ui_(std::make_unique<Ui::TopologyRouteWidget>()) {
  ui_->setupUi(this);
  setObjectName("topologyRouteWidget");

  ui_->button_delete->setShortcut(QKeySequence::Delete);
  ui_->button_delete->setToolTip("删除路径 (Delete/Backspace)");

  connect(ui_->comboBox_controller, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &TopologyRouteWidget::SlotUpdateValue);
  connect(ui_->comboBox_goal_checker,
          QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          &TopologyRouteWidget::SlotUpdateValue);
  connect(ui_->spinBox_speed_limit, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &TopologyRouteWidget::SlotUpdateValue);

  connect(ui_->button_delete, &QPushButton::clicked, [this]() {
    RouteInfo info;
    info.route_name = ui_->lineEdit_route_name->text();
    info.controller = ui_->comboBox_controller->currentText().toStdString();
    info.goal_checker = ui_->comboBox_goal_checker->currentText().toStdString();
    double speed_limit = ui_->spinBox_speed_limit->value();
    info.speed_limit = (speed_limit > 0) ? speed_limit : 1.0;
    emit SignalHandleOver(HandleResult::kDelete, info);
  });

  connect(ui_->button_cancel, &QPushButton::clicked, [this]() {
    RouteInfo info;
    info.route_name = ui_->lineEdit_route_name->text();
    info.controller = ui_->comboBox_controller->currentText().toStdString();
    info.goal_checker = ui_->comboBox_goal_checker->currentText().toStdString();
    double speed_limit = ui_->spinBox_speed_limit->value();
    info.speed_limit = (speed_limit > 0) ? speed_limit : 1.0;
    emit SignalHandleOver(HandleResult::kCancel, info);
  });
}

TopologyRouteWidget::~TopologyRouteWidget() = default;

void TopologyRouteWidget::SetEditMode(bool is_edit) {
  ui_->comboBox_controller->setEnabled(is_edit);
  ui_->comboBox_goal_checker->setEnabled(is_edit);
  ui_->spinBox_speed_limit->setEnabled(is_edit);
  ui_->button_delete->setEnabled(is_edit);
}

void TopologyRouteWidget::SetSupportControllers(
    const std::vector<std::string> &controllers) {
  ui_->comboBox_controller->clear();
  for (const auto &controller : controllers) {
    ui_->comboBox_controller->addItem(controller.c_str());
    std::cout << "add controller:" << controller << std::endl;
  }
}

void TopologyRouteWidget::SetSupportGoalCheckers(
    const std::vector<std::string> &goal_checkers) {
  ui_->comboBox_goal_checker->clear();
  for (const auto &goal_checker : goal_checkers) {
    ui_->comboBox_goal_checker->addItem(goal_checker.c_str());
    std::cout << "add goal_checker:" << goal_checker << std::endl;
  }
}

void TopologyRouteWidget::SlotUpdateValue() {
  RouteInfo info;
  info.route_name = ui_->lineEdit_route_name->text();
  info.controller = ui_->comboBox_controller->currentText().toStdString();
  info.goal_checker = ui_->comboBox_goal_checker->currentText().toStdString();

  double speed_limit = ui_->spinBox_speed_limit->value();
  if (speed_limit > 0 && speed_limit <= 10.0) {
    info.speed_limit = speed_limit;
  } else {
    info.speed_limit = 1.0;
    ui_->spinBox_speed_limit->setValue(1.0);
  }

  emit SignalRouteInfoChanged(info);
}

void TopologyRouteWidget::SetRouteInfo(const RouteInfo &info) {
  if (IsAnyControlBeingEdited()) {
    return;
  }
  ui_->lineEdit_route_name->blockSignals(true);
  ui_->comboBox_controller->blockSignals(true);
  ui_->comboBox_goal_checker->blockSignals(true);
  ui_->spinBox_speed_limit->blockSignals(true);

  ui_->lineEdit_route_name->setText(info.route_name);

  int controller_index = ui_->comboBox_controller->findText(info.controller.c_str());
  if (controller_index >= 0) {
    ui_->comboBox_controller->setCurrentIndex(controller_index);
  } else {
    ui_->comboBox_controller->setCurrentIndex(0);
  }

  int goal_checker_index = ui_->comboBox_goal_checker->findText(info.goal_checker.c_str());
  if (goal_checker_index >= 0) {
    ui_->comboBox_goal_checker->setCurrentIndex(goal_checker_index);
  } else {
    ui_->comboBox_goal_checker->setCurrentIndex(0);
  }

  ui_->spinBox_speed_limit->setValue(info.speed_limit);

  ui_->lineEdit_route_name->blockSignals(false);
  ui_->comboBox_controller->blockSignals(false);
  ui_->comboBox_goal_checker->blockSignals(false);
  ui_->spinBox_speed_limit->blockSignals(false);
}

bool TopologyRouteWidget::IsAnyControlBeingEdited() const {
  return ui_->comboBox_controller->hasFocus() ||
         ui_->comboBox_goal_checker->hasFocus() ||
         ui_->spinBox_speed_limit->hasFocus();
}
