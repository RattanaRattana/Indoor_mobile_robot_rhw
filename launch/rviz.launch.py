from launch import LaunchDescription
from launch.substitutions import PathJoinSubstitution, Command, FindExecutable

from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare

def generate_launch_description():

    ld = LaunchDescription()
    xacro_file = PathJoinSubstitution(
        [FindPackageShare("idr_robot_hw_cpp"), "urdf", "amr_bot_real.xacro"]
    )

    robot_description = Command(
        [PathJoinSubstitution([FindExecutable(name='xacro')]), ' ', xacro_file]
    )

    rviz_file = PathJoinSubstitution(
        [FindPackageShare("idr_robot_hw_cpp"), "rviz", "view_basic.rviz"]
    )

    robot_state_publisher = Node(
        package='robot_state_publisher',
        executable='robot_state_publisher',
        name='robot_state_publisher',
        namespace="idr_robot", # namepace is use to specify the topic "robot_description" when we have multiple urdf model for robot
        output='screen',
        parameters=[
            {'robot_description': robot_description}
        ],
    )
    
    joint_state_publisher_gui_node = Node(
        package='joint_state_publisher_gui',
        executable='joint_state_publisher_gui',
        namespace="idr_robot",
        parameters=[
            {'robot_description': robot_description},
        ],
    )

    joint_state_publisher_node = Node(
        package='manual_con',
        executable='to_jointstate_node',
        namespace="idr_robot",
        parameters=[
            {'robot_description': robot_description},
        ],
    )

    odom_to_joint_states = Node(
        package='idr_robot_hw_cpp',
        executable='odom_to_joint_state_node',

        parameters=[
            {'robot_description': robot_description},
        ],
    )

    rviz2 = Node(
        package='rviz2',
        executable='rviz2',
        name='rviz2',
        output='screen',
        arguments=['-d', rviz_file],
        parameters=[
            {'robot_description': robot_description},
        ]
    )

    ld.add_action(robot_state_publisher)
    #ld.add_action(joint_state_publisher_node)
    #ld.add_action(joint_state_publisher_gui_node)
    ld.add_action(odom_to_joint_states)
    ld.add_action(rviz2)

    return ld
