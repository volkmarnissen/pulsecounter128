#pragma once
#include "hardware.hpp"
enum OutputConfigurationType { NoType, EMeterType, WaterMeterType};
struct OutputConfiguration {
    OutputConfigurationType type;
};
namespace Pulsecounter{
extern bool inputHasChanged(uint16_t outPin, uint16_t inPin);
extern void readPorts( OutputConfigurationType type);
extern void setPreviousInputMaskForTest(imask_t *previousMask);    
extern void configureOutput(uint8_t port, OutputConfiguration config);
}


