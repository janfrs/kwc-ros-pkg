
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

#ifndef TRANSMISSION_H
#define TRANSMISSION_H

#include "mechanism_model/joint.h"
#include "hw_interface/hardware_interface.h"

namespace mechanism {

class Transmission
{
public:
  Transmission() {}
  virtual ~Transmission() {}

  // Uses encoder data to fill out joint position and velocities
  virtual void propagatePosition() = 0;

  // Uses commanded joint efforts to fill out commanded motor currents
  virtual void propagateEffort() = 0;
};


class SimpleTransmission : public Transmission
{
public:
  SimpleTransmission() {}
  SimpleTransmission(Joint *joint, Actuator *actuator, double mechanicalReduction, double motorTorqueConstant, double ticksPerRadian);
  ~SimpleTransmission() {}

  Actuator *actuator;
  Joint *joint;

  double mechanicalReduction;
  double motorTorqueConstant;
  double pulsesPerRevolution;

  void propagatePosition();
  void propagateEffort();
};

#if 0
class CoupledTransmission : public Transmission{

public:

  void CoupledTranmission(Actuator *actuator, Joint *joint, double mechanicalReduction, double motorTorqueConstant);

};

class NonlinearTransmission : public Transmission{

public:

  NonlinearTransmission(Actuator *actuator, Joint *joint, double mechanicalReduction, double motorTorqueConstant);

  Actuator *actuator;

  Joint *joint;

  // ?? Lookup table
};
#endif

} // namespace mechanism

#endif
