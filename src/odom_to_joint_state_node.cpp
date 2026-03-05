#include <rclcpp/rclcpp.hpp>
#include <nav_msgs/msg/odometry.hpp>
#include <sensor_msgs/msg/joint_state.hpp>
#include <tf2/utils.h>
#include <tf2_geometry_msgs/tf2_geometry_msgs.hpp>

class OdomToJointState : public rclcpp::Node
{
public:
  OdomToJointState()
  : Node("odom_to_joint_state")
  {
    // -------- Parameters --------
    wheel_radius_ = this->declare_parameter("wheel_radius", 0.1);
    wheel_sep_    = this->declare_parameter("wheel_separation", 0.5);

    // -------- Publishers / Subscribers --------
    joint_pub_ = this->create_publisher<sensor_msgs::msg::JointState>(
      "/idr_robot/joint_states", 10);

    odom_sub_ = this->create_subscription<nav_msgs::msg::Odometry>(
      "feedback_odom", 10,
      std::bind(&OdomToJointState::odomCallback, this, std::placeholders::_1));

    // Timer: always publish joint states
    joint_timer_ = this->create_wall_timer(
      std::chrono::milliseconds(50),
      std::bind(&OdomToJointState::timerPublish, this));

    RCLCPP_INFO(this->get_logger(),
                "odom_to_joint_state started (publishing zeros until odom arrives)");
  }

private:
  void odomCallback(const nav_msgs::msg::Odometry::SharedPtr msg)
  {
    last_stamp_ = msg->header.stamp;

    double x = msg->pose.pose.position.x;
    double y = msg->pose.pose.position.y;
    double yaw = tf2::getYaw(msg->pose.pose.orientation);

    if (!initialized_) {
      x_prev_ = x;
      y_prev_ = y;
      yaw_prev_ = yaw;
      initialized_ = true;
      return;
    }

    double dx = x - x_prev_;
    double dy = y - y_prev_;
    double dyaw = yaw - yaw_prev_;

    double ds = std::sqrt(dx * dx + dy * dy);

    double ds_left  = ds - (wheel_sep_ / 2.0) * dyaw;
    double ds_right = ds + (wheel_sep_ / 2.0) * dyaw;

    left_wheel_pos_  += ds_left  / wheel_radius_;
    right_wheel_pos_ += ds_right / wheel_radius_;

    x_prev_ = x;
    y_prev_ = y;
    yaw_prev_ = yaw;
  }

  void timerPublish()
  {
    sensor_msgs::msg::JointState js;

    js.header.stamp = initialized_ ? last_stamp_ : this->now();

    js.name = {
      "middle_left_wheel_joint_left",
      "middle_right_wheel_joint_right",
      "lift_joint"
    };

    if (!initialized_) {
      // Publish zero joint states
      js.position = {0.0, 0.0, 0.0};
    } else {
      js.position = {
        left_wheel_pos_,
        right_wheel_pos_,
        lift_joint_
      };
    }

    joint_pub_->publish(js);
  }

  // -------- ROS --------
  rclcpp::Publisher<sensor_msgs::msg::JointState>::SharedPtr joint_pub_;
  rclcpp::Subscription<nav_msgs::msg::Odometry>::SharedPtr odom_sub_;
  rclcpp::TimerBase::SharedPtr joint_timer_;

  // -------- State --------
  bool initialized_ = false;
  double x_prev_ = 0.0, y_prev_ = 0.0, yaw_prev_ = 0.0;
  double left_wheel_pos_ = 0.0;
  double right_wheel_pos_ = 0.0;
  double lift_joint_ = 0.0;

  rclcpp::Time last_stamp_;

  // -------- Robot params --------
  double wheel_radius_;
  double wheel_sep_;
};

int main(int argc, char **argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<OdomToJointState>());
  rclcpp::shutdown();
  return 0;
}
