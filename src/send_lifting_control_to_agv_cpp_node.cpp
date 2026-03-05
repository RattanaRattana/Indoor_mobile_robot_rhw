#include <rclcpp/rclcpp.hpp>
#include <std_srvs/srv/trigger.hpp>

#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_no_tls_client.hpp>
#include <nlohmann/json.hpp>

#include <thread>
#include <mutex>
#include <chrono>

using json = nlohmann::json;
typedef websocketpp::client<websocketpp::config::asio_client> ws_client;

class LiftWebSocketNode : public rclcpp::Node
{
public:
    LiftWebSocketNode()
        : Node("send_lifting_control_websocket_AGV_node"),
          ws_connected_(false)
    {
        declare_parameter("agv_ip", "192.168.31.7");
        declare_parameter("port", 1202);

        agv_ip_ = get_parameter("agv_ip").as_string();
        port_ = get_parameter("port").as_int();

        RCLCPP_INFO(get_logger(), "========== send Lifting control WebSocket AGV Node ==========");
        // RCLCPP_INFO(get_logger(), "AGV IP: %s", agv_ip_.c_str());
        // RCLCPP_INFO(get_logger(), "Port  : %d", port_);

        // Test connection at startup
        ws_connected_ = testConnection();

        // if (ws_connected_) {
        //     RCLCPP_INFO(get_logger(), "✓ WebSocket CONNECTED");
        //     RCLCPP_INFO(get_logger(), "✓ Lift node READY. Services available.");
        // } else {
        //     RCLCPP_WARN(get_logger(),
        //         "✓ WebSocket CONNECTED");
        // }

        // Create services
        lift_up_srv_ = create_service<std_srvs::srv::Trigger>(
            "lift_up",
            std::bind(&LiftWebSocketNode::liftUp, this,
                      std::placeholders::_1, std::placeholders::_2));

        lift_down_srv_ = create_service<std_srvs::srv::Trigger>(
            "lift_down",
            std::bind(&LiftWebSocketNode::liftDown, this,
                      std::placeholders::_1, std::placeholders::_2));

        lift_stop_srv_ = create_service<std_srvs::srv::Trigger>(
            "lift_stop",
            std::bind(&LiftWebSocketNode::liftStop, this,
                      std::placeholders::_1, std::placeholders::_2));
    }

private:

    /* ===================== Connection Test ===================== */

    bool testConnection()
    {
        std::string uri = "ws://" + agv_ip_ + ":" + std::to_string(port_);

        RCLCPP_INFO(get_logger(), "Connecting to %s ...", uri.c_str());

        try {
            ws_client client;
            client.init_asio();

            client.clear_access_channels(websocketpp::log::alevel::all);
            client.clear_error_channels(websocketpp::log::elevel::all);

            std::atomic<bool> connected(false);
            std::atomic<bool> failed(false);

            client.set_open_handler([&](websocketpp::connection_hdl) {
                connected = true;
            });

            client.set_fail_handler([&](websocketpp::connection_hdl) {
                failed = true;
            });

            websocketpp::lib::error_code ec;
            auto con = client.get_connection(uri, ec);
            if (ec) {
                RCLCPP_ERROR(get_logger(), "Connection setup error: %s",
                            ec.message().c_str());
                return false;
            }

            client.connect(con);

            std::thread ws_thread([&client]() { client.run(); });

            // Wait up to 2 seconds
            auto start = std::chrono::steady_clock::now();
            while (!connected && !failed) {
                if (std::chrono::steady_clock::now() - start >
                    std::chrono::seconds(2))
                    break;

                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }

            client.stop();
            if (ws_thread.joinable())
                ws_thread.join();

            if (connected) {
                RCLCPP_INFO(get_logger(), "✓ WebSocket CONNECTED");
                RCLCPP_INFO(get_logger(), "✓ Lift node READY. Services available.");
                return true;
            } else {
                RCLCPP_ERROR(get_logger(), "✗ WebSocket CONNECTION FAILED");
                return false;
            }
        }
        catch (const std::exception &e) {
            RCLCPP_ERROR(get_logger(), "WebSocket exception: %s", e.what());
            return false;
        }
    }

    /* ===================== Core Send Function ===================== */

    bool sendLiftCommand(int state)
    {
        std::vector<std::string> data(8, "0");

        if (state > 0) {           // UP
            data[0] = "1";
            data[1] = "1";
            RCLCPP_INFO(get_logger(), "Sending LIFT UP command");
        }
        else if (state < 0) {      // DOWN
            data[0] = "0";
            data[1] = "0";
            RCLCPP_INFO(get_logger(), "Sending LIFT DOWN command");
        }
        else {                     // STOP
            data[0] = "0";
            data[1] = "1";
            RCLCPP_INFO(get_logger(), "Sending LIFT STOP command");
        }

        json cmd = {
            {"packet", {
                {"cmd", "region"},
                {"region", "TableDeal"},
                {"index", 1}
            }},
            {"msg", {
                {"talk", "setTable"},
                {"address", "1440"},
                {"size", 8},
                {"data", data},
                {"format", "uint8"}
            }}
        };

        std::string uri = "ws://" + agv_ip_ + ":" + std::to_string(port_);

        try {
            ws_client client;
            client.init_asio();

            client.clear_access_channels(websocketpp::log::alevel::all);
            client.clear_error_channels(websocketpp::log::elevel::all);

            websocketpp::lib::error_code ec;
            auto con = client.get_connection(uri, ec);
            if (ec) {
                RCLCPP_ERROR(get_logger(), "Connection error: %s", ec.message().c_str());
                return false;
            }

            client.connect(con);

            std::thread ws_thread([&client]() { client.run(); });

            std::this_thread::sleep_for(std::chrono::milliseconds(200));

            client.send(con->get_handle(),
                        cmd.dump(),
                        websocketpp::frame::opcode::text);

            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            client.stop();
            if (ws_thread.joinable())
                ws_thread.join();

            RCLCPP_INFO(get_logger(), "Lift command sent successfully.");
            return true;
        }
        catch (const std::exception &e) {
            RCLCPP_ERROR(get_logger(), "WebSocket exception: %s", e.what());
            return false;
        }
    }

    /* ===================== Services ===================== */

    void liftUp(const std::shared_ptr<std_srvs::srv::Trigger::Request>,
                std::shared_ptr<std_srvs::srv::Trigger::Response> res)
    {
        bool success = sendLiftCommand(1);
        res->success = success;
        res->message = success ? "Lift UP sent" : "Failed to send Lift UP";
    }

    void liftDown(const std::shared_ptr<std_srvs::srv::Trigger::Request>,
                  std::shared_ptr<std_srvs::srv::Trigger::Response> res)
    {
        bool success = sendLiftCommand(-1);
        res->success = success;
        res->message = success ? "Lift DOWN sent" : "Failed to send Lift DOWN";
    }

    void liftStop(const std::shared_ptr<std_srvs::srv::Trigger::Request>,
                  std::shared_ptr<std_srvs::srv::Trigger::Response> res)
    {
        bool success = sendLiftCommand(0);
        res->success = success;
        res->message = success ? "Lift STOP sent" : "Failed to send Lift STOP";
    }

    /* ===================== Members ===================== */

    std::string agv_ip_;
    int port_;

    bool ws_connected_;

    rclcpp::Service<std_srvs::srv::Trigger>::SharedPtr lift_up_srv_;
    rclcpp::Service<std_srvs::srv::Trigger>::SharedPtr lift_down_srv_;
    rclcpp::Service<std_srvs::srv::Trigger>::SharedPtr lift_stop_srv_;
};

/* ===================== main ===================== */

int main(int argc, char **argv)
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<LiftWebSocketNode>());
    rclcpp::shutdown();
    return 0;
}