#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/empty.hpp>
#include <std_srvs/srv/empty.hpp>

class CallbackGroupTest : public rclcpp::Node
{
public:
  CallbackGroupTest();

private:
  void this_service_should_work(
    const std_srvs::srv::Empty::Request::SharedPtr req,
    std_srvs::srv::Empty::Response::SharedPtr res);

  rclcpp::Subscription<std_msgs::msg::Empty>::SharedPtr sub_empty_dummy_;
  rclcpp::Service<std_srvs::srv::Empty>::SharedPtr service_;
  rclcpp::TimerBase::SharedPtr timer_;
};

CallbackGroupTest::CallbackGroupTest(): Node("callback_group_test")
{
  rclcpp::CallbackGroup::SharedPtr dummy_callback_group;
  dummy_callback_group = this->create_callback_group(rclcpp::CallbackGroupType::MutuallyExclusive);

  service_ = this->create_service<std_srvs::srv::Empty>("test_service",
    std::bind(&CallbackGroupTest::this_service_should_work, this, std::placeholders::_1, std::placeholders::_2),
    rclcpp::ServicesQoS().get_rmw_qos_profile(), dummy_callback_group);

  const auto period_ns = rclcpp::Rate(1.0).period();
  timer_ = rclcpp::create_timer(this, get_clock(), period_ns, [this](){std::cout << "timer called" << std::endl;} , dummy_callback_group);

  // auto dummy_sub_opt = rclcpp::SubscriptionOptions();
  // dummy_sub_opt.callback_group = dummy_callback_group;

  // sub_empty_dummy_ = this->create_subscription<std_msgs::msg::Empty>(
  //   "/dummy_topic", 1, [this](std_msgs::msg::Empty::ConstSharedPtr msg) { (void)msg; }, dummy_sub_opt);
}


void CallbackGroupTest::this_service_should_work(
  const std_srvs::srv::Empty::Request::SharedPtr req,
  std_srvs::srv::Empty::Response::SharedPtr res)
{
  (void)req; (void)res;
  std::cout << "service called!!!!!!!!!!!!!!" << std::endl;
}


int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  auto callback_group_test = std::make_shared<CallbackGroupTest>();
  rclcpp::executors::MultiThreadedExecutor exec;
  exec.add_node(callback_group_test);
  exec.spin();
  rclcpp::shutdown();
  return 0;
}
