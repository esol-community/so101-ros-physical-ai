from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import (
    LaunchConfiguration,
    PathJoinSubstitution,
    PythonExpression,
)
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare
from launch.actions import LogInfo


def generate_launch_description():
    leader_ns = LaunchConfiguration("leader_namespace")
    follower_ns = LaunchConfiguration("follower_namespace")
    arm_controller = LaunchConfiguration("arm_controller")
    params_file = LaunchConfiguration("params_file")

    bridge_params_file = LaunchConfiguration("bridge_params_file")    

    realtime_bridge = Node(
        package='so101_teleop_force_feedback',
        executable='force_feedback_bridge',
        name='force_feedback_bridge',
        parameters=[bridge_params_file],
        output='screen'
    )

    realtime_teleop = Node(
        package='so101_teleop_force_feedback',
        executable='force_feedback_teleop',
        name="follower_command_relay",
        parameters=[params_file],
        output='screen'
    )

    default_params = PathJoinSubstitution(
        [FindPackageShare("so101_teleop_force_feedback"), "config", "force_feedback_teleop.yaml"]
    )
    default_bridge_params = PathJoinSubstitution(
        [FindPackageShare("so101_teleop_force_feedback"), "config", "force_feedback_bridge.yaml"]
    )

    return LaunchDescription(
        [
            DeclareLaunchArgument("leader_namespace", default_value="leader"),
            DeclareLaunchArgument("follower_namespace", default_value="follower"),
            DeclareLaunchArgument(
                "arm_controller", default_value="forward_controller"
            ),  # trajectory_controller|forward_controller
            DeclareLaunchArgument("params_file", default_value=default_params),
            DeclareLaunchArgument("bridge_params_file",default_value=default_bridge_params),
            realtime_bridge,
            realtime_teleop,
            
            LogInfo(msg=["bridge yaml = ", bridge_params_file]),
            LogInfo(msg=["teleop yaml = ", params_file]),
        ]
    )
