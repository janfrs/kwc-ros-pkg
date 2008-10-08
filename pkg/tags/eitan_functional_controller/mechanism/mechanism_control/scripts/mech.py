#! /usr/bin/env python
# Provides quick access to the services exposed by MechanismControlNode

import rostools, time
rostools.update_path('mechanism_control')

import rospy, sys
from mechanism_control import mechanism

def print_usage(exit_code = 0):
    print '''Commands:
    lt  - List controller Types
    lc  - List active controllers
    sp  - Spawn a controller using the xml passed over stdin
    kl <name>  - Kills the controller named <name>
    shutdown - Ends whole process'''
    sys.exit(exit_code)

if __name__ == '__main__':
    if len(sys.argv) < 2:
        print_usage()
    time.sleep(2) #FIXME: added by john, this might have removed assert(robot) failure in controller initXml calls on startup. need to investigate why if any memory corruption or race condition for MC stack.
    if sys.argv[1] == 'lt':
        mechanism.list_controller_types()
    elif sys.argv[1] == 'lc':
        mechanism.list_controllers()
    elif sys.argv[1] == 'sp':
        xml = ""
        if len(sys.argv) > 2:
            f = open(sys.argv[2])
            xml = f.read()
            f.close()
        else:
            xml = sys.stdin.read()
        mechanism.spawn_controller(xml)
    elif sys.argv[1] == 'kl':
        mechanism.kill_controller(sys.argv[2])
    elif sys.argv[1] == 'shutdown':
        mechanism.shutdown()
    else:
        print_usage(1)
