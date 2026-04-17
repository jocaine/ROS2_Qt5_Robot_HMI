#include "widgets/edit_toolbar_widget.h"

#include "ui_edit_toolbar.h"

EditToolBarWidget::EditToolBarWidget(QWidget *parent)
    : QWidget(parent), ui_(std::make_unique<Ui::EditToolBarWidget>()) {
  ui_->setupUi(this);
  setObjectName("editToolBar");

  auto configure_button = [](QToolButton *button, const QString &tooltip,
                             const QString &icon_path) {
    button->setProperty("variant", "edit-toolbar");
    button->setToolTip(tooltip);
    button->setCursor(Qt::PointingHandCursor);
    button->setIconSize(QSize(24, 24));
    QIcon icon;
    icon.addFile(icon_path, QSize(), QIcon::Normal, QIcon::Off);
    button->setIcon(icon);
  };

  configure_button(ui_->normal_cursor_btn, QString::fromUtf8("鼠标"),
                   QString::fromUtf8(":/images/cursor_point_btn.svg"));
  configure_button(ui_->add_point_btn, QString::fromUtf8("添加工位点"),
                   QString::fromUtf8(":/images/point_btn.svg"));
  configure_button(ui_->add_topology_path_btn, QString::fromUtf8("连接工位点"),
                   QString::fromUtf8(":/images/topo_link_btn.svg"));
  configure_button(ui_->add_region_btn, QString::fromUtf8("添加区域"),
                   QString::fromUtf8(":/images/region_btn.svg"));
  configure_button(ui_->erase_btn, QString::fromUtf8("橡皮擦"),
                   QString::fromUtf8(":/images/erase_btn.svg"));
  configure_button(ui_->draw_pen_btn, QString::fromUtf8("障碍物绘制"),
                   QString::fromUtf8(":/images/pen.svg"));
  configure_button(ui_->draw_line_btn, QString::fromUtf8("线段绘制"),
                   QString::fromUtf8(":/images/line_btn.svg"));

  ui_->add_topology_path_btn->setEnabled(true);
}

EditToolBarWidget::~EditToolBarWidget() = default;

QToolButton *EditToolBarWidget::normalCursorButton() const { return ui_->normal_cursor_btn; }
QToolButton *EditToolBarWidget::addPointButton() const { return ui_->add_point_btn; }
QToolButton *EditToolBarWidget::addTopologyPathButton() const { return ui_->add_topology_path_btn; }
QToolButton *EditToolBarWidget::addRegionButton() const { return ui_->add_region_btn; }
QToolButton *EditToolBarWidget::eraseButton() const { return ui_->erase_btn; }
QToolButton *EditToolBarWidget::drawPenButton() const { return ui_->draw_pen_btn; }
QToolButton *EditToolBarWidget::drawLineButton() const { return ui_->draw_line_btn; }
