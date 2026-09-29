#include <memory>
#include <vector>
#include <string>

#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/joint_state.hpp"
#include "so101_msgs/srv/set_torque.hpp"

using SetTorque = so101_msgs::srv::SetTorque;

class teleop_force_feedbackBridge : public rclcpp::Node
{
public:
  teleop_force_feedbackBridge()
      : Node("force_feedback_bridge")
  {

    auto names = this->list_parameters({}, 10);

    using std::placeholders::_1;

    // get parameter
    follower_state_topic = this->declare_parameter<std::string>("follower_state_topic", "/follower/joint_states");
    leader_state_topic = this->declare_parameter<std::string>("leader_state_topic", "/leader/joint_states");
    leader_state_pub_topic = this->declare_parameter<std::string>("leader_state_pub_topic", "/leader/teleop_force_feedback_joint_states");
    lock_duration = this->declare_parameter<double>("lock_duration", 3.0);

    joint_names = this->declare_parameter<std::vector<std::string>>("joint_names", {"shoulder_pan", "shoulder_lift", "elbow_flex", "wrist_flex", "wrist_roll", "gripper"});
    joint_ids = this->declare_parameter<std::vector<int64_t>>("joint_ids", {1, 2, 3, 4, 5, 6});
    current_lock_thresholds = this->declare_parameter<std::vector<double>>("current_lock_thresholds", {20.0, 20.0, 30.0, 20.0, 20.0, 20.0});
    current_unlock_thresholds = this->declare_parameter<std::vector<double>>("current_unlock_thresholds", {0.5, 0.5, 10.0, 0.5, 0.5, 0.5});

    // QoS
    rclcpp::QoS qos(10);
    qos.best_effort();

    // Subscriber (Follower)
    follower_sub = this->create_subscription<sensor_msgs::msg::JointState>(
        follower_state_topic,
        qos,
        std::bind(&teleop_force_feedbackBridge::follower_callback, this, _1));

    // Subscriber (Leader)
    leader_sub = this->create_subscription<sensor_msgs::msg::JointState>(
        leader_state_topic,
        qos,
        std::bind(&teleop_force_feedbackBridge::leader_callback, this, _1));

    // Publisher (Leader)
    leader_pub = this->create_publisher<sensor_msgs::msg::JointState>(
        leader_state_pub_topic,
        qos);

    client_ = this->create_client<SetTorque>("/set_torque");

    // wait for service
    while (!client_->wait_for_service(std::chrono::seconds(1))){}

    // check yaml settings
    RCLCPP_INFO(this->get_logger(), "teleop_force_feedback Bridge Started");
    RCLCPP_INFO(this->get_logger(), "Follower state topic: %s", follower_state_topic.c_str());
    RCLCPP_INFO(this->get_logger(), "Leader state topic: %s", leader_state_topic.c_str());
    RCLCPP_INFO(this->get_logger(), "Leader state pub topic: %s", leader_state_pub_topic.c_str());
    RCLCPP_INFO(this->get_logger(), "Lock duration: %.2f sec", lock_duration);
    for (size_t i = 0; i < current_lock_thresholds.size(); ++i)
    {
      RCLCPP_INFO(this->get_logger(),"current lock thresholds[%zu] = %.2f",i,current_lock_thresholds[i]);
    }

    // yaml setting check
    if (joint_names.size() != joint_ids.size() ||
        joint_names.size() != current_lock_thresholds.size() ||
        joint_names.size() != current_unlock_thresholds.size())
    {
      throw std::runtime_error("Joint parameter size mismatch at force_feedback_bridge.yaml");
    }

    // init timer
    current_time = this->now();
  }

private:
  void set_torque_req(bool req, int id)
  {
    auto request = std::make_shared<SetTorque::Request>();
    request->ids = {static_cast<uint8_t>(id)};
    request->enable = req;
    RCLCPP_INFO(this->get_logger(), "MotorID[%i]/[%s] Torque request:%s", id, joint_names[id].c_str(), (req ? "ON" : "OFF"));

    auto future = client_->async_send_request(request,std::bind(&teleop_force_feedbackBridge::response_callback, this, std::placeholders::_1));
  }

  void response_callback(rclcpp::Client<SetTorque>::SharedFuture future)
  {
    auto response = future.get();
    if (response->success)
    {
      RCLCPP_INFO(this->get_logger(), "Torque service send success");
    }
    else
    {
      RCLCPP_WARN(this->get_logger(), "Torque service send failed");
    }
  }

  void leader_callback(const sensor_msgs::msg::JointState::SharedPtr msg)
  {
    leader_state.name = msg->name;
    leader_state.position = msg->position;
    leader_set = true;
  }

  bool can_send_command(int i, std::string state)
  {
    current_time = this->now();
    if ((current_time - last_lock_time[i]).seconds() > lock_duration)
    {
      if (state == "lock")
      {
        last_lock_time[i] = current_time;
      }
      return true;
    }
    return false;
  }

  // for check joint index
  int find_joint_index(const std::string &name)
  {
    for (size_t i = 0; i < joint_names.size(); ++i)
    {
      if (joint_names[i] == name)
        return i;
    }
    return -1;
  }

  void follower_callback(const sensor_msgs::msg::JointState::SharedPtr msg)
  {
    // check leader init position
    if (!leader_set || leader_state.position.size() != msg->position.size())
    {
      return;
    }

    auto now = this->now();

    sensor_msgs::msg::JointState out;
    out.header.stamp = now;
    out.name = msg->name;
    out.position = leader_state.position;

    // initialize lock position
    if (locked_position.empty())
    {
      locked_position = leader_state.position;
      is_locked.resize(leader_state.position.size(), false);
      last_lock_time.resize(leader_state.position.size(), now);
    }

    // check motor current val
    for (size_t i = 0; i < msg->effort.size(); i++)
    {
      const std::string &joint_name = msg->name[i];
      int idx = find_joint_index(joint_name);
      if (idx < 0)
      {
        RCLCPP_WARN(this->get_logger(), "Unknown joint: %s", joint_name.c_str());
      }
      else
      {
        // lock
        if (msg->effort[i] > current_lock_thresholds[idx])
        {
          if (!is_locked[i])
          {
            locked_position[i] = leader_state.position[i];
            if (can_send_command(i, "lock"))
            {
              set_torque_req(true, joint_ids[idx]);
              is_locked[i] = true;
              RCLCPP_WARN(this->get_logger(), "Lock joint %s / current:%.2f", msg->name[i].c_str(), msg->effort[i]);
            }
          }
        }
        // Unlock
        else if (msg->effort[i] < current_unlock_thresholds[idx])
        {
          if (is_locked[i])
          {
            if (can_send_command(i, "unlock"))
            {
              set_torque_req(false, joint_ids[idx]);
              is_locked[i] = false;
              RCLCPP_INFO(this->get_logger(), "Unlock joint %s / current:%.2f", msg->name[i].c_str(), msg->effort[i]);
            }
          }
        }
      }

      if (is_locked[i])
      {
        out.position[i] = locked_position[i];
      }
      else
      {
        out.position[i] = leader_state.position[i];
      }
    }

    leader_pub->publish(out);
  }

  std::string follower_state_topic;
  std::string leader_state_topic;
  std::string leader_state_pub_topic;
  double lock_duration;
  bool leader_set = false;
  std::vector<double> locked_position;
  std::vector<bool> is_locked;
  sensor_msgs::msg::JointState leader_state;
  rclcpp::Time current_time;
  std::vector<rclcpp::Time> last_lock_time;
  std::vector<std::string> joint_names;
  std::vector<int64_t> joint_ids;
  std::vector<double> current_lock_thresholds;
  std::vector<double> current_unlock_thresholds;

  rclcpp::Subscription<sensor_msgs::msg::JointState>::SharedPtr follower_sub;
  rclcpp::Subscription<sensor_msgs::msg::JointState>::SharedPtr leader_sub;
  rclcpp::Publisher<sensor_msgs::msg::JointState>::SharedPtr leader_pub;
  rclcpp::Client<SetTorque>::SharedPtr client_;
};

int main(int argc, char **argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<teleop_force_feedbackBridge>());
  rclcpp::shutdown();
  return 0;
}
