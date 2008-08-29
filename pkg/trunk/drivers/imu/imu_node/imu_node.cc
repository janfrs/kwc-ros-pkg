/*
 * hokuyourg_player
 * Copyright (c) 2008, Willow Garage, Inc.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the <ORGANIZATION> nor the names of its
 *       contributors may be used to endorse or promote products derived from
 *       this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/**

@mainpage

@htmlinclude manifest.html

@b 3dmgx-node

<hr>

@section information Information

<hr>

@section usage Usage

@par Example

@verbatim
$ 3dmgx2_node
@endverbatim

<hr>

@section topic ROS topics

Subscribes to (name/type):
- None

Publishes to (name / type):
- None

<hr>

@section parameters ROS parameters

Reads the following parameters from the parameter server

 **/

#include <assert.h>
#include <math.h>
#include <iostream>

#include "3dmgx2.h"

#include <ros/node.h>
#include <std_msgs/ImuData.h>
#include <std_msgs/EulerAngles.h>
#include "ros/time.h"

#include "self_test/self_test.h"

#include <pthread.h>

using namespace std;

class ImuNode: public ros::node
{
public:
  MS_3DMGX2::IMU imu;
  std_msgs::ImuData reading;
  std_msgs::EulerAngles euler;

  string port;

  MS_3DMGX2::IMU::cmd cmd;

  int count;
  ros::Time next_time;

  SelfTest<ImuNode> self_test_;

  bool running;
  
  ImuNode() : ros::node("imu"), count(0), self_test_(this)
  {
    advertise<std_msgs::ImuData>("imu_data", 100);
    advertise<std_msgs::EulerAngles>("euler_angles", 100);

    param("~port", port, string("/dev/ttyUSB0"));

    string type;
    param("~type", type, string("accel_angrate"));

    check_msg_type(type);
    
    running = false;

    self_test_.setPretest(&ImuNode::pretest);
    self_test_.addTest(&ImuNode::InterruptionTest);
    self_test_.addTest(&ImuNode::ConnectTest);
    self_test_.addTest(&ImuNode::GyroBiasTest);
    self_test_.addTest(&ImuNode::StreamedDataTest);
    self_test_.addTest(&ImuNode::GravityTest);
    self_test_.addTest(&ImuNode::DisconnectTest);
    self_test_.addTest(&ImuNode::ResumeTest);
  }

  void check_msg_type(string type)
  {
    if (type == string("euler"))
      cmd = MS_3DMGX2::IMU::CMD_EULER;
    else
      cmd = MS_3DMGX2::IMU::CMD_ACCEL_ANGRATE;
  }

  ~ImuNode()
  {
    stop();
  }

  int start()
  {
    stop();

    self_test_.lock();

    try
    {
      imu.open_port(port.c_str());

      printf("initializing gyros...\n");

      imu.init_gyros();

      printf("initializing time...\n");

      imu.init_time();

      printf("READY!\n");

      imu.set_continuous(cmd);

      running = true;

    } catch (MS_3DMGX2::exception& e) {
      printf("Exception thrown while starting imu.\n %s\n", e.what());
      self_test_.unlock();
      return -1;
    }

    next_time = ros::Time::now();

    self_test_.unlock();
    return(0);
  }
  
  int stop()
  {
    self_test_.lock();

    if(running)
    {
      try
      {
        imu.close_port();
      } catch (MS_3DMGX2::exception& e) {
        printf("Exception thrown while stopping imu.\n %s\n", e.what());
      }
      running = false;
    }

    self_test_.unlock();
    return(0);
  }

  int publish_datum()
  {

    self_test_.lock();

    try
    {
      uint64_t time;

      switch (cmd) {
      case MS_3DMGX2::IMU::CMD_EULER:
        double roll;
        double pitch;
        double yaw;

        imu.receive_euler(&time, &roll, &pitch, &yaw);

        euler.roll = (float)(roll);
        euler.pitch = (float)(pitch);
        euler.yaw = (float)(yaw);

        euler.header.stamp = ros::Time(time);

        publish("euler_angles", reading);
        printf("%g %g %g\n", roll, pitch, yaw);
        break;

      case MS_3DMGX2::IMU::CMD_ACCEL_ANGRATE:
        double accel[3];
        double angrate[3];

        imu.receive_accel_angrate(&time, accel, angrate);

        reading.accel.x = accel[0];
        reading.accel.y = accel[1];
        reading.accel.z = accel[2];
 
        reading.angrate.x = angrate[0];
        reading.angrate.y = angrate[1];
        reading.angrate.z = angrate[2];

        reading.header.stamp = ros::Time(time);

        publish("imu_data", reading);
        break;

      default:
        printf("Unhandled message type!\n");
        self_test_.unlock();
        return -1;

      }
        
    } catch (MS_3DMGX2::exception& e) {
      printf("Exception thrown while trying to get the reading.\n%s\n", e.what());
      self_test_.unlock();
      return -1;
    }

    count++;
    ros::Time now_time = ros::Time::now();
    if (now_time > next_time) {
      std::cout << count << " scans/sec at " << now_time << std::endl;
      count = 0;
      next_time = next_time + ros::Duration(1,0);
    }

    self_test_.unlock();
    sched_yield();
    return(0);
  }

  bool spin()
  {
    // Start up the laser
    while (ok())
    {
      if (start() == 0)
      {
        while(ok()) {
          if(publish_datum() < 0)
            break;
        }
      } else {
        usleep(1000000);
      }
    }

    stop();

    return true;
  }


  void pretest()
  {
    try
    {
      imu.close_port();
    } catch (MS_3DMGX2::exception& e) {
    }
  }

  void InterruptionTest(robot_msgs::DiagnosticStatus& status)
  {
    status.name = "Interruption Test";

    if (num_subscribers("imu_data") == 0 && num_subscribers("euler_angles") == 0)
    {
      status.level = 0;
      status.message = "No operation interrupted.";
    }
    else
    {
      status.level = 1;
      status.message = "There were active subscribers.  Running of self test interrupted operations.";
    }
  }

  void ConnectTest(robot_msgs::DiagnosticStatus& status)
  {
    status.name = "Connection Test";

    imu.open_port(port.c_str());

    status.level = 0;
    status.message = "Connected successfully.";
  }

  void GyroBiasTest(robot_msgs::DiagnosticStatus& status)
  {
    status.name = "Gyro Bias Test";

    double bias_x;
    double bias_y;
    double bias_z;
    
    imu.init_gyros(&bias_x, &bias_y, &bias_z);

    status.level = 0;
    status.message = "Successfully calculated gyro biases.";

    status.set_values_size(3);
    status.values[0].value_label = "Bias_X";
    status.values[0].value       = bias_x;
    status.values[1].value_label = "Bias_Y";
    status.values[1].value       = bias_y;
    status.values[2].value_label = "Bias_Z";
    status.values[2].value       = bias_z;

  }


  void StreamedDataTest(robot_msgs::DiagnosticStatus& status)
  {
    status.name = "Streamed Data Test";

    uint64_t time;
    double accel[3];
    double angrate[3];

    if (!imu.set_continuous(MS_3DMGX2::IMU::CMD_ACCEL_ANGRATE))
    {
      status.level = 2;
      status.message = "Could not start streaming data.";
    } else {

      for (int i = 0; i < 100; i++)
      {
        imu.receive_accel_angrate(&time, accel, angrate);
      }
      
      imu.stop_continuous();

      status.level = 0;
      status.message = "Data streamed successfully.";
    }
  }

  void GravityTest(robot_msgs::DiagnosticStatus& status)
  {
    status.name = "Streamed Data Test";

    uint64_t time;
    double accel[3];
    double angrate[3];

    double grav = 0.0;

    double grav_x = 0.0;
    double grav_y = 0.0;
    double grav_z = 0.0;

    if (!imu.set_continuous(MS_3DMGX2::IMU::CMD_ACCEL_ANGRATE))
    {
      status.level = 2;
      status.message = "Could not start streaming data.";
    } else {

      int num = 200;

      for (int i = 0; i < num; i++)
      {
        imu.receive_accel_angrate(&time, accel, angrate);
        
        grav_x += accel[0];
        grav_y += accel[1];
        grav_z += accel[2];

      }
      
      imu.stop_continuous();

      grav += sqrt( pow(grav_x / (double)(num), 2.0) + 
                    pow(grav_y / (double)(num), 2.0) + 
                    pow(grav_z / (double)(num), 2.0));
      
      //      double err = (grav - MS_3DMGX2::G);
      double err = (grav - 9.796);
      
      if (fabs(err) < .05)
      {
        status.level = 0;
        status.message = "Gravity detected correctly.";
      } else {
        status.level = 2;
        ostringstream oss;
        oss << "Measured gravity deviates by " << err;
        status.message = oss.str();
      }

      status.set_values_size(2);
      status.values[0].value_label = "Measured gravity";
      status.values[0].value       = grav;
      status.values[1].value_label = "Gravity error";
      status.values[1].value       = err;
    }
  }


  void DisconnectTest(robot_msgs::DiagnosticStatus& status)
  {
    status.name = "Disconnect Test";

    imu.close_port();

    status.level = 0;
    status.message = "Disconnected successfully.";
  }

  void ResumeTest(robot_msgs::DiagnosticStatus& status)
  {
    status.name = "Resume Test";

    if (running)
    {

      imu.open_port(port.c_str());

      if (imu.set_continuous(cmd) != true)
      {
        status.level = 2;
        status.message = "Failed to resume previous mode of operation.";
        return;
      }
    }

    status.level = 0;
    status.message = "Previous operation resumed successfully.";    
  }
};

int
main(int argc, char** argv)
{
  ros::init(argc, argv);

  ImuNode in;

  in.spin();

  ros::fini();

  return(0);
}
