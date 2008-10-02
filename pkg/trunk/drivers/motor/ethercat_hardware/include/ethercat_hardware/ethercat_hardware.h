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

#ifndef ETHERCAT_HARDWARE_H
#define ETHERCAT_HARDWARE_H

#include <tinyxml/tinyxml.h>

#include <mechanism_control/mechanism_control.h>
#include <hardware_interface/hardware_interface.h>

#include <al/ethercat_AL.h>
#include <al/ethercat_master.h>
#include <al/ethercat_slave_handler.h>

#include "ethercat_hardware/ethercat_device.h"

#include <misc_utils/realtime_publisher.h>

class EthercatHardware
{
public:
  /*!
   * \brief Constructor
   */
  EthercatHardware();

  /*!
   * \brief Destructor
   */
  ~EthercatHardware();

  /*!
   * \brief update send most recent motor commands and retrieve updates. This command must be run at a sufficient rate or else the motors will be disabled.
   */
  void update();

  /*!
   * \brief Initialize the EtherCAT Master Library.
   */
  void init(char *interface, bool allow_unprogrammed);

  /*!
   * \brief Register actuators with mechanism control
   */
  void initXml(TiXmlElement *config, bool allow_override);

  /*!
   * \brief Publish diagnostic messages
   */
  void publishDiagnostics();

  HardwareInterface *hw_;

private:
  struct netif *ni_;
  string interface_;

  EtherCAT_AL *al_;
  EtherCAT_Master *em_;

  EthercatDevice *configSlave(EtherCAT_SlaveHandler *sh);
  EthercatDevice **slaves_;
  unsigned int num_slaves_;

  unsigned char *current_buffer_;
  unsigned char *last_buffer_;
  unsigned char *buffers_;
  unsigned int buffer_size_;

  misc_utils::RealtimePublisher<robot_msgs::DiagnosticMessage> publisher_;
  struct {
    struct {
      double roundtrip_;
    } iteration_[1000];
    double max_roundtrip_;
  } diagnostics_;
  robot_msgs::DiagnosticMessage diagnostic_message_;
};

#endif /* ETHERCAT_HARDWARE_H */
