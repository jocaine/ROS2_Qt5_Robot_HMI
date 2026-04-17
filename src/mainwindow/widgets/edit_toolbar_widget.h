#pragma once

#include <memory>

#include <QToolButton>
#include <QWidget>

namespace Ui {
class EditToolBarWidget;
}

class EditToolBarWidget : public QWidget {
  Q_OBJECT

 public:
  explicit EditToolBarWidget(QWidget *parent = nullptr);
  ~EditToolBarWidget() override;

  QToolButton *normalCursorButton() const;
  QToolButton *addPointButton() const;
  QToolButton *addTopologyPathButton() const;
  QToolButton *addRegionButton() const;
  QToolButton *eraseButton() const;
  QToolButton *drawPenButton() const;
  QToolButton *drawLineButton() const;

 private:
  std::unique_ptr<Ui::EditToolBarWidget> ui_;
};
