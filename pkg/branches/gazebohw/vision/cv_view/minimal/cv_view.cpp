#include <cstdio>
#include <vector>
#include "opencv/cxcore.h"
#include "opencv/cv.h"
#include "opencv/highgui.h"
#include "ros/node.h"
#include "std_msgs/Image.h"
#include "image_utils/cv_bridge.h"

using namespace std;
using namespace ros;

class CvView : public node
{
public:
  std_msgs::Image image_msg;
  CvBridge<std_msgs::Image> cv_bridge;

  CvView() : node("cv_view"), cv_bridge(&image_msg, CvBridge<std_msgs::Image>::CORRECT_BGR)
  { 
    cvNamedWindow("cv_view", CV_WINDOW_AUTOSIZE);
    subscribe("image", image_msg, &CvView::image_cb);
  }
  void image_cb()
  {
    IplImage *cv_image;

    if (cv_bridge.to_cv(&cv_image))
    {
      cvShowImage("cv_view", cv_image);
      cvWaitKey(3);
      cvReleaseImage(&cv_image);
    }
  }
};

int main(int argc, char **argv)
{
  ros::init(argc, argv);
  CvView view;
  view.spin();
  ros::fini();
  return 0;
}

