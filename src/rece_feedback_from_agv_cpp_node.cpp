#include <rclcpp/rclcpp.hpp>
#include <nav_msgs/msg/odometry.hpp>
#include <std_msgs/msg/bool.hpp>

#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_no_tls_client.hpp>
#include <nlohmann/json.hpp>

#include <mutex>
#include <thread>
#include <atomic>
#include <cmath>
#include <chrono>

using json = nlohmann::json;
typedef websocketpp::client<websocketpp::config::asio_client> ws_client;

class OdomWebSocketNode : public rclcpp::Node
{
public:
    OdomWebSocketNode()
        : Node("rece_odom_websocket_agv_node"),
          ws_connected_(false),
          calibrated_(false),
          stop_thread_(false)
    {
        /* ================= Parameters ================= */

        declare_parameter("agv_ip", "192.168.31.7");
        declare_parameter("port", 1202);
        declare_parameter("publish_rate", 100.0);
        declare_parameter("stale_timeout", 0.5);
        declare_parameter("auto_reconnect", false);
        declare_parameter("reconnect_delay", 1.0);
        declare_parameter("auto_calibrate", true);

        agv_ip_         = get_parameter("agv_ip").as_string();
        port_           = get_parameter("port").as_int();
        publish_rate_   = get_parameter("publish_rate").as_double();
        stale_timeout_  = get_parameter("stale_timeout").as_double();
        auto_reconnect_ = get_parameter("auto_reconnect").as_bool();
        reconnect_delay_= get_parameter("reconnect_delay").as_double();
        auto_calibrate_ = get_parameter("auto_calibrate").as_bool();

        ws_url_ = "ws://" + agv_ip_ + ":" + std::to_string(port_);

        RCLCPP_INFO(get_logger(), "========== Rece ODOM WebSocket AGV Node ==========");
        // RCLCPP_INFO(get_logger(), "AGV IP: %s", agv_ip_.c_str());
        // RCLCPP_INFO(get_logger(), "Port  : %d", port_);
        // RCLCPP_INFO(get_logger(), "Full URL: %s", ws_url_.c_str());

        /* ================= ROS Interfaces ================= */

        feedback_pub_ = create_publisher<nav_msgs::msg::Odometry>("/feedback_odom", 10);

        calibrate_sub_ = create_subscription<std_msgs::msg::Bool>(
            "/calibrate_odom", 10,
            std::bind(&OdomWebSocketNode::calibrateCallback, this, std::placeholders::_1));

        double period = 1.0 / publish_rate_;
        timer_ = create_wall_timer(
            std::chrono::duration<double>(period),
            std::bind(&OdomWebSocketNode::publishCallback, this));

        /* ================= Start WebSocket Thread ================= */

        ws_thread_ = std::thread(&OdomWebSocketNode::websocketLoop, this);

        RCLCPP_INFO(get_logger(), "Odom WebSocket Node started");
    }

    ~OdomWebSocketNode()
    {
        stop_thread_ = true;

        try {
            ws_client_.stop();
        } catch (...) {}

        if (ws_thread_.joinable())
            ws_thread_.join();
    }

private:

    /* ================= Calibration ================= */

    void calibrateCallback(const std_msgs::msg::Bool::SharedPtr msg)
    {
        if (msg->data)
            resetCalibration();
    }

    void resetCalibration()
    {
        std::lock_guard<std::mutex> lock(data_mutex_);
        calibrated_ = false;
        calibration_samples_.clear();
        offset_x_ = offset_y_ = offset_theta_ = 0.0;
        RCLCPP_INFO(get_logger(), "Calibration reset.");
    }

    void applyCalibration(double raw_x, double raw_y, double raw_theta,
                          double &cal_x, double &cal_y, double &cal_theta)
    {
        cal_theta = raw_theta - offset_theta_;
        cal_theta = atan2(sin(cal_theta), cos(cal_theta));

        double dx = raw_x - offset_x_;
        double dy = raw_y - offset_y_;

        double cos_o = cos(-offset_theta_);
        double sin_o = sin(-offset_theta_);

        cal_x = dx * cos_o - dy * sin_o;
        cal_y = dx * sin_o + dy * cos_o;
    }

    /* ================= WebSocket ================= */

    void websocketLoop()
    {
        while (!stop_thread_)
        {
            connectOnce();

            if (!auto_reconnect_ || stop_thread_)
                break;

            RCLCPP_WARN(get_logger(),
                        "Reconnecting in %.1f seconds...",
                        reconnect_delay_);

            std::this_thread::sleep_for(
                std::chrono::duration<double>(reconnect_delay_));
        }
    }

    void connectOnce()
    {
        RCLCPP_INFO(get_logger(), "Connecting to %s ...", ws_url_.c_str());

        ws_connected_ = false;

        ws_client_.clear_access_channels(websocketpp::log::alevel::all);
        ws_client_.init_asio();

        ws_client_.set_open_handler([this](websocketpp::connection_hdl hdl) {
            hdl_ = hdl;
            ws_connected_ = true;

            RCLCPP_INFO(get_logger(), "✓ WebSocket CONNECTED");

            sendJson({
                {"packet", {{"cmd","join"},{"region","ControlModel"}}},
                {"msg", {}}
            });

            sendJson({
                {"packet", {{"cmd","join"},{"region","odom"}}},
                {"msg", {}}
            });
        });

        ws_client_.set_fail_handler([this](websocketpp::connection_hdl) {
            ws_connected_ = false;
            RCLCPP_ERROR(get_logger(), "✗ WebSocket CONNECTION FAILED");
        });

        ws_client_.set_close_handler([this](websocketpp::connection_hdl) {
            ws_connected_ = false;
            RCLCPP_WARN(get_logger(), "WebSocket CLOSED");
        });

        ws_client_.set_message_handler(
            [this](websocketpp::connection_hdl, ws_client::message_ptr msg)
        {
            handleMessage(msg->get_payload());
        });

        websocketpp::lib::error_code ec;
        auto con = ws_client_.get_connection(ws_url_, ec);

        if (ec) {
            RCLCPP_ERROR(get_logger(),
                         "Connection setup error: %s",
                         ec.message().c_str());
            return;
        }

        ws_client_.connect(con);

        try {
            ws_client_.run();
        }
        catch (const std::exception &e) {
            RCLCPP_ERROR(get_logger(), "WebSocket exception: %s", e.what());
        }

        RCLCPP_INFO(get_logger(), "Exiting connection loop");
    }

    void sendJson(const json &j)
    {
        if (!ws_connected_) return;

        ws_client_.send(hdl_, j.dump(),
                        websocketpp::frame::opcode::text);
    }

    /* ================= Message Handling ================= */

    void handleMessage(const std::string &payload)
    {
        json data;
        try { data = json::parse(payload); }
        catch (...) { return; }

        if (!data.contains("msg")) return;

        auto msg = data["msg"];
        if (!msg.contains("odomx")) return;

        double raw_x = msg.value("odomx", 0.0);
        double raw_y = msg.value("odomy", 0.0);
        double raw_theta = msg.value("odomtheta", 0.0);
        double velx = msg.value("velx", 0.0);
        double vely = msg.value("vely", 0.0);
        double veltheta = msg.value("veltheta", 0.0);

        std::lock_guard<std::mutex> lock(data_mutex_);

        if (auto_calibrate_ && !calibrated_)
        {
            calibration_samples_.push_back({raw_x, raw_y, raw_theta});
            if (calibration_samples_.size() >= 10)
            {
                for (auto &s : calibration_samples_) {
                    offset_x_ += s[0];
                    offset_y_ += s[1];
                    offset_theta_ += s[2];
                }

                offset_x_ /= calibration_samples_.size();
                offset_y_ /= calibration_samples_.size();
                offset_theta_ /= calibration_samples_.size();

                calibrated_ = true;
                RCLCPP_INFO(get_logger(), "Calibration complete");
            }
        }

        double cal_x = 0, cal_y = 0, cal_theta = 0;

        if (calibrated_)
            applyCalibration(raw_x, raw_y, raw_theta, cal_x, cal_y, cal_theta);

        latest_odom_ = {cal_x, cal_y, cal_theta, velx, vely, veltheta};
        last_rx_time_ = now().seconds();
    }

    /* ================= Publish ================= */

    void publishCallback()
    {
        if (!ws_connected_) return;

        std::lock_guard<std::mutex> lock(data_mutex_);

        if ((now().seconds() - last_rx_time_) > stale_timeout_)
            return;

        nav_msgs::msg::Odometry msg;
        msg.header.stamp = now();
        msg.header.frame_id = "odom";
        msg.child_frame_id = "base_link";

        msg.pose.pose.position.x = latest_odom_[0];
        msg.pose.pose.position.y = latest_odom_[1];

        double theta = latest_odom_[2];
        msg.pose.pose.orientation.z = sin(theta/2.0);
        msg.pose.pose.orientation.w = cos(theta/2.0);

        msg.twist.twist.linear.x = latest_odom_[3];
        msg.twist.twist.linear.y = latest_odom_[4];
        msg.twist.twist.angular.z = latest_odom_[5];

        feedback_pub_->publish(msg);
    }

    /* ================= Members ================= */

    rclcpp::Publisher<nav_msgs::msg::Odometry>::SharedPtr feedback_pub_;
    rclcpp::Subscription<std_msgs::msg::Bool>::SharedPtr calibrate_sub_;
    rclcpp::TimerBase::SharedPtr timer_;

    ws_client ws_client_;
    websocketpp::connection_hdl hdl_;
    std::thread ws_thread_;
    std::atomic<bool> ws_connected_;
    bool calibrated_;
    std::atomic<bool> stop_thread_;

    std::mutex data_mutex_;
    std::vector<std::array<double,3>> calibration_samples_;
    std::vector<double> latest_odom_{0,0,0,0,0,0};

    double offset_x_ = 0, offset_y_ = 0, offset_theta_ = 0;
    double last_rx_time_ = 0;

    std::string agv_ip_;
    int port_;
    std::string ws_url_;

    double publish_rate_;
    double stale_timeout_;
    bool auto_reconnect_;
    double reconnect_delay_;
    bool auto_calibrate_;
};

/* ================= main ================= */

int main(int argc, char **argv)
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<OdomWebSocketNode>());
    rclcpp::shutdown();
    return 0;
}