#pragma once

#include <memory>

#include "algorithm.h"
#include "point_type.h"
#include "widgets/floating_panel_widget.h"

namespace Ui {
class NavGoalWidget;
}

using namespace basic;

class NavGoalWidget : public FloatingPanelWidget {
  Q_OBJECT
 public:
  enum HandleResult { kSend = 0,
                      kRemove = 1,
                      kCancel = 2,
                      kChangeName = 3 };
  struct PointInfo {
    RobotPose pose;
    QString name;
  };

 private:
  std::unique_ptr<Ui::NavGoalWidget> ui_;
  QString original_name_;

  bool IsAnyControlBeingEdited() const;

 signals:
  void SignalPoseChanged(const RobotPose &pose);
  void SignalHandleOver(const HandleResult &flag, const RobotPose &pose, const QString &name);
  void SignalPointNameChanged(const QString &name);

 public slots:
  void SetPose(const PointInfo &pose);
  void SetEditMode(bool is_edit);

 private slots:
  void SlotUpdateValue(double);

 public:
  explicit NavGoalWidget(QWidget *parent = 0);
  ~NavGoalWidget() override;
};
