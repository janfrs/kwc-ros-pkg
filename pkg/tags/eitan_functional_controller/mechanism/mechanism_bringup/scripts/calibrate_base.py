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

# Author: Stuart Glaser

import rostools
import copy
import threading

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
    try:
        do_calibration = rospy.ServiceProxy("/%s/calibrate" % name, CalibrateJoint)
        do_calibration()
    finally:
        kill_controller(name)
    print "Calibrated"


class FunThread(threading.Thread):
    def __init__(self, fun, *args):
        self.fun = fun
        self.start()

    def run():
        self.fun(*args)


template = '''
<controller name="cal_caster_SUFFIX" topic="cal_caster_SUFFIX" type="JointCalibrationControllerNode">
  <calibrate joint="caster_SUFFIX_joint"
             actuator="caster_SUFFIX_motor"
             transmission="caster_SUFFIX_trans"
             velocity="1.0" />
  <pid p="30" i="2.0" d="0" iClamp="5.0" />
</controller>
'''

calibrate_optically(template.replace('SUFFIX', 'front_left'))
calibrate_optically(template.replace('SUFFIX', 'front_right'))
calibrate_optically(template.replace('SUFFIX', 'rear_left'))
calibrate_optically(template.replace('SUFFIX', 'rear_right'))

"""
I have left the following calls here in case the above lines don't actually work

calibrate_optically('''
<controller name="cal_caster_front_left" topic="cal_caster_front_left" type="JointCalibrationControllerNode">
  <calibrate joint="caster_front_left_joint"
             actuator="caster_front_left_motor"
             transmission="caster_front_left_trans"
             velocity="1.0" />
  <pid p="14" i="0.5" d="0" iClamp="1.0" />
</controller>
''')

calibrate_optically('''
<controller name="cal_caster_front_right" topic="cal_caster_front_right" type="JointCalibrationControllerNode">
  <calibrate joint="caster_front_right_joint"
             actuator="caster_front_right_motor"
             transmission="caster_front_right_trans"
             velocity="1.0" />
  <pid p="14" i="0.5" d="0" iClamp="1.0" />
</controller>
''')

calibrate_optically('''
<controller name="cal_caster_rear_right" topic="cal_caster_rear_right" type="JointCalibrationControllerNode">
  <calibrate joint="caster_rear_right_joint"
             actuator="caster_rear_right_motor"
             transmission="caster_rear_right_trans"
             velocity="1.0" />
  <pid p="100" i="2.0" d="0" iClamp="5.0" />
</controller>
''')

calibrate_optically('''
<controller name="cal_caster_rear_left" topic="cal_caster_rear_left" type="JointCalibrationControllerNode">
  <calibrate joint="caster_rear_left_joint"
             actuator="caster_rear_left_motor"
             transmission="caster_rear_left_trans"
             velocity="1.0" />
  <pid p="200" i="2.0" d="0" iClamp="20.0" />
</controller>
''')
"""
