## Service and timer both works fine
```
CallbackGroupTest::CallbackGroupTest(): Node("callback_group_test")
{
  rclcpp::CallbackGroup::SharedPtr dummy_callback_group;
  dummy_callback_group = this->create_callback_group(rclcpp::CallbackGroupType::MutuallyExclusive);

  service_ = this->create_service<std_srvs::srv::Empty>("test_service",
    std::bind(&CallbackGroupTest::this_service_should_work, this, std::placeholders::_1, std::placeholders::_2),
    rclcpp::ServicesQoS().get_rmw_qos_profile(), dummy_callback_group);

  const auto period_ns = rclcpp::Rate(1.0).period();
  timer_ = rclcpp::create_timer(this, get_clock(), period_ns, [this](){std::cout << "timer called" << std::endl;} , dummy_callback_group);

  auto dummy_sub_opt = rclcpp::SubscriptionOptions();
  dummy_sub_opt.callback_group = dummy_callback_group;

  sub_empty_dummy_ = this->create_subscription<std_msgs::msg::Empty>(
    "/dummy_topic", 1, [this](std_msgs::msg::Empty::ConstSharedPtr msg) { (void)msg; }, dummy_sub_opt);
}
```

## Service and timer both do not work
```
CallbackGroupTest::CallbackGroupTest(): Node("callback_group_test")
{
  rclcpp::CallbackGroup::SharedPtr dummy_callback_group;
  dummy_callback_group = this->create_callback_group(rclcpp::CallbackGroupType::MutuallyExclusive);

  service_ = this->create_service<std_srvs::srv::Empty>("test_service",
    std::bind(&CallbackGroupTest::this_service_should_work, this, std::placeholders::_1, std::placeholders::_2),
    rclcpp::ServicesQoS().get_rmw_qos_profile(), dummy_callback_group);

  const auto period_ns = rclcpp::Rate(1.0).period();
  timer_ = rclcpp::create_timer(this, get_clock(), period_ns, [this](){std::cout << "timer called" << std::endl;} , dummy_callback_group);
}
```