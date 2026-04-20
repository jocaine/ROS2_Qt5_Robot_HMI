#pragma once
#include <QWidget>
#include <memory>
#include "node_group.h"

namespace Ui {
class NodeHealthWidget;
}

class NodeHealthWidget : public QWidget {
  Q_OBJECT
 public:
  explicit NodeHealthWidget(QWidget *parent = nullptr);
  ~NodeHealthWidget() override;
  void update(const SystemHealthStatus &status);

 private:
  std::unique_ptr<Ui::NodeHealthWidget> ui_;
};
