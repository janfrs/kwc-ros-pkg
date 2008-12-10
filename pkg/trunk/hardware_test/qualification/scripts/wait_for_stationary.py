#!/usr/bin/env python
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

PKG = "qualification"
NAME = "waiter"

ISDONE = False

import rostools; rostools.update_path(PKG)

import sys
import time
import rospy
from robot_msgs.msg import *

from optparse import OptionParser


def callback(msg, name):
##    print "Looking for ", name
    names = name.split()
    total = len(names)
    for joint in msg.joint_states:
        if joint.name in names:
            if abs(joint.velocity) < 0.1:
##                print "DONE"
                total = total -1
    if total == 0:        
        global ISDONE
        ISDONE = True



if __name__ == '__main__':
    parser = OptionParser(usage="usage: %prog [options]", prog='viewFrames.py')
    parser.add_option("--wait", metavar="wait period",
                      type="float", help="Seconds to wait",
                      dest="wait")
    parser.add_option("--joint", metavar="Joint Name",
                      type="string", help="Joint to observe",
                      dest="joint")
    options, args = parser.parse_args()

    if not options.joint:
        print "Please enter a joint name as an argument"
        exit(-1)
    else:
        if options.wait:
            time.sleep(options.wait)
        rospy.Subscriber("/mechanism_state", MechanismState, callback, options.joint)
        rospy.init_node(NAME, anonymous=True)
        while not ISDONE:
##            print ISDONE
            time.sleep(0.5)
