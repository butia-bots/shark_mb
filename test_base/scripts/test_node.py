#!/usr/bin/env python
import rospy
import serial

from geometry_msgs.msg import Twist

class BaseComm(object):
    def __init__(self):
        self.__comm = serial.Serial("/dev/ttyUSB0",115200)
        rospy.Subscriber("/cmd_vel", Twist, self.__callback, queue_size=1)
        self.L = 0.4
        self.R = 0.2032
        # self.timer = rospy.Timer(0.01,self.__tCallback)
    
    def __callback(self, data :  Twist):
        
        omegaD = (data.linear.x + (data.angular.z* self.L / 2))/self.R
        omegaE = (data.linear.x - (data.angular.z* self.L / 2))/self.R

        velD = omegaD / 0.10472
        velE = omegaE / 0.10472

        speed = (velD + velE)/2
        steer = (velD - speed)*2

        payload = bytearray([0xAB,0xCD])
        payload.extend(int(steer).to_bytes(2,"little", signed=True))
        payload.extend(int(speed).to_bytes(2,"little", signed=True))
        checksum = 0xABCD ^ int(steer) ^ int(speed)
        payload.extend(checksum.to_bytes(2,"little"))
        self.__comm.write(payload)
    
    def __del__(self):
        self.__comm.close()


def main() -> None:
    try:
        rospy.init_node("base_test")
        node  =  BaseComm()
        rospy.spin()
    except KeyboardInterrupt:
        pass


if __name__ == "__main__":
    main()