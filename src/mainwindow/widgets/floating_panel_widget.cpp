#include "widgets/floating_panel_widget.h"

#include <QPainter>
#include <QPalette>

FloatingPanelWidget::FloatingPanelWidget(QWidget *parent) : QWidget(parent) {
  setAutoFillBackground(true);
  setAttribute(Qt::WA_OpaquePaintEvent);

  QPalette pal = palette();
  pal.setColor(QPalette::Window, QColor(255, 255, 255));
  pal.setColor(QPalette::Base, QColor(255, 255, 255));
  setPalette(pal);
}

void FloatingPanelWidget::paintEvent(QPaintEvent *event) {
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);
  painter.setPen(QPen(QColor(224, 224, 224), 1));
  painter.setBrush(QColor(255, 255, 255));
  painter.drawRoundedRect(rect(), 8, 8);

  QWidget::paintEvent(event);
}
