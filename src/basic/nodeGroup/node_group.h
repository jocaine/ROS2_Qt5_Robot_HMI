#pragma once
#include <string>
#include <vector>
#include <algorithm>

enum class HealthLevel { Normal, Degraded, Fault };

struct NodeStatus {
    std::string name;
    bool online;
};

struct TopicStatus {
    std::string topic_name;
    int last_received_seconds_ago;  // -1 表示从未收到
    bool timeout;
};

struct NodeGroupStatus {
    std::string group_name;
    bool critical;
    std::vector<NodeStatus> nodes;
    std::vector<TopicStatus> topics;
    int total_count;
    int online_count;
    bool healthy() const { return online_count == total_count; }
    bool topics_healthy() const {
        return std::all_of(topics.begin(), topics.end(),
            [](const TopicStatus& t) { return !t.timeout; });
    }
};

struct SystemHealthStatus {
    std::vector<NodeGroupStatus> groups;
    std::vector<NodeStatus> ungrouped_nodes;
    HealthLevel overall_level;
};