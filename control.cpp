#include "rclcpp/rclcpp.hpp"
#include "geometry_msgs/msg/twist.hpp"
#include "turtlesim/msg/pose.hpp"
#include "turtlesim/srv/set_pen.hpp"

class TurtleController : public rclcpp::Node
{
public:
    TurtleController() : Node("turtle_controller"), last_side_is_right_(false)
    {
        // 1. Publisher for velocity commands
        cmd_vel_pub_ = this->create_publisher<geometry_msgs::msg::Twist>("/turtle1/cmd_vel", 10);

        // 2. Subscriber for turtle pose updates
        pose_sub_ = this->create_subscription<turtlesim::msg::Pose>(
            "/turtle1/pose", 10,
            std::bind(&TurtleController::poseCallback, this, std::placeholders::_1));

        // 3. Service client for changing the pen color
        set_pen_client_ = this->create_client<turtlesim::srv::SetPen>("/turtle1/set_pen");
    }

private:
    void poseCallback(const turtlesim::msg::Pose::SharedPtr pose)
    {
        auto twist_msg = geometry_msgs::msg::Twist();

        // Determine if the turtle is on the right or left side of the screen (Midpoint is ~5.5)
        bool is_right = (pose->x > 5.5);

        // Adjust velocity based on which side the turtle is on
        if (is_right) {
            twist_msg.linear.x = 2.0;
            twist_msg.angular.z = -1.8;
        } else {
            twist_msg.linear.x = 1.0;
            twist_msg.angular.z = 1.5;
        }

        // Publish velocity command
        cmd_vel_pub_->publish(twist_msg);

        // Check if the turtle crossed sides to avoid spamming the service every cycle
        if (is_right != last_side_is_right_) {
            last_side_is_right_ = is_right;
            
            if (is_right) {
                // Right side: Red pen (R: 255, G: 0, B: 0), width: 3
                callSetPen(255, 0, 0, 3, 0);
            } else {
                // Left side: Green pen (R: 0, G: 255, B: 0), width: 3
                callSetPen(0, 255, 0, 3, 0);
            }
        }
    }

    // Helper method to call the set_pen service asynchronously
    void callSetPen(uint8_t r, uint8_t g, uint8_t b, uint8_t width, uint8_t off)
    {
        if (!set_pen_client_->wait_for_service(std::chrono::seconds(1))) {
            RCLCPP_WARN(this->get_logger(), "Set pen service not available.");
            return;
        }

        auto request = std::make_shared<turtlesim::srv::SetPen::Request>();
        request->r = r;
        request->g = g;
        request->b = b;
        request->width = width;
        request->off = off;

        // Send request asynchronously so it doesn't block the callback loop
        set_pen_client_->async_send_request(request);
    }

    rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr cmd_vel_pub_;
    rclcpp::Subscription<turtlesim::msg::Pose>::SharedPtr pose_sub_;
    rclcpp::Client<turtlesim::srv::SetPen>::SharedPtr set_pen_client_;
    bool last_side_is_right_;
};

int main(int argc, char * argv[])
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<TurtleController>());
    rclcpp::shutdown();
    return 0;
}
