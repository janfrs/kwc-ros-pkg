#include <unistd.h>
#include <math.h>
#include "ros/node.h"
#include "joy/Joy.h"
#include "std_msgs/BaseVel.h"

using namespace ros;

class TeleopBase : public node
{
public:
  std_msgs::BaseVel cmd, cmd_passthrough;
  joy::Joy joy;
  double req_vx, req_vw, max_vx, max_vw;
  int axis_vx, axis_vw;
  int deadman_button, passthrough_button;

  TeleopBase() : node("teleop_base"), max_vx(0.6), max_vw(0.3)
  {
    cmd.vx = cmd.vw = 0;
    if (!has_param("max_vx") || !get_param("max_vx", max_vx))
      log(WARNING, "maximum linear velocity (max_vx) not set. Assuming 0.6");
    if (!has_param("max_vw") || !get_param("max_vw", max_vx))
      log(WARNING, "maximum angular velocity (max_vw) not set. Assuming 0.3");
    param<int>("axis_vx", axis_vx, 1);
    param<int>("axis_vw", axis_vw, 0);
    param<int>("deadman_button", deadman_button, 0);
    param<int>("passthrough_button", passthrough_button, 1);

    printf("max_vx: %.3f m/s\n", max_vx);
    printf("max_vw: %.3f deg/s\n", max_vw*180.0/M_PI);
    printf("axis_vx: %d\n", axis_vx);
    printf("axis_vw: %d\n", axis_vw);
    printf("deadman_button: %d\n", deadman_button);
    printf("passthrough_button: %d\n", passthrough_button);

    advertise<std_msgs::BaseVel>("cmd_vel", 1);
    subscribe("joy", joy, &TeleopBase::joy_cb, 1);
    subscribe("cmd_passthrough", cmd_passthrough, &TeleopBase::passthrough_cb, 1);
    printf("done with ctor\n");
  }
  void joy_cb()
  {
    /*
    printf("axes: ");
    for(int i=0;i<joy.get_axes_size();i++)
      printf("%.3f ", joy.axes[i]);
    puts("");
    printf("buttons: ");
    for(int i=0;i<joy.get_buttons_size();i++)
      printf("%d ", joy.buttons[i]);
    puts("");
    */

    if((axis_vx >= 0) && (((unsigned int)axis_vx) < joy.get_axes_size()))
      req_vx = joy.axes[axis_vx] * max_vx;
    else
      req_vx = 0.0;
    if((axis_vw >= 0) && (((unsigned int)axis_vw) < joy.get_axes_size()))
      req_vw = joy.axes[axis_vw] * max_vw;
    else
      req_vw = 0.0;
  }
  void passthrough_cb() { }
  void send_cmd_vel()
  {
    joy.lock();
    if((deadman_button < 0) || 
       ((((unsigned int)deadman_button) < joy.get_buttons_size()) &&
        joy.buttons[deadman_button]))
    {
      if (passthrough_button >= 0 && 
          passthrough_button < (int)joy.get_buttons_size() &&
          joy.buttons[passthrough_button])
      {
        // pass through commands that we have received (e.g. from wavefront)
        cmd_passthrough.lock();
        cmd = cmd_passthrough;
        cmd_passthrough.unlock();
      }
      else
      {
        // use commands from the local sticks
        cmd.vx = req_vx;
        cmd.vw = req_vw;
      }
    }
    else
      cmd.vx = cmd.vw = 0;
    joy.unlock();
    publish("cmd_vel", cmd);
  }
};

int main(int argc, char **argv)
{
  ros::init(argc, argv);
  TeleopBase teleop_base;
  while (teleop_base.ok())
  {
    usleep(50000);
    teleop_base.send_cmd_vel();
  }
  ros::fini();
  exit(0);
  return 0;
}

