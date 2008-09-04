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

#include "ethercat_hardware/ethercat_hardware.h"

#include <ethercat/ethercat_xenomai_drv.h>
#include <dll/ethercat_dll.h>

#include <ros/node.h>

EthercatHardware::EthercatHardware() :
  hw_(0), ni_(0), current_buffer_(0), last_buffer_(0), buffer_size_(0)
{
}

EthercatHardware::~EthercatHardware()
{
  if (ni_)
  {
    close_socket(ni_);
  }
  if (buffers_)
  {
    delete buffers_;
  }
  if (hw_)
  {
    delete hw_;
  }
}

static const int NSEC_PER_SEC = 1e+9;

static double now()
{
  struct timespec now;
  clock_gettime(CLOCK_REALTIME, &now);
  return double(now.tv_nsec) / NSEC_PER_SEC + now.tv_sec;
}

void EthercatHardware::init(char *interface)
{
  ros::node *node = ros::node::instance();

  // Initialize network interface
  if ((ni_ = init_ec(interface)) == NULL)
  {
    node->log(ros::FATAL, "Unable to initialize interface: %s", interface);
  }

  // Initialize Application Layer (AL)
  EtherCAT_DataLinkLayer::instance()->attach(ni_);
  if ((al_ = EtherCAT_AL::instance()) == NULL)
  {
    node->log(ros::FATAL, "Unable to initialize Application Layer (AL): %08x", al_);
  }

  unsigned int num_slaves = al_->get_num_slaves();
  if (num_slaves == 0)
  {
    node->log(ros::FATAL, "Unable to locate any slaves");
  }

  // Initialize Master
  if ((em_ = EtherCAT_Master::instance()) == NULL)
  {
    node->log(ros::FATAL, "Unable to initialize EtherCAT_Master: %08x", em_);
  }

  slaves_ = new EthercatDevice*[num_slaves];

  unsigned int num_actuators = 0;
  for (unsigned int slave = 0; slave < num_slaves; ++slave)
  {
    EC_FixedStationAddress fsa(slave + 1);
    EtherCAT_SlaveHandler *sh = em_->get_slave_handler(fsa);
    if (sh == NULL)
    {
      node->log(ros::FATAL, "Unable to get slave handler #%d", slave);
    }

    if ((slaves_[slave] = configSlave(sh)) != NULL)
    {
      num_actuators += slaves_[slave]->has_actuator_;
      buffer_size_ += slaves_[slave]->command_size_ + slaves_[slave]->status_size_;
      if (!sh->to_state(EC_OP_STATE))
      {
        node->log(ros::FATAL, "Unable change to OP_STATE");
      }
    } else {
      node->log(ros::FATAL, "Unable to configure slave #%d, product code: %d", slave, sh->get_product_code());
    }
  }
  buffers_ = new unsigned char[2 * buffer_size_];
  current_buffer_ = buffers_;
  last_buffer_ = buffers_ + buffer_size_;


  // Create HardwareInterface
  hw_ = new HardwareInterface(num_actuators);
  hw_->current_time_ = now();
}

void EthercatHardware::initXml(TiXmlElement *config, MechanismControl &mc)
{
  int i = 0;
  // Determine configuration from XML file 'configuration'
  // TODO: match actuator name to name supplied by board
  for (TiXmlElement *elt = config->FirstChildElement("actuator"); elt; elt = elt->NextSiblingElement("actuator"))
  {
    hw_->actuators_[i]->name_ = elt->Attribute("name");
    ++i;
  }
}
void EthercatHardware::update()
{
  unsigned char *current, *last;

  // Convert HW Interface commands to MCB-specific buffers
  current = current_buffer_;
  for (unsigned int i = 0; i < hw_->actuators_.size(); ++i)
  {
    hw_->actuators_[i]->state_.last_requested_effort_ = hw_->actuators_[i]->command_.effort_;
    slaves_[i]->truncateCurrent(hw_->actuators_[i]->command_);
    slaves_[i]->convertCommand(hw_->actuators_[i]->command_, current);
    current += slaves_[i]->command_size_ + slaves_[i]->status_size_;
  }

  // Transmit process data
  em_->txandrx_PD(buffer_size_, current_buffer_);

  // Convert status back to HW Interface
  current = current_buffer_;
  last = last_buffer_;
  for (unsigned int i = 0; i < hw_->actuators_.size(); ++i)
  {
    slaves_[i]->verifyState(current);
    slaves_[i]->convertState(hw_->actuators_[i]->state_, current, last);
    current += slaves_[i]->command_size_ + slaves_[i]->status_size_;
    last += slaves_[i]->command_size_ + slaves_[i]->status_size_;
  }

  // Update current time
  hw_->current_time_ = now();

  unsigned char *tmp = current_buffer_;
  current_buffer_ = last_buffer_;
  last_buffer_ = tmp;
}

EthercatDevice *
EthercatHardware::configSlave(EtherCAT_SlaveHandler *sh)
{
  static int startAddress = 0x00010000;

  EthercatDevice *p = DeviceFactory::Instance().CreateObject(sh->get_product_code());
  p->configure(startAddress, sh);
  return p;
}
