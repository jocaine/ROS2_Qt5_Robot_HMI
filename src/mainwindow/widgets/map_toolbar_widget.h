#pragma once

#include <memory>

#include <QLabel>
#include <QProgressBar>
#include <QToolButton>
#include <QWidget>

namespace Ui {
class MapToolBarWidget;
}

class MapToolBarWidget : public QWidget {
  Q_OBJECT

 public:
  explicit MapToolBarWidget(QWidget *parent = nullptr);
  ~MapToolBarWidget() override;

  QToolButton *viewMenuButton() const;
  QToolButton *relocButton() const;
  QToolButton *editMapButton() const;
  QToolButton *openMapButton() const;
  QToolButton *saveMapButton() const;
  QToolButton *saveAsButton() const;
  QProgressBar *batteryBar() const;
  QLabel *powerLabel() const;
  QLabel *healthIndicator() const;

 private:
  std::unique_ptr<Ui::MapToolBarWidget> ui_;
};
