#! /usr/bin/env python
# Copyright (c) 2008, Willow Garage, Inc.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in the
#       documentation and/or other materials provided with the distribution.
#     * Neither the name of the Willow Garage, Inc. nor the names of its
#       contributors may be used to endorse or promote products derived from
#       this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

# Author: Stuart Glaser

import rostools; rostools.update_path('teleop_spacenav')
import rospy, sys
from robot_mechanism_controllers.srv import GetVector, SetVectorCommand
from std_msgs.msg import Vector3

def print_usage(code = 0):
    print sys.argv[0], '<cartesian position controller topic>'
    sys.exit(code)


if __name__ == '__main__':
    if len(sys.argv) < 2:
        print_usage(1)

    topic = sys.argv[1]
    get_position = rospy.ServiceProxy(topic + '/get_actual', GetVector)
    set_command = rospy.ServiceProxy(topic + '/set_command', SetVectorCommand)

    pos = get_position().v

    i = 0
    def spacenav_updated(msg):
        global i
        FACTOR = 0.00001
        pos.x += FACTOR * msg.x
        pos.y += FACTOR * msg.y
        pos.z += FACTOR * msg.z
        i += 1
        if i % 10 != 0:
          return
        print "Position: (%.2f, %.2f, %.2f)" % (pos.x, pos.y, pos.z)
        set_command(pos.x, pos.y, pos.z)

    rospy.subscribe_topic("/spacenav/offset", Vector3, spacenav_updated)
    rospy.init_node('spacenav_teleop')
    rospy.spin()

'''
TODO:
reset to current position occasionally
'''
