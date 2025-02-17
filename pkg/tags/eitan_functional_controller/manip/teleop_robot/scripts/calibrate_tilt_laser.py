#!/usr/bin/python
# Software License Agreement (BSD License)
#
# Copyright (c) 2008, Willow Garage, Inc.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
#  * Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#  * Redistributions in binary form must reproduce the above
#    copyright notice, this list of conditions and the following
#    disclaimer in the documentation and/or other materials provided
#    with the distribution.
#  * Neither the name of the Willow Garage nor the names of its
#    contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
# FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
# COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
# ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#

# Written by Timothy Hunter <tjhunter@willowgarage.com> 2008

import rostools
import copy

# Loads interface with the robot.
rostools.update_path('teleop_robot')
import rospy
from mechanism_control.srv import *
from robot_mechanism_controllers.srv import *

def slurp(filename):
    f = open(filename)
    stuff = f.read()
    f.close()
    return stuff

rospy.wait_for_service('spawn_controller')
spawn_controller = rospy.ServiceProxy('spawn_controller', SpawnController)
kill_controller = rospy.ServiceProxy('kill_controller', KillController)


def calibrate_optically(config):
    resp = spawn_controller(config)
    if len(resp.ok) != 1 or not resp.ok[0]:
        print "FAIL"
        return
    name = resp.name[0]
    try:
        do_calibration = rospy.ServiceProxy("/%s/calibrate" % name, CalibrateJoint)
        do_calibration()
    finally:
        kill_controller(name)
    print "Calibrated"

def calibrate_manually(config):
    resp = spawn_controller(config)
    if len(resp.ok) != 1 or not resp.ok[0]:
        print "FAIL"
        return
    name = resp.name[0]
    begin = rospy.ServiceProxy("/%s/begin_manual_calibration" % name, CalibrateJoint)
    end = rospy.ServiceProxy("/%s/end_manual_calibration" % name, CalibrateJoint)
    begin()
    print "Move the joint to the limits, and then hit enter"
    raw_input()
    end()
    kill_controller(name)
    print "Calibrated manually"

# Hits the joint stops
def calibrate_blindly(config):
    resp = spawn_controller(config)
    if len(resp.ok) != 1 or not resp.ok[0]:
        print "FAIL"
        return
    name = resp.name[0]
    do_calibration = rospy.ServiceProxy("/%s/calibrate" % name, CalibrateJoint)
    do_calibration()
    kill_controller(name)
    print "Calibrated"

#calibrate_optically('''
#<controller name="laser_tilt_calibration" topic="laser_tilt_calibration" type="JointCalibrationControllerNode">
#  <param joint_name="tilt_laser_mount_joint" actuator_name="tilt_laser_motor" transmission_name="tilt_laser_mount_trans" velocity="0.2"/>
#  <controller name="laser_controller" topic="laser_controller" type="JointVelocityController">
#    <filter smoothing_factor="0.1" />
#    <joint name="tilt_laser_mount_joint">
#      <pid p="1" i="0" d="0.00" iClamp="1" />
#    </joint>
#  </controller>
#</controller>
#''')

calibrate_optically('''
<controller name="cal_laser_tilt" topic="cal_laser_tilt" type="JointCalibrationControllerNode">
  <calibrate joint="tilt_laser_mount_joint"
             actuator="tilt_laser_motor"
             transmission="tilt_laser_mount_trans"
             velocity="-0.6" />
  <pid p=".25" i="0.1" d="0" iClamp="1.0" />
</controller>
''')
