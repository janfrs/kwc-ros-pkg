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

## Gazebo test cameras validation 

PKG = 'gazebo_plugin'
NAME = 'test_camera'

import math
import rostools
rostools.update_path(PKG)
rostools.update_path('rostest')
rostools.update_path('std_msgs')
rostools.update_path('robot_msgs')
rostools.update_path('rostest')
rostools.update_path('rospy')

import sys, unittest
import os, os.path, threading, time
import rospy, rostest
from std_msgs.msg import *
from PIL import Image      as pili
from PIL import ImageChops as pilic

FRAME_TARGET = "cam_sen-0050.ppm"
FRAME_DIR = "test_camera_frames"
TOTAL_ERROR_TOL = 5
TEST_DURATION   = 10
TEST_INIT_WAIT  = 40

class PollCameraThread(threading.Thread):
    def __init__(self, target, dir):
        super(PollCameraThread, self).__init__()
        self.target = target
        self.dir = dir
    
    def run(self):
        while 1:
            time.sleep(0.01)
            if (os.path.isdir(self.dir)):
                ls = os.listdir(self.dir)
                #print "Dir: " + str(ls)
                for file in ls:
                    if (file == FRAME_TARGET):
                        self.target.onTargetFrame()
                        return

class TestCameras(unittest.TestCase):
    def __init__(self, *args):
        super(TestCameras, self).__init__(*args)
        self.success = False
        #self.pollThread = PollCameraThread(self, FRAME_DIR)

    def onTargetFrame(self):
        time.sleep(0.5) #Safety, to make sure the image is really done being written.
        ps = "diff -q " + FRAME_DIR + "/" + FRAME_TARGET + " test/test_camera.valid.ppm"
        #print "CMD: " + ps + "\n"
        result = os.system(ps)
        if (result == 0):
            self.success = True
            #print "Success\n"
        else:
            self.success = False
            #print "Failure\n"
        rospy.signal_shutdown('test done')

    def images_are_the_same(self,i0,i1):
        print "comparing images... "
        error_count = 0
        error_total = 0
        pixel_tol = 0

        # thumbnail-ize images
        size = 100,100
        i0.thumbnail(size,pili.ANTIALIAS)
        i1.thumbnail(size,pili.ANTIALIAS)
        ic = pilic.difference(i0,i1)
        i0.save("t_0.ppm")
        i1.save("t_1.ppm")
        ic.save("t_d.ppm")

        # get raw data for comparison
        i0d = i0.getdata()
        i1d = i1.getdata()

        # assert len(i0)==len(i1)
        if len(i0d) != len(i1d):
          print "lengths not equal ",len(i0d), len(i1d)
          return False
        print "thumbnail lengths ",len(i0d), len(i1d)

        #compare thumbnails only
        for i in range(len(i0d)):
          (r0,g0,b0) = i0d[i]
          (r1,g1,b1) = i1d[i]
          #if abs(r0-r1) > 0 or abs(g0-g1) > 0 or abs(b0-b1) > 0:
          #  print "debug errors ",i,abs(r0-r1),abs(g0-g1),abs(b0-b1)
          if abs(r0-r1) > pixel_tol or abs(g0-g1) > pixel_tol or abs(b0-b1) > pixel_tol:
            error_count = error_count + 1
            error_total = error_total + abs(r0-r1) + abs(g0-g1) + abs(b0-b1)
        error_avg = float(error_total)/float(len(i0d))
        print "total error count:",error_count
        print "average error:    ",error_avg
        if error_avg > TOTAL_ERROR_TOL:
          return False
        else:
          return True

    def imageInput(self,image):

        print " got image message from ROS, begin comparing images "

        print "  - load validation image from file test_camera.valid.ppm "
        if os.path.isfile("test_camera.valid.ppm"):
          im0 = pili.open("test_camera.valid.ppm")
        elif os.path.isfile("test/test_camera.valid.ppm"):
          im0 = pili.open("test/test_camera.valid.ppm")
        else:
          print "cannot find validation file: test_camera.valid.ppm"
          self.success = False
          return

        print "  - load image from ROS "
        size = image.width,image.height
        im1 = pili.new("RGBA",size)
        im1 = pili.frombuffer("RGB",size,str(image.data));
        im1 = im1.transpose(pili.FLIP_LEFT_RIGHT).rotate(180);
        imc = pilic.difference(im0,im1)


        print "  - comparing images "
        im1.save("test_1.ppm") # uncomment this line to capture a new valid frame when things change
        im0.save("test_0.ppm")
        imc.save("test_d.ppm")
        #im1.show()
        #im0.show()
        #imc.show()
        comp_result = self.images_are_the_same(im0,im1)
        print "test comparison ", comp_result
        #print "proofcomparison ", self.images_are_the_same(im1.getdata(),im1.getdata())
        if (self.success == False): # test if we have not succeeded
          if (comp_result == 1):
            print "  - images are the Same "
            self.success = True
          else:
            print "  - images differ "
            self.success = False

    def test_camera(self):
        print " wait TEST_INIT_WAIT sec for objects to settle and arms to tuck "
        time.sleep(TEST_INIT_WAIT)
        print " subscribe stereo left image from ROS "
        #rospy.TopicSub("test_camera/image", Image, self.imageInput)  # this is a test camera, simply looking at the cups
        rospy.TopicSub("stereo_l/image", Image, self.imageInput) # this is a camera mounted on PR2 head (left stereo camera)
        rospy.init_node(NAME, anonymous=True)
        #self.pollThread.start()
        timeout_t = time.time() + TEST_DURATION
        while not rospy.is_shutdown() and not self.success and time.time() < timeout_t:
            time.sleep(0.1)
        self.assert_(self.success)
        
    


if __name__ == '__main__':
    #while (os.path.isfile(FRAME_DIR + "/" + FRAME_TARGET)):
    #    print "Old test case still alive."
    #    time.sleep(1)
    
    print " starting test "
    rostest.run(PKG, sys.argv[0], TestCameras, sys.argv)


