#include "hardware.hpp"
#ifdef NATIVE_TEST
    omask_t readOutputPorts(){
        return 0x08;
    };

    imask_t readInputPorts(){
        return 0x0F;
    };

    void writeOutputs( omask_t mask){};   
#endif

