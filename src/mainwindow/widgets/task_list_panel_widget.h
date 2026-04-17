#pragma once

#include <memory>

#include <QCheckBox>
#include <QLayout>
#include <QPushButton>
#include <QWidget>

namespace Ui {
class TaskListPanelWidget;
}

class TaskListPanelWidget : public QWidget {
  Q_OBJECT

 public:
  explicit TaskListPanelWidget(QWidget *parent = nullptr);
  ~TaskListPanelWidget() override;

  QWidget *tableContainer() const;
  QLayout *tableContainerLayout() const;
  QPushButton *addGoalButton() const;
  QPushButton *startTaskChainButton() const;
  QCheckBox *loopTaskCheckBox() const;
  QPushButton *loadTaskChainButton() const;
  QPushButton *saveTaskChainButton() const;

 private:
  std::unique_ptr<Ui::TaskListPanelWidget> ui_;
};
