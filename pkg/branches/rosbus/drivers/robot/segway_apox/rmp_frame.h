/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2003  John Sweeney & Brian Gerkey
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
#ifndef RMP_FRAME_H
#define RMP_FRAME_H
#include <sys/types.h>
#include <stdint.h>

//#include "apox_usbcan.h"
#include "rxdata.h"

#define RMP_CAN_ID_SHUTDOWN	0x0412
#define RMP_CAN_ID_COMMAND	0x0413
#define RMP_CAN_ID_MSG1		0x0400
#define RMP_CAN_ID_MSG2		0x0401
#define RMP_CAN_ID_MSG3		0x0402
#define RMP_CAN_ID_MSG4		0x0403
#define RMP_CAN_ID_MSG5		0x0404

#define RMP_CAN_CMD_NONE		0
#define RMP_CAN_CMD_MAX_VEL		10
#define RMP_CAN_CMD_MAX_ACCL		11
#define RMP_CAN_CMD_MAX_TURN		12
#define RMP_CAN_CMD_GAIN_SCHED		13
#define RMP_CAN_CMD_CURR_LIMIT		14
#define RMP_CAN_CMD_RST_INT		50

#define RMP_CAN_RST_RIGHT		0x01
#define RMP_CAN_RST_LEFT		0x02
#define RMP_CAN_RST_YAW			0x04
#define RMP_CAN_RST_FOREAFT		0x08
#define RMP_CAN_RST_ALL			(RMP_CAN_RST_RIGHT | \
					 RMP_CAN_RST_LEFT | \
					 RMP_CAN_RST_YAW | \
					 RMP_CAN_RST_FOREAFT)

#define RMP_COUNT_PER_M			33215
#define RMP_COUNT_PER_DEG		7.8
#define RMP_COUNT_PER_M_PER_S		332
#define RMP_COUNT_PER_DEG_PER_S		7.8
#define RMP_COUNT_PER_MM_PER_S		0.32882963
#define RMP_COUNT_PER_DEG_PER_SS	7.8
#define RMP_COUNT_PER_REV               112644

#define RMP_MAX_TRANS_VEL_MM_S		3576
#define RMP_MAX_ROT_VEL_DEG_S		18	// from rmi_demo: 1300*0.013805056
#define RMP_MAX_TRANS_VEL_COUNT		1176
#define RMP_MAX_ROT_VEL_COUNT		1024

#define RMP_GEOM_WHEEL_SEP 0.54

// this holds all the RMP data it gives us
class rmp_frame_t
{
  public:
    int16_t pitch;
    int16_t pitch_dot;
    int16_t roll;
    int16_t roll_dot;
    uint32_t yaw;
    int16_t yaw_dot;
    uint32_t left;
    int16_t left_dot;
    uint32_t right;
    int16_t right_dot;
    uint32_t foreaft;

    uint16_t frames;
    uint16_t battery;
    uint8_t  ready;

    rmp_frame_t() : ready(0) {}

    // Adds a new packet to this frame
//    void AddPacket(const RxData *pkt);
    void AddPacket(unsigned id, const unsigned char *data);

    // Is this frame ready (i.e., did we get all 5 messages)?
    bool IsReady() { return ready == 0x1F; }

		uint16_t GetSlot(int s, const uint8_t *data)
		{
			return (uint16_t)((data[s*2] << 8) | (data[s*2+1]));
		}

};



/* Takes a CAN packet from the RMP and parses it into a
 * rmp_frame_t struct.  sets the ready bitfield 
 * depending on which CAN packet we have.  when
 * ready == 0x1F, then we have gotten 5 packets, so everything
 * is filled in.
 *
 * returns: 
 */
inline void
rmp_frame_t::AddPacket(unsigned id, const unsigned char *data)// const RxData *pkt)
//rmp_frame_t::AddPacket(const RxData *pkt)
{
  bool known = true;

  switch(id) 
  {
    case RMP_CAN_ID_MSG1:
      battery = GetSlot(2,data);
      //battery = *(unsigned short *)(pkt->data + 2);
      break;

    case RMP_CAN_ID_MSG2:
			pitch = GetSlot(0,data);
      pitch_dot = GetSlot(1,data);
      roll = GetSlot(2,data);
      roll_dot = GetSlot(3,data);
      break;
    case RMP_CAN_ID_MSG3:
      left_dot = (int16_t) GetSlot(0, data);
      right_dot = (int16_t) GetSlot(1, data);
      yaw_dot = (int16_t) GetSlot(2, data);
      frames = GetSlot(3, data);
      break;

    case RMP_CAN_ID_MSG4:
      left = (uint32_t)(((uint32_t)GetSlot(1,data) << 16) | 
                        (uint32_t)GetSlot(0,data));
      right = (uint32_t)(((uint32_t)GetSlot(3,data) << 16) | 
                         (uint32_t)GetSlot(2,data));
      break;

    case RMP_CAN_ID_MSG5:
      foreaft = (uint32_t)(((uint32_t)GetSlot(1,data) << 16) | 
                           (uint32_t)GetSlot(0,data));
      yaw = (uint32_t)(((uint32_t)GetSlot(3,data) << 16) | 
                       (uint32_t)GetSlot(2,data));
      break;
    default:
      known = false;
      break;
  }

  // now set the ready flags
  if(known) 
    ready |= (1 << (id & 0xF));
}

#endif
