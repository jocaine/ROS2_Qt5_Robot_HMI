/*
 * @Author: chengyang chengyangkj@outlook.com
 * @Date: 2023-04-20 15:46:29
 * @LastEditors: chengyangkj chengyangkj@qq.com
 * @LastEditTime: 2023-10-07 14:16:09
 * @FilePath: /ros_qt5_gui_app/include/rclcomm.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置
 * 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef RCLCOMM_H
#define RCLCOMM_H
#include "sensor_msgs/msg/image.hpp"

#include <cv_bridge/cv_bridge.h>
#include <memory>
#include <mutex>
#include <thread>
#include <atomic>
#include <chrono>
#include <rclcpp/rclcpp.hpp>
#include <rclcpp_action/rclcpp_action.hpp>
#include "algorithm.h"
#include "geometry_msgs/msg/pose_stamped.hpp"
#include "geometry_msgs/msg/pose_with_covariance_stamped.hpp"
#include "geometry_msgs/msg/twist.hpp"
#include "geometry_msgs/msg/polygon_stamped.hpp"
#include "nav_msgs/msg/occupancy_grid.hpp"
#include "nav_msgs/msg/odometry.hpp"
#include "nav_msgs/msg/path.hpp"
#include "point_type.h"
#include "sensor_msgs/msg/battery_state.hpp"
#include "sensor_msgs/msg/laser_scan.hpp"
#include "tf2/LinearMath/Quaternion.h"
#include "tf2_geometry_msgs/tf2_geometry_msgs.h"
#include "tf2_ros/buffer.h"
#include "tf2_ros/transform_listener.h"
#include "virtual_channel_node.h"
#include "topology_msgs/msg/topology_map.hpp"
#include "core/framework/framework.h"

class rclcomm : public VirtualChannelNode {
 public:
  rclcomm();
  ~rclcomm() override;

  // ---- 生命周期 (继承自 VirtualChannelNode) ----
  bool Start() override;
  bool Stop() override;
  void Process() override;
  std::string Name() override { return "ROS2"; };

  // ---- 发布指令到 ROS 话题 ----
  void PubRelocPose(const basic::RobotPose &pose);   // 发布重定位位姿
  void PubNavGoal(const basic::RobotPose &pose);      // 发布导航目标点
  void PubRobotSpeed(const basic::RobotSpeed &speed);  // 发布速度控制指令

  // ---- 数据转换工具 ----
  basic::RobotPose getTransform(std::string from, std::string to);
  TopologyMap ConvertFromRosMsg(const topology_msgs::msg::TopologyMap::SharedPtr msg);
  topology_msgs::msg::TopologyMap ConvertToRosMsg(const TopologyMap& topology_map);

 private:
  // ---- 地图类回调：接收栅格地图数据 ----
  void map_callback(const nav_msgs::msg::OccupancyGrid::SharedPtr msg);             // 全局占据栅格地图
  void localCostMapCallback(const nav_msgs::msg::OccupancyGrid::SharedPtr msg);     // 局部代价地图（导航避障用）
  void globalCostMapCallback(const nav_msgs::msg::OccupancyGrid::SharedPtr msg);    // 全局代价地图（路径规划用）

  // ---- 传感器类回调：接收传感器原始/处理数据 ----
  void laser_callback(const sensor_msgs::msg::LaserScan::SharedPtr msg);            // 激光雷达扫描点云
  void odom_callback(const nav_msgs::msg::Odometry::SharedPtr msg);                 // 里程计（线速度/角速度/位姿）
  void BatteryCallback(const sensor_msgs::msg::BatteryState::SharedPtr msg);        // 电池状态（电压/电量百分比）

  // ---- 路径类回调：接收规划路径 ----
  void path_callback(const nav_msgs::msg::Path::SharedPtr msg);                     // 全局路径（从起点到目标的完整规划）
  void local_path_callback(const nav_msgs::msg::Path::SharedPtr msg);               // 局部路径（实时避障的短期轨迹）

  // ---- 机器人状态类回调 ----
  void robotFootprintCallback(const geometry_msgs::msg::PolygonStamped::SharedPtr msg);  // 机器人轮廓多边形
  void topologyMapCallback(const topology_msgs::msg::TopologyMap::SharedPtr msg);         // 拓扑地图（站点+路线）

  // ---- 内部辅助 ----
  void getRobotPose();       // 通过 TF 查询机器人在 map 坐标系下的位姿
  void checkNodeHealth();    // 检查各节点组在线状态和话题数据流

  // ---- Publishers ----
  rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr speed_publisher_;
  rclcpp::Publisher<geometry_msgs::msg::PoseWithCovarianceStamped>::SharedPtr reloc_pose_publisher_;
  rclcpp::Publisher<geometry_msgs::msg::PoseStamped>::SharedPtr nav_goal_publisher_;
  rclcpp::Publisher<topology_msgs::msg::TopologyMap>::SharedPtr topology_map_update_publisher_;

  // ---- Subscribers ----
  rclcpp::Subscription<nav_msgs::msg::OccupancyGrid>::SharedPtr map_subscriber_;
  rclcpp::Subscription<nav_msgs::msg::OccupancyGrid>::SharedPtr local_cost_map_subscriber_;
  rclcpp::Subscription<nav_msgs::msg::OccupancyGrid>::SharedPtr global_cost_map_subscriber_;
  rclcpp::Subscription<sensor_msgs::msg::LaserScan>::SharedPtr laser_scan_subscriber_;
  rclcpp::Subscription<sensor_msgs::msg::BatteryState>::SharedPtr battery_state_subscriber_;
  rclcpp::Subscription<nav_msgs::msg::Odometry>::SharedPtr odometry_subscriber_;
  rclcpp::Subscription<nav_msgs::msg::Path>::SharedPtr local_path_subscriber_;
  rclcpp::Subscription<nav_msgs::msg::Path>::SharedPtr global_path_subscriber_;
  rclcpp::Subscription<geometry_msgs::msg::PolygonStamped>::SharedPtr robot_footprint_subscriber_;
  rclcpp::Subscription<topology_msgs::msg::TopologyMap>::SharedPtr topology_map_subscriber_;
  std::vector<rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr> image_subscriber_list_;

  // ---- TF 坐标变换 ----
  std::unique_ptr<tf2_ros::Buffer> tf_buffer_;
  std::shared_ptr<tf2_ros::TransformListener> transform_listener_;

  // ---- 节点与执行器 ----
  std::shared_ptr<rclcpp::Node> node;
  std::unique_ptr<rclcpp::executors::MultiThreadedExecutor> m_executor;
  rclcpp::CallbackGroup::SharedPtr callback_group_laser;
  rclcpp::CallbackGroup::SharedPtr callback_group_other;

  // ---- 内部状态 ----
  basic::OccupancyMap occ_map_;                // 缓存的全局地图，局部代价地图叠加时用
  basic::RobotPose m_currPose;
  std::atomic_bool init_flag_{false};
  Framework::MessageBus::CallbackId nav_goal_sub_id_{0};
  Framework::MessageBus::CallbackId reloc_pose_sub_id_{0};
  Framework::MessageBus::CallbackId robot_speed_sub_id_{0};
  Framework::MessageBus::CallbackId topology_update_sub_id_{0};
  Framework::MessageBus::CallbackId reload_node_group_sub_id_{0};
  mutable std::mutex health_mutex_;
  std::map<std::string, std::chrono::steady_clock::time_point> topic_last_received_;

  // ---- 健康检测独立线程 ----
  std::thread health_check_thread_;
  std::atomic_bool health_check_running_{false};
};

#endif  // RCLCOMM_H
