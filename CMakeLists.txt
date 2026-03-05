cmake_minimum_required(VERSION 3.8)
project(idr_robot_hw_cpp)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(trajectory_msgs REQUIRED)
find_package(linkattacher_msgs REQUIRED)
find_package(nav_msgs REQUIRED)
find_package(tf2 REQUIRED)
find_package(tf2_geometry_msgs REQUIRED)
find_package(std_srvs REQUIRED)

install(DIRECTORY launch meshes models urdf rviz config
    DESTINATION share/${PROJECT_NAME}/
)

add_executable(odom_to_joint_state_node src/odom_to_joint_state_node.cpp)

ament_target_dependencies(odom_to_joint_state_node
  rclcpp
  nav_msgs
  sensor_msgs
  tf2
  tf2_geometry_msgs
)

install(TARGETS
  odom_to_joint_state_node
  DESTINATION lib/${PROJECT_NAME}
)


add_executable(odom_to_tf_path_node src/odom_to_tf_path_node.cpp)
ament_target_dependencies(
  odom_to_tf_path_node
  rclcpp
  nav_msgs
  geometry_msgs
  tf2_ros
)

install(TARGETS
  odom_to_tf_path_node
  DESTINATION lib/${PROJECT_NAME}
)

add_executable(send_cmd_vel_to_agv_cpp_node
  src/send_cmd_vel_to_agv_cpp_node.cpp
)

target_include_directories(send_cmd_vel_to_agv_cpp_node PUBLIC
  /usr/include
)

target_link_libraries(send_cmd_vel_to_agv_cpp_node
  pthread
)

ament_target_dependencies(send_cmd_vel_to_agv_cpp_node
  rclcpp
  geometry_msgs
)

install(TARGETS
  send_cmd_vel_to_agv_cpp_node
  DESTINATION lib/${PROJECT_NAME}
)

add_executable(rece_feedback_from_agv_cpp_node src/rece_feedback_from_agv_cpp_node.cpp)

ament_target_dependencies(rece_feedback_from_agv_cpp_node
  rclcpp
  nav_msgs
  std_msgs
)

target_link_libraries(rece_feedback_from_agv_cpp_node pthread)

install(TARGETS
  rece_feedback_from_agv_cpp_node
  DESTINATION lib/${PROJECT_NAME}
)

add_executable(send_lifting_control_to_agv_cpp_node src/send_lifting_control_to_agv_cpp_node.cpp)

ament_target_dependencies(send_lifting_control_to_agv_cpp_node
  rclcpp
  std_srvs
)

target_link_libraries(send_lifting_control_to_agv_cpp_node pthread)

install(TARGETS
  send_lifting_control_to_agv_cpp_node
  DESTINATION lib/${PROJECT_NAME}
)

add_executable(joy_lift_control_node src/joy_lift_control_node.cpp)
ament_target_dependencies(joy_lift_control_node rclcpp sensor_msgs std_srvs)

install(TARGETS
  joy_lift_control_node
  DESTINATION lib/${PROJECT_NAME}
)

ament_package()