
///////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2008, Eric Berger
//
// Redistribution and use in source and binary forms, with or without 
// modification, are permitted provided that the following conditions are met:
//   * Redistributions of source code must retain the above copyright notice, 
//     this list of conditions and the following disclaimer.
//   * Redistributions in binary form must reproduce the above copyright 
//     notice, this list of conditions and the following disclaimer in the 
//     documentation and/or other materials provided with the distribution.
//   * Neither the name of Stanford University nor the names of its 
//     contributors may be used to endorse or promote products derived from 
//     this software without specific prior written permission.
//   
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE 
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
// POSSIBILITY OF SUCH DAMAGE.
//////////////////////////////////////////////////////////////////////////////

#ifndef HARDWARE_INTERFACE_H
#define HARDWARE_INTERFACE_H

class ActuatorState{
  public:
    ActuatorState(){
      lastCalibrationHighTransition = 0;
      lastCalibrationLowTransition = 0;
      isEnabled = 0;
      runStopHit = 0;
      lastRequestedCurrent = 0;
      lastCommandedCurrent = 0;
      lastMeasuredCurrent = 0;
      numEncoderErrors = 0;
    }
  int encoderCount;
  double timestamp;
  double encoderVelocity;
  bool calibrationReading;
  int lastCalibrationHighTransition;
  int lastCalibrationLowTransition;
  bool isEnabled;
  bool runStopHit;

  double lastRequestedCurrent;
  double lastCommandedCurrent;
  double lastMeasuredCurrent;

  int motorVoltage;

  int numEncoderErrors;
  int numCommunicationErrors;
};

class ActuatorCommand{
  public:
  ActuatorCommand(){
    enable = 0;
    current = 0;
  }
  bool enable;
  double current;
};

class Actuator{
  public:
  ActuatorState state;
  ActuatorCommand command;
};

class HardwareInterface{
  public:
  HardwareInterface(int numActuators){
    actuator = new Actuator[numActuators];
    this->numActuators = numActuators;  
  }
  Actuator *actuator;
  int numActuators;
  double current_time_;
};

#endif
