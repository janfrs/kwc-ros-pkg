#define _USE_MATH_DEFINES
#include <math.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include "cloud_viewer/cloud_viewer.h"


CloudViewer::CloudViewer() : 
	cam_x(0), cam_y(0), cam_z(0),
	cam_azi(M_PI), cam_ele(0), cam_rho(3),
	look_tgt_x(0), look_tgt_y(0), look_tgt_z(0),
	left_button_down(false), right_button_down(false),
  hide_axes(false)
{
}

CloudViewer::~CloudViewer()
{
}

void CloudViewer::clear_cloud()
{
	points.clear();
}

void CloudViewer::add_point(float x, float y, float z, uint8_t r, uint8_t g, uint8_t b)
{
	points.push_back(CloudViewerPoint(x,y,z,r,g,b));
}

void CloudViewer::add_point(CloudViewerPoint p)
{
	points.push_back(p);
}

void drawrectgrid(float x_distance, float y_distance, float z_distance, float center_x, float center_y, float center_z, float x_div, float y_div, float z_div)
{
  float i = 0;
  float j = 0;

  glPushMatrix();
  glTranslatef(center_x,center_y,center_z);

  glColor3f(0.2, 0.2, 0.4);

  for(i =-x_distance/2; i<=x_distance/2; i+=x_div)
    {
      glBegin(GL_LINES);
      glVertex3f(i, 0.0,  z_distance/2);
      glVertex3f(i, 0.0, -z_distance/2);
      glEnd();
    }

  for(i =-z_distance/2; i<=z_distance/2; i+=z_div)
    {
      glBegin(GL_LINES);
      glVertex3f(x_distance/2, 0.0,  i);
      glVertex3f(-x_distance/2, 0.0, i);
      glEnd();
    }

  glColor3f(0.2, 0.4, 0.2);
  for(i =-x_distance/2; i<=x_distance/2; i+=x_div)
    {
      glBegin(GL_LINES);
      glVertex3f(i, y_distance/2,  0.0);
      glVertex3f(i, -y_distance/2, 0.0);
      glEnd();
    }

  for(i =-y_distance/2; i<=y_distance/2; i+=y_div)
    {
      glBegin(GL_LINES);
      glVertex3f(x_distance/2, i,  0.0);
      glVertex3f(-x_distance/2, i, 0.0);
      glEnd();
    }

  glPopMatrix();
}


void CloudViewer::render()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glScalef(1.0f, 1.0f, 1.0f);
	// convert camera from spherical coordinates to cartesian...
	cam_x = cam_rho * sinf((float)(M_PI/2) - cam_ele) * cosf(cam_azi) + look_tgt_x;
	cam_y = cam_rho * cosf((float)(M_PI/2) - cam_ele)                 + look_tgt_y;
	cam_z = cam_rho * sinf((float)(M_PI/2) - cam_ele) * sinf(cam_azi) + look_tgt_z;
	gluLookAt(cam_x, cam_y, cam_z, look_tgt_x, look_tgt_y, look_tgt_z, 0, 1, 0);

  if (!hide_axes)
  {
	  glPushMatrix();
  	glTranslatef(look_tgt_x, look_tgt_y, look_tgt_z);
  	const float axes_length = 0.1f;
  	glLineWidth(2);
  	glBegin(GL_LINES);
  		glColor3f(1,0.5,0.5);
  		glVertex3f(0,0,0);
  		glVertex3f(axes_length,0,0);
  		glColor3f(0.5,1,0.5);
  		glVertex3f(0,0,0);
  		glVertex3f(0,axes_length,0);
  		glColor3f(0.5,0.5,1);
  		glVertex3f(0,0,0);
    	glVertex3f(0,0,axes_length);
  	glEnd();
  	glLineWidth(1);
  	glPopMatrix();
	
    // draw a vector from the origin so we don't get lost
	glBegin(GL_LINES);
  		glColor3f(1,1,1);
  		glVertex3f(0,0,0);
  		glVertex3f(look_tgt_x, look_tgt_y, look_tgt_z);
  	glEnd();

	drawrectgrid(4, 4, 4, 2, 0, 0, 1, 1, 1);
  }

	glBegin(GL_POINTS);
	for (size_t i = 0; i < points.size(); i++)
	{
		glColor3ub(points[i].r, points[i].g, points[i].b);
		glVertex3f(points[i].x, points[i].y, points[i].z);
	}
	glEnd();
}

void CloudViewer::set_opengl_params(unsigned width, unsigned height)
{
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glViewport(0, 0, (GLint)width, (GLint)height);
	glEnable(GL_DEPTH_TEST);
	double aspect_ratio = (double)width / height;
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(60.0, aspect_ratio, 0.01, 50.0);
}

void CloudViewer::mouse_button(int x, int y, int button, bool is_down)
{
	if (button == 0)
		left_button_down = is_down;
	else if (button == 2)
		right_button_down = is_down;
}

void CloudViewer::mouse_motion(int x, int y, int dx, int dy)
{
	if (left_button_down)
	{
		cam_azi += 0.01f * dx;
		cam_ele += 0.01f * dy;
		// saturate cam_ele to prevent blowing through the singularity at cam_ele = +/- PI/2
		if (cam_ele > 1.5f)
			cam_ele = 1.5f;
		else if (cam_ele < -1.5f)
			cam_ele = -1.5f;
	}
	else if (right_button_down)
		cam_rho *= (1.0f + 0.01f * dy);
}

void CloudViewer::keypress(char c)
{
	switch(c)
	{
		case 'w': look_tgt_z += 0.05; break;
		case 'x': look_tgt_z -= 0.05; break;
		case 'a': look_tgt_x -= 0.05; break;
		case 'd': look_tgt_x += 0.05; break;
		case 'i': look_tgt_y += 0.05; break;
		case 'k': look_tgt_y -= 0.05; break;
    case 'h': hide_axes = !hide_axes; break;
	}
}
