#pragma once

#include <memory>

#include "map/topology_map.h"
#include "widgets/floating_panel_widget.h"

namespace Ui {
class TopologyRouteWidget;
}

class TopologyRouteWidget : public FloatingPanelWidget {
  Q_OBJECT
 public:
  enum HandleResult {
    kDelete = 0,
    kCancel = 1
  };

  struct RouteInfo {
    QString route_name;
    std::string controller;
    std::string goal_checker;
    double speed_limit = 1.0;
  };

 private:
  std::unique_ptr<Ui::TopologyRouteWidget> ui_;

  bool IsAnyControlBeingEdited() const;

 signals:
  void SignalRouteInfoChanged(const RouteInfo &info);
  void SignalHandleOver(const HandleResult &flag, const RouteInfo &info);

 public slots:
  void SetRouteInfo(const RouteInfo &info);
  void SetEditMode(bool is_edit);
  void SetSupportControllers(const std::vector<std::string> &controllers);
  void SetSupportGoalCheckers(const std::vector<std::string> &goal_checkers);

 private slots:
  void SlotUpdateValue();

 public:
  explicit TopologyRouteWidget(QWidget *parent = 0);
  ~TopologyRouteWidget() override;
};
