#pragma once
#include "hardware.hpp"
#include <thread>
struct PulseCounterType{
   u_int8_t numInputPort;
   uint8_t numOutPort;
   char name[16];
   uint32_t counter;
  
 };
enum OutputConfigurationType { NoType, EMeterType, WaterMeterType};
struct OutputConfiguration {
    OutputConfigurationType type;
};
namespace Pulsecounter{
extern bool inputHasRisingEdge( PulseCounterType& pulseCounter);
extern void readPorts( OutputConfigurationType type);
extern void setOutputConfiguration(uint8_t port, OutputConfiguration config);
extern void init();
extern void stopThread();
extern bool readInputsRisingEdge( );
extern void setPulseCounter( uint8_t outputPort, uint8_t inputPort, const char *name);
extern void countPulses();
}
