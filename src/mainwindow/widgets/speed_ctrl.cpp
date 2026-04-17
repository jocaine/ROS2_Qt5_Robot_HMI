#include "widgets/speed_ctrl.h"

#include <QHBoxLayout>
#include <QPushButton>

#include "ui_speed_ctrl.h"

namespace {
void applySpeedButtonStyle(QPushButton *button, const QString &normal_image,
                           const QString &pressed_image) {
  button->setStyleSheet(QString::fromUtf8(
      "QPushButton{border-image: url(%1);}\n"
      "QPushButton{border:none;}\n"
      "QPushButton:pressed{border-image: url(%2);}"
      ).arg(normal_image, pressed_image));
}

char directionKeyFromJoystick(int value, bool is_all) {
  switch (value) {
    case JoyStick::Direction::upleft:
      return is_all ? 'U' : 'u';
    case JoyStick::Direction::up:
      return is_all ? 'I' : 'i';
    case JoyStick::Direction::upright:
      return is_all ? 'O' : 'o';
    case JoyStick::Direction::left:
      return is_all ? 'J' : 'j';
    case JoyStick::Direction::right:
      return is_all ? 'L' : 'l';
    case JoyStick::Direction::down:
      return is_all ? 'M' : 'm';
    case JoyStick::Direction::downleft:
      return is_all ? '<' : ',';
    case JoyStick::Direction::downright:
      return is_all ? '>' : '.';
    default:
      return '\0';
  }
}

char directionKeyFromButton(char button_key, bool is_all) {
  switch (button_key) {
    case 'u':
      return is_all ? 'U' : 'u';
    case 'i':
      return is_all ? 'I' : 'i';
    case 'o':
      return is_all ? 'O' : 'o';
    case 'j':
      return is_all ? 'J' : 'j';
    case 'l':
      return is_all ? 'L' : 'l';
    case 'm':
      return is_all ? 'M' : 'm';
    case ',':
      return is_all ? '<' : ',';
    case '.':
      return is_all ? '>' : '.';
    default:
      return '\0';
  }
}

std::map<char, std::vector<float>> moveBindings() {
  return {{'i', {1, 0, 0, 0}},   {'o', {1, 0, 0, -1}}, {'j', {0, 0, 0, 1}},
          {'l', {0, 0, 0, -1}},  {'u', {1, 0, 0, 1}},  {',', {-1, 0, 0, 0}},
          {'.', {-1, 0, 0, 1}},  {'m', {-1, 0, 0, -1}}, {'O', {1, -1, 0, 0}},
          {'I', {1, 0, 0, 0}},   {'J', {0, 1, 0, 0}},  {'L', {0, -1, 0, 0}},
          {'U', {1, 1, 0, 0}},   {'<', {-1, 0, 0, 0}}, {'>', {-1, -1, 0, 0}},
          {'M', {-1, 1, 0, 0}},  {'t', {0, 0, 1, 0}},  {'b', {0, 0, -1, 0}},
          {'k', {0, 0, 0, 0}},   {'K', {0, 0, 0, 0}}};
}
}  // namespace

SpeedCtrlWidget::SpeedCtrlWidget(QWidget *parent)
    : QWidget(parent), ui_(std::make_unique<Ui::SpeedCtrlWidget>()) {
  ui_->setupUi(this);

  checkBox_use_all_ = ui_->checkBox_use_all_;
  horizontalSlider_raw_ = ui_->horizontalSlider_raw_;
  horizontalSlider_linear_ = ui_->horizontalSlider_linear_;

  joyStick_widget_ = new JoyStick(ui_->joystickContainer);
  joyStick_widget_->setMinimumSize(QSize(200, 200));
  auto *joystick_layout = new QHBoxLayout(ui_->joystickContainer);
  joystick_layout->setContentsMargins(0, 0, 0, 0);
  joystick_layout->addWidget(joyStick_widget_);

  ui_->button_u->setShortcut(QKeySequence(QStringLiteral("u")));
  ui_->button_i->setShortcut(QKeySequence(QStringLiteral("i")));
  ui_->button_o->setShortcut(QKeySequence(QStringLiteral("o")));
  ui_->button_j->setShortcut(QKeySequence(QStringLiteral("j")));
  ui_->button_l->setShortcut(QKeySequence(QStringLiteral("l")));
  ui_->button_m->setShortcut(QKeySequence(QStringLiteral("m")));
  ui_->button_back->setShortcut(QKeySequence(QStringLiteral(",")));
  ui_->button_backr->setShortcut(QKeySequence(QStringLiteral(".")));
  ui_->btn_stop->setShortcut(QKeySequence(QStringLiteral("s")));

  applySpeedButtonStyle(ui_->button_u, "://images/up_left.png", "://images/up_left_2.png");
  applySpeedButtonStyle(ui_->button_i, "://images/up.png", "://images/up_2.png");
  applySpeedButtonStyle(ui_->button_o, "://images/up_right.png", "://images/up_right_2.png");
  applySpeedButtonStyle(ui_->button_j, "://images/left.png", "://images/left_2.png");
  applySpeedButtonStyle(ui_->button_l, "://images/right.png", "://images/right_2.png");
  applySpeedButtonStyle(ui_->button_m, "://images/down_left.png", "://images/down_left_2.png");
  applySpeedButtonStyle(ui_->button_back, "://images/down.png", "://images/down_2.png");
  applySpeedButtonStyle(ui_->button_backr, "://images/down_right.png", "://images/down_right_2.png");

  connect(ui_->button_i, SIGNAL(clicked()), this, SLOT(slotSpeedControl()));
  connect(ui_->button_u, SIGNAL(clicked()), this, SLOT(slotSpeedControl()));
  connect(ui_->button_o, SIGNAL(clicked()), this, SLOT(slotSpeedControl()));
  connect(ui_->button_j, SIGNAL(clicked()), this, SLOT(slotSpeedControl()));
  connect(ui_->button_l, SIGNAL(clicked()), this, SLOT(slotSpeedControl()));
  connect(ui_->button_m, SIGNAL(clicked()), this, SLOT(slotSpeedControl()));
  connect(ui_->button_back, SIGNAL(clicked()), this, SLOT(slotSpeedControl()));
  connect(ui_->button_backr, SIGNAL(clicked()), this, SLOT(slotSpeedControl()));

  connect(joyStick_widget_, SIGNAL(keyNumchanged(int)), this,
          SLOT(slotJoyStickKeyChange(int)));

  ui_->label_raw_value->setText(
      QString::number(rad2deg(horizontalSlider_raw_->value() * 0.01)) + " deg/s");
  ui_->label_linear_value->setText(
      QString::number(horizontalSlider_linear_->value() * 0.01) + " m/s");

  connect(horizontalSlider_raw_, &QSlider::valueChanged, this, [this](qreal value) {
    ui_->label_raw_value->setText(QString::number(rad2deg(value * 0.01)) + " deg/s");
  });
  connect(horizontalSlider_linear_, &QSlider::valueChanged, this, [this](qreal value) {
    ui_->label_linear_value->setText(QString::number(value * 0.01) + " m/s");
  });
  connect(ui_->btn_stop, &QPushButton::clicked,
          [this]() { emit signalControlSpeed(RobotSpeed()); });

  ui_->btn_stop->setStyleSheet(
      "QPushButton {background-color: red; color: white; font-size: 14px;}"
      "QPushButton:hover {background-color: darkred;}");
}

SpeedCtrlWidget::~SpeedCtrlWidget() = default;

void SpeedCtrlWidget::slotSpeedControl() {
  QPushButton *btn = qobject_cast<QPushButton *>(sender());
  if (!btn || btn->text().isEmpty()) {
    return;
  }

  const char button_key = btn->text().toStdString()[0];
  const float liner = horizontalSlider_linear_->value() * 0.01;
  const float turn = horizontalSlider_raw_->value() * 0.01;
  const bool is_all = checkBox_use_all_->isChecked();
  const char key = directionKeyFromButton(button_key, is_all);
  if (key == '\0') {
    return;
  }

  const auto bindings = moveBindings();
  emit signalControlSpeed(
      RobotSpeed(bindings.at(key)[0] * liner, bindings.at(key)[1] * liner,
                 bindings.at(key)[3] * turn));
}

void SpeedCtrlWidget::slotJoyStickKeyChange(int value) {
  const float liner = horizontalSlider_linear_->value() * 0.01;
  const float turn = horizontalSlider_raw_->value() * 0.01;
  const bool is_all = checkBox_use_all_->isChecked();
  const char key = directionKeyFromJoystick(value, is_all);
  if (key == '\0') {
    return;
  }

  std::cout << "joy stic value:" << value << std::endl;
  const auto bindings = moveBindings();
  emit signalControlSpeed(
      RobotSpeed(bindings.at(key)[0] * liner, bindings.at(key)[1] * liner,
                 bindings.at(key)[3] * turn));
}
