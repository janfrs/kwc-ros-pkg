#include <map>
#include <queue>
#include <list>
#include <string>
#include <stdexcept>
#include <string.h>
#include <boost/format.hpp>
#include <deque>
#include <vector>
#include "ros/common.h"
#include "ros/time.h"
#include "borg.h"
#include "cam_dc1394.h"
#include "opencv/highgui.h"
#include "newmat10/newmat.h"
#include "newmat10/newmatio.h"
#include "newmat10/newmatap.h"
#include <boost/thread/thread.hpp>

using namespace borg;
using std::string;
using std::map;
using std::list;
using std::deque;
using std::vector;
using boost::format;
using std::runtime_error;
using NEWMAT::Matrix;

bool borg::g_silent = false;

Borg::Borg(uint32_t opts) : cam(NULL), stage(NULL),
  left(50), right(50), scan_duty(700), return_duty(600),
  map_x(NULL), map_y(NULL), image_queue_size(15), laser_thresh(10),
  tx(1), ty(0), tz(0), enc_offset(0), laser_roll(0), max_stripe_width(5),
  tilt(30)
{
  if (opts & INIT_SILENT)
    g_silent = true; // don't print diagnostic / progress information 
  string cfg_path = ros::get_package_path("borg");
  cfg_path += "/borg-config";
  FILE *f = fopen(cfg_path.c_str(), "r");
  if (!f)
  {
    printf("Unable to open the borg-config file in the borg package. Please "
           "create a symlink from one of the borg configuration files in "
           "borg/config, like this:\n"
           "  roscd borg\n"
           "  ln -s config/mobileborg.config borg-config\n");
    exit(1);
  }
  string cam_str;
  map<string, uint32_t> cam_settings;
  map<string, string> stage_settings;
  for (int line = 1; !feof(f); line++)
  {
    char linebuf[200];
    char key[50], value[50];
    if (!fgets(linebuf, sizeof(linebuf), f))
      continue;
    if (linebuf[0] == '#')
      continue;
    int n = sscanf(linebuf, "%50s %50s\n", key, value);
    if (n == 0)
      continue;
    if (n != 2)
    {
      printf("unable to parse line %d of the borg-config file\n", line);
      throw std::runtime_error("borg init failed");
    }
    if (!strcmp(key, "fps"))
      fps = atoi(value);
    else if (!strcmp(key, "left"))
      left = atof(value);
    else if (!strcmp(key, "right"))
      right = atof(value);
    else if (!strcmp(key, "scan_duty"))
      scan_duty = atoi(value);
    else if (!strcmp(key, "return_duty"))
      return_duty = atoi(value);
    else if (!strcmp(key, "laser_thresh"))
      laser_thresh = atoi(value);
    else if (!strcmp(key, "image_queue_size"))
      image_queue_size = atoi(value);
    else if (!strcmp(key, "max_stripe_width" ))
      max_stripe_width = atof(value);
    else if (!strcmp(key, "tilt"))
      tilt = atof(value);
    else if (!strcmp(key, "cam"))
      cam_str = string(value);
    else if (!strncmp(key, "cam_", 4))
      cam_settings[string(key+4)] = atoi(value);
    else if (!strncmp(key, "stage_", 6))
      stage_settings[string(key+6)] = string(value);
    else if (!strncmp(key, "calib_", 6))
      calib_set(key+6, atof(value));
    else
      printf("unknown key = [%s] with value = [%s]\n", key, value);
  }
  fclose(f);
  if (opts & INIT_CAM)
  {
    if (cam_str == string("dc1394"))
    {
      cam = new CamDC1394();
      if (!cam->init())
        throw std::runtime_error("unable to init camera.\n");
      for (map<string, uint32_t>::iterator s = cam_settings.begin();
           s != cam_settings.end(); ++s)
        cam->set(s->first.c_str(), s->second);
    }
    else
    {
      printf("unknown camera\n");
      throw std::runtime_error("borg init failed");
    }
  }
  if (opts & INIT_STAGE)
  {
    stage = new Stage();
    for (map<string,string>::iterator s = stage_settings.begin();
        s != stage_settings.end(); ++s)
      stage->set(s->first.c_str(), s->second);
  }
  intrinsics = cvCreateMat(3, 3, CV_32FC1);
  distortion = cvCreateMat(1, 4, CV_32FC1);
  cvSetZero(intrinsics);
  CV_MAT_ELEM(*intrinsics, float, 0, 0) = fx;
  CV_MAT_ELEM(*intrinsics, float, 1, 1) = fy;
  CV_MAT_ELEM(*intrinsics, float, 0, 2) = x0;
  CV_MAT_ELEM(*intrinsics, float, 1, 2) = y0;
  CV_MAT_ELEM(*intrinsics, float, 2, 2) = 1.0f;
  CV_MAT_ELEM(*distortion, float, 0, 0) = k1;
  CV_MAT_ELEM(*distortion, float, 0, 1) = k2;
  CV_MAT_ELEM(*distortion, float, 0, 2) = k3;
  CV_MAT_ELEM(*distortion, float, 0, 3) = k4;
}

Borg::~Borg()
{
  if (cam)
  {
    cam->shutdown();
    delete cam;
  }
  cvReleaseMat(&intrinsics);
  cvReleaseMat(&distortion);
  if (map_x)
    cvReleaseMat(&map_x);
  if (map_y)
    cvReleaseMat(&map_y);
}

void Borg::scan(list<Image *> &images)
{
  images.clear();
  uint8_t *flush_raster = new uint8_t[640*480];
  cam->startImageStream();
  cam->prepareStill();
  for (int flush = 0; flush < 5; flush++)
    cam->savePhoto(flush_raster);
  stage->setDuty(return_duty);
  stage->gotoPosition(left, true);
  Image *still_image = new Image(new uint8_t[640*480],
                                 ros::Time::now().to_double(), 0);
  if (!cam->savePhoto(still_image->raster))
    printf("woah! couldn't grab still photo\n");
  cam->prepareScan();
  for (int flush = 0; flush < 5; flush++)
    cam->savePhoto(flush_raster);
  stage->setDuty(scan_duty);
  stage->gotoPosition(right, false);
  stage->laser(true);
  ros::Time t_start(ros::Time::now());
  images.push_back(still_image);
  double pos = 0;
  for (ros::Time t_now(ros::Time::now()); 
       (t_now - t_start).to_double() < 15 && fabs(pos - right) > 0.5;
       t_now = ros::Time::now())
  {
    pos = stage->getPosition(1.0);
    Image *image = new Image(new uint8_t[640*480], 
                             ros::Time::now().to_double(), pos);
    if (!cam->savePhoto(image->raster))
    {
      cam->stopImageStream();
      throw runtime_error("couldn't grab photo");
    }
    images.push_back(image);
  }
  stage->laser(false);
  double dt = (ros::Time::now() - t_start).to_double();
  if (!g_silent)
    printf("captured %d images in %.3f seconds (%.3f fps)\n", 
           images.size(), dt, images.size() / dt);
  cam->stopImageStream();
  delete[] flush_raster;
}

void Borg::saveScan(list<Image *> &images, const char *prefix)
{
  if (images.empty())
    return;
  // flush to disk
  for (list<Image *>::iterator i = images.begin(); i != images.end(); ++i)
  {
    char fname[100];
    snprintf(fname, sizeof(fname), "%s_%.6f_%.6f.pgm", 
             prefix, (*i)->t, (*i)->angle);
    if (!cam->writePgm(fname, (*i)->raster))
      throw runtime_error(string("couldn't write to ") + string(prefix));
      /*
    delete[] (*i)->raster;
    delete *i;
    */
  }
  /*
  char fname[100];
  snprintf(fname, sizeof(fname), "out/img_%.6f_%.6f.pgm",
           still_image->t, still_image->angle);
  if (!cam->writePgm(fname, still_image->raster))
    throw std::runtime_error("couldn't open pgm file for still image output");
  */
  if (!cam->writePgm("out/still.pgm", (*images.begin())->raster))
    throw runtime_error("couldn't open pgm file for still image output");
    /*
  images.clear();
  delete still_image;
  */
}

bool Borg::calib_set(const char *setting, double value)
{
  if (!strcmp(setting, "fx"))      fx = value;
  else if (!strcmp(setting, "fy")) fy = value;
  else if (!strcmp(setting, "x0")) x0 = value;
  else if (!strcmp(setting, "y0")) y0 = value;
  else if (!strcmp(setting, "k1")) k1 = value;
  else if (!strcmp(setting, "k2")) k2 = value;
  else if (!strcmp(setting, "k3")) k3 = value;
  else if (!strcmp(setting, "k4")) k4 = value;
  else if (!strcmp(setting, "tx")) tx = value;
  else if (!strcmp(setting, "ty")) ty = value;
  else if (!strcmp(setting, "tz")) tz = value;
  else if (!strcmp(setting, "enc_offset")) enc_offset = value;
  else if (!strcmp(setting, "laser_roll")) laser_roll = value;
  else
    throw std::runtime_error((format("unknown calibration setting %s") % 
                              setting).str());
  return true;
}

void remap_worker(std::list<Borg::Image *> &images, CvMat *map_x, CvMat *map_y)
{
  printf("in worker, size = %d\n", images.size());
  for (std::list<Borg::Image *>::iterator image = images.begin();
       image != images.end(); ++image)
  {
    IplImage *cv_image = cvCreateImageHeader(cvSize(640, 480), IPL_DEPTH_8U, 1);
    cvSetData(cv_image, (*image)->raster, 640);
    (*image)->remapped = cvCreateImage(cvSize(640, 480), IPL_DEPTH_8U, 1);
    cvRemap(cv_image, (*image)->remapped, map_x, map_y);
    cvReleaseImageHeader(&cv_image);
  }
  printf("leaving worker, size = %d\n", images.size());
}

void Borg::extract(std::list<Image *> &images, bool show_gui)
{
  if (show_gui)
    cvNamedWindow("debug");
  if (!map_x || !map_y)
  {
    map_x = cvCreateMat(480, 640, CV_32FC1);
    map_y = cvCreateMat(480, 640, CV_32FC1);
    cvInitUndistortMap(intrinsics, distortion, map_x, map_y);
  }
  deque<IplImage *> image_deque;
  ros::Time t_start(ros::Time::now());
  IplImage *still_remapped;
  vector< vector< vector<double> > > hits(640); // reject blinking stuff
  for (int row = 0; row < 480; row++)
    hits[row].resize(640); // prepare histogram
  // split the images list into multiple chunks
  const int NUM_WORKERS = 4;
  list<Image *> work[NUM_WORKERS];
  int work_idx = 0;
  for (list<Image *>::iterator image = images.begin();
       image != images.end(); ++image)
  {
    work[work_idx].push_back(*image);
    if (++work_idx >= NUM_WORKERS)
      work_idx = 0;
  }
  // farm out these chunks to all cores
  boost::thread workers[NUM_WORKERS];
  for (int i = 0; i < NUM_WORKERS; i++)
    workers[i] = boost::thread(remap_worker, boost::ref(work[i]), map_x, map_y);
  // bring threads back together
  for (int i = 0; i < NUM_WORKERS; i++)
    workers[i].join();
  for (std::list<Image *>::iterator image = images.begin();
       image != images.end(); ++image)
  {
    IplImage *remapped = (*image)->remapped;
    if (image == images.begin())
    {
      cvSaveImage("still_remapped.jpg", remapped);
      cvSaveImage("still_remapped.pgm", remapped);
      still_remapped = remapped; // pull intensity values from this one
      continue; // this one will be brighter; don't difference it
    }
    if (image_deque.size() == 0)
    {
      image_deque.push_back(remapped);
      continue;
    }
    IplImage *ref_image = image_deque.front();
    IplImage *diff_image = cvCreateImage(cvSize(640, 480), IPL_DEPTH_8U, 1);
    IplImage *ref_scaled = cvCreateImage(cvSize(640, 480), IPL_DEPTH_8U, 1);
    CvScalar ref_sum = cvSum(ref_image);
    CvScalar tgt_sum = cvSum(remapped);
    cvConvertScale(ref_image, ref_scaled, tgt_sum.val[0] / ref_sum.val[0]);
    cvSetZero(diff_image);
    for (int row = 0; row < 480; row++)
      for (int col = 0; col < 640; col++)
      {
        int cur = CV_IMAGE_ELEM(remapped, unsigned char, row, col);
        int ref = CV_IMAGE_ELEM(ref_scaled, unsigned char, row, col);
        if (cur - ref > laser_thresh) // laser always injects light
        {
          CV_IMAGE_ELEM(diff_image, unsigned char, row, col) = cur - ref;
          hits[row][col].push_back((*image)->angle);
        }
      }
    for (int row = 0; row < 480; row++)
    {
      vector< vector<int> > clusters;
      vector<int> cluster;
      bool in_cluster = false;
      for (int col = 0; col < 640; col++)
      {
        int v = CV_IMAGE_ELEM(diff_image, unsigned char, row, col);
        if (v)
        {
          cluster.push_back(col);
          in_cluster = true;
        }
        if (!v || col == 639)
        {
          if (in_cluster)
          {
            clusters.push_back(cluster);
            in_cluster = false;
            cluster.clear();
          }
        }
      }
      if (clusters.size() == 0)
        continue; // nothing happened on this row.
      float max_avg = -1, max_centroid = -1;
      vector<int> max_cluster;
      for (size_t c = 0; c < clusters.size(); c++)
      {
        float avg = 0, centroid = 0;
        for (size_t i = 0; i < clusters[c].size(); i++)
        {
          unsigned char v = CV_IMAGE_ELEM(diff_image, unsigned char,
              row, clusters[c][i]);
          avg += v;
          centroid += v * clusters[c][i];
        }
        centroid /= avg;
        avg /= clusters[c].size();
        if (avg > max_avg)
        {
          max_avg = avg;
          max_centroid = centroid;
          max_cluster = clusters[c];
        }
      }
      (*image)->centroids.push_back(Centroid(max_centroid, row, 
            CV_IMAGE_ELEM(still_remapped, unsigned char, row,
                          (int)max_centroid)));
      (*image)->centroids.back().cluster = max_cluster;
    }
    // forward pass
    const double MIN_SLOPE = 0.5;
    const int MAX_ROW_SKIP = 1;
    for (vector<Centroid>::iterator c = (*image)->centroids.begin();
         c != (*image)->centroids.end() && c + 1 != (*image)->centroids.end();
         ++c)
    {
      vector<Centroid>::iterator next_c = c + 1;
      if (fabs((next_c->row - c->row) / (next_c->col - c->col)) < MIN_SLOPE ||
          next_c->row - c->row > MAX_ROW_SKIP)
        c->noisy++;
    }
    // backward pass
    for (vector<Centroid>::reverse_iterator c = (*image)->centroids.rbegin();
         c != (*image)->centroids.rend() && c + 1 != (*image)->centroids.rend();
         ++c)
    {
      vector<Centroid>::reverse_iterator next_c = c + 1;
      if (fabs((c->row - next_c->row) / (next_c->col - c->col)) < MIN_SLOPE ||
          c->row - next_c->row > MAX_ROW_SKIP)
        c->noisy++;
    }
    // toss out points that are marked noisy by forward and backward passes
    for (vector<Centroid>::iterator c = (*image)->centroids.begin();
         c != (*image)->centroids.end();)
    {
      if (c->noisy == 2)
        c = (*image)->centroids.erase(c);
      else
        ++c;
    }
    if (show_gui)
    {
      IplImage *cent_image = cvCreateImage(cvSize(640, 480), IPL_DEPTH_8U, 3);
      cvConvertImage(diff_image, cent_image);
      for (vector<Centroid>::iterator c = (*image)->centroids.begin();
           c != (*image)->centroids.end(); ++c)
      {
        if (c->noisy == 0)
          CV_IMAGE_ELEM(cent_image, unsigned char, c->row, 
                        3 * ((int)c->col)+1) = 255;
        else
          CV_IMAGE_ELEM(cent_image, unsigned char, c->row,
                        3 * ((int)c->col)+2) = 255;
      }
      cvShowImage("debug", cent_image);//diff_image);
      cvWaitKey(10);
      cvReleaseImage(&cent_image);
    }
    cvReleaseImage(&diff_image);
    cvReleaseImage(&ref_scaled);
    if (images.size() > image_queue_size)
    {
      image_deque.pop_front();
      cvReleaseImage(&ref_image);
    }
    image_deque.push_back(remapped);
  }
  if (!g_silent)
    printf("processed %d images in %.3f seconds\n", images.size(), 
           (ros::Time::now() - t_start).to_double());
  // purge the queue
  while (!image_deque.empty())
  {
    IplImage *image = image_deque.front();
    image_deque.pop_front();
    cvReleaseImage(&image);
  }
  cvReleaseImage(&still_remapped);
  // now, go through and prune out pixels of the image that returned 
  // a wide range of disparity
  for (int row = 0; row < 480; row++)
    for (int col = 0; col < 640; col++)
    {
      sort(hits[row][col].begin(), hits[row][col].end());
      if (hits[row][col].size())
      {
        double diff = hits[row][col].back() - hits[row][col].front();
        if (diff > max_stripe_width)
        {
          // go through and blow away all clusters which touched this point
          for (list<Image *>::iterator image = images.begin();
               image != images.end(); ++image)
            for (vector<Centroid>::iterator c = (*image)->centroids.begin();
                 c != (*image)->centroids.end();)
            {
              bool bad_centroid = false;
              if (c->row == row)
                for (vector<int>::iterator j = c->cluster.begin();
                     j != c->cluster.end(); ++j)
                  if (*j == col)
                    bad_centroid = true;
              if (bad_centroid)
                c = (*image)->centroids.erase(c);
              else
                ++c;
            }
        }
      }
    }
}

Borg::Image::Image(const char *filename)
{
  FILE *f = fopen(filename, "rb");
  if (!f)
    throw std::runtime_error((format("couldn't open %s") % filename).str());
  int w, h, max_v;
  if (3 != fscanf(f, "P5\n%d %d\n%d\n", &w, &h, &max_v) || 
      w != 640 || h != 480 || max_v != 255)
    throw std::runtime_error((format("%s is not a 640x480 standard PGM") % 
                             filename).str());
  raster = new uint8_t[640*480];
  if (640*480 != fread(raster, 1, 640*480, f))
    throw std::runtime_error((format("couldn't read a full raster from %s") %
                             filename).str());
  fclose(f);
  const char *name = strrchr(filename, '/');
  if (!name)
    name = filename;
  else
    name++;
  if (strlen(name) >= 32)
  {
    char abuf[50], tbuf[50];
    strncpy(abuf, name + 23, 9);
    abuf[9] = 0;
    strncpy(tbuf, name + 5, 16);
    tbuf[16] = 0;
    angle = atof(abuf);
    t = atof(tbuf);
  }
}

void Borg::printExtraction(std::list<Image *> &images)
{
  for (list<Image *>::iterator image = images.begin();
       image != images.end(); ++image)
    for (vector<Centroid>::iterator c = (*image)->centroids.begin();
         c != (*image)->centroids.end(); ++c)
      printf("%f %d %f %f %f %f\n", (*image)->angle, c->row, c->col,
             c->val / 255.0, c->val / 255.0, c->val / 255.0);
}

void rotate_y(const double th, const double *v, double *w)
{
  w[0] =  cos(th) * v[0] + sin(th) * v[2];
  w[1] = v[1];
  w[2] = -sin(th) * v[0] + cos(th) * v[2];
}

void rotate_z(const double th, const double *v, double *w)
{
  w[0] =  cos(th) * v[0] + sin(th) * v[1];
  w[1] = -sin(th) * v[0] + cos(th) * v[1];
  w[2] =  v[2];
}

void Borg::project(const vector<SensedPoint> &sensed,
                   vector<ProjectedPoint> &projected)
{
  projected.clear();
  for (vector<SensedPoint>::const_iterator p = sensed.begin();
       p != sensed.end(); ++p)
  {
    // compute pixel ray
    const double xc = (p->col - x0) / fx;
    const double yc = (p->row - y0) / fy;
    const double ray_norm = sqrt(xc*xc + yc*yc + 1);
    const double ray[3] = { xc / ray_norm, yc / ray_norm, 1.0 / ray_norm };
    // rotate laser plane normal
    double rolled_laser[3], laser[3];
    const double stationary_laser_norm[3] = { 1, 0, 0 };
    rotate_z(laser_roll * 3.14159 / 180.0, stationary_laser_norm, rolled_laser);
    rotate_y(enc_offset + p->angle * 3.14159 / 180.0, 
             rolled_laser, laser);
    // calculate intersection
    const double t =  (    tx*laser[0] +     ty*laser[1] +     tz*laser[2]) /
                      (ray[0]*laser[0] + ray[1]*laser[1] + ray[2]*laser[2]);
    projected.push_back(ProjectedPoint(t * ray[0], t * ray[1], t * ray[2],
                                       p->row, p->col, p->r, p->g, p->b));
  }
}

void Borg::loadExtractionFile(const char *filename, 
                              std::vector<SensedPoint> &extraction)
{
  FILE *f = fopen(filename, "r");
  if (!f)
    throw runtime_error((format("couldn't open laser extraction file %s") %
                         filename).str());
  while (!feof(f))
  {
    float angle, col, r, g, b;
    int row;
    if (6 != fscanf(f, "%f %d %f %f %f %f\n", &angle, &row, &col, &r, &g, &b))
      throw runtime_error("parse error");
    extraction.push_back(SensedPoint(angle, col, row,
                                     (uint8_t)(r * 255.0),
                                     (uint8_t)(g * 255.0),
                                     (uint8_t)(b * 255.0)));
  }
  //printf("loaded %d points\n", extraction.size());
  fclose(f);
}

#define SHOW_IMAGES

void Borg::calibrate(const double size, const uint32_t x, const uint32_t y,
                     const list<string> &filename_prefixes)
{
  const int horiz = x-1, vert = y-1;
  const int num_corners = horiz * vert;
  vector<CalibrationScene *> scenes;
  for (list<string>::const_iterator fn = filename_prefixes.begin();
       fn != filename_prefixes.end(); ++fn)
  {
    CalibrationScene *cs = new CalibrationScene(size);
    IplImage *img = cvLoadImage((*fn + ".pgm").c_str(),
                                CV_LOAD_IMAGE_GRAYSCALE);
    if (!img)
      throw runtime_error((format("couldn't load %s") % (*fn + ".pgm")).str());
#ifdef SHOW_IMAGES
    cvNamedWindow("checkers", CV_WINDOW_AUTOSIZE);
#endif
    CvSize pattern_size = cvSize(horiz, vert);
    CvPoint2D32f corners[num_corners];
    int corner_count;
    int ok = cvFindChessboardCorners(img, pattern_size,
                                     corners, &corner_count,
                                     CV_CALIB_CB_ADAPTIVE_THRESH |
                                     CV_CALIB_CB_NORMALIZE_IMAGE |
                                     CV_CALIB_CB_FILTER_QUADS);
    if (ok)
    {
      cvFindCornerSubPix(img, corners, corner_count, cvSize(10, 10),
                         cvSize(-1, -1),
                         cvTermCriteria(CV_TERMCRIT_ITER, 10, 0.1f));
      for (int j = 0; j < corner_count; j++)
        cs->corners.push_back(CheckerCorner(corners[j].y, corners[j].x));
      loadExtractionFile((*fn + ".laser").c_str(), cs->points);
      //project(cs->points, cs->proj);
      scenes.push_back(cs);
    }
    else
      delete cs;
#ifdef SHOW_IMAGES
    cvDrawChessboardCorners(img, pattern_size, corners, corner_count, ok);
    cvShowImage("checkers", img);
    cvWaitKey(0);
#endif
    cvReleaseImage(&img);
  }
  printf("%d of %d images had all corners detected\n", 
         scenes.size(), filename_prefixes.size());
  if (!scenes.size())
    throw runtime_error("couldn't find checkerboards on any scenes");
  // now, we hammer on the scenes to make them more reasonable
  // estimate gradient in our directions
  const double t_step = 0.001, enc_step = 0.01, roll_step = 0.01;
  for (int iter = 0; iter < 200; iter++)
  {
    project(scenes);
    double obj = calibration_objective(scenes);
    printf("obj=%.5f tx=%.5f tz=%.5f ofs=%.5f roll=%.5f\n", 
           obj, tx, tz, enc_offset, laser_roll);

    tx += t_step;
    project(scenes);
    double sobj = calibration_objective(scenes);
    double dtx = (sobj - obj) / t_step;
    tx -= t_step;
    
    tz += t_step;
    project(scenes);
    sobj = calibration_objective(scenes);
    double dtz = (sobj - obj) / t_step;
    tz -= t_step;

    enc_offset += enc_step;
    project(scenes);
    sobj = calibration_objective(scenes);
    double drot = (sobj - obj) / enc_step;
    enc_offset -= enc_step;
    
    laser_roll += roll_step;
    project(scenes);
    sobj = calibration_objective(scenes);
    double droll = (sobj - obj) / roll_step;
    laser_roll -= roll_step;

    double grad_len = sqrt(dtx*dtx + dtz*dtz + drot*drot + droll*droll);
    dtx /= grad_len;
    dtz /= grad_len;
    drot /= grad_len;
    droll /= grad_len;
    const double step_len = 0.001;
    tx += step_len * -dtx;
    tz += step_len * -dtz;
    enc_offset += step_len * -drot;
    laser_roll += step_len * -droll;
  }
  scenes[0]->writeFile("final.proj");
}

void Borg::project(vector<CalibrationScene *> &scenes)
{
  for (vector<CalibrationScene *>::iterator cs = scenes.begin();
       cs != scenes.end(); ++cs)
    project((*cs)->points, (*cs)->proj);
}

double Borg::calibration_objective(vector<CalibrationScene *> &scenes)
{
  // first, compute a measure of coplanarity of the points.
  double obj = 0;
  for (vector<CalibrationScene *>::iterator cs = scenes.begin();
       cs != scenes.end(); ++cs)
    obj += (*cs)->objective();
  return obj;
}

double Borg::CalibrationScene::objective()
{
  projectCorners();
  // compute svd of the corners to find their plane
  // first center them
  double cx = 0, cy = 0, cz = 0;
  for (vector<CheckerCorner>::iterator i = corners.begin();
       i != corners.end(); ++i)
  {
    cx += i->x;
    cy += i->y;
    cz += i->z;
  }
  cx /= corners.size();
  cy /= corners.size();
  cz /= corners.size();
  Matrix c(corners.size(), 3);
  int c_row = 1;
  for (vector<CheckerCorner>::iterator i = corners.begin();
       i != corners.end(); ++i, c_row++)
  {
    c(c_row, 1) = i->x - cx;
    c(c_row, 2) = i->y - cy;
    c(c_row, 3) = i->z - cz;
  }
  Matrix U, V;
  NEWMAT::DiagonalMatrix D;
  SVD(c, D, U, V);
  //cout << D << V;
  V(1,1) = V(1,2) = V(1,3) = 0;
  V(2,1) = V(2,2) = V(2,3) = 0;
  // project onto the third principal component
  Matrix c_proj = c * (V.t() * V); 
//  Matrix summer(3, 1);
//  summer << 1 << 1 << 1;
  double fnorm = c_proj.SumAbsoluteValue(); //NormFrobenius(); //Matrix norms = (SP(c_proj, c_proj) * summer;
  double obj = .8 * fnorm;
  //Matrix diff = c - c_proj;
//  cout << c_proj;

  // now, look for the min/max distance between adjacent squares
  vector<double> min_dists;
  for (vector<CheckerCorner>::iterator i = corners.begin();
       i != corners.end(); ++i)
  {
    double min_dist = 1e100;
    for (vector<CheckerCorner>::iterator j = corners.begin();
         j != corners.end(); ++j)
    {
      if (i == j)
        continue;
      double dist = sqrt((i->x - j->x) * (i->x - j->x) +
                         (i->y - j->y) * (i->y - j->y) +
                         (i->z - j->z) * (i->z - j->z));
      if (dist < min_dist)
        min_dist = dist;
    }
    min_dists.push_back(min_dist);
    if (rand() % 50 == 0)
      printf("             %f\n", min_dist);
  }
  //printf("fobj = %f\n", obj);
  for (vector<double>::iterator d = min_dists.begin(); 
       d != min_dists.end(); ++d)
    obj += fabs(*d - sq_size);// * (*d - sq_size);

  return obj;
}

class CornerProjection
{
public:
  Borg::ProjectedPoint *pt; // into projected points matrix
  double dist; // distance from the corner to this point
  CornerProjection(Borg::ProjectedPoint *_pt, double _dist)
  : pt(_pt), dist(_dist) { }
};

bool operator< (const CornerProjection &lhs, const CornerProjection &rhs)
{
  return lhs.dist < rhs.dist;
}

void Borg::CalibrationScene::projectCorners()
{
  for (vector<CheckerCorner>::iterator c = corners.begin();
       c != corners.end(); ++c)
  {
    priority_queue<CornerProjection> q;
    // it would be smart to do an octree or whatever, but we'll brute-force it
    //double min_sq_dist = 1e100;
    //Borg::ProjectedPoint *closest = NULL;
    for (vector<Borg::ProjectedPoint>::iterator p = proj.begin();
         p != proj.end(); ++p)
    {
      double sq_dist = (p->row - c->row) * (p->row - c->row) +
                       (p->col - c->col) * (p->col - c->col);
      //printf("(%f, %f) -> (%d, %f) sq_dist = %f\n", c->row, c->col, p->row, p->col, sq_dist);
      if (sq_dist < 3)
      q.push(CornerProjection(&(*p), sqrt(sq_dist)));
      /*
      if (sq_dist < min_sq_dist)
      {
        min_sq_dist = sq_dist;
        closest = &(*p);
      }
      */
    }
    // take top 10 guys, weight according to exp(-dist)
    double wx = 0, wy = 0, wz = 0;
    int n = 0;
    for (int i = 0; i < 10 && !q.empty(); i++)
    {
      n++;
      CornerProjection cp = q.top();
      q.pop();
      wx += cp.pt->x;
      wy += cp.pt->y;
      wz += cp.pt->z;
    }
    /*
    wx /= 10;
    wy /= 10;
    wz /= 10;
    */
    c->x = wx / n; //closest->x;
    c->y = wy / n; //closest->y;
    c->z = wz / n; //closest->z;
  }
}
    
void Borg::CalibrationScene::writeFile(const char *filename)
{
  FILE *f = fopen(filename, "w");
  for (vector<Borg::ProjectedPoint>::iterator p = proj.begin();
       p != proj.end(); ++p)
    fprintf(f,"%.3f %.3f %.3f 0 0 0 %.3f %.3f %.3f 0\n", 
            p->x, p->y, p->z,
            p->r / 255.0, p->g / 255.0, p->b / 255.0);
  fclose(f);
}

void Borg::sensedPoints(list<Image *> &images, vector<SensedPoint> &pts)
{
  for (list<Image *>::iterator image = images.begin();
       image != images.end(); ++image)
  {
    Image *img = *image;
    for (vector<Centroid>::iterator c = img->centroids.begin();
         c != img->centroids.end(); ++c)
      pts.push_back(SensedPoint(img->angle, c->col, c->row, c->val, c->val, c->val));
  }
}

