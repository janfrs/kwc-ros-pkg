#!/usr/bin/env python

import rostools
rostools.update_path('mechanism_control')

import sys, traceback, logging, rospy
from mechanism_control.msg import MechanismState

NAME = 'joint_listener'

def callback(data):
    print rospy.get_caller_id(), "I heard [%s], %d"%(data.time, len(data.joint_states))
    for j in data.joint_states:
      print "  joint: %s" % j.name
      print "    position:         %.4f" % j.position
      print "    velocity:         %.4f" % j.velocity
      print "    applied_effort:   %.4f" % j.applied_effort
      print "    commanded_effort: %.4f" % j.commanded_effort
    for a in data.actuator_states:
      print "  actuator: %s" % a.name
      print "    encoder_count:         %d" % a.encoder_count
      print "    position:         %d" % a.position
      print "    timestamp: %.4f" % a.timestamp
      print "    encoder_velocity: %.4f" % a.encoder_velocity
      print "    velocity: %.4f" % a.velocity
      print "    calibration_reading: %d" % a.calibration_reading
      print "    last_calibration_high_transition: %d" % a.last_calibration_high_transition
      print "    last_calibration_low_transition: %d" % a.last_calibration_low_transition
      print "    is_enabled: %d" % a.is_enabled
      print "    run_stop_hit: %d" % a.run_stop_hit
      print "    last_requested_current: %.4f" % a.last_requested_current
      print "    last_commanded_current: %.4f" % a.last_commanded_current
      print "    last_measured_current: %.4f" % a.last_measured_current
      print "    last_requested_effort: %.4f" % a.last_requested_effort
      print "    last_commanded_effort: %.4f" % a.last_commanded_effort
      print "    last_measured_effort: %.4f" % a.last_measured_effort
      print "    motor_voltage: %.4f" % a.motor_voltage
      print "    num_encoder_errors: %d" % a.num_encoder_errors

   

def listener_with_user_data():
    rospy.TopicSub("/mechanism_state", MechanismState, callback)
    rospy.ready(NAME, anonymous=True)
    rospy.spin()

if __name__ == '__main__':
    try:
        listener_with_user_data()
    except KeyboardInterrupt, e:
        pass
    print "exiting"
