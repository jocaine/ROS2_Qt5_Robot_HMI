#include "widgets/nav_goal_widget.h"

#include <QKeySequence>
#include <QPushButton>

#include "ui_nav_goal_widget.h"

NavGoalWidget::NavGoalWidget(QWidget *parent)
    : FloatingPanelWidget(parent), ui_(std::make_unique<Ui::NavGoalWidget>()) {
  ui_->setupUi(this);
  setObjectName("navGoalWidget");

  ui_->button_remove->setShortcut(QKeySequence::Delete);
  ui_->button_remove->setToolTip("删除点位 (Delete/Backspace)");

  connect(ui_->spinBox_x, SIGNAL(valueChanged(double)), this,
          SLOT(SlotUpdateValue(double)));
  connect(ui_->spinBox_y, SIGNAL(valueChanged(double)), this,
          SLOT(SlotUpdateValue(double)));
  connect(ui_->spinBox_theta, SIGNAL(valueChanged(double)), this,
          SLOT(SlotUpdateValue(double)));

  connect(ui_->button_edit_name, &QPushButton::clicked, [this]() {
    if (ui_->lineEdit_name->isReadOnly()) {
      original_name_ = ui_->lineEdit_name->text();
      ui_->lineEdit_name->setReadOnly(false);
      ui_->lineEdit_name->setFocus();
      ui_->button_edit_name->setText("保存名称");
    } else {
      QString new_name = ui_->lineEdit_name->text();
      if (new_name != original_name_) {
        emit SignalHandleOver(HandleResult::kChangeName,
                              RobotPose(ui_->spinBox_x->value(), ui_->spinBox_y->value(),
                                        deg2rad(ui_->spinBox_theta->value())),
                              new_name);
      }
      ui_->lineEdit_name->setReadOnly(true);
      ui_->button_edit_name->setText("编辑名称");
    }
  });

  connect(ui_->button_send, &QPushButton::clicked, [this]() {
    emit SignalHandleOver(HandleResult::kSend,
                          RobotPose(ui_->spinBox_x->value(), ui_->spinBox_y->value(),
                                    deg2rad(ui_->spinBox_theta->value())),
                          ui_->lineEdit_name->text());
  });

  connect(ui_->button_cancel, &QPushButton::clicked, [this]() {
    emit SignalHandleOver(HandleResult::kCancel,
                          RobotPose(ui_->spinBox_x->value(), ui_->spinBox_y->value(),
                                    deg2rad(ui_->spinBox_theta->value())),
                          ui_->lineEdit_name->text());
  });

  connect(ui_->button_remove, &QPushButton::clicked, [this]() {
    emit SignalHandleOver(HandleResult::kRemove,
                          RobotPose(ui_->spinBox_x->value(), ui_->spinBox_y->value(),
                                    deg2rad(ui_->spinBox_theta->value())),
                          ui_->lineEdit_name->text());
  });
}

NavGoalWidget::~NavGoalWidget() = default;

void NavGoalWidget::SlotUpdateValue(double) {
  emit SignalPoseChanged(RobotPose(ui_->spinBox_x->value(), ui_->spinBox_y->value(),
                                   deg2rad(ui_->spinBox_theta->value())));
}

void NavGoalWidget::SetEditMode(bool flag) {
  ui_->spinBox_x->setEnabled(flag);
  ui_->spinBox_y->setEnabled(flag);
  ui_->spinBox_theta->setEnabled(flag);
  ui_->lineEdit_name->setEnabled(flag);
  ui_->button_remove->setVisible(flag);
  ui_->button_edit_name->setVisible(flag);
  ui_->button_send->setVisible(!flag);
}

void NavGoalWidget::SetPose(const PointInfo &info) {
  if (IsAnyControlBeingEdited()) {
    return;
  }
  ui_->spinBox_x->blockSignals(true);
  ui_->spinBox_y->blockSignals(true);
  ui_->spinBox_theta->blockSignals(true);
  ui_->lineEdit_name->blockSignals(true);
  ui_->spinBox_x->setValue(info.pose.x);
  ui_->spinBox_y->setValue(info.pose.y);
  ui_->spinBox_theta->setValue(rad2deg(info.pose.theta));
  ui_->lineEdit_name->setText(info.name);
  original_name_ = info.name;
  ui_->lineEdit_name->setReadOnly(true);
  ui_->button_edit_name->setText("编辑名称");
  ui_->spinBox_x->blockSignals(false);
  ui_->spinBox_y->blockSignals(false);
  ui_->spinBox_theta->blockSignals(false);
  ui_->lineEdit_name->blockSignals(false);
}

bool NavGoalWidget::IsAnyControlBeingEdited() const {
  return ui_->spinBox_x->hasFocus() || ui_->spinBox_y->hasFocus() ||
         ui_->spinBox_theta->hasFocus() || ui_->lineEdit_name->hasFocus();
}
