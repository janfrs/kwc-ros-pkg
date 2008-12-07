#include "ros/node.h"

#include "std_msgs/VisualizationMarker.h"

int main( int argc, char** argv )
{
  ros::init( argc, argv );

  ros::node* node = new ros::node( "MarkerTest", ros::node::DONT_HANDLE_SIGINT );

  while ( !node->ok() )
  {
    usleep( 10000 );
  }

  node->advertise<std_msgs::VisualizationMarker>( "visualizationMarker", 0 );

  usleep( 1000000 );

  for ( int i = -50; i < 50; ++i )
  {
    std_msgs::VisualizationMarker marker;
    marker.header.frame_id = "map";
    marker.header.stamp = ros::Time();
    marker.id = i;
    marker.type = std_msgs::VisualizationMarker::CUBE;
    marker.action = 0;
    marker.x = 1;
    marker.y = (i*2);
    marker.z = 0;
    marker.yaw = 0.0;
    marker.pitch = 0.0;
    marker.roll = 0.0;
    marker.xScale = 0.2;
    marker.yScale = 0.2;
    marker.zScale = 0.2;
    marker.alpha = 100;
    marker.r = 0;
    marker.g = 255;
    marker.b = 0;
    node->publish( "visualizationMarker", marker );
  }

  std_msgs::VisualizationMarker line_marker;
  line_marker.header.frame_id = "map";
  line_marker.header.stamp = ros::Time();
  line_marker.id = 1000;
  line_marker.type = std_msgs::VisualizationMarker::LINE_STRIP;
  line_marker.action = 0;
  line_marker.x = 0;
  line_marker.y = 0;
  line_marker.z = 0;
  line_marker.yaw = 0.0;
  line_marker.pitch = 0.0;
  line_marker.roll = 0.0;
  line_marker.xScale = 0.05;
  line_marker.alpha = 100;
  line_marker.r = 0;
  line_marker.g = 255;
  line_marker.b = 0;
  for ( int i = -50; i < 50; ++i )
  {
    std_msgs::Position p;
    p.x = 1;
    p.y = (i*2);
    p.z = 0;
    line_marker.points.push_back(p);
  }

  node->publish( "visualizationMarker", line_marker );

  usleep( 1000000 );

  node->shutdown();
  delete node;

  ros::fini();
}
