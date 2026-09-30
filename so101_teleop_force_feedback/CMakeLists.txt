cmake_minimum_required(VERSION 3.8)
project(so101_teleop_force_feedback)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_action REQUIRED)
find_package(std_msgs REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(trajectory_msgs REQUIRED)
find_package(control_msgs REQUIRED)
find_package(so101_msgs REQUIRED)
find_package(rclcpp_realtime REQUIRED)

# Add executables 
add_executable(force_feedback_bridge src/force_feedback_bridge.cpp)
add_executable(force_feedback_teleop src/force_feedback_teleop.cpp)

# Link dependencies
ament_target_dependencies(force_feedback_bridge
	rclcpp
	std_msgs
  sensor_msgs
  trajectory_msgs
  so101_msgs
  rclcpp_realtime)
ament_target_dependencies(force_feedback_teleop
	rclcpp
  rclcpp_action
	std_msgs
  sensor_msgs
  trajectory_msgs
  control_msgs
  so101_msgs
  rclcpp_realtime)

# Install executables
install(TARGETS 
  force_feedback_bridge
	force_feedback_teleop
	DESTINATION lib/${PROJECT_NAME})

# Install share files, launch 
install(DIRECTORY launch config
  DESTINATION share/${PROJECT_NAME}
)


if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  # the following line skips the linter which checks for copyrights
  # comment the line when a copyright and license is added to all source files
  set(ament_cmake_copyright_FOUND TRUE)
  # the following line skips cpplint (only works in a git repo)
  # comment the line when this package is in a git repo and when
  # a copyright and license is added to all source files
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
