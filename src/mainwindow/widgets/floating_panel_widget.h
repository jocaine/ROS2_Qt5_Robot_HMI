#pragma once

#include <QWidget>

class FloatingPanelWidget : public QWidget {
  Q_OBJECT

 protected:
  void paintEvent(QPaintEvent *event) override;

 public:
  explicit FloatingPanelWidget(QWidget *parent = nullptr);
  ~FloatingPanelWidget() override = default;
};
