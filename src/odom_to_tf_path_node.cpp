#include <rclcpp/rclcpp.hpp>

#include <nav_msgs/msg/odometry.hpp>
#include <nav_msgs/msg/path.hpp>
#include <geometry_msgs/msg/pose_stamped.hpp>
#include <geometry_msgs/msg/transform_stamped.hpp>

#include <tf2_ros/transform_broadcaster.h>

class OdomToTFAndPath : public rclcpp::Node
{
public:
    OdomToTFAndPath()
    : Node("odom_to_tf_and_path"),
      received_odom_(false)
    {
        odom_sub_ = this->create_subscription<nav_msgs::msg::Odometry>(
            "/feedback_odom",
            10,
            std::bind(&OdomToTFAndPath::odomCallback, this, std::placeholders::_1)
        );

        tf_broadcaster_ = std::make_unique<tf2_ros::TransformBroadcaster>(*this);

        path_pub_ = this->create_publisher<nav_msgs::msg::Path>("/path", 10);
        path_.header.frame_id = "odom";

        // Timer to publish default TF when odom is missing
        tf_timer_ = this->create_wall_timer(
            std::chrono::milliseconds(50),
            std::bind(&OdomToTFAndPath::publishDefaultTF, this)
        );

        RCLCPP_INFO(this->get_logger(),
                    "Waiting for /feedback_odom (publishing default TF until received)");
    }

private:
    void odomCallback(const nav_msgs::msg::Odometry::SharedPtr msg)
    {
        received_odom_ = true;
        last_odom_ = *msg;

        // Append to Path
        geometry_msgs::msg::PoseStamped pose;
        pose.header = msg->header;
        pose.pose = msg->pose.pose;

        path_.header.stamp = msg->header.stamp;
        path_.poses.push_back(pose);

        path_pub_->publish(path_);
    }

    void publishDefaultTF()
    {
        geometry_msgs::msg::TransformStamped tf_msg;

        tf_msg.header.stamp = this->now();
        tf_msg.header.frame_id = "odom";
        tf_msg.child_frame_id = "base_footprint";

        if (!received_odom_)
        {
            // Default transform: (0,0,0)
            tf_msg.transform.translation.x = 0.0;
            tf_msg.transform.translation.y = 0.0;
            tf_msg.transform.translation.z = 0.0;

            tf_msg.transform.rotation.x = 0.0;
            tf_msg.transform.rotation.y = 0.0;
            tf_msg.transform.rotation.z = 0.0;
            tf_msg.transform.rotation.w = 1.0;
        }
        else
        {
            // Use real odom
            tf_msg.header.stamp = last_odom_.header.stamp;

            tf_msg.transform.translation.x = last_odom_.pose.pose.position.x;
            tf_msg.transform.translation.y = last_odom_.pose.pose.position.y;
            tf_msg.transform.translation.z = last_odom_.pose.pose.position.z;

            tf_msg.transform.rotation = last_odom_.pose.pose.orientation;
        }

        tf_broadcaster_->sendTransform(tf_msg);
    }

    // ROS interfaces
    rclcpp::Subscription<nav_msgs::msg::Odometry>::SharedPtr odom_sub_;
    std::unique_ptr<tf2_ros::TransformBroadcaster> tf_broadcaster_;
    rclcpp::Publisher<nav_msgs::msg::Path>::SharedPtr path_pub_;
    rclcpp::TimerBase::SharedPtr tf_timer_;

    // Data
    nav_msgs::msg::Path path_;
    nav_msgs::msg::Odometry last_odom_;
    bool received_odom_;
};

int main(int argc, char ** argv)
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<OdomToTFAndPath>());
    rclcpp::shutdown();
    return 0;
}
