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

#include <iomanip>

#include <math.h>

#include <ros/node.h>

#include <ethercat_hardware/wg0x.h>

#include <dll/ethercat_dll.h>
#include <al/ethercat_AL.h>
#include <dll/ethercat_device_addressed_telegram.h>
#include <dll/ethercat_frame.h>

#include <boost/crc.hpp>

static bool reg05 = DeviceFactory::Instance().Register(WG05::PRODUCT_CODE, deviceCreator<WG05> );
static bool reg06 = DeviceFactory::Instance().Register(WG06::PRODUCT_CODE, deviceCreator<WG06> );

static unsigned int rotateRight8(unsigned in)
{
  in &= 0xff;
  in = (in >> 1) | (in << 7);
  in &= 0xff;
  return in;
}

static unsigned computeChecksum(void const *data, unsigned length)
{
  unsigned char *d = (unsigned char *)data;
  unsigned int checksum = 0x42;
  for (unsigned int i = 0; i < length; ++i)
  {
    checksum = rotateRight8(checksum);
    checksum ^= d[i];
    checksum &= 0xff;
  }
  return checksum;
}

void WG0XMbxHdr::build(uint16_t address, uint16_t length, bool write_nread)
{
  address_ = address;
  length_ = length - 1;
  pad_ = 0;
  write_nread_ = write_nread;
  checksum_ = rotateRight8(computeChecksum(this, sizeof(*this) - 1));
}

bool WG0XMbxHdr::verifyChecksum(void) const
{
  return computeChecksum(this, sizeof(*this)) != 0;
}

void WG0XMbxCmd::build(unsigned address, unsigned length, bool write_nread, void const *data)
{
  this->hdr_.build(address, length, write_nread);
  if (data != NULL)
  {
    memcpy(data_, data, length);
  }
  else
  {
    memset(data_, 0, length);
  }
  unsigned int checksum = rotateRight8(computeChecksum(data_, length));
  data_[length] = checksum;
}

EthercatDevice *WG0X::configure(int &startAddress, EtherCAT_SlaveHandler *sh)
{
  sh_ = sh;
  bool isWG06 = sh->get_product_code() == WG06::PRODUCT_CODE;

  EC_FMMU *statusFMMU = new EC_FMMU(startAddress, // Logical start address
                                    sizeof(WG0XStatus), // Logical length
                                    0x00, // Logical StartBit
                                    0x07, // Logical EndBit
                                    STATUS_PHY_ADDR, // Physical Start address
                                    0x00, // Physical StartBit
                                    true, // Read Enable
                                    false, // Write Enable
                                    true); // Enable

  startAddress += sizeof(WG0XStatus);

  EC_FMMU *commandFMMU = new EC_FMMU(startAddress, // Logical start address
                                    sizeof(WG0XCommand),// Logical length
                                    0x00, // Logical StartBit
                                    0x07, // Logical EndBit
                                    COMMAND_PHY_ADDR, // Physical Start address
                                    0x00, // Physical StartBit
                                    false, // Read Enable
                                    true, // Write Enable
                                    true); // Enable

  startAddress += sizeof(WG0XCommand);

  EtherCAT_FMMU_Config *fmmu;
  if (isWG06)
  {
    EC_FMMU *pressureFMMU = new EC_FMMU(startAddress, // Logical start address
                                      sizeof(WG06Pressure), // Logical length
                                      0x00, // Logical StartBit
                                      0x07, // Logical EndBit
                                      PRESSURE_PHY_ADDR, // Physical Start address
                                      0x00, // Physical StartBit
                                      true, // Read Enable
                                      false, // Write Enable
                                      true); // Enable

    startAddress += sizeof(WG06Pressure);

    fmmu = new EtherCAT_FMMU_Config(3);
    (*fmmu)[2] = *pressureFMMU;
  }
  else
  {
    fmmu = new EtherCAT_FMMU_Config(2);
  }
  (*fmmu)[0] = *statusFMMU;
  (*fmmu)[1] = *commandFMMU;
  sh->set_fmmu_config(fmmu);


  EtherCAT_PD_Config *pd = isWG06 ? new EtherCAT_PD_Config(5) : new EtherCAT_PD_Config(4);

  // Sync managers
  EC_SyncMan *commandSM = new EC_SyncMan(COMMAND_PHY_ADDR, sizeof(WG0XCommand), EC_BUFFERED, EC_WRITTEN_FROM_MASTER);
  commandSM->ChannelEnable = true;
  commandSM->ALEventEnable = true;

  EC_SyncMan *statusSM = new EC_SyncMan(STATUS_PHY_ADDR, sizeof(WG0XStatus));
  statusSM->ChannelEnable = true;

  EC_SyncMan *pressureSM = new EC_SyncMan(PRESSURE_PHY_ADDR, sizeof(WG06Pressure));
  pressureSM->ChannelEnable = true;

  EC_SyncMan *mbxCommandSM = new EC_SyncMan(MBX_COMMAND_PHY_ADDR, MBX_COMMAND_SIZE, EC_QUEUED, EC_WRITTEN_FROM_MASTER);
  mbxCommandSM->ChannelEnable = true;
  mbxCommandSM->ALEventEnable = true;

  EC_SyncMan *mbxStatusSM = new EC_SyncMan(MBX_STATUS_PHY_ADDR, MBX_STATUS_SIZE, EC_QUEUED);
  mbxStatusSM->ChannelEnable = true;

  (*pd)[0] = *commandSM;
  (*pd)[1] = *statusSM;
  (*pd)[2] = *mbxCommandSM;
  (*pd)[3] = *mbxStatusSM;
  if (isWG06) (*pd)[4] = *pressureSM;

  sh->set_pd_config(pd);

  return this;
}

int WG0X::initialize(Actuator *actuator, bool allow_unprogrammed)
{
  ros::node *node = ros::node::instance();

  unsigned int revision = sh_->get_revision();
  unsigned int major = (revision >> 8) & 0xff;
  unsigned int minor = revision & 0xff;

  printf("Device #%02d: WG0%d (%#08x) Firmware Revision %d.%02d, PCB Revision %c.%02d\n", sh_->get_ring_position(),
         sh_->get_product_code() == WG05::PRODUCT_CODE ? 5 : 6,
         sh_->get_product_code(), major, minor,
         'A' + ((revision >> 24) & 0xff) - 1, (revision >> 16) & 0xff);

  if (sh_->get_product_code() == WG05::PRODUCT_CODE)
  {
    if (major != 1 || minor < 2)
    {
      node->log(ros::FATAL, "Unsupported firmware revision %d.%02d\n", major, minor);
      return -1;
    }
  }
  else
  {
    if (major != 0 || minor < 4)
    {
      node->log(ros::FATAL, "Unsupported firmware revision %d.%02d\n", major, minor);
      return -1;
    }
  }

  if (readMailbox(sh_, WG0XConfigInfo::CONFIG_INFO_BASE_ADDR, &config_info_, sizeof(config_info_)) != 0)
  {
    node->log(ros::FATAL, "Unable to load configuration information");
    return -1;
  }
  printf("Device #%02d: Serial #: %05d\n", sh_->get_ring_position(), config_info_.device_serial_number_);

  if (readEeprom(sh_) < 0)
  {
    node->log(ros::FATAL, "Unable to read actuator info from EEPROM\n");
    return -1;
  }

  boost::crc_32_type crc32;
  crc32.process_bytes(&actuator_info_, sizeof(actuator_info_)-sizeof(actuator_info_.crc32_));
  if (actuator_info_.crc32_ == crc32.checksum())
  {
    actuator->name_ = actuator_info_.name_;

    printf("read eeprom:\n");
    printf("  revision: %d.%d\n", actuator_info_.major_, actuator_info_.minor_);
    printf("  id: %08x\n", actuator_info_.id_);
    printf("  name: %s\n", actuator_info_.name_);
    printf("  motor make: %s\n", actuator_info_.motor_make_);
    printf("  motor model: %s\n", actuator_info_.motor_model_);
    printf("  max current: %f\n", actuator_info_.max_current_);
    printf("  speed constant: %f\n", actuator_info_.speed_constant_);
    printf("  motor torque: %f\n", actuator_info_.motor_torque_constant_);
    printf("  pulses per revolution: %d\n", actuator_info_.pulses_per_revolution_);
    printf("  sign: %d\n", actuator_info_.sign_);
    printf("  crc32: %08x\n", actuator_info_.crc32_);
  }
  else if (allow_unprogrammed)
  {
    printf("WARNING: Device #%02d is not programmed\n", sh_->get_ring_position());
    actuator_info_.crc32_ = 0;
  }
  else
  {
    node->log(ros::FATAL, "Device #%02d: Invalid CRC32 in actuator_info_", sh_->get_ring_position());
    return -1;
  }

  return 0;
}

#define GET_ATTR(a) \
{ \
  TiXmlElement *c; \
  attr = elt->Attribute((a)); \
  if (!attr) { \
    c = elt->FirstChildElement((a)); \
    if (!c || !(attr = c->GetText())) { \
      node->log(ros::FATAL, "Actuator is missing the attribute "#a"\n"); \
    } \
  } \
}

void WG0X::initXml(TiXmlElement *elt)
{
  ros::node *node = ros::node::instance();
  printf("Overriding actuator: %s\n", actuator_info_.name_);

  const char *attr;
  GET_ATTR("name");
  strcpy(actuator_info_.name_, attr);

  GET_ATTR("motorTorqueConstant");
  actuator_info_.motor_torque_constant_ = atof(attr);

  GET_ATTR("pulsesPerRevolution");
  actuator_info_.pulses_per_revolution_ = atof(attr);

  GET_ATTR("sign");
  actuator_info_.sign_ = atoi(attr);

  GET_ATTR("maxCurrent");
  actuator_info_.max_current_ = atof(attr);
}

void WG0X::convertCommand(ActuatorCommand &command, unsigned char *buffer)
{
  WG0XCommand *c = (WG0XCommand *)(buffer + sizeof(WG0XStatus));

  memset(c, 0, sizeof(WG0XCommand));

  c->programmed_current_ = int(command.current_ / config_info_.nominal_current_scale_);
  c->mode_ = command.enable_ ? (MODE_ENABLE | MODE_CURRENT | MODE_SAFETY_RESET) : MODE_OFF;
  c->checksum_ = rotateRight8(computeChecksum(c, sizeof(WG0XCommand) - 1));
}

void WG0X::computeCurrent(ActuatorCommand &command)
{
  command.current_ = command.effort_ / actuator_info_.motor_torque_constant_ * actuator_info_.sign_;
}

void WG0X::truncateCurrent(ActuatorCommand &command)
{
  command.current_ = max(min(command.current_, actuator_info_.max_current_), -actuator_info_.max_current_);
}

void WG06::convertState(ActuatorState &state, unsigned char *current_buffer, unsigned char *last_buffer)
{
  WG06Pressure *p = (WG06Pressure *)(current_buffer + sizeof(WG0XCommand) + sizeof(WG0XStatus));

  if (p->timestamp_ != last_pressure_time_)
  {
    if (publisher_.trylock())
    {
      publisher_.msg_.set_data0_size(22);
      publisher_.msg_.set_data1_size(22);
      for (int i = 0; i < 22; ++i ) {
        publisher_.msg_.data0[i] = ((p->data0_[i] >> 8) & 0xff) | ((p->data0_[i] << 8) & 0xff00);
        publisher_.msg_.data1[i] = ((p->data1_[i] >> 8) & 0xff) | ((p->data1_[i] << 8) & 0xff00);
      }
      publisher_.unlockAndPublish();
    }
  }

  WG0X::convertState(state, current_buffer, last_buffer);
  last_pressure_time_ = p->timestamp_;
}

void WG0X::convertState(ActuatorState &state, unsigned char *this_buffer, unsigned char *prev_buffer)
{
  WG0XStatus *this_status, *prev_status;

  this_status = (WG0XStatus *)this_buffer;
  prev_status = (WG0XStatus *)prev_buffer;

  state.timestamp_ = this_status->timestamp_ / 1e+6;
  state.encoder_count_ = this_status->encoder_count_;
  state.position_ = double(this_status->encoder_count_) / actuator_info_.pulses_per_revolution_ * 2 * M_PI - state.zero_offset_;
  state.encoder_velocity_ = double(int(this_status->encoder_count_ - prev_status->encoder_count_))
      / (this_status->timestamp_ - prev_status->timestamp_) * 1e+6;
  state.velocity_ = state.encoder_velocity_ / actuator_info_.pulses_per_revolution_ * 2 * M_PI;
  state.calibration_reading_ = this_status->calibration_reading_ & LIMIT_SENSOR_0_STATE;
  state.last_calibration_high_transition_ = double(this_status->last_calibration_high_transition_) / actuator_info_.pulses_per_revolution_ * 2 * M_PI;
  state.last_calibration_low_transition_ = double(this_status->last_calibration_low_transition_) / actuator_info_.pulses_per_revolution_ * 2 * M_PI;
  state.is_enabled_ = this_status->mode_ != MODE_OFF;
  state.run_stop_hit_ = (this_status->mode_ & MODE_UNDERVOLTAGE) != 0;

  state.last_commanded_current_ = this_status->programmed_current_ * config_info_.nominal_current_scale_;
  state.last_measured_current_ = this_status->measured_current_ * config_info_.nominal_current_scale_;

  state.last_commanded_effort_ = this_status->programmed_current_ * config_info_.nominal_current_scale_ * actuator_info_.motor_torque_constant_ * actuator_info_.sign_;
  state.last_measured_effort_ = this_status->measured_current_ * config_info_.nominal_current_scale_ * actuator_info_.motor_torque_constant_ * actuator_info_.sign_;

  state.num_encoder_errors_ = this_status->num_encoder_errors_;
  state.num_communication_errors_ = 0; // TODO: communication errors are no longer reported in the process data

  state.motor_voltage_ = this_status->motor_voltage_ * config_info_.nominal_voltage_scale_;
}

void WG0X::verifyState(ActuatorState &state, unsigned char *buffer)
{
#if 0
  if (voltage_offset_ == 0)
  {
    voltage_offset_ = state.motor_voltage_;
    printf("setting voltage_offset to %f\n", voltage_offset_);
  }
  WG0XStatus *status = (WG0XStatus *)buffer;
  double volt_est = 30.6 * double(status->programmed_pwm_value_) / 0x4000;
  double volt_est2 = status->supply_voltage_ * config_info_.nominal_voltage_scale_ * double(status->programmed_pwm_value_) / 0x4000;
  double backemf = 1.0 / (actuator_info_.speed_constant_ * 2 * M_PI * 1.0/60);
  double expected_voltage = state.last_measured_current_ * actuator_info_.resistance_ + state.velocity_ * actuator_info_.sign_ * backemf;
  double error = fabs(expected_voltage - (state.motor_voltage_ - voltage_offset_));
  double error2 = fabs(expected_voltage - volt_est2);

  printf("%20s: est: %f, est2: %f, supply: %f, err: %f\n", actuator_info_.name_, volt_est, volt_est2,  status->supply_voltage_ * config_info_.nominal_voltage_scale_, error2);
  printf("%20s: exp: %10f, real: %10f, err: %10f, d: %f\n", actuator_info_.name_, expected_voltage, state.motor_voltage_ - voltage_offset_, error, fabs(error2-error));

  WG0XStatus status;

  memcpy(&status, buffer, sizeof(status));

  // Check board shutdown status
  // Report temperature shutdown, UV lockout, etc.
  // Report lots of diagnostics information

  // Check back-EMF consistency
  expected_voltage = status.measured_current_ * resistance + motor_velocity * backemf_constant_;
  voltage_error = fabs(expected_voltage - status.motor_voltage_); //Scaled to volts
  if(voltage_error> 5)
  { //Arbitary threshold
    //Something is wrong with the encoder, the motor, or the motor board
    //Disable motors
    //Try to diagnose further
    //motor_velocity == 0 -> encoder failure likely
    //measured_current_ ~= 0 -> motor open-circuit likely
    //motor_voltage_ ~= 0 -> motor short-circuit likely
    //else -> current-sense failure likely
    //Print error messages
  }

  //Check current-loop performance
  double current_error = status.measured_current_ - status.last_commanded_current;
  if(current_error> threshold);
  //complain and shut down

  //TODO: filter errors so that one-frame spikes don't shut down the system.
#endif
}

int WG0X::readData(EtherCAT_SlaveHandler *sh, EC_UINT address, void* buffer, EC_UINT length)
{
  unsigned char *p = (unsigned char *)buffer;
  EtherCAT_DataLinkLayer *dll = EtherCAT_DataLinkLayer::instance();
  EC_Logic *logic = EC_Logic::instance();

  // Build read telegram, use slave position
  APRD_Telegram status(logic->get_idx(), // Index
                       -sh->get_ring_position(), // Slave position on ethercat chain (auto increment address)
                       address, // ESC physical memory address (start address)
                       logic->get_wkc(), // Working counter
                       length, // Data Length,
                       p); // Buffer to put read result into

  // Put read telegram in ethercat/ethernet frame
  EC_Ethernet_Frame frame(&status);

  // Send/Recv data from slave
  if (!dll->txandrx(&frame))
  {
    status.set_wkc(logic->get_wkc());
    status.set_idx(logic->get_idx());
    if (!dll->txandrx(&frame))
    {
      return -1;
    }
  }

  // In some cases (clearing status mailbox) this is expected to occur
  if (status.get_wkc() != 1)
  {
    return -2;
  }

  return 0;
}

// Writes <length> amount of data from ethercat slave <sh_hub> from physical address <address> to <buffer>
int WG0X::writeData(EtherCAT_SlaveHandler *sh, EC_UINT address, void const* buffer, EC_UINT length)
{
  unsigned char *p = (unsigned char *)buffer;
  EtherCAT_DataLinkLayer *m_dll_instance = EtherCAT_DataLinkLayer::instance();
  EC_Logic *m_logic_instance = EC_Logic::instance();

  // Build write telegram, use slave position
  APWR_Telegram command(m_logic_instance->get_idx(), // Index
                        -sh->get_ring_position(), // Slave position on ethercat chain (auto increment address) (
                        address, // ESC physical memory address (start address)
                        m_logic_instance->get_wkc(), // Working counter
                        length, // Data Length,
                        p); // Buffer to put read result into

  // Put read telegram in ethercat/ethernet frame
  EC_Ethernet_Frame frame(&command);

  // Send/Recv data from slave
  if (!m_dll_instance->txandrx(&frame))
  {
    command.set_wkc(m_logic_instance->get_wkc());
    command.set_idx(m_logic_instance->get_idx());
    if (!m_dll_instance->txandrx(&frame))
    {
      return -1;
    }
  }

  if (command.get_wkc() != 1)
  {
    return -2;
  }

  return 0;
}

int WG0X::sendSpiCommand(EtherCAT_SlaveHandler *sh, WG0XSpiEepromCmd const * cmd)
{
  // Send command
  if (writeMailbox(sh, WG0XSpiEepromCmd::SPI_COMMAND_ADDR, cmd, sizeof(*cmd)))
  {
    fprintf(stderr, "ERROR WRITING EEPROM COMMAND\n");
    return -1;
  }

  for (int tries = 0; tries < 10; ++tries)
  {
    WG0XSpiEepromCmd stat;
    if (readMailbox(sh, WG0XSpiEepromCmd::SPI_COMMAND_ADDR, &stat, sizeof(stat)))
    {
      fprintf(stderr, "ERROR READING EEPROM BUSY STATUS\n");
      return -1;
    }

    if (stat.operation_ != cmd->operation_)
    {
      fprintf(stderr, "READBACK OF OPERATION INVALID : got 0x%X, expected 0x%X\n", stat.operation_, cmd->operation_);
      return -1;
    }

    // Keep looping while SPI command is running
    if (!stat.busy_)
    {
      return 0;
    }

    fprintf(stderr, "eeprom busy reading again, waiting...\n");
    usleep(100);
  }

  fprintf(stderr, "ERROR : EEPROM READING BUSY AFTER 10 TRIES\n");
  return -1;
}

int WG0X::readEeprom(EtherCAT_SlaveHandler *sh)
{
  assert(sizeof(actuator_info_) == 264);
  WG0XSpiEepromCmd cmd;
  cmd.build_read(ACTUATOR_INFO_PAGE);
  if (sendSpiCommand(sh, &cmd)) {
    fprintf(stderr, "ERROR SENDING SPI EEPROM READ COMMAND\n");
    return -1;
  }
  // Read buffered data in multiple chunks
  if (readMailbox(sh, WG0XSpiEepromCmd::SPI_BUFFER_ADDR, &actuator_info_, sizeof(actuator_info_))) {
    fprintf(stderr, "ERROR READING BUFFERED EEPROM PAGE DATA\n");
    return -1;
  }

  return 0;

}

void WG0X::program(WG0XActuatorInfo *info)
{

  writeMailbox(sh_, WG0XSpiEepromCmd::SPI_BUFFER_ADDR, info, sizeof(WG0XActuatorInfo));
  WG0XSpiEepromCmd cmd;
  cmd.build_write(ACTUATOR_INFO_PAGE);
  if (sendSpiCommand(sh_, &cmd)) {
    fprintf(stderr, "ERROR SENDING SPI EEPROM WRITE COMMAND\n");
  }

  char data[2];
  memset(data, 0, sizeof(data));
  data[0] = 0xD7;

  if (writeMailbox(sh_, WG0XSpiEepromCmd::SPI_BUFFER_ADDR, data, sizeof(data))) {
    fprintf(stderr, "ERROR WRITING EEPROM COMMAND BUFFER\n");
  }


  { // Start arbitrary command
    WG0XSpiEepromCmd cmd;
    cmd.build_arbitrary(sizeof(data));
    if (sendSpiCommand(sh_, &cmd)) {
      printf("reading eeprom status failed");
    }
  }


  if (readMailbox(sh_, WG0XSpiEepromCmd::SPI_BUFFER_ADDR, data, sizeof(data))) {
    fprintf(stderr, "ERROR READING EEPROM COMMAND BUFFER\n");
  }
  printf("data[1] = %08x\n", data[1]);
}

int WG0X::readMailbox(EtherCAT_SlaveHandler *sh, int address, void *data, EC_UINT length)
{
  // first (re)read current status mailbox data to prevent issues with
  // the status mailbox being full (and unread) from last command
  WG0XMbxCmd stat;
  int result = readData(sh, MBX_STATUS_PHY_ADDR, &stat, sizeof(stat));

  if ((result != 0) && (result != -2))
  {
    fprintf(stderr, "CLEARING STATUS MBX FAILED result = %d\n", result);
    return -1;
  }

  // Build mailbox message and send read command
  WG0XMbxCmd cmd;
  cmd.build(address, length, false /*read*/, data);
  int tries;
  for (tries = 0; tries < 10; ++tries)
  {
    int result = writeData(sh, MBX_COMMAND_PHY_ADDR, &cmd, sizeof(cmd));
    if (result == -2)
    {
      // FPGA hasn't written responded with status data, wait a
      // tiny bit and try again.
      usleep(100); // 1/10th of a millisecond
      continue;
    }
    else if (result == 0)
    {
      // Successful read of status data
      break;
    }
    else
    {
      fprintf(stderr, "WRITING COMMAND MBX FAILED\n");
      return -1;
    }
  }
  if (tries >= 10)
  {
    fprintf(stderr, "do_mailbox_write : Too many tries writing mailbox\n");
    return -1;
  }

  for (tries = 0; tries < 10; ++tries)
  {
    int result = readData(sh, MBX_STATUS_PHY_ADDR, &stat, sizeof(stat));
    if (result == -2)
    {
      // FPGA hasn't written responded with status data, wait a
      // tiny bit and try again.
      usleep(100); // 1/10th of a millisecond
      continue;
    }
    else if (result == 0)
    {
      // Successfull read of status data
      break;
    }
    else
    {
      fprintf(stderr, "READING MBX STATUS FAILED\n");
      return -1;
    }
  }
  if (tries >= 10)
  {
    fprintf(stderr, "do_mailbox_read : Too many tries reading mailbox\n");
    return -1;
  }

  if (computeChecksum(&stat, length + 1) != 0)
  {
    fprintf(stderr, "CHECKSUM ERROR READING MBX DATA\n");
    return -1;
  }
  memcpy(data, &stat, length);
  return 0;
}

// Write <length> byte of <data> to <address> on FPGA local bus using the ethercat mailbox for communication
// Returns 0 for success and non-zero for failure.
int WG0X::writeMailbox(EtherCAT_SlaveHandler *sh, int address, void const *data, EC_UINT length)
{
  // Build mailbox message and write command
  {
    WG0XMbxCmd cmd;
    cmd.build(address, length, true /*write*/, data);
    int tries;
    for (tries = 0; tries < 10; ++tries)
    {
      int result = writeData(sh, MBX_COMMAND_PHY_ADDR, &cmd, sizeof(cmd));
      if (result == -2)
      {
        // FPGA hasn't written responded with status data, wait a
        // tiny bit and try again.
        usleep(100); // 1/10th of a millisecond
        continue;
      }
      else if (result == 0)
      {
        // Successfull read of status data
        return 0;
      }
      else
      {
        fprintf(stderr, "WRITING COMMAND MBX FAILED\n");
        return -1;
      }
    }
    if (tries >= 10)
    {
      fprintf(stderr, "do_mailbox_write : Too many tries writing mailbox\n");
      return -1;
    }
  }

  return 0;
}

#define ADD_STRING(lab, val) \
  s.label = (lab); \
  s.value = (val); \
  strings.push_back(s)
#define ADD_VALUE(lab, val) \
  v.label = (lab); \
  v.value = (val); \
  values.push_back(v)
void WG0X::diagnostics(robot_msgs::DiagnosticStatus &d)
{
  vector<robot_msgs::DiagnosticString> strings;
  vector<robot_msgs::DiagnosticValue> values;
  robot_msgs::DiagnosticValue v;
  robot_msgs::DiagnosticString s;

  stringstream str;
  str << "EtherCAT Device #" << sh_->get_ring_position();
  d.name = str.str();
  d.message = "OK";
  d.level = 0;

  str.str("");
  str << (sh_->get_product_code() == WG05::PRODUCT_CODE ? "WG05 (" : "WG06 (") << sh_->get_product_code() << ")";
  ADD_STRING("Product code", str.str());
  ADD_STRING("Name", actuator_info_.name_);
  ADD_STRING("Robot", actuator_info_.robot_name_);
  str.str("");
  str << actuator_info_.motor_make_ << " " << actuator_info_.motor_model_;
  ADD_STRING("Motor", str.str());

  str.str("");
  str << setfill('0') << config_info_.product_id_ / 100000 << "-" << setw(5) << config_info_.product_id_ % 100000 << "-" << setw(5) << config_info_.device_serial_number_;
  ADD_STRING("Serial Number", str.str());
  ADD_VALUE("Nominal Current Scale", config_info_.nominal_current_scale_);
  ADD_VALUE("Nominal Voltage Scale", config_info_.nominal_voltage_scale_);
  ADD_VALUE("Max Current", actuator_info_.max_current_);
  ADD_VALUE("Speed Constant", actuator_info_.speed_constant_);
  ADD_VALUE("Resistance", actuator_info_.resistance_);
  ADD_VALUE("Motor Torque Constant", actuator_info_.motor_torque_constant_);
  ADD_VALUE("Pulses Per Revolution", actuator_info_.pulses_per_revolution_);
  ADD_VALUE("Sign", actuator_info_.sign_);

  d.set_strings_vec(strings);
  d.set_values_vec(values);
}
