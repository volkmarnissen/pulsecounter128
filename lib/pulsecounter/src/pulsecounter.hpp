#pragma once
#include "hardware.hpp"
#include "common.hpp"
#include <thread>
struct PulseCounterType
{
  uint8_t numInputPort;
  uint8_t numOutPort;
  uint32_t counter;
};
struct OutputConfiguration
{
  OutputConfigurationType type;
};
const uint8_t noInputPort = 0xFF;
namespace Pulsecounter
{
  extern bool inputHasRisingEdge(PulseCounterType &pulseCounter);
  extern void readPorts(OutputConfigurationType type);
  extern void setOutputConfiguration(uint8_t port, OutputConfiguration config);
  extern void init();
  extern void joinThread();
  extern void stopThread();
  extern void startThread();
  extern bool readInputsRisingEdge();
  extern void setPulseCounter(uint8_t outputPort, uint8_t inputPort);
  extern void countPulses();
}
