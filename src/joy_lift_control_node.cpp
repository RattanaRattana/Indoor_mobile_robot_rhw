#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/joy.hpp>
#include <std_srvs/srv/trigger.hpp>

class JoyLiftControlNode : public rclcpp::Node
{
public:
    JoyLiftControlNode()
    : Node("joy_lift_control_node")
    {
        RCLCPP_INFO(get_logger(), "Joy Lift Control Node Started");

        // Subscriber
        joy_sub_ = create_subscription<sensor_msgs::msg::Joy>(
            "/joy",
            10,
            std::bind(&JoyLiftControlNode::joyCallback, this, std::placeholders::_1)
        );

        // Service Clients
        lift_up_client_ = create_client<std_srvs::srv::Trigger>("lift_up");
        lift_down_client_ = create_client<std_srvs::srv::Trigger>("lift_down");
        lift_stop_client_ = create_client<std_srvs::srv::Trigger>("lift_stop");
    }

private:

    void joyCallback(const sensor_msgs::msg::Joy::SharedPtr msg)
    {
        if (msg->buttons.size() < 4) {
            RCLCPP_WARN(get_logger(), "Joy message does not have enough buttons.");
            return;
        }

        // Button mapping
        bool lift_up_pressed   = (msg->buttons[3] == 1);
        bool lift_down_pressed = (msg->buttons[1] == 1);
        bool lift_stop_pressed = (msg->buttons[2] == 1);

        if (lift_up_pressed) {
            callService(lift_up_client_, "Lift UP");
        }
        else if (lift_down_pressed) {
            callService(lift_down_client_, "Lift DOWN");
        }
        else if (lift_stop_pressed) {
            callService(lift_stop_client_, "Lift STOP");
        }
        // if all are 0 → do nothing
    }

    void callService(rclcpp::Client<std_srvs::srv::Trigger>::SharedPtr client,
                     const std::string &action_name)
    {
        if (!client->wait_for_service(std::chrono::milliseconds(200))) {
            RCLCPP_WARN(get_logger(), "%s service not available", action_name.c_str());
            return;
        }

        auto request = std::make_shared<std_srvs::srv::Trigger::Request>();

        auto future = client->async_send_request(request);

        RCLCPP_INFO(get_logger(), "%s requested", action_name.c_str());
    }

    rclcpp::Subscription<sensor_msgs::msg::Joy>::SharedPtr joy_sub_;

    rclcpp::Client<std_srvs::srv::Trigger>::SharedPtr lift_up_client_;
    rclcpp::Client<std_srvs::srv::Trigger>::SharedPtr lift_down_client_;
    rclcpp::Client<std_srvs::srv::Trigger>::SharedPtr lift_stop_client_;
};


int main(int argc, char **argv)
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<JoyLiftControlNode>());
    rclcpp::shutdown();
    return 0;
}