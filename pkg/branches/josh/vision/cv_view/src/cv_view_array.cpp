#include <cstdio>
#include <vector>
#include <map>
#include "opencv/cxcore.h"
#include "opencv/cv.h"
#include "opencv/highgui.h"
#include "ros/node.h"
#include "std_msgs/ImageArray.h"
#include "image_utils/cv_bridge.h"

#include "colorcalib.h"

#include <sys/stat.h>

using namespace std;

struct imgData
{
  string label;
  IplImage *cv_image;
  CvBridge<std_msgs::Image> *bridge;
  CvMat* color_cal;
};

class CvView : public ros::node
{
public:
  std_msgs::ImageArray image_msg;

  ros::thread::mutex cv_mutex;

  char dir_name[256];
  int img_cnt;
  bool made_dir;

  bool fix_color;
  bool recompand;

  map<string, imgData> images;

  CvView() : node("cv_view", ros::node::ANONYMOUS_NAME), 
             img_cnt(0), made_dir(false), fix_color(false), recompand(false)
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
      if (i->second.color_cal)
        cvReleaseMat(&i->second.color_cal);
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

        images[l].color_cal = cvCreateMat(3, 3, CV_32FC1);
        cvSetIdentity(images[l].color_cal, cvScalar(1.0));

        std::string color_cal_str = map_name("images") + std::string("/") + l + std::string("/color_cal");
        if (has_param(color_cal_str))
        {
          XmlRpc::XmlRpcValue xml_color_cal;
          get_param(color_cal_str, xml_color_cal);
          for (int i = 0; i < 3; i++)
            for (int j = 0; j < 3; j++)
              cvmSet(images[l].color_cal, i, j, (double)(xml_color_cal[3*i + j]));
        }
      } else {

        if (j->second.cv_image)
          cvReleaseImage(&j->second.cv_image);

        if (j->second.bridge->to_cv(&j->second.cv_image))
        {
          
          if (fix_color)
          {
            IplImage* img = cvCreateImage(cvGetSize(j->second.cv_image), IPL_DEPTH_32F, 3);

            decompand(j->second.cv_image, img);

            if (j->second.cv_image->nChannels == 3)
              cvTransform(img, img, j->second.color_cal);

            if (recompand)
            {
              compand(img, j->second.cv_image);
              cvShowImage(j->second.label.c_str(), j->second.cv_image);
            } else {
              cvShowImage(j->second.label.c_str(), img);
            }
            cvReleaseImage(&img);
          }
          else
          {
            cvShowImage(j->second.label.c_str(), j->second.cv_image);
          }
        }
      }
      
    }

    cv_mutex.unlock();
  }

  void check_keys() 
  {
    cv_mutex.lock();
    int key = cvWaitKey(3);

    switch (key) {
    case 10:
      fix_color = !fix_color;
      break;
    case 32:
      recompand = !recompand;
    }

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

