
#!/usr/bin/env python
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

PKG = 'pr2_power_board'

import rostools
rostools.update_path(PKG)

import sys
import rospy
from robot_msgs.msg import *
from pr2_power_board.srv import *

import wx
from wx import xrc

import threading, time

class PowerBoardPanel(wx.Panel):
    def __init__(self, parent):
        wx.Panel.__init__(self, parent, wx.ID_ANY)
        
        self._mutex = threading.Lock()
        
        xrc_path = rostools.packspec.get_pkg_dir(PKG) + '/pr2_power_board.xrc'
        
        self._xrc = xrc.XmlResource(xrc_path)
        self._real_panel = self._xrc.LoadPanel(self, 'PowerBoardPanel')
        sizer = wx.BoxSizer(wx.VERTICAL)
        sizer.Add(self._real_panel, 1, wx.EXPAND)
        self.SetSizer(sizer)

        self._real_panel.Bind(wx.EVT_BUTTON, self.EnableCB0, id=xrc.XRCID('m_button1'))
        self._real_panel.Bind(wx.EVT_BUTTON, self.EnableCB1, id=xrc.XRCID('m_button11'))
        self._real_panel.Bind(wx.EVT_BUTTON, self.EnableCB2, id=xrc.XRCID('m_button12'))

        self._real_panel.Bind(wx.EVT_BUTTON, self.StandbyCB0, id=xrc.XRCID('m_button2'))
        self._real_panel.Bind(wx.EVT_BUTTON, self.StandbyCB1, id=xrc.XRCID('m_button21'))
        self._real_panel.Bind(wx.EVT_BUTTON, self.StandbyCB2, id=xrc.XRCID('m_button22'))

        self._real_panel.Bind(wx.EVT_BUTTON, self.ResetCB0, id=xrc.XRCID('m_button3'))
        self._real_panel.Bind(wx.EVT_BUTTON, self.ResetCB1, id=xrc.XRCID('m_button31'))
        self._real_panel.Bind(wx.EVT_BUTTON, self.ResetCB2, id=xrc.XRCID('m_button32'))

        self._real_panel.Bind(wx.EVT_BUTTON, self.DisableCB0, id=xrc.XRCID('cb0_disable'))
        self._real_panel.Bind(wx.EVT_BUTTON, self.DisableCB1, id=xrc.XRCID('cb1_disable'))
        self._real_panel.Bind(wx.EVT_BUTTON, self.DisableCB2, id=xrc.XRCID('cb2_disable'))

        self._real_panel.Bind(wx.EVT_BUTTON, self.ResetCurrent, id=xrc.XRCID('button_reset_current'))
        self._real_panel.Bind(wx.EVT_BUTTON, self.ResetTransitions, id=xrc.XRCID('button_reset_transitions'))
        rospy.Subscriber("/diagnostics", DiagnosticMessage, self.diagnostics_callback)
        
        self.power_control = rospy.ServiceProxy('power_board_control', PowerBoardCommand)


        self.voltages = [0,0,0]
        self.breaker_state = ["", "", ""]
        self.estop_wireless_status = "uninitialized"
        self.estop_button_status = "uninitialized"

        self.breaker0_status = xrc.XRCCTRL(self._real_panel, 'm_textCtrl1')
        self.breaker1_status = xrc.XRCCTRL(self._real_panel, 'm_textCtrl11')
        self.breaker2_status = xrc.XRCCTRL(self._real_panel, 'm_textCtrl12')


        self.estop_status = xrc.XRCCTRL(self._real_panel, 'm_textCtrl9')

        self.breaker0_status.SetEditable(False)
        self.breaker1_status.SetEditable(False)
        self.breaker2_status.SetEditable(False)
        self.estop_status.SetEditable(False)

        self.breaker0_status.SetEditable(False)
        self.breaker1_status.SetEditable(False)
        self.breaker2_status.SetEditable(False)
        self.estop_status.SetEditable(False)

#fixme        self.textboxes = [xrc.XRCCTRL(self._xrc, 'm_textCtrl1'), 0, 0]
#        self.textboxes[0].value = "hi"

        self._messages = []
        
    def diagnostics_callback(self, message):
        self._mutex.acquire()
        
        self._messages.append(message)
        
        self._mutex.release()
        
        wx.CallAfter(self.new_message)
        
    def new_message(self):


        self._mutex.acquire()
        

        for message in self._messages:
            for status in message.status:
                if (status.name == "Power board 0"):
                    for value in status.values:
                        if (value.label == "Breaker 0 Voltage"):
                            self.voltages[0] = value.value
                        if (value.label == "Breaker 1 Voltage"):
                            self.voltages[1] = value.value
                        if (value.label == "Breaker 2 Voltage"):
                            self.voltages[2] = value.value
                        if (value.label == "RunStop Button Status"):
                            if value.value > 0.5:
                                self.estop_wireless_status = "Run"
                            else:
                                self.estop_wireless_status = "Stop"
                        if (value.label == "RunStop Status"):
                            if value.value > 0.5:
                                self.estop_button_status = "Run"
                            else:
                                if self.estop_wireless_status == "Stop":
                                    self.estop_button_status = "Unknown"
                                else:
                                    self.estop_button_status = "Stop"

                    for strvals in status.strings:
                        if (strvals.label == "Breaker 0 State"):
                            self.breaker_state[0] = strvals.value
                        if (strvals.label == "Breaker 1 State"):
                            self.breaker_state[1] = strvals.value
                        if (strvals.label == "Breaker 2 State"):
                            self.breaker_state[2] = strvals.value
                    


##                    print "Voltages: %.1f %.1f %.1f"%(self.voltages[0],self.voltages[1], self.voltages[2])
##                    print "States: %s %s %s"%(self.breaker_state[0], self.breaker_state[1], self.breaker_state[2])

                    self.breaker0_status.SetValue("%s @ %.2fV"%(self.breaker_state[0], self.voltages[0]))
                    self.breaker1_status.SetValue("%s @ %.2fV"%(self.breaker_state[1], self.voltages[1]))
                    self.breaker2_status.SetValue("%s @ %.2fV"%(self.breaker_state[2], self.voltages[2]))
                    if self.breaker_state[0] == "Standby":
                        self.breaker0_status.SetBackgroundColour("Orange")
                    elif self.breaker_state[0] == "On":
                        self.breaker0_status.SetBackgroundColour("Light Green")
                    else:
                        self.breaker0_status.SetBackgroundColour("Red")

                    if self.breaker_state[1] == "Standby":
                        self.breaker1_status.SetBackgroundColour("Orange")
                    elif self.breaker_state[1] == "On":
                        self.breaker1_status.SetBackgroundColour("Light Green")
                    else:
                        self.breaker1_status.SetBackgroundColour("Red")

                    if self.breaker_state[2] == "Standby":
                        self.breaker2_status.SetBackgroundColour("Orange")
                    elif self.breaker_state[2] == "On":
                        self.breaker2_status.SetBackgroundColour("Light Green")
                    else:
                        self.breaker2_status.SetBackgroundColour("Red")

                    if self.estop_button_status == "Stop" or  self.estop_wireless_status == "Stop":
                        estop_status_temp = "Stop"
                        self.estop_status.SetBackgroundColour("Red")
                    else:
                        estop_status_temp = "Run"
                        self.estop_status.SetBackgroundColour("Light Green")

                    self.estop_status.SetValue("RunStop Status: %s      Button(%s) Wireless(%s)"%(estop_status_temp, self.estop_button_status, self.estop_wireless_status))

        

##        self.textboxes[0].value = "hi"       
        self._messages = []
        
        self._mutex.release()
        
        self.Refresh()
        


    def EnableCB0(self, event):
        try:
            self.power_control(0, "start", 0)
        except rospy.ServiceException, e:
            print "Service Call Failed: %s"%e
        #print "Enable CB0"
    def EnableCB1(self, event):
        try:
            self.power_control(1, "start", 0)
        except rospy.ServiceException, e:
            print "Service Call Failed: %s"%e
        #print "Enable CB1"
    def EnableCB2(self, event):
        try:
            self.power_control(2, "start", 0)
        except rospy.ServiceException, e:
            print "Service Call Failed: %s"%e
        #print "Enable CB2"

    def StandbyCB0(self, event):
        try:
            self.power_control(0, "stop", 0)
        except rospy.ServiceException, e:
            print "Service Call Failed: %s"%e
        #print "Standby CB0"
    def StandbyCB1(self, event):
        try:
            self.power_control(1, "stop", 0)
        except rospy.ServiceException, e:
            print "Service Call Failed: %s"%e
        #print "Standby CB1"
    def StandbyCB2(self, event):
        try:
            self.power_control(2, "stop", 0)
        except rospy.ServiceException, e:
            print "Service Call Failed: %s"%e
        #print "Standby CB2"

    def ResetCB0(self, event):
        try:
            self.power_control(0, "reset", 0)
        except rospy.ServiceException, e:
            print "Service Call Failed: %s"%e
        #print "Reset CB0"
    def ResetCB1(self, event):
        try:
            self.power_control(1, "reset", 0)
        except rospy.ServiceException, e:
            print "Service Call Failed: %s"%e
        #print "Reset CB1"
    def ResetCB2(self, event):
        try:
            self.power_control(2, "reset", 0)
        except rospy.ServiceException, e:
            print "Service Call Failed: %s"%e
        #print "Reset CB2"

    def DisableCB0(self, event):
        try:
            self.power_control(0, "disable", 0)
        except rospy.ServiceException, e:
            print "Service Call Failed: %s"%e
        #print "Disable CB0"
    def DisableCB1(self, event):
        try:
            self.power_control(1, "disable", 0)
        except rospy.ServiceException, e:
            print "Service Call Failed: %s"%e
        #print "Disable CB1"
    def DisableCB2(self, event):
        try:
            self.power_control(2, "disable", 0)
        except rospy.ServiceException, e:
            print "Service Call Failed: %s"%e
        #print "Disable CB2"

    def ResetCurrent(self, event):
        try:
            self.power_control(0, "none", 1)
        except rospy.ServiceException, e:
            print "Service Call Failed: %s"%e
    def ResetTransitions(self, event):
        try:
            self.power_control(0, "none", 2)
        except rospy.ServiceException, e:
            print "Service Call Failed: %s"%e

