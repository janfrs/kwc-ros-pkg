#!/usr/bin/env python

import rostools
rostools.update_path('robot_mechanism_controllers')

import rospy, sys
from robot_mechanism_controllers.srv import *
from robot_srvs.srv import *
from std_msgs.msg import *

def list_controllers():
    rospy.wait_for_service('list_controllers')
    s = rospy.ServiceProxy('list_controllers', ListControllers)
    resp = s.call(ListControllersRequest())
    for t in resp.controllers:
        print t

def set_controller(controller, command):
    rospy.wait_for_service(controller + '/set_command')
    s = rospy.ServiceProxy(controller + '/set_command', SetCommand)
    resp = s.call(SetCommandRequest(command))
    print resp.command

def set_controller_vector(controller, command):
    rospy.wait_for_service(controller + '/set_command')
    s = rospy.ServiceProxy(controller + '/set_command', SetVector)
    resp = s(Vector3(*command))

def get_controller(controller):
    rospy.wait_for_service(controller + '/get_command')
    s = rospy.ServiceProxy(controller + '/get_command', JointCmd)
    resp = s.call(JointCmdRequest())
    print str(resp.name) + ": " + str(resp.positions)+ ": " + str(resp.velocity)

def get_controller_vector(controller):
    rospy.wait_for_service(controller + '/get_actual')
    s = rospy.ServiceProxy(controller + '/get_actual', GetVector)
    resp = s()
    print "(%f, %f, %f)" % (resp.v.x, resp.v.y, resp.v.z)

def set_position(controller, command):
    rospy.wait_for_service(controller + '/set_position')
    s = rospy.ServiceProxy(controller + '/set_position', SetPosition)
    resp = s.call(SetPositionRequest(command))
    print resp.command

def get_position(controller):
    rospy.wait_for_service(controller + '/get_position')
    s = rospy.ServiceProxy(controller + '/get_position', GetPosition)
    resp = s.call(GetPositionRequest())
    print str(resp.time) + ": " + str(resp.command)


