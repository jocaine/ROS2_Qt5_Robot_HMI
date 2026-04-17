#pragma once

#include <QCheckBox>
#include <QSlider>
#include <QWidget>

#include <map>
#include <memory>
#include <vector>

#include "algorithm.h"
#include "point_type.h"
#include "widgets/joystick.h"

namespace Ui {
class SpeedCtrlWidget;
}

using namespace basic;

class SpeedCtrlWidget : public QWidget {
  Q_OBJECT

 private:
  std::unique_ptr<Ui::SpeedCtrlWidget> ui_;
  QCheckBox *checkBox_use_all_{nullptr};
  JoyStick *joyStick_widget_{nullptr};
  QSlider *horizontalSlider_raw_{nullptr};
  QSlider *horizontalSlider_linear_{nullptr};

 signals:
  void signalControlSpeed(const RobotSpeed &speed);

 private slots:
  void slotSpeedControl();
  void slotJoyStickKeyChange(int value);

 public:
  explicit SpeedCtrlWidget(QWidget *parent = 0);
  ~SpeedCtrlWidget() override;
};
