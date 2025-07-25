#pragma once
#include "hardware.hpp"
#include "common.hpp"
#include "config.hpp"
#include <thread>
typedef struct
{
  uint8_t numInputPort;
  uint8_t numOutPort;
  uint32_t counter;
} PulseCounterType;
struct OutputData
{
  uint16_t currentCount;
  uint16_t maxCount;
  imask_t previousInputMask;
  imask_t currentInputMask;
  omask_t offMask;
  omask_t onMask;
  omask_t pcMask;
};
struct NoOutputData
{
  imask_t previousInputMask;
  imask_t currentInputMask;
};

const uint8_t maxPulseCounters = 128;
const uint8_t noInputPort = 0xFF;
namespace Pulsecounter
{
  extern bool inputHasRisingEdge(PulseCounterType &pulseCounter);
  extern void readPorts(OutputConfigurationType type);
  extern void setOutputConfiguration(const OutputConfig &output, const Config &config);
  extern void init();
  extern void setConfig(const Config &cfg);
  extern void joinThread();
  extern void stopThread();
  extern void startThread();
  extern bool readInputsRisingEdge();
  extern void setPulseCounter(uint8_t outputPort, uint8_t inputPort);
  extern void countPulses();
  extern uint32_t getCounts(uint8_t outputPort, uint8_t inputPort);
  extern void reset();
#ifdef NATIVE
  extern OutputData *getOutputData();
  extern NoOutputData *getNoOutputData();
#endif
}
