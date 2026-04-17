#include "widgets/map_toolbar_widget.h"

#include <QPixmap>

#include "ui_map_toolbar.h"

MapToolBarWidget::MapToolBarWidget(QWidget *parent)
    : QWidget(parent), ui_(std::make_unique<Ui::MapToolBarWidget>()) {
  ui_->setupUi(this);
  setObjectName("mapToolBar");

  auto configure_button = [](QToolButton *button) {
    button->setProperty("variant", "map-toolbar");
    button->setIconSize(QSize(20, 20));
    button->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
  };

  configure_button(ui_->view_menu_btn);
  configure_button(ui_->reloc_btn);
  configure_button(ui_->edit_map_btn);
  configure_button(ui_->open_map_btn);
  configure_button(ui_->save_map_btn);
  configure_button(ui_->re_save_map_btn);

  QIcon view_icon;
  view_icon.addFile(QString::fromUtf8(":/images/list_view.svg"), QSize(32, 32),
                    QIcon::Normal, QIcon::Off);
  ui_->view_menu_btn->setIcon(view_icon);
  ui_->view_menu_btn->setPopupMode(QToolButton::InstantPopup);

  QIcon reloc_icon;
  reloc_icon.addFile(QString::fromUtf8(":/images/reloc2.svg"), QSize(32, 32),
                     QIcon::Normal, QIcon::Off);
  ui_->reloc_btn->setIcon(reloc_icon);

  QIcon edit_icon;
  edit_icon.addFile(QString::fromUtf8(":/images/edit.svg"), QSize(32, 32),
                    QIcon::Normal, QIcon::Off);
  ui_->edit_map_btn->setIcon(edit_icon);

  QIcon open_icon;
  open_icon.addFile(QString::fromUtf8(":/images/open.svg"), QSize(32, 32),
                    QIcon::Normal, QIcon::Off);
  ui_->open_map_btn->setIcon(open_icon);

  QIcon save_icon;
  save_icon.addFile(QString::fromUtf8(":/images/save.svg"), QSize(32, 32),
                    QIcon::Normal, QIcon::Off);
  ui_->save_map_btn->setIcon(save_icon);

  QIcon save_as_icon;
  save_as_icon.addFile(QString::fromUtf8(":/images/re_save.svg"), QSize(32, 32),
                       QIcon::Normal, QIcon::Off);
  ui_->re_save_map_btn->setIcon(save_as_icon);

  ui_->battery_bar_->setAutoFillBackground(true);
  ui_->battery_icon_label->setPixmap(
      QPixmap(QString::fromUtf8(":/images/power-v.png")));
}

MapToolBarWidget::~MapToolBarWidget() = default;

QToolButton *MapToolBarWidget::viewMenuButton() const { return ui_->view_menu_btn; }
QToolButton *MapToolBarWidget::relocButton() const { return ui_->reloc_btn; }
QToolButton *MapToolBarWidget::editMapButton() const { return ui_->edit_map_btn; }
QToolButton *MapToolBarWidget::openMapButton() const { return ui_->open_map_btn; }
QToolButton *MapToolBarWidget::saveMapButton() const { return ui_->save_map_btn; }
QToolButton *MapToolBarWidget::saveAsButton() const { return ui_->re_save_map_btn; }
QProgressBar *MapToolBarWidget::batteryBar() const { return ui_->battery_bar_; }
QLabel *MapToolBarWidget::powerLabel() const { return ui_->label_power_; }
