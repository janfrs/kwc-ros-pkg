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

#include <map>
#include <stdio.h>
#include <getopt.h>
#include <sys/mman.h>

#include <ethercat/ethercat_xenomai_drv.h>
#include <dll/ethercat_dll.h>
#include <al/ethercat_AL.h>
#include <al/ethercat_master.h>
#include <al/ethercat_slave_handler.h>

#define WG0X_STANDALONE
#include <ethercat_hardware/wg0x.h>

#include <boost/crc.hpp>
#include <boost/foreach.hpp>

vector<WG0X *> devices;

typedef pair<string, string> ActuatorPair;
map<string, string> actuators;

typedef pair<string, WG0XActuatorInfo> MotorPair;
map<string, WG0XActuatorInfo> motors;

void init(char *interface)
{
  struct netif *ni;

  // Initialize network interface
  if ((ni = init_ec(interface)) == NULL)
  {
    fprintf(stderr, "Unable to initialize interface: %s", interface);
    exit(-1);
  }

  // Initialize Application Layer (AL)
  EtherCAT_DataLinkLayer::instance()->attach(ni);
  EtherCAT_AL *al;
  if ((al = EtherCAT_AL::instance()) == NULL)
  {
    fprintf(stderr, "Unable to initialize Application Layer (AL): %08x", (int)al);
    exit(-1);
  }

  uint32_t num_slaves = al->get_num_slaves();
  if (num_slaves == 0)
  {
    fprintf(stderr, "Unable to locate any slaves");
    exit(-1);
  }

  // Initialize Master
  EtherCAT_Master *em;
  if ((em = EtherCAT_Master::instance()) == NULL)
  {
    fprintf(stderr, "Unable to initialize EtherCAT_Master: %08x", int(em));
    exit(-1);
  }

  static int startAddress = 0x00010000;

  for (unsigned int slave = 0; slave < num_slaves; ++slave)
  {
    EC_FixedStationAddress fsa(slave + 1);
    EtherCAT_SlaveHandler *sh = em->get_slave_handler(fsa);
    if (sh == NULL)
    {
      fprintf(stderr, "Unable to get slave handler #%d", slave);
      exit(-1);
    }

    if (sh->get_product_code() == WG05::PRODUCT_CODE)
    {
      printf("found a WG05 at #%d\n", slave);
      WG05 *dev = new WG05();
      dev->configure(startAddress, sh);
      devices.push_back(dev);
    }
    else if (sh->get_product_code() == WG06::PRODUCT_CODE)
    {
      printf("found a WG06 at #%d\n", slave);
      WG06 *dev = new WG06();
      dev->configure(startAddress, sh);
      devices.push_back(dev);
    }
    else
    {
      devices.push_back(NULL);
    }
  }

  BOOST_FOREACH(WG0X *device, devices)
  {
    Actuator a;
    if (!device) continue;
    if (!device->sh_->to_state(EC_OP_STATE))
    {
      fprintf(stderr, "Unable set device %d into OP_STATE", device->sh_->get_ring_position());
    }
    device->initialize(&a, true);
  }

  BOOST_FOREACH(WG0X *device, devices)
  {
    if (!device) continue;
    printf("isProgrammed = %d\n", device->isProgrammed());
  }
}

void programDevice(int device, WG0XActuatorInfo &config, char *name)
{
  if (devices[device])
  {
    printf("Programming device %d, to be named: %s\n", device, name);
    strcpy(config.name_, name);
    boost::crc_32_type crc32;
    crc32.process_bytes(&config, sizeof(config)-sizeof(config.crc32_));
    config.crc32_ = crc32.checksum();
    devices[device]->program(&config);
  }
  else
  {
    printf("There is no device at position #%d\n", device);
  }
}

static struct
{
  char *program_name_;
  char *interface_;
  char *name_;
  bool program_;
  int device_;
  string motor_;
} g_options;

void Usage(string msg = "")
{
  fprintf(stderr, "Usage: %s [options]\n", g_options.program_name_);
  fprintf(stderr, " -i, --interface <i>    Use the network interface <i>\n");
  fprintf(stderr, " -d, --device <d>       Select the device to program\n");
  fprintf(stderr, " -p, --program          Program a motor control board\n");
  fprintf(stderr, " -n, --name <n>         Set the name of the motor control board to <n>\n");
  fprintf(stderr, " -m, --motor <m>        Set the configuration for motor <m>\n");
  fprintf(stderr, "     Legal motor values are:\n");
  BOOST_FOREACH(MotorPair p, motors)
  {
    string name = p.first;
    WG0XActuatorInfo info = p.second;
    printf("        %s - %s %s\n", name.c_str(), info.motor_make_, info.motor_model_);
  }
  fprintf(stderr, " -h, --help    Print this message and exit\n");
  if (msg != "")
  {
    fprintf(stderr, "Error: %s\n", msg.c_str());
    exit(-1);
  }
  else
  {
    exit(0);
  }
}

void parseConfig(TiXmlElement *config)
{
  TiXmlElement *actuatorElt = config->FirstChildElement("actuators");
  TiXmlElement *motorElt = config->FirstChildElement("motors");

  for (TiXmlElement *elt = actuatorElt->FirstChildElement("actuator");
       elt;
       elt = elt->NextSiblingElement("actuator"))
  {
    const char *name = elt->Attribute("name");
    const char *motor = elt->Attribute("motor");
    actuators[name] = motor;
  }

  WG0XActuatorInfo info;
  memset(&info, 0, sizeof(info));
  info.minor_ = 1;
  strcpy(info.robot_name_, "PR2");
  for (TiXmlElement *elt = motorElt->FirstChildElement("motor");
       elt;
       elt = elt->NextSiblingElement("motor"))
  {
    const char *name = elt->Attribute("name");
    TiXmlElement *params = elt->FirstChildElement("params");
    TiXmlElement *encoder = elt->FirstChildElement("encoder");

    strcpy(info.motor_make_, params->Attribute("make"));
    strcpy(info.motor_model_, params->Attribute("model"));

    info.max_current_ = atof(params->Attribute("max_current"));
    info.speed_constant_ = atof(params->Attribute("speed_constant"));
    info.resistance_ = atof(params->Attribute("resistance"));
    info.motor_torque_constant_ = atof(params->Attribute("motor_torque_constant"));

    info.pulses_per_revolution_ = atoi(encoder->Attribute("pulses_per_revolution"));
    info.sign_ = atoi(encoder->Attribute("sign"));

    motors[name] = info;
  }
}

int main(int argc, char *argv[])
{
  // Keep the kernel from swapping us out
  mlockall(MCL_CURRENT | MCL_FUTURE);

  // Parse configuration file
  TiXmlDocument xml("actuators.conf");
  if (xml.LoadFile())
  {
    parseConfig(xml.RootElement());
  }
  else
  {
    Usage("Unable to load configuration file");
  }
  //
  // Parse options
  g_options.program_name_ = argv[0];
  g_options.device_ = -1;
  while (1)
  {
    static struct option long_options[] = {
      {"help", no_argument, 0, 'h'},
      {"interface", required_argument, 0, 'i'},
      {"name", required_argument, 0, 'n'},
      {"device", required_argument, 0, 'd'},
      {"motor", required_argument, 0, 'm'},
      {"program", no_argument, 0, 'p'},
    };
    int option_index = 0;
    int c = getopt_long(argc, argv, "d:hi:m:n:p", long_options, &option_index);
    if (c == -1) break;
    switch (c)
    {
      case 'h':
        Usage();
        break;
      case 'd':
        g_options.device_ = atoi(optarg);
        break;
      case 'i':
        g_options.interface_ = optarg;
        break;
      case 'n':
        g_options.name_ = optarg;
        break;
      case 'm':
        g_options.motor_ = optarg;
        break;
      case 'p':
        g_options.program_ = 1;
        break;
    }
  }

  if (optind < argc)
  {
    Usage("Extra arguments");
  }

  if (!g_options.interface_)
    Usage("You must specify a network interface");


  init(g_options.interface_);

  if (g_options.program_)
  {
    if (!g_options.name_)
      Usage("You must specify a name");
    if (g_options.motor_ == "")
    {
      if (actuators.find(g_options.name_) == actuators.end())
        Usage("No default motor for this name");
      g_options.motor_ = actuators[g_options.name_];
    }
    if (g_options.device_ == -1)
      Usage("You must specify a device #");
    if (motors.find(g_options.motor_) == motors.end())
      Usage("You must specify a valid motor");

    programDevice(g_options.device_, motors[g_options.motor_], g_options.name_);
  }

  return 0;
}
