/*********************************************************************
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2008, Willow Garage, Inc.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of the Willow Garage nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *********************************************************************/

#include <ethercat_hardware/ek1122.h>
#include <iomanip>

#include <rosconsole/rosconsole.h>

static bool reg = DeviceFactory::Instance().Register(EK1122::PRODUCT_CODE, deviceCreator<EK1122>);

EthercatDevice *EK1122::configure(int &startAddress, EtherCAT_SlaveHandler *sh)
{
  sh_ = sh;
  sh->set_fmmu_config( new EtherCAT_FMMU_Config(0) );
  sh->set_pd_config( new EtherCAT_PD_Config(0) );
  return this;
}

int EK1122::initialize(Actuator *, bool)
{
  ROS_INFO("Device #%02d: EK1122 (%#08x)", sh_->get_ring_position(), sh_->get_product_code());
  return 0;
}
void EK1122::diagnostics(robot_msgs::DiagnosticStatus &d, unsigned char *)
{
  vector<robot_msgs::DiagnosticString> strings;
  vector<robot_msgs::DiagnosticValue> values;
  robot_msgs::DiagnosticValue v;
  robot_msgs::DiagnosticString s;

  stringstream str;
  str << "EtherCAT Device #" << setw(2) << setfill('0') << sh_->get_ring_position() << " (EK1122)";
  d.name = str.str();
  d.message = "OK";
  d.level = 0;

  s.label = "Product code";
  str.str("");
  str << "EK1122 (" << sh_->get_product_code() << ")";
  s.value = str.str();
  strings.push_back(s);

  d.set_strings_vec(strings);
  d.set_values_vec(values);
}
