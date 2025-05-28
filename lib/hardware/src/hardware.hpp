#pragma once
#include <cstdint>

#define omask_t uint8_t
#define imask_t uint16_t

    const int numOutPorts = 8;
     const int numInputPorts = 16;
     extern omask_t readOutputPorts();
     extern imask_t readInputPorts();
     extern void writeOutputs( omask_t mask);


