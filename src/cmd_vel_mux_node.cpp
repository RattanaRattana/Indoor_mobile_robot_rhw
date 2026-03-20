#include <rclcpp/rclcpp.hpp>
#include <geometry_msgs/msg/twist.hpp>
#include <chrono>

using namespace std::chrono_literals;

class CmdVelMux : public rclcpp::Node
{
public:
  CmdVelMux() : Node("cmd_vel_mux")
  {
    // Parameters
    this->declare_parameter<double>("tele_timeout", 0.5);   // seconds before tele priority expires
    this->declare_parameter<double>("publish_rate", 20.0);  // Hz

    tele_timeout_ = this->get_parameter("tele_timeout").as_double();
    double publish_rate = this->get_parameter("publish_rate").as_double();

    // Publisher
    pub_cmd_vel_ = this->create_publisher<geometry_msgs::msg::Twist>("/cmd_vel_raw", 10);

    // Subscribers
    sub_tele_ = this->create_subscription<geometry_msgs::msg::Twist>(
      "/cmd_vel_tele", 10,
      [this](const geometry_msgs::msg::Twist::SharedPtr msg) {
        last_tele_msg_ = msg;
        last_tele_time_ = this->now();
        RCLCPP_DEBUG(this->get_logger(), "Received /cmd_vel_tele");
      });

    sub_nav_ = this->create_subscription<geometry_msgs::msg::Twist>(
      "/cmd_vel_nav", 10,
      [this](const geometry_msgs::msg::Twist::SharedPtr msg) {
        last_nav_msg_ = msg;
        RCLCPP_DEBUG(this->get_logger(), "Received /cmd_vel_nav");
      });

    // Timer for periodic publishing
    auto period = std::chrono::duration<double>(1.0 / publish_rate);
    timer_ = this->create_wall_timer(
      std::chrono::duration_cast<std::chrono::nanoseconds>(period),
      std::bind(&CmdVelMux::timerCallback, this));

    RCLCPP_INFO(this->get_logger(), "cmd_vel_mux started.");
    //RCLCPP_INFO(this->get_logger(), "  Priority 1 (HIGH): /cmd_vel_tele  (timeout: %.2fs)", tele_timeout_);
    //RCLCPP_INFO(this->get_logger(), "  Priority 2 (LOW) : /cmd_vel_nav");
  }

private:
  void timerCallback()
  {
    auto now = this->now();
    bool tele_active = isTeleActive(now);

    geometry_msgs::msg::Twist out_msg;

    if (tele_active && last_tele_msg_) {
      // HIGH priority: teleop is active and recent
      out_msg = *last_tele_msg_;
      if (active_source_ != "tele") {
        RCLCPP_INFO(this->get_logger(), "[MUX] Switched to /cmd_vel_tele (teleop priority)");
        active_source_ = "tele";
      }
    } else if (last_nav_msg_) {
      // LOW priority: navigation
      out_msg = *last_nav_msg_;
      if (active_source_ != "nav") {
        RCLCPP_INFO(this->get_logger(), "[MUX] Switched to /cmd_vel_nav (navigation)");
        active_source_ = "nav";
      }
    } else {
      // No source available — publish zero velocity for safety
      out_msg = geometry_msgs::msg::Twist();
      if (active_source_ != "none") {
        RCLCPP_WARN(this->get_logger(), "[MUX] No active source — publishing zero velocity");
        active_source_ = "none";
      }
    }

    pub_cmd_vel_->publish(out_msg);
  }

  bool isTeleActive(const rclcpp::Time & now)
  {
    if (!last_tele_msg_) return false;
    double elapsed = (now - last_tele_time_).seconds();
    return elapsed < tele_timeout_;
  }

  // ROS interfaces
  rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr pub_cmd_vel_;
  rclcpp::Subscription<geometry_msgs::msg::Twist>::SharedPtr sub_tele_;
  rclcpp::Subscription<geometry_msgs::msg::Twist>::SharedPtr sub_nav_;
  rclcpp::TimerBase::SharedPtr timer_;

  // State
  geometry_msgs::msg::Twist::SharedPtr last_tele_msg_{nullptr};
  geometry_msgs::msg::Twist::SharedPtr last_nav_msg_{nullptr};
  rclcpp::Time last_tele_time_;
  std::string active_source_{"none"};

  double tele_timeout_;
};

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<CmdVelMux>());
  rclcpp::shutdown();
  return 0;
}