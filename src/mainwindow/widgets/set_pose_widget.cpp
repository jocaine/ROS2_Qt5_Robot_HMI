#include "widgets/set_pose_widget.h"

#include "ui_set_pose_widget.h"

SetPoseWidget::SetPoseWidget(QWidget *parent)
    : FloatingPanelWidget(parent), ui_(std::make_unique<Ui::SetPoseWidget>()) {
  ui_->setupUi(this);
  setObjectName("setPoseWidget");

  connect(ui_->spinBox_x, SIGNAL(valueChanged(double)), this,
          SLOT(SlotUpdateValue(double)));
  connect(ui_->spinBox_y, SIGNAL(valueChanged(double)), this,
          SLOT(SlotUpdateValue(double)));
  connect(ui_->spinBox_theta, SIGNAL(valueChanged(double)), this,
          SLOT(SlotUpdateValue(double)));
  connect(ui_->button_ok, &QPushButton::clicked, [this]() {
    emit SignalHandleOver(true,
                          RobotPose(ui_->spinBox_x->value(), ui_->spinBox_y->value(),
                                    deg2rad(ui_->spinBox_theta->value())));
  });
  connect(ui_->button_cancel, &QPushButton::clicked, [this]() {
    emit SignalHandleOver(false,
                          RobotPose(ui_->spinBox_x->value(), ui_->spinBox_y->value(),
                                    deg2rad(ui_->spinBox_theta->value())));
  });
}

SetPoseWidget::~SetPoseWidget() = default;

void SetPoseWidget::SlotUpdateValue(double) {
  emit SignalPoseChanged(RobotPose(ui_->spinBox_x->value(), ui_->spinBox_y->value(),
                                   deg2rad(ui_->spinBox_theta->value())));
}

void SetPoseWidget::SetPose(const RobotPose &pose) {
  ui_->spinBox_x->blockSignals(true);
  ui_->spinBox_y->blockSignals(true);
  ui_->spinBox_theta->blockSignals(true);
  ui_->spinBox_x->setValue(pose.x);
  ui_->spinBox_y->setValue(pose.y);
  ui_->spinBox_theta->setValue(rad2deg(pose.theta));
  ui_->spinBox_x->blockSignals(false);
  ui_->spinBox_y->blockSignals(false);
  ui_->spinBox_theta->blockSignals(false);
}
