#include <cstdio>
#include <vector>
#include <map>
#include "opencv/cxcore.h"
#include "opencv/cv.h"
#include "opencv/highgui.h"
#include "ros/node.h"
#include "std_msgs/ImageArray.h"
#include "image_utils/cv_bridge.h"

#include <sys/stat.h>

using namespace std;

struct imgData
{
  string label;
  IplImage *cv_image;
  CvBridge<std_msgs::Image> *bridge;
};

class CvView : public ros::node
{
public:
  std_msgs::ImageArray image_msg;

  ros::thread::mutex cv_mutex;

  char dir_name[256];
  int img_cnt;
  bool made_dir;

  map<string, imgData> images;

  CvView() : node("cv_view", ros::node::ANONYMOUS_NAME), 
             img_cnt(0), made_dir(false)
  { 
    subscribe("images", image_msg, &CvView::image_cb, 1);

    time_t rawtime;
    struct tm* timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    sprintf(dir_name, "%s_images_%.2d%.2d%.2d_%.2d%.2d%.2d", 
            name.c_str()+1, timeinfo->tm_mon + 1, timeinfo->tm_mday,
            timeinfo->tm_year - 100,timeinfo->tm_hour, timeinfo->tm_min, 
            timeinfo->tm_sec);
  }

  ~CvView()
  {
    for (map<string, imgData>::iterator i = images.begin(); i != images.end(); i++)
    {
      if (i->second.cv_image)
        cvReleaseImage(&i->second.cv_image);
    }
  }

  void image_cb()
  {
    cv_mutex.lock();

    for (uint32_t i = 0; i < image_msg.get_images_size(); i++)
    {
      string l = image_msg.images[i].label;
      map<string, imgData>::iterator j = images.find(l);

      if (j == images.end())
      {
        images[l].label = image_msg.images[i].label;
        images[l].bridge = new CvBridge<std_msgs::Image>(&image_msg.images[i], CvBridge<std_msgs::Image>::CORRECT_BGR | CvBridge<std_msgs::Image>::MAXDEPTH_8U);
        cvNamedWindow(l.c_str(), CV_WINDOW_AUTOSIZE);
        images[l].cv_image = 0;
      } else {

        if (j->second.cv_image)
          cvReleaseImage(&j->second.cv_image);

        if (j->second.bridge->to_cv(&j->second.cv_image))
        {
          cvShowImage(j->second.label.c_str(), j->second.cv_image);
        }
      }
      
    }

    cv_mutex.unlock();
  }

  void check_keys() 
  {
    cv_mutex.lock();
    if (cvWaitKey(3) == 10)
    { }
      //      save_image();
    cv_mutex.unlock();
  }

  /*
  void save_image() 
  {
    if (!made_dir) 
    {
      if (mkdir(dir_name, 0755)) 
      {
        printf("Failed to make directory: %s\n", dir_name);
        return;
      } 
      else 
        made_dir = true;
    }
    std::ostringstream oss;
    oss << dir_name << "/Img" << img_cnt++ << ".png";
    cvSaveImage(oss.str().c_str(), cv_image);
  }
  */
};

int main(int argc, char **argv)
{
  ros::init(argc, argv);
  CvView view;
  while (view.ok()) 
  {
    usleep(10000);
    view.check_keys();
  }
  ros::fini();
  return 0;
}

