#include "widgets/task_list_panel_widget.h"

#include <QVBoxLayout>

#include "ui_task_list_panel.h"

TaskListPanelWidget::TaskListPanelWidget(QWidget *parent)
    : QWidget(parent), ui_(std::make_unique<Ui::TaskListPanelWidget>()) {
  ui_->setupUi(this);
  setObjectName("taskListPanel");
}

TaskListPanelWidget::~TaskListPanelWidget() = default;

QWidget *TaskListPanelWidget::tableContainer() const { return ui_->table_container; }
QLayout *TaskListPanelWidget::tableContainerLayout() const { return ui_->table_container->layout(); }
QPushButton *TaskListPanelWidget::addGoalButton() const { return ui_->btn_add_one_goal; }
QPushButton *TaskListPanelWidget::startTaskChainButton() const { return ui_->btn_start_task_chain; }
QCheckBox *TaskListPanelWidget::loopTaskCheckBox() const { return ui_->loop_task_checkbox; }
QPushButton *TaskListPanelWidget::loadTaskChainButton() const { return ui_->btn_load_task_chain; }
QPushButton *TaskListPanelWidget::saveTaskChainButton() const { return ui_->btn_save_task_chain; }
