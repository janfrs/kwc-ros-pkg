#include "std_msgs/LaserScan.h"
#include "ros/node.h"
#include "tf/transform_listener.h"

class Test
{
  public:
    Test()
    {
      node_ = new ros::node("test");
      tf_ = new tf::TransformListener(*node_, true, 
                                      10000000000ULL);
      tf_->setExtrapolationLimit( ros::Duration().fromSec(0.2));

      node_->subscribe("base_scan", scan_, &Test::laser_cb, this, -1);
    }
    ~Test()
    {
      delete tf_;
      delete node_;
    }
    
    void laser_cb()
    {
      // Get the robot's pose 
      tf::Stamped<tf::Pose> ident;
      tf::Stamped<btTransform> odom_pose;
      ident.setIdentity();
      ident.frame_id_ = "base";
      ident.stamp_ = scan_.header.stamp;
      try
      {
        this->tf_->transformPose("odom", ident, odom_pose);
      }
      catch(tf::TransformException e)
      {
        ROS_WARN("Failed to compute odom pose, skipping scan (%s)", e.what());
        return;
      }
      double yaw,pitch,roll;
      btMatrix3x3 mat =  odom_pose.getBasis();
      mat.getEulerZYX(yaw, pitch, roll);

      printf("%f: %.6f %.6f %.6f\n",
             scan_.header.stamp.toSec(),
             odom_pose.getOrigin().x(),
             odom_pose.getOrigin().y(),
             yaw);
    }
    void spin() { node_->spin(); }

  private:
    ros::node* node_;
    tf::TransformListener* tf_;
    std_msgs::LaserScan scan_;
};

int
main(int argc, char** argv)
{
  ros::init(argc, argv);

  Test t;
  t.spin();

  ros::fini();
  return 0;
}

