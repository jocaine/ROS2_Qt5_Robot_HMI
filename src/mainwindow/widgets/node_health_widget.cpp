#include "node_health_widget.h"
#include "ui_node_health_widget.h"
#include <set>
#include <QString>

NodeHealthWidget::NodeHealthWidget(QWidget *parent)
    : QWidget(parent), ui_(std::make_unique<Ui::NodeHealthWidget>()) {
  ui_->setupUi(this);
  ui_->tree_->setColumnWidth(0, 180);
}

NodeHealthWidget::~NodeHealthWidget() = default;

void NodeHealthWidget::update(const SystemHealthStatus &status) {
  switch (status.overall_level) {
    case HealthLevel::Normal:
      ui_->overall_label_->setText("系统健康：正常");
      ui_->overall_label_->setStyleSheet("font-weight:bold; border-radius:4px;"
                                         "background:#2d7a2d; color:white;");
      break;
    case HealthLevel::Degraded:
      ui_->overall_label_->setText("系统健康：降级");
      ui_->overall_label_->setStyleSheet("font-weight:bold; border-radius:4px;"
                                         "background:#b8860b; color:white;");
      break;
    case HealthLevel::Fault:
      ui_->overall_label_->setText("系统健康：故障");
      ui_->overall_label_->setStyleSheet("font-weight:bold; border-radius:4px;"
                                         "background:#a02020; color:white;");
      break;
  }

  // 保存当前展开状态
  std::set<QString> expanded_groups;
  for (int i = 0; i < ui_->tree_->topLevelItemCount(); ++i) {
    auto *item = ui_->tree_->topLevelItem(i);
    if (item->isExpanded())
      expanded_groups.insert(item->text(0));
  }
  bool first_update = expanded_groups.empty() && ui_->tree_->topLevelItemCount() == 0;

  ui_->tree_->clear();

  for (const auto &group : status.groups) {
    QString group_text = QString::fromStdString(group.group_name);
    if (group.critical) group_text += " [关键]";
    QString group_status = QString("%1/%2 在线").arg(group.online_count).arg(group.total_count);

    auto *group_item = new QTreeWidgetItem(ui_->tree_, {group_text, group_status});
    group_item->setForeground(1, group.healthy() ? QColor("#2d7a2d") : QColor("#a02020"));

    for (const auto &node : group.nodes) {
      auto *node_item = new QTreeWidgetItem(group_item, {
          QString::fromStdString(node.name), node.online ? "在线" : "离线"});
      node_item->setForeground(1, node.online ? QColor("#2d7a2d") : QColor("#a02020"));
    }

    for (const auto &topic : group.topics) {
      QString topic_status = topic.timeout
          ? QString("超时 (%1s)").arg(topic.last_received_seconds_ago)
          : "正常";
      auto *topic_item = new QTreeWidgetItem(group_item, {
          QString::fromStdString(topic.topic_name), topic_status});
      topic_item->setForeground(1, topic.timeout ? QColor("#a02020") : QColor("#2d7a2d"));
    }

    // 首次加载时故障组自动展开；后续保留用户操作的展开状态
    bool was_expanded = expanded_groups.count(group_text) > 0;
    group_item->setExpanded(first_update ? !group.healthy() : was_expanded);
  }

  if (!status.ungrouped_nodes.empty()) {
    QString ungrouped_key = "未分组节点";
    auto *ungrouped = new QTreeWidgetItem(ui_->tree_, {ungrouped_key, ""});
    for (const auto &node : status.ungrouped_nodes) {
      auto *item = new QTreeWidgetItem(ungrouped, {
          QString::fromStdString(node.name), node.online ? "在线" : "离线"});
      item->setForeground(1, node.online ? QColor("#2d7a2d") : QColor("#a02020"));
    }
    bool was_expanded = expanded_groups.count(ungrouped_key) > 0;
    ungrouped->setExpanded(first_update ? true : was_expanded);
  }
}

