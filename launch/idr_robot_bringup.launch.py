from launch import LaunchDescription
from launch.substitutions import PathJoinSubstitution, Command, FindExecutable

from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare

from ament_index_python.packages import get_package_share_directory
import os

def generate_launch_description():

    # Get package share directory
    pkg_share = get_package_share_directory('idr_robot_hw_cpp')

    # Path to YAML config
    param_file = os.path.join(pkg_share, 'config', 'AGV_params.yaml')

    ld = LaunchDescription()

    send_cmd_vel_to_AGV = Node(
        package='idr_robot_hw_cpp',
        executable='send_cmd_vel_to_agv_cpp_node',
        name='send_cmd_vel_websocket_agv_node',
        output='screen',
        parameters=[param_file]

    )

    read_odom_from_AGV = Node(
        package='idr_robot_hw_cpp',
        executable='rece_feedback_from_agv_cpp_node',
        name='rece_odom_websocket_agv_node',
        output='screen',
        parameters=[param_file]

    )

    send_lifting_state_to_AGV = Node(
        package='idr_robot_hw_cpp',
        executable='send_lifting_control_to_agv_cpp_node',
        name='send_lifting_control_websocket_agv_node',
        output='screen',
        parameters=[param_file]

    )

    joy_lift_control = Node(
        package='idr_robot_hw_cpp',
        executable='joy_lift_control_node',
        name='joy_lift_control_node',
        output='screen'
    )

    odom_to_tf_and_path = Node(
        package='idr_robot_hw_cpp',
        executable='odom_to_tf_path_node',

    )

    ld.add_action(send_cmd_vel_to_AGV)
    ld.add_action(read_odom_from_AGV)
    ld.add_action(send_lifting_state_to_AGV)
    ld.add_action(joy_lift_control)
    ld.add_action(odom_to_tf_and_path)

    return ld
