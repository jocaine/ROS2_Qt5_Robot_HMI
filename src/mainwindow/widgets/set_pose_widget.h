#pragma once

#include <memory>

#include "algorithm.h"
#include "point_type.h"
#include "widgets/floating_panel_widget.h"

namespace Ui {
class SetPoseWidget;
}

using namespace basic;

class SetPoseWidget : public FloatingPanelWidget {
  Q_OBJECT

 private:
  std::unique_ptr<Ui::SetPoseWidget> ui_;

 signals:
  void SignalPoseChanged(const RobotPose &pose);
  void SignalHandleOver(const bool &is_submit, const RobotPose &pose);

 public slots:
  void SetPose(const RobotPose &pose);

 private slots:
  void SlotUpdateValue(double);

 public:
  explicit SetPoseWidget(QWidget *parent = 0);
  ~SetPoseWidget() override;
};
