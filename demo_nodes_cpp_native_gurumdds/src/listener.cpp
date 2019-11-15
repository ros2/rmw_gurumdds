// Copyright 2019 GurumNetworks, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <cstdio>
#include <memory>
#include <string>

#include "rclcpp/rclcpp.hpp"
#include "rcutils/cmdline_parser.h"

#include "std_msgs/msg/string.hpp"

#include "rmw_gurumdds_cpp/get_entities.hpp"

class Listener : public rclcpp::Node
{
public:
  Listener()
  : Node("listener_native")
  {
    {
      rcl_node_t * rcl_node = get_node_base_interface()->get_rcl_node_handle();
      rmw_node_t * rmw_node = rcl_node_get_rmw_handle(rcl_node);
      dds_DomainParticipant * dp = rmw_gurumdds_cpp::get_participant(rmw_node);
      RCLCPP_INFO(this->get_logger(), "dds_DomainParticipant * %zu", reinterpret_cast<size_t>(dp));
    }

    auto callback =
      [this](const std_msgs::msg::String::SharedPtr msg) -> void
      {
        RCLCPP_INFO(this->get_logger(), "I heard: [%s]", msg->data.c_str());
      };
    sub_ = create_subscription<std_msgs::msg::String>("chatter", 10, callback);

    {
      rcl_subscription_t * rcl_sub = (sub_->get_subscription_handle()).get();
      rmw_subscription_t * rmw_sub = rcl_subscription_get_rmw_handle(rcl_sub);
      dds_Subscriber * s = rmw_gurumdds_cpp::get_subscriber(rmw_sub);
      dds_DataReader * dr = rmw_gurumdds_cpp::get_data_reader(rmw_sub);
      RCLCPP_INFO(this->get_logger(), "dds_Subscriber * %zu", reinterpret_cast<size_t>(s));
      RCLCPP_INFO(this->get_logger(), "dds_DataReader * %zu", reinterpret_cast<size_t>(dr));
    }
  }

private:
  rclcpp::Subscription<std_msgs::msg::String>::SharedPtr sub_;
};

int main(int argc, char * argv[])
{
  // Force flush of the stdout buffer.
  // This ensures a correct sync of all prints
  // even when executed simultaneously within the launch file.
  setvbuf(stdout, NULL, _IONBF, BUFSIZ);

  rclcpp::init(argc, argv);
  auto node = std::make_shared<Listener>();
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}
