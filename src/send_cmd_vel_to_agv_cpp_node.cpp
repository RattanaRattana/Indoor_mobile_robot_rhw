#include <rclcpp/rclcpp.hpp>
#include <geometry_msgs/msg/twist.hpp>

#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_no_tls_client.hpp>

#include <nlohmann/json.hpp>

#include <mutex>
#include <thread>
#include <string>

using json = nlohmann::json;
typedef websocketpp::client<websocketpp::config::asio_client> ws_client;

class CmdVelToWebSocket : public rclcpp::Node
{
public:
    CmdVelToWebSocket()
    : Node("send_cmd_vel_websocket_agv_node"),
      connected_(false)
    {
        /* ---------------- Parameters ---------------- */

        declare_parameter("agv_ip", "192.168.31.7");
        declare_parameter("port", 1202);
        declare_parameter("send_rate", 10.0);

        agv_ip_   = get_parameter("agv_ip").as_string();
        port_     = get_parameter("port").as_int();
        send_rate_ = get_parameter("send_rate").as_double();

        ws_url_ = "ws://" + agv_ip_ + ":" + std::to_string(port_);

        RCLCPP_INFO(get_logger(), "========== Send CMD_VEL WebSocket AGV Node ==========");
        // RCLCPP_INFO(get_logger(), "AGV IP: %s", agv_ip_.c_str());
        // RCLCPP_INFO(get_logger(), "Port  : %d", port_);
        // RCLCPP_INFO(get_logger(), "Full URL: %s", ws_url_.c_str());

        /* ---------------- Subscriber ---------------- */

        sub_ = create_subscription<geometry_msgs::msg::Twist>(
            "/cmd_vel_raw",
            10,
            std::bind(&CmdVelToWebSocket::cmdVelCallback, this, std::placeholders::_1)
        );

        /* ---------------- Timer ---------------- */

        timer_ = create_wall_timer(
            std::chrono::duration<double>(1.0 / send_rate_),
            std::bind(&CmdVelToWebSocket::sendCmdVel, this)
        );

        /* ---------------- WebSocket ---------------- */

        initWebSocket();

        RCLCPP_INFO(get_logger(), "cmd_vel → WebSocket bridge started");
    }

    ~CmdVelToWebSocket()
    {
        connected_ = false;

        try {
            ws_client_.stop();
        } catch (...) {}

        if (ws_thread_.joinable())
            ws_thread_.join();
    }

private:

    /* ================= WebSocket ================= */

    void initWebSocket()
    {
        try {
            RCLCPP_INFO(get_logger(), "Connecting to %s ...", ws_url_.c_str());

            ws_client_.clear_access_channels(websocketpp::log::alevel::all);
            ws_client_.clear_error_channels(websocketpp::log::elevel::all);
            ws_client_.init_asio();

            ws_client_.set_open_handler(
                [this](websocketpp::connection_hdl hdl) {
                    hdl_ = hdl;
                    connected_ = true;
                    RCLCPP_INFO(get_logger(), "✓ WebSocket CONNECTED");
                });

            ws_client_.set_fail_handler(
                [this](websocketpp::connection_hdl) {
                    connected_ = false;
                    RCLCPP_ERROR(get_logger(), "✗ WebSocket CONNECTION FAILED");
                });

            ws_client_.set_close_handler(
                [this](websocketpp::connection_hdl) {
                    connected_ = false;
                    RCLCPP_WARN(get_logger(), "WebSocket CLOSED");
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

            ws_thread_ = std::thread([this]() {
                try {
                    ws_client_.run();
                } catch (const std::exception &e) {
                    RCLCPP_ERROR(get_logger(),
                                 "WebSocket run exception: %s",
                                 e.what());
                }
            });

        } catch (const std::exception &e) {
            RCLCPP_ERROR(get_logger(),
                         "WebSocket init exception: %s",
                         e.what());
        }
    }

    /* ================= ROS callbacks ================= */

    void cmdVelCallback(const geometry_msgs::msg::Twist::SharedPtr msg)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        latest_cmd_ = *msg;
    }

    void sendCmdVel()
    {
        if (!connected_)
            return;

        geometry_msgs::msg::Twist cmd;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            cmd = latest_cmd_;
        }

        json j;
        j["packet"] = {
            {"cmd", "region"},
            {"region", "cmd_vel"},
            {"index", 1}
        };

        j["msg"] = {
            {"xvel", cmd.linear.x},
            {"yvel", cmd.linear.y},
            {"thetavel", cmd.angular.z}
        };

        try {
            ws_client_.send(
                hdl_,
                j.dump(),
                websocketpp::frame::opcode::text
            );
        }
        catch (const std::exception &e) {
            RCLCPP_WARN(get_logger(),
                        "WebSocket send failed: %s",
                        e.what());
            connected_ = false;
        }
    }

    /* ================= Members ================= */

    rclcpp::Subscription<geometry_msgs::msg::Twist>::SharedPtr sub_;
    rclcpp::TimerBase::SharedPtr timer_;

    ws_client ws_client_;
    websocketpp::connection_hdl hdl_;
    std::thread ws_thread_;

    std::mutex mutex_;
    geometry_msgs::msg::Twist latest_cmd_;

    std::string agv_ip_;
    int port_;
    std::string ws_url_;

    double send_rate_;
    bool connected_;
};

/* ================= main ================= */

int main(int argc, char **argv)
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<CmdVelToWebSocket>());
    rclcpp::shutdown();
    return 0;
}