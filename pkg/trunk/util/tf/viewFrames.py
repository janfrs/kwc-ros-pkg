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
# Revision $Id: gossipbot.py 1013 2008-05-21 01:08:56Z sfkwc $

## Simple demo of a rospy service client that calls a service to add
## two integers. 

from __future__ import with_statement

PKG = 'tf' # this package name

import rostools; rostools.update_path(PKG) 

import sys
import os
import string
import re
import subprocess
import distutils.version

from optparse import OptionParser

import rospy
from tf.srv import *


def generate(node_name):
    rospy.wait_for_service('listener/tf_frames')

    try:
        tf_frames_proxy = rospy.ServiceProxy('listener/tf_frames', FrameGraph)

        output = tf_frames_proxy.call(FrameGraphRequest())
##        print output.dot_graph



    except rospy.ServiceException, e:
        print "Service call failed: %s"%e


    with open('frames.gv', 'w') as outfile:
        outfile.write(output.dot_graph)


    try:
        # Check version, make postscript if too old to make pdf
        args = ["dot", "-V"]
        vstr = subprocess.Popen(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE).communicate()[1]
        v = distutils.version.StrictVersion('2.16')
        r = re.compile(".*version ([^ ]*).*")
        print vstr
        m = r.search(vstr)
        if not m or not m.group(1):
          print 'Warning: failed to determine your version of dot.  Assuming v2.16'
        else:
          version = distutils.version.StrictVersion(m.group(1))
          print 'Detected dot version %s' % (version)
        if version > distutils.version.StrictVersion('2.8'):
          subprocess.check_call(["dot", "-Tpdf", "frames.gv", "-o", "frames.pdf"])
          print "frames.pdf generated"
        else:
          subprocess.check_call(["dot", "-Tps2", "frames.gv", "-o", "frames.ps"])
          print "frames.ps generated"
    except subprocess.CalledProcessError:
        print >> sys.stderr, "failed to generate frames.pdf"        


if __name__ == '__main__':
    parser = OptionParser(usage="usage: %prog [options]", prog='viewFrames.py')
    parser.add_option("--node", metavar="node name",
                      type="string", help="Node to get frames from",
                      dest="node")
    options, args = parser.parse_args()

    if not options.node:
        print "Please enter a node name as an argument"
        exit(-1)
    else:
        generate(options.node)
